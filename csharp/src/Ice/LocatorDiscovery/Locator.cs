// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice.LocatorDiscovery
{
    /// <summary>Implements interface Ice::Locator by forwarding all calls to the discovered locator. We cannot simply
    /// forward the requests using ForwardAsync because we need to occasionally perform transcoding. This locator is
    /// hosted in an ice2 object adapter and typically receives 2.0-encoded requests, and the discovered locator proxy
    /// can be an ice1/1.1 proxy that understands only 1.1-encoded requests.</summary>
    internal class Locator : IAsyncLocator
    {
        private TaskCompletionSource<ILocatorPrx>? _completionSource;
        private Task<ILocatorPrx?>? _findLocatorTask;
        private string _instanceName;
        private ILocatorPrx? _locator;
        private readonly ILookupPrx _lookup;
        private readonly Dictionary<ILookupPrx, ILookupReplyPrx> _lookups = new ();

        private readonly string _lookupTraceCategory;
        private readonly int _lookupTraceLevel;
        private readonly object _mutex = new object();
        private TimeSpan _nextRetry;

        private readonly string _pluginName;

        private readonly int _retryCount;
        private readonly TimeSpan _retryDelay;
        private readonly TimeSpan _timeout;

        // "Overrides" the generated DispatchAsync to forward as-is when the encoding match (this includes unknown
        // operations and binary contexts). Otherwise, use the generated code to perform transcoding back and forth.
        public async ValueTask<OutgoingResponseFrame> DispatchAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            ILocatorPrx? locator = await GetLocatorAsync().ConfigureAwait(false);

            if (locator != null && current.Encoding == locator.Encoding)
            {
                return await ForwardRequestAsync(
                    locator =>
                    locator?.ForwardAsync(current.IsOneway, request, cancel: cancel).AsTask() ??
                        // In the unlikely event locator is now null (e.g. after a failed attempt), we use the
                        // "transcoding dispatch method" which will in turn return null/empty with a null locator.
                        // See comments below.
                        IAsyncLocator.DispatchAsync(this, request, current, cancel).AsTask()).ConfigureAwait(false);
            }
            else
            {
                // Calls the base DispathAsync, which calls FindAdapterByIdAsync etc.
                // The transcoding is naturally limited to the known Ice::Locator operations. Other operations
                // cannot be transcoded and result in OperationNotExistException.
                return await IAsyncLocator.DispatchAsync(this, request, current, cancel).ConfigureAwait(false);
            }
        }

        // Forwards the request to the discovered locator; if this discovered locator is null, returns a null proxy.
        public ValueTask<IObjectPrx?> FindAdapterByIdAsync(
            string adapterId,
            Current current,
            CancellationToken cancel) =>
            ForwardRequestAsync(locator =>
                                locator?.FindAdapterByIdAsync(adapterId, current.Context, cancel: cancel) ??
                                    Task.FromResult<IObjectPrx?>(null));

        public ValueTask<IObjectPrx?> FindObjectByIdAsync(
            Identity identity,
            string? facet,
            Current current,
            CancellationToken cancel) =>
            ForwardRequestAsync(locator =>
                                locator?.FindObjectByIdAsync(identity, facet, cancel: cancel) ??
                                    Task.FromResult<IObjectPrx?>(null));
        public ValueTask<ILocatorRegistryPrx?> GetRegistryAsync(Current current, CancellationToken cancel) =>
            ForwardRequestAsync(locator =>
                                locator?.GetRegistryAsync(current.Context, cancel: cancel) ??
                                    Task.FromResult<ILocatorRegistryPrx?>(null));

        public ValueTask<(IEnumerable<EndpointData>, IEnumerable<string>)> ResolveLocationAsync(
            string[] location,
            Current current,
            CancellationToken cancel) =>
            ForwardRequestAsync<(IEnumerable<EndpointData>, IEnumerable<string>)>(
                async locator =>
                {
                    if (locator != null)
                    {
                        return await locator.ResolveLocationAsync(
                            location,
                            current.Context,
                            cancel: cancel).ConfigureAwait(false);
                    }
                    else
                    {
                        return (ImmutableArray<EndpointData>.Empty, ImmutableArray<string>.Empty);
                    }
                });

        public ValueTask<(IEnumerable<EndpointData>, IEnumerable<string>)> ResolveWellKnownProxyAsync(
            Identity identity,
            string facet,
            Current current,
            CancellationToken cancel) =>
            ForwardRequestAsync<(IEnumerable<EndpointData>, IEnumerable<string>)>(
                async locator =>
                {
                    if (locator != null)
                    {
                        return await locator.ResolveWellKnownProxyAsync(
                            identity,
                            facet,
                            current.Context,
                            cancel: cancel).ConfigureAwait(false);
                    }
                    else
                    {
                        return (ImmutableArray<EndpointData>.Empty, ImmutableArray<string>.Empty);
                    }
                });

        internal Locator(
            string pluginName,
            ILookupPrx lookup,
            Communicator communicator,
            string instanceName,
            ILookupReplyPrx lookupReply)
        {
            _pluginName = pluginName;
            _lookup = lookup;
            _timeout = communicator.GetPropertyAsTimeSpan($"{_pluginName}.Timeout") ?? TimeSpan.FromMilliseconds(300);
            if (_timeout == System.Threading.Timeout.InfiniteTimeSpan)
            {
                _timeout = TimeSpan.FromMilliseconds(300);
            }
            _retryCount = Math.Max(communicator.GetPropertyAsInt($"{_pluginName}.RetryCount") ?? 3, 1);
            _retryDelay = communicator.GetPropertyAsTimeSpan($"{_pluginName}.RetryDelay") ??
                TimeSpan.FromMilliseconds(2000);
            _lookupTraceLevel = communicator.GetPropertyAsInt($"{_pluginName}.Trace.Lookup") ?? 0;
            _lookupTraceCategory = $"{_pluginName}.Lookup";
            _instanceName = instanceName;
            _locator = lookup.Communicator.DefaultLocator;

            // Create one lookup proxy per endpoint from the given proxy. We want to send a multicast
            // datagram on each endpoint.
            foreach (Endpoint endpoint in lookup.Endpoints)
            {
                // lookup's invocation mode is Datagram
                Debug.Assert(endpoint.Transport == Transport.UDP);

                ILookupPrx key = lookup.Clone(endpoints: ImmutableArray.Create(endpoint));

                if (endpoint["interface"] is string mcastInterface && mcastInterface.Length > 0)
                {
                    Endpoint? q = lookupReply.Endpoints.FirstOrDefault(e => e.Host == mcastInterface);

                    if (q != null)
                    {
                        _lookups[key] = lookupReply.Clone(endpoints: ImmutableArray.Create(q));
                    }
                }

                if (!_lookups.ContainsKey(key))
                {
                    // Fallback: just use the given lookup reply proxy if no matching endpoint found.
                    _lookups[key] = lookupReply;
                }
            }
            Debug.Assert(_lookups.Count > 0);
        }

        internal void FoundLocator(ILocatorPrx locator)
        {
            lock (_mutex)
            {
                if (_instanceName.Length > 0 && locator.Identity.Category != _instanceName)
                {
                    if (_lookupTraceLevel > 2)
                    {
                        _lookup.Communicator.Logger.Trace(_lookupTraceCategory,
                            @$"ignoring locator reply: instance name doesn't match\nexpected = {_instanceName
                            } received = {locator.Identity.Category}");
                    }
                    return;
                }

                // If we already have a locator assigned, ensure the given locator has the same identity, facet and
                // protocol, otherwise ignore it.
                if (_locator != null)
                {
                    if (locator.Identity != _locator.Identity || locator.Facet != _locator.Facet)
                    {
                        var sb = new StringBuilder(_pluginName);
                        sb.Append(": received Ice locator with different identities:\n")
                          .Append("using = `").Append(_locator).Append("'\n")
                          .Append("received = `").Append(locator).Append("'\n")
                          .Append("This is typically the case if multiple Ice locators with different ")
                          .Append("instance names are deployed and the property `")
                          .Append(_pluginName).Append(".InstanceName' is not set.");
                        locator.Communicator.Logger.Warning(sb.ToString());
                        return;
                    }

                    if (locator.Protocol != _locator.Protocol)
                    {
                        var sb = new StringBuilder(_pluginName);
                        sb.Append(": ignoring Ice locator with different protocol:\n")
                          .Append("using = `").Append(_locator.Protocol).Append("'\n")
                          .Append("received = `").Append(locator.Protocol).Append("'\n");
                        locator.Communicator.Logger.Warning(sb.ToString());
                        return;
                    }
                }

                if (_lookupTraceLevel > 0)
                {
                    var sb = new StringBuilder("locator lookup succeeded:\nlocator = ");
                    sb.Append(locator);
                    if (_instanceName.Length > 0)
                    {
                        sb.Append("\ninstance name = ").Append(_instanceName);
                    }

                    _lookup.Communicator.Logger.Trace(_lookupTraceCategory, sb.ToString());
                }

                if (_locator == null)
                {
                    _locator = locator;
                    if (_instanceName.Length == 0)
                    {
                        _instanceName = _locator.Identity.Category; // Stick to the first locator
                    }
                    Debug.Assert(_completionSource != null);
                    _completionSource.TrySetResult(locator);
                }
                else
                {
                    // We found another locator replica, append its endpoints to the current locator proxy endpoints,
                    // while eliminating duplicates.
                    _locator = _locator.Clone(endpoints: _locator.Endpoints.Concat(locator.Endpoints).Distinct());
                }
            }
        }

        private async Task<ILocatorPrx?> FindLocatorAsync()
        {
            lock (_mutex)
            {
                Debug.Assert(_locator == null);
                Debug.Assert(_findLocatorTask == null);
                _completionSource = new TaskCompletionSource<ILocatorPrx>();
            }

            if (_lookupTraceLevel > 1)
            {
                var sb = new StringBuilder("looking up locator:\nlookup = ");
                sb.Append(_lookup);
                if (_instanceName.Length > 0)
                {
                    sb.Append("\ninstance name = ").Append(_instanceName);
                }
                _lookup.Communicator.Logger.Trace(_lookupTraceCategory, sb.ToString());
            }

            int failureCount = 0;
            for (int i = 0; i < _retryCount; ++i)
            {
                foreach ((ILookupPrx lookup, ILookupReplyPrx lookupReply) in _lookups)
                {
                    try
                    {
                        await lookup.FindLocatorAsync(_instanceName, lookupReply).ConfigureAwait(false);
                    }
                    catch (Exception ex)
                    {
                        lock (_mutex)
                        {
                            if (++failureCount == _lookups.Count)
                            {
                                // All the lookup calls failed, return null
                                if (_lookupTraceLevel > 0)
                                {
                                    var sb = new StringBuilder("locator lookup failed:\nlookup = ");
                                    sb.Append(_lookup);
                                    if (_instanceName.Length > 0)
                                    {
                                        sb.Append("\ninstance name = ").Append(_instanceName);
                                    }
                                    sb.Append('\n');
                                    sb.Append(ex);
                                    _lookup.Communicator.Logger.Trace(_lookupTraceCategory, sb.ToString());
                                }
                                return null;
                            }
                        }
                    }
                }

                Task t = await Task.WhenAny(_completionSource.Task, Task.Delay(_timeout)).ConfigureAwait(false);
                if (t == _completionSource.Task)
                {
                    return await _completionSource.Task.ConfigureAwait(false);
                }
            }

            lock (_mutex)
            {
                if (_completionSource.Task.IsCompleted)
                {
                    // we got a concurrent reply after the timeout
                    Debug.Assert(_locator != null);
                    return _locator;
                }
                else
                {
                    // Locator lookup timeout and no more retries
                    if (_lookupTraceLevel > 0)
                    {
                        var sb = new StringBuilder("locator lookup timed out:\nlookup = ");
                        sb.Append(_lookup);
                        if (_instanceName.Length > 0)
                        {
                            sb.Append("\ninstance name = ").Append(_instanceName);
                        }
                        _lookup.Communicator.Logger.Trace(_lookupTraceCategory, sb.ToString());
                    }

                    _nextRetry = Time.Elapsed + _retryDelay;
                    return null;
                }
            }
        }

        // This helper method calls "callAsync" with the discovered locator or null when no locator was discovered.
        private async ValueTask<TResult> ForwardRequestAsync<TResult>(Func<ILocatorPrx?, Task<TResult>> callAsync)
        {
            ILocatorPrx? badLocator = null;
            Exception? exception = null;
            while (true)
            {
                // Get the locator to send the request to (this will return null if no locator is found)
                ILocatorPrx? newLocator = await GetLocatorAsync().ConfigureAwait(false);
                if (newLocator != null && !newLocator.Equals(badLocator))
                {
                    try
                    {
                        return await callAsync(newLocator).ConfigureAwait(false);
                    }
                    catch (RemoteException ex)
                    {
                        // If we receive a RemoteException, we just forward it as-is to the caller (typically a
                        // colocated LocatorInfo).
                        ex.ConvertToUnhandled = false;
                        throw;
                    }
                    catch (Exception ex)
                    {
                        badLocator = newLocator;

                        // If we get some local exception, we attempt to find a new locator and try again.
                        // TODO: this could lead to an infinite loop if we keep alternating between 2 different
                        // locators.
                        lock (_mutex)
                        {
                            // If the current locator is equal to the one we use to send the request,
                            // clear it and retry, this will trigger the lookup of a new locator.
                            if (_locator != null && _locator.Equals(newLocator))
                            {
                                _locator = null;
                            }
                        }
                        exception = ex;
                    }
                }
                else
                {
                    if (exception != null)
                    {
                        // Could not find any locator or we got the same locator or a null locator after a failure.
                        _lookup.Communicator.Logger.Warning(
                            $"{_pluginName}: failed to send request to discovered locator:\n{exception}");
                    }

                    return await callAsync(null).ConfigureAwait(false);
                }
            }
        }

        private async Task<ILocatorPrx?> GetLocatorAsync()
        {
            Task<ILocatorPrx?> findLocatorTask;
            lock (_mutex)
            {
                if (_locator != null)
                {
                    // If we already have a locator we use it.
                    return _locator;
                }
                else if (Time.Elapsed < _nextRetry)
                {
                    // If the retry delay has not elapsed since the last failure return null
                    return null;
                }
                else if (_findLocatorTask == null)
                {
                    // If a locator lookup is running we await on it otherwise we start a new lookup.
                    _findLocatorTask = FindLocatorAsync();
                }
                findLocatorTask = _findLocatorTask;
            }

            ILocatorPrx? locator = await findLocatorTask.ConfigureAwait(false);
            lock (_mutex)
            {
                _findLocatorTask = null;
            }
            return locator;
        }

    }

    internal class LookupReply : ILookupReply
    {
        private readonly Locator _locatorServant;

        public void FoundLocator(ILocatorPrx locator, Current current, CancellationToken cancel) =>
            _locatorServant.FoundLocator(locator);

        internal LookupReply(Locator locatorServant) => _locatorServant = locatorServant;
    }
}
