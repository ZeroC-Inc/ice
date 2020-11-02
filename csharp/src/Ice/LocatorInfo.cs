// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using EndpointList = System.Collections.Generic.IReadOnlyList<ZeroC.Ice.Endpoint>;
using Location = System.Collections.Generic.IReadOnlyList<string>;

namespace ZeroC.Ice
{
    /// <summary>The locator info class caches information specific to a given locator proxy. The communicator holds a
    /// locator info instance per locator proxy set either with Ice.Default.Locator or the proxy's Locator property. It
    /// caches the locator registry proxy and keeps track of requests to the locator to prevent multiple concurrent
    /// identical requests.</summary>
    internal sealed class LocatorInfo
    {
        internal ILocatorPrx Locator { get; }

        private static readonly IEqualityComparer<(Location, Protocol)> _locationComparer = new LocationComparer();

        private readonly bool _background;

        private readonly ConcurrentDictionary<(Location, Protocol), (TimeSpan InsertionTime, EndpointList Endpoints, Location Location)> _locationCache =
            new (_locationComparer);

        private readonly Dictionary<(Location, Protocol), Task<(EndpointList, Location)>> _locationRequests =
             new (_locationComparer);

        private ILocatorRegistryPrx? _locatorRegistry;

        // _mutex protects _locationRequests and _wellKnownProxyRequests
        private readonly object _mutex = new ();

        private readonly ConcurrentDictionary<(Identity, string, Protocol), (TimeSpan InsertionTime, EndpointList Endpoints, Location Location)> _wellKnownProxyCache =
            new ();

        private readonly Dictionary<(Identity, string, Protocol), Task<(EndpointList, Location)>> _wellKnownProxyRequests =
            new ();

        internal LocatorInfo(ILocatorPrx locator, bool background)
        {
            Locator = locator;
            _background = background;
        }

        internal void ClearCache(Reference reference)
        {
            Debug.Assert(reference.IsIndirect);

            if (reference.IsWellKnown)
            {
                if (_wellKnownProxyCache.TryRemove(
                    (reference.Identity, reference.Facet, reference.Protocol),
                    out (TimeSpan _, EndpointList Endpoints, Location Location) entry))
                {
                    if (entry.Endpoints.Count > 0)
                    {
                        if (reference.Communicator.TraceLevels.Locator >= 2)
                        {
                            Trace("removed well-known proxy with endpoints from locator cache",
                                  reference,
                                  entry.Endpoints,
                                  entry.Location);
                        }
                    }
                    else
                    {
                        Debug.Assert(entry.Location.Count > 0);
                        if (reference.Communicator.TraceLevels.Locator >= 2)
                        {
                            Trace("removed well-known proxy without endpoints from locator cache",
                                  reference,
                                  entry.Location);
                        }

                        ClearCache(entry.Location, reference.Protocol, reference.Communicator);
                    }
                }
            }
            else
            {
                ClearCache(reference.Location, reference.Protocol, reference.Communicator);
            }
        }

        /// <summary>Resolves an indirect reference using the locator proxy or cache.</summary>
        internal async ValueTask<(EndpointList Endpoints, Location Location, bool Cached)> ResolveIndirectReferenceAsync(
            Reference reference,
            CancellationToken cancel)
        {
            Debug.Assert(reference.IsIndirect);

            EndpointList endpoints = ImmutableArray<Endpoint>.Empty;
            Location newLocation = ImmutableArray<string>.Empty;
            bool cached = false;

            Location locationToResolve = reference.Location;

            if (reference.IsWellKnown)
            {
                // First, we check the cache.
                (endpoints, newLocation, cached) = GetResolvedWellKnownProxyFromCache(reference,
                                                                                      reference.LocatorCacheTimeout);
                if (!cached)
                {
                    if (_background && (endpoints.Count > 0 || newLocation.Count > 0))
                    {
                        // Entry is returned from the cache but TTL was reached, if backgrounds updates are configured,
                        // we make a new resolution to refresh the cache but use the stale info to not block the caller.
                        _ = ResolveWellKnownProxyAsync(reference, cancel: default);
                    }
                    else
                    {
                        (endpoints, newLocation) =
                            await ResolveWellKnownProxyAsync(reference, cancel).ConfigureAwait(false);
                    }
                }

                locationToResolve = endpoints.Count == 0 ? newLocation : ImmutableArray<string>.Empty;
            }

            if (locationToResolve.Count > 0)
            {
                bool cachedLocation;

                (endpoints, newLocation, cachedLocation) =
                    GetResolvedLocationFromCache(locationToResolve, reference.Protocol, reference.LocatorCacheTimeout);

                if (!cachedLocation)
                {
                    if (_background && endpoints.Count > 0)
                    {
                        // Endpoints are returned from the cache but TTL was reached, if backgrounds updates
                        // are configured, we obtain new endpoints but continue using the stale endpoints to
                        // not block the caller.
                        _ = ResolveLocationAsync(locationToResolve,
                                                 reference.Protocol,
                                                 reference.Communicator,
                                                 cancel: default);
                    }
                    else
                    {
                        try
                        {
                            (endpoints, newLocation) =
                                await ResolveLocationAsync(locationToResolve,
                                                           reference.Protocol,
                                                           reference.Communicator,
                                                           cancel).ConfigureAwait(false);
                        }
                        finally
                        {
                            // If we can't resolve the location we clear the resolved well-known proxy from the cache.
                            if (endpoints.Count == 0 && reference.IsWellKnown)
                            {
                                ClearCache(reference);
                            }
                        }
                    }
                }

                if (!reference.IsWellKnown)
                {
                    cached = cachedLocation;
                }
            }

            if (reference.Communicator.TraceLevels.Locator >= 1)
            {
                if (endpoints.Count > 0)
                {
                    if (reference.IsWellKnown)
                    {
                        Trace(cached ? $"found entry for well-known proxy in locator cache" :
                                  $"resolved well-known proxy using locator, adding to locator cache",
                              reference,
                              endpoints,
                              newLocation);
                    }
                    else
                    {
                        Trace(cached ? $"found entry for location in locator cache" :
                                  $"resolved location using locator, adding to locator cache",
                              locationToResolve,
                              reference.Protocol,
                              endpoints,
                              newLocation,
                              reference.Communicator);
                    }
                }
                else
                {
                    Communicator communicator = reference.Communicator;
                    var sb = new System.Text.StringBuilder();
                    sb.Append("could not find endpoint(s) for ");
                    if (reference.Location.Count > 0)
                    {
                        sb.Append("location ");
                        sb.Append(reference.Location.ToLocationString());
                    }
                    else
                    {
                        sb.Append("well-known proxy ");
                        sb.Append(reference);
                    }
                    communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
                }
            }

            return (endpoints, newLocation, cached);
        }

        internal async ValueTask<ILocatorRegistryPrx?> GetLocatorRegistryAsync()
        {
            if (_locatorRegistry == null)
            {
                ILocatorRegistryPrx? registry = await Locator.GetRegistryAsync().ConfigureAwait(false);

                // The locator registry can't be located.
                _locatorRegistry = registry?.Clone(clearLocator: true);
            }
            return _locatorRegistry;
        }

        private static bool CheckTTL(TimeSpan insertionTime, TimeSpan ttl) =>
            ttl == Timeout.InfiniteTimeSpan || (Time.Elapsed - insertionTime) <= ttl;

        private static void Trace(
            string msg,
            Location location,
            Protocol protocol,
            EndpointList endpoints,
            Location newLocation,
            Communicator communicator)
        {
            var sb = new System.Text.StringBuilder(msg);
            sb.Append("\nlocation = ");
            sb.Append(location.ToLocationString());
            sb.Append("\nprotocol = ");
            sb.Append(protocol.GetName());
            sb.Append("\nendpoints = ");
            sb.AppendEndpointList(endpoints);
            if (protocol != Protocol.Ice1)
            {
                sb.Append("\nnew location = ");
                sb.Append(newLocation.ToLocationString());
            }
            communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private static void Trace(string msg, Reference wellKnownProxy, EndpointList endpoints, Location location)
        {
            var sb = new System.Text.StringBuilder(msg);
            sb.Append("\nwell-known proxy = ");
            sb.Append(wellKnownProxy);
            sb.Append("\nendpoints = ");
            sb.AppendEndpointList(endpoints);
            if (wellKnownProxy.Protocol != Protocol.Ice1)
            {
                sb.Append("\nlocation = ");
                sb.Append(location.ToLocationString());
            }
            wellKnownProxy.Communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private static void Trace(string msg, Reference wellKnownProxy, Location location)
        {
            var sb = new System.Text.StringBuilder(msg);
            sb.Append("\nwell-known proxy = ");
            sb.Append(wellKnownProxy);
            sb.Append("\nlocation = ");
            sb.Append(location.ToLocationString());
            wellKnownProxy.Communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private static void TraceInvalid(Location location, Location newLocation, Communicator communicator)
        {
            var sb = new System.Text.StringBuilder("locator returned only a location when resolving location ");
            sb.Append(location.ToLocationString());
            sb.Append("\n received = ");
            sb.Append(newLocation.ToLocationString());
            communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private static void TraceInvalid(Location location, Reference invalidReference)
        {
            var sb = new System.Text.StringBuilder("locator returned an invalid proxy when resolving location ");
            sb.Append(location.ToLocationString());
            sb.Append("\n received = ");
            sb.Append(invalidReference);
            invalidReference.Communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private static void TraceInvalid(Reference reference, Reference invalidReference)
        {
            var sb = new System.Text.StringBuilder("locator returned an invalid proxy when resolving ");
            sb.Append(reference);
            sb.Append("\n received = ");
            sb.Append(invalidReference);
            reference.Communicator.Logger.Trace(TraceLevels.LocatorCategory, sb.ToString());
        }

        private void ClearCache(Location location, Protocol protocol, Communicator communicator)
        {
            if (_locationCache.TryRemove(
                (location, protocol),
                out (TimeSpan _, EndpointList Endpoints, Location Location) entry))
            {
                if (communicator.TraceLevels.Locator >= 2)
                {
                    Trace("removed endpoints for location from locator cache",
                          location,
                          protocol,
                          entry.Endpoints,
                          entry.Location,
                          communicator);
                }
            }
        }

        private (EndpointList Endpoints, Location Location, bool Cached) GetResolvedLocationFromCache(
            Location location,
            Protocol protocol,
            TimeSpan ttl)
        {
            if (ttl != TimeSpan.Zero && _locationCache.TryGetValue(
                (location, protocol),
                out (TimeSpan InsertionTime, EndpointList Endpoints, Location Location) entry))
            {
                return (entry.Endpoints, entry.Location, CheckTTL(entry.InsertionTime, ttl));
            }
            else
            {
                return (ImmutableArray<Endpoint>.Empty, ImmutableArray<string>.Empty, false);
            }
        }

        private (EndpointList Endpoints, Location Location, bool Cached) GetResolvedWellKnownProxyFromCache(
            Reference reference,
            TimeSpan ttl)
        {
            if (ttl != TimeSpan.Zero &&
                _wellKnownProxyCache.TryGetValue(
                    (reference.Identity, reference.Facet, reference.Protocol),
                    out (TimeSpan InsertionTime, EndpointList Endpoints, Location Location) entry))
            {
                return (entry.Endpoints, entry.Location, CheckTTL(entry.InsertionTime, ttl));
            }
            else
            {
                return (ImmutableArray<Endpoint>.Empty, ImmutableArray<string>.Empty, false);
            }
        }

        private async Task<(EndpointList, Location)> ResolveLocationAsync(
            Location location,
            Protocol protocol,
            Communicator communicator,
            CancellationToken cancel)
        {
            if (communicator.TraceLevels.Locator > 0)
            {
                communicator.Logger.Trace(TraceLevels.LocatorCategory,
                    $"resolving location\nlocation = {location.ToLocationString()}");
            }

            Task<(EndpointList, Location)>? task;
            lock (_mutex)
            {
                if (!_locationRequests.TryGetValue((location, protocol), out task))
                {
                    // If there is no request in progress for this location, we invoke one and cache the request to
                    // prevent concurrent identical requests. It's removed once the response is received.
                    task = PerformResolveLocationAsync(location, protocol, communicator);
                    if (!task.IsCompleted)
                    {
                        // If PerformResolveLocationAsync completed, don't add the task (it would leak since
                        // PerformResolveLocationAsync is responsible for removing it).
                        // Since PerformResolveLocationAsync locks _mutex in its finally block, the only way it can be
                        // completed now is if completed synchronously.
                        _locationRequests.Add((location, protocol), task);
                    }
                }
            }

            return await task.WaitAsync(cancel).ConfigureAwait(false);

            async Task<(EndpointList, Location)> PerformResolveLocationAsync(
                Location location,
                Protocol protocol,
                Communicator communicator)
            {
                Debug.Assert(location.Count > 0);

                try
                {
                    EndpointList endpoints = ImmutableArray<Endpoint>.Empty;
                    Location newLocation = ImmutableArray<string>.Empty;

                    if (protocol == Protocol.Ice1)
                    {
                        IObjectPrx? proxy = null;
                        try
                        {
                            proxy = await Locator.FindAdapterByIdAsync(
                                location[0],
                                cancel: CancellationToken.None).ConfigureAwait(false);
                        }
                        catch (AdapterNotFoundException)
                        {
                            // We treat AdapterNotFoundException just like a null return value.
                            proxy = null;
                        }

                        Reference? resolved = proxy?.IceReference;

                        if (resolved != null && (resolved.IsIndirect || resolved.Protocol != Protocol.Ice1))
                        {
                            if (communicator.TraceLevels.Locator >= 1)
                            {
                                TraceInvalid(location, resolved);
                            }
                            resolved = null;
                        }

                        endpoints = resolved?.Endpoints ?? endpoints;
                    }
                    else
                    {
                        EndpointData[] dataArray;

                        // This will throw OperationNotExistException if it's an old Locator, and that's fine.
                        (dataArray, newLocation) = await Locator.ResolveLocationAsync(
                            location,
                            cancel: CancellationToken.None).ConfigureAwait(false);

                        if (dataArray.Length > 0)
                        {
                            endpoints = dataArray.ToEndpointList(communicator);
                        }
                        else if (newLocation.Count > 0)
                        {
                            TraceInvalid(location, newLocation, communicator);
                            newLocation = ImmutableArray<string>.Empty;
                        }
                    }

                    if (endpoints.Count == 0)
                    {
                        ClearCache(location, protocol, communicator);
                        return (endpoints, newLocation);
                    }
                    else
                    {
                        // Cache the resolved location
                        _locationCache[(location, protocol)] = (Time.Elapsed, endpoints, newLocation);
                        return (endpoints, newLocation);
                    }
                }
                catch (Exception exception)
                {
                    if (communicator.TraceLevels.Locator > 0)
                    {
                        communicator.Logger.Trace(
                            TraceLevels.LocatorCategory,
                            @$"could not contact the locator to resolve location `{location.ToLocationString()
                                }'\nreason = {exception}");
                    }
                    throw;
                }
                finally
                {
                    lock (_mutex)
                    {
                        _locationRequests.Remove((location, protocol));
                    }
                }
            }
        }

        private async Task<(EndpointList, Location)> ResolveWellKnownProxyAsync(
            Reference reference,
            CancellationToken cancel)
        {
            if (reference.Communicator.TraceLevels.Locator > 0)
            {
                reference.Communicator.Logger.Trace(TraceLevels.LocatorCategory,
                    $"searching for well-known object\nwell-known proxy = {reference}");
            }

            Task<(EndpointList, Location)>? task;
            lock (_mutex)
            {
                if (!_wellKnownProxyRequests.TryGetValue((reference.Identity, reference.Facet, reference.Protocol),
                                                         out task))
                {
                    // If there's no locator request in progress for this object, we make one and cache it to prevent
                    // making too many requests on the locator. It's removed once the locator response is received.
                    task = PerformResolveWellKnownProxyAsync(reference);
                    if (!task.IsCompleted)
                    {
                        // If PerformGetObjectProxyAsync completed, don't add the task (it would leak since
                        // PerformGetObjectProxyAsync is responsible for removing it).
                        _wellKnownProxyRequests.Add((reference.Identity, reference.Facet, reference.Protocol), task);
                    }
                }
            }

            return await task.WaitAsync(cancel).ConfigureAwait(false);

            async Task<(EndpointList, Location)> PerformResolveWellKnownProxyAsync(Reference reference)
            {
                try
                {
                    EndpointList endpoints;
                    Location location;

                    if (reference.Protocol == Protocol.Ice1)
                    {
                        IObjectPrx? proxy = null;
                        try
                        {
                            proxy = await Locator.FindObjectByIdAsync(
                                reference.Identity,
                                reference.Facet,
                                cancel: CancellationToken.None).ConfigureAwait(false);
                        }
                        catch (ObjectNotFoundException)
                        {
                            // We treat ObjectNotFoundException just like a null return value.
                            proxy = null;
                        }

                        Reference? resolved = proxy?.IceReference;

                        if (resolved != null && (resolved.IsWellKnown || resolved.Protocol != Protocol.Ice1))
                        {
                            if (reference.Communicator.TraceLevels.Locator >= 1)
                            {
                                TraceInvalid(reference, resolved);
                            }
                            resolved = null;
                        }

                        endpoints = resolved?.Endpoints ?? ImmutableArray<Endpoint>.Empty;
                        location = resolved?.Location ?? ImmutableArray<string>.Empty;
                    }
                    else
                    {
                        EndpointData[] dataArray;
                        (dataArray, location) = await Locator.ResolveWellKnownProxyAsync(
                            reference.Identity,
                            reference.Facet,
                            cancel: CancellationToken.None).ConfigureAwait(false);

                        endpoints = dataArray.ToEndpointList(reference.Communicator);
                    }

                    if (endpoints.Count == 0 && location.Count == 0)
                    {
                        ClearCache(reference);
                        return (endpoints, location);
                    }
                    else
                    {
                        _wellKnownProxyCache[(reference.Identity, reference.Facet, reference.Protocol)] =
                            (Time.Elapsed, endpoints, location);
                        return (endpoints, location);
                    }
                }
                catch (Exception exception)
                {
                    if (reference.Communicator.TraceLevels.Locator > 0)
                    {
                        reference.Communicator.Logger.Trace(
                            TraceLevels.LocatorCategory,
                            @$"could not contact the locator to retrieve endpoints for well-known proxy `{reference
                                }'\nreason = {exception}");
                    }
                    throw;
                }
                finally
                {
                    lock (_mutex)
                    {
                        _wellKnownProxyRequests.Remove((reference.Identity, reference.Facet, reference.Protocol));
                    }
                }
            }
        }

        private sealed class LocationComparer : IEqualityComparer<(Location Location, Protocol Protocol)>
        {
            public bool Equals(
                (Location Location, Protocol Protocol) lhs,
                (Location Location, Protocol Protocol) rhs) =>
                lhs.Location.SequenceEqual(rhs.Location) && lhs.Protocol == rhs.Protocol;

            public int GetHashCode((Location Location, Protocol Protocol) obj)
            {
                var hash = new HashCode();
                foreach (string s in obj.Location)
                {
                    hash.Add(s);
                }
                hash.Add(obj.Protocol);
                return hash.ToHashCode();
            }
        }
    }
}
