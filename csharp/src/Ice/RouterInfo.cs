//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    /// <summary>The router info class caches information specific to a given router. The communicator holds a router
    /// info instance per router proxy set either with Ice.Default.Router or the proxy's Router property. It caches
    /// the router's server and client proxies as well as proxies that were added to the router's routing table (when
    /// a routed proxy obtains a new connection request handler (in Reference.GetConnectionRequestHandlerAsync). If an
    /// object adapter is created with the router, this class also keeps track of this object adapter. This is used to
    /// associate connections established from routed proxies to the object adapter to allow bi-dir communications.
    /// TODO: review how we handled routers. The communicator holds a per-proxy router info table right now, should
    /// it be per-router identity instead? Also fix #228.</summary>
    internal sealed class RouterInfo
    {
        public IRouterPrx Router { get; }
        public ObjectAdapter? Adapter
        {
            get => _adapter;
            set => _adapter = value;
        }
        private volatile ObjectAdapter? _adapter;
        private IReadOnlyList<Endpoint>? _clientEndpoints;
        private readonly List<Identity> _evictedIdentities = new List<Identity>();
        private bool _hasRoutingTable;
        private readonly HashSet<Identity> _identities = new HashSet<Identity>();
        private readonly object _mutex = new object();

        internal RouterInfo(IRouterPrx router) => Router = router;

        internal async ValueTask AddProxyAsync(IObjectPrx proxy)
        {
            Debug.Assert(proxy != null);
            lock (_mutex)
            {
                if (!_hasRoutingTable)
                {
                    // The router implementation doesn't maintain a routing table.
                    return;
                }
                if (_identities.Contains(proxy.Identity))
                {
                    // Only add the proxy to the router if it's not already in our local map.
                    return;
                }
            }

            // TODO: fix the Slice method addProxies to return non-nullable proxies.
            IObjectPrx?[] evictedProxies = await Router.AddProxiesAsync(new IObjectPrx[] { proxy });

            lock (_mutex)
            {
                // Check if the proxy hasn't already been evicted by a concurrent addProxies call. If it's the case,
                // don't add it to our local map.
                int index = _evictedIdentities.IndexOf(proxy.Identity);
                if (index >= 0)
                {
                    _evictedIdentities.RemoveAt(index);
                }
                else
                {
                    // If we successfully added the proxy to the router, we add it to our local map.
                    _identities.Add(proxy.Identity);
                }

                // We also must remove whatever proxies the router evicted.
                for (int i = 0; i < evictedProxies.Length; ++i)
                {
                    if (evictedProxies[i] != null)
                    {
                        if (!_identities.Remove(evictedProxies[i]!.Identity))
                        {
                            // It's possible for the proxy to not have been added yet in the local map if two threads
                            // concurrently call addProxies.
                            _evictedIdentities.Add(evictedProxies[i]!.Identity);
                        }
                    }
                }
            }
        }

        internal void ClearCache(Reference reference)
        {
            lock (_mutex)
            {
                _identities.Remove(reference.Identity);
            }
        }

        internal async ValueTask<IReadOnlyList<Endpoint>> GetClientEndpointsAsync()
        {
            lock (_mutex)
            {
                if (_clientEndpoints != null) // Lazy initialization.
                {
                    return _clientEndpoints;
                }
            }

            (IObjectPrx? clientProxy, bool? hasRoutingTable) = await Router.GetClientProxyAsync().ConfigureAwait(false);

            lock (_mutex)
            {
                if (_clientEndpoints == null)
                {
                    _hasRoutingTable = hasRoutingTable ?? true;
                    if (clientProxy == null)
                    {
                        //
                        // If getClientProxy() return nil, use router endpoints.
                        //
                        _clientEndpoints = Router.IceReference.Endpoints;
                    }
                    else
                    {
                        clientProxy = clientProxy.Clone(clearRouter: true); // The client proxy cannot be routed.

                        //
                        // In order to avoid creating a new connection to the
                        // router, we must use the same timeout as the already
                        // existing connection.
                        //
                        Connection? connection = Router.GetConnection();
                        if (connection != null)
                        {
                            clientProxy = clientProxy.Clone(connectionTimeout: connection.Timeout);
                        }

                        _clientEndpoints = clientProxy.IceReference.Endpoints;
                    }
                }
                return _clientEndpoints;
            }
        }

        internal IReadOnlyList<Endpoint> GetServerEndpoints()
        {
            IObjectPrx? serverProxy = Router.GetServerProxy();
            if (serverProxy == null)
            {
                throw new InvalidConfigurationException($"router `{Router.Identity}' has no server endpoints");
            }

            return serverProxy.IceReference.Endpoints;
        }
    }
}
