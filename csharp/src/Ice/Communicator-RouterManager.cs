//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using System.Diagnostics;

namespace ZeroC.Ice
{
    public sealed class RouterInfo
    {
        public interface IGetClientEndpointsCallback
        {
            void SetEndpoints(IReadOnlyList<Endpoint> endpoints);
            void SetException(System.Exception ex);
        }

        public interface IAddProxyCallback
        {
            void AddedProxy();
            void SetException(System.Exception ex);
        }

        internal RouterInfo(IRouterPrx router) => Router = router;

        public void Destroy()
        {
            lock (_mutex)
            {
                _clientEndpoints = System.Array.Empty<Endpoint>();
                _adapter = null;
                _identities.Clear();
            }
        }

        public override bool Equals(object? obj)
        {
            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            return obj is RouterInfo rhs && Router.Equals(rhs.Router);
        }

        public override int GetHashCode() => Router.GetHashCode();

        // No mutex lock necessary, _router is immutable.
        public IRouterPrx Router { get; }

        public IReadOnlyList<Endpoint> GetClientEndpoints()
        {
            lock (_mutex)
            {
                if (_clientEndpoints != null) // Lazy initialization.
                {
                    return _clientEndpoints;
                }
            }

            (IObjectPrx? proxy, bool? hasRoutingTable) = Router.GetClientProxy();
            return SetClientEndpoints(proxy!, hasRoutingTable ?? true);
        }

        public void GetClientEndpoints(IGetClientEndpointsCallback callback)
        {
            IReadOnlyList<Endpoint>? clientEndpoints = null;
            lock (_mutex)
            {
                clientEndpoints = _clientEndpoints;
            }

            if (clientEndpoints != null) // Lazy initialization.
            {
                callback.SetEndpoints(clientEndpoints);
                return;
            }

            Router.GetClientProxyAsync().ContinueWith(
                (t) =>
                {
                    try
                    {
                        (IObjectPrx? prx, bool? hasRoutingTable) = t.Result;
                        callback.SetEndpoints(SetClientEndpoints(prx!, hasRoutingTable ?? true));
                    }
                    catch (System.AggregateException ae)
                    {
                        callback.SetException(ae.InnerException!);
                    }
                },
                System.Threading.Tasks.TaskScheduler.Current);
        }

        public IReadOnlyList<Endpoint> GetServerEndpoints()
        {
            IObjectPrx? serverProxy = Router.GetServerProxy();
            if (serverProxy == null)
            {
                throw new InvalidConfigurationException($"router `{Router.Identity}' has no server endpoints");
            }

            serverProxy = serverProxy.Clone(clearRouter: true); // The server proxy cannot be routed.
            return serverProxy.IceReference.Endpoints;
        }

        public void AddProxy(IObjectPrx proxy)
        {
            Debug.Assert(proxy != null);
            lock (_mutex)
            {
                if (_identities.Contains(proxy.Identity))
                {
                    //
                    // Only add the proxy to the router if it's not already in our local map.
                    //
                    return;
                }
            }

            AddAndEvictProxies(proxy, Router.AddProxies(new IObjectPrx[] { proxy }) as IObjectPrx[]);
        }

        public bool AddProxy(IObjectPrx proxy, IAddProxyCallback callback)
        {
            Debug.Assert(proxy != null);
            lock (_mutex)
            {
                if (!_hasRoutingTable)
                {
                    return true; // The router implementation doesn't maintain a routing table.
                }
                if (_identities.Contains(proxy.Identity))
                {
                    //
                    // Only add the proxy to the router if it's not already in our local map.
                    //
                    return true;
                }
            }

            Router.AddProxiesAsync(new IObjectPrx[] { proxy }).ContinueWith(
                (t) =>
                {
                    try
                    {
                        AddAndEvictProxies(proxy, t.Result as IObjectPrx[]);
                        callback.AddedProxy();
                    }
                    catch (System.AggregateException ae)
                    {
                        callback.SetException(ae.InnerException!);
                    }
                },
                System.Threading.Tasks.TaskScheduler.Current);
            return false;
        }

        public ObjectAdapter? Adapter
        {
            get
            {
                lock (_mutex)
                {
                    return _adapter;
                }
            }
            set
            {
                lock (_mutex)
                {
                    _adapter = value;
                }
            }
        }

        public void ClearCache(Reference reference)
        {
            lock (_mutex)
            {
                _identities.Remove(reference.Identity);
            }
        }

        private IReadOnlyList<Endpoint> SetClientEndpoints(IObjectPrx clientProxy, bool hasRoutingTable)
        {
            lock (_mutex)
            {
                if (_clientEndpoints == null)
                {
                    _hasRoutingTable = hasRoutingTable;
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
                        if (Router.GetConnection() != null)
                        {
                            clientProxy = clientProxy.Clone(connectionTimeout: Router.GetConnection().Timeout);
                        }

                        _clientEndpoints = clientProxy.IceReference.Endpoints;
                    }
                }
                return _clientEndpoints;
            }
        }

        private void AddAndEvictProxies(IObjectPrx proxy, IObjectPrx[] evictedProxies)
        {
            lock (_mutex)
            {
                //
                // Check if the proxy hasn't already been evicted by a
                // concurrent addProxies call. If it's the case, don't
                // add it to our local map.
                //
                int index = _evictedIdentities.IndexOf(proxy.Identity);
                if (index >= 0)
                {
                    _evictedIdentities.RemoveAt(index);
                }
                else
                {
                    //
                    // If we successfully added the proxy to the router,
                    // we add it to our local map.
                    //
                    _identities.Add(proxy.Identity);
                }

                //
                // We also must remove whatever proxies the router evicted.
                //
                for (int i = 0; i < evictedProxies.Length; ++i)
                {
                    if (!_identities.Remove(evictedProxies[i].Identity))
                    {
                        //
                        // It's possible for the proxy to not have been
                        // added yet in the local map if two threads
                        // concurrently call addProxies.
                        //
                        _evictedIdentities.Add(evictedProxies[i].Identity);
                    }
                }
            }
        }

        private ObjectAdapter? _adapter;
        private IReadOnlyList<Endpoint>? _clientEndpoints;
        private readonly List<Identity> _evictedIdentities = new List<Identity>();
        private bool _hasRoutingTable;
        private readonly HashSet<Identity> _identities = new HashSet<Identity>();
        private readonly object _mutex = new object();
    }

    public sealed partial class Communicator
    {
        // Returns router info for a given router. Automatically creates
        // the router info if it doesn't exist yet.
        public RouterInfo? GetRouterInfo(IRouterPrx? rtr)
        {
            if (rtr == null)
            {
                return null;
            }

            //
            // The router cannot be routed.
            //
            IRouterPrx router = rtr.Clone(clearRouter: true);

            lock (_routerInfoTable)
            {
                if (!_routerInfoTable.TryGetValue(router, out RouterInfo? info))
                {
                    info = new RouterInfo(router);
                    _routerInfoTable.Add(router, info);
                }
                return info;
            }
        }

        //
        // Returns router info for a given router. Automatically creates
        // the router info if it doesn't exist yet.
        //
        public RouterInfo? EraseRouterInfo(IRouterPrx? rtr)
        {
            RouterInfo? info = null;
            if (rtr != null)
            {
                //
                // The router cannot be routed.
                //
                IRouterPrx router = rtr.Clone(clearRouter: true);

                lock (_routerInfoTable)
                {
                    if (_routerInfoTable.TryGetValue(router, out info))
                    {
                        _routerInfoTable.Remove(router);
                    }
                }
            }
            return info;
        }

        private readonly Dictionary<IRouterPrx, RouterInfo> _routerInfoTable = new Dictionary<IRouterPrx, RouterInfo>();
    }
}
