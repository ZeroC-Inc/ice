// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

using ZeroC.Ice;

namespace ZeroC.IceDiscovery
{
    internal class Locator : ILocator
    {
        private readonly Lookup _lookup;
        private readonly ILocatorRegistryPrx _registry;

        public ValueTask<IObjectPrx?> FindAdapterByIdAsync(
            string adapterId,
            Current current,
            CancellationToken cancel) =>
            _lookup.FindAdapterAsync(adapterId);

        public ValueTask<IObjectPrx?> FindObjectByIdAsync(
            Identity id,
            Current current,
            CancellationToken cancel) =>
            _lookup.FindObjectAsync(id);

        public ILocatorRegistryPrx? GetRegistry(Current current, CancellationToken cancel) => _registry;

        internal Locator(Lookup lookup, ILocatorRegistryPrx registry)
        {
            _lookup = lookup;
            _registry = registry;
        }
    }

    internal class LocatorRegistry : ILocatorRegistry
    {
        private readonly Dictionary<string, IObjectPrx> _adapters = new ();
        private readonly object _mutex = new ();
        private readonly Dictionary<string, HashSet<string>> _replicaGroups = new ();

        public ValueTask SetAdapterDirectProxyAsync(
            string adapterId,
            IObjectPrx? proxy,
            Current current,
            CancellationToken cancel)
        {
            lock (_mutex)
            {
                if (proxy != null)
                {
                    _adapters[adapterId] = proxy.Clone(clearLocator: true, clearRouter: true);
                }
                else
                {
                    _adapters.Remove(adapterId);
                }
            }
            return default;
        }

        public ValueTask SetReplicatedAdapterDirectProxyAsync(
            string adapterId,
            string replicaGroupId,
            IObjectPrx? proxy,
            Current current,
            CancellationToken cancel)
        {
            lock (_mutex)
            {
                HashSet<string>? adapterIds;
                if (proxy != null)
                {
                    if (_replicaGroups.TryGetValue(replicaGroupId, out adapterIds))
                    {
                        if (_adapters.TryGetValue(adapterIds.First(), out IObjectPrx? registeredProxy) &&
                            registeredProxy.Protocol != proxy.Protocol)
                        {
                            throw new InvalidProxyException(
                                $"The proxy protocol {proxy.Protocol} doesn't match the replica group protocol");
                        }
                    }
                    else
                    {
                        adapterIds = new HashSet<string>();
                        _replicaGroups.Add(replicaGroupId, adapterIds);
                    }
                    _adapters[adapterId] = proxy.Clone(clearLocator: true, clearRouter: true);
                    adapterIds.Add(adapterId);
                }
                else
                {
                    _adapters.Remove(adapterId);
                    if (_replicaGroups.TryGetValue(replicaGroupId, out adapterIds))
                    {
                        adapterIds.Remove(adapterId);
                        if (adapterIds.Count == 0)
                        {
                            _replicaGroups.Remove(replicaGroupId);
                        }
                    }
                }
            }
            return default;
        }

        public ValueTask SetServerProcessProxyAsync(
            string id,
            IProcessPrx process,
            Current current,
            CancellationToken cancel) => default;

        internal (IObjectPrx? Proxy, bool IsReplicaGroup) FindAdapter(string adapterId)
        {
            lock (_mutex)
            {
                if (_adapters.TryGetValue(adapterId, out IObjectPrx? result))
                {
                    return (result, false);
                }

                if (_replicaGroups.TryGetValue(adapterId, out HashSet<string>? adapterIds))
                {
                    var endpoints = new List<Endpoint>();
                    Debug.Assert(adapterIds.Count > 0);
                    foreach (string id in adapterIds)
                    {
                        if (!_adapters.TryGetValue(id, out IObjectPrx? proxy))
                        {
                            continue; // TODO: Inconsistency
                        }
                        result ??= proxy;

                        endpoints.AddRange(proxy.Endpoints);
                    }

                    return (result?.Clone(endpoints: endpoints), result != null);
                }

                return (null, false);
            }
        }

        internal IObjectPrx? FindObject(Identity identity)
        {
            lock (_mutex)
            {
                if (identity.Name.Length == 0)
                {
                    return null;
                }

                foreach ((string key, HashSet<string> ids) in _replicaGroups)
                {
                    try
                    {
                        // We retrieve and clone this proxy _only_ for its protocol and encoding. All the other
                        // information in the proxy is wiped out or replaced.

                        IObjectPrx proxy = _adapters[ids.First()];
                        proxy = proxy.Clone(IObjectPrx.Factory,
                                            endpoints: ImmutableArray<Endpoint>.Empty,
                                            identity: identity,
                                            location: ImmutableArray.Create(key));
                        proxy.IcePing();
                        return proxy;
                    }
                    catch
                    {
                        // Ignore.
                    }
                }

                foreach ((string key, IObjectPrx registeredProxy) in _adapters)
                {
                    try
                    {
                        IObjectPrx proxy = registeredProxy.Clone(IObjectPrx.Factory,
                                                                 endpoints: ImmutableArray<Endpoint>.Empty,
                                                                 identity: identity,
                                                                 location: ImmutableArray.Create(key));
                        proxy.IcePing();
                        return proxy;
                    }
                    catch
                    {
                        // Ignore.
                    }
                }
                return null;
            }
        }
    }
}
