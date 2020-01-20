//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace IceInternal
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using Ice;

    public sealed class RouterInfo
    {
        public interface GetClientEndpointsCallback
        {
            void setEndpoints(Endpoint[] endpoints);
            void setException(Ice.LocalException ex);
        }

        public interface AddProxyCallback
        {
            void addedProxy();
            void setException(Ice.LocalException ex);
        }

        internal RouterInfo(IRouterPrx router)
        {
            _router = router;

            Debug.Assert(_router != null);
        }

        public void destroy()
        {
            lock (this)
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

            RouterInfo? rhs = obj as RouterInfo;
            return rhs == null ? false : _router.Equals(rhs._router);
        }

        public override int GetHashCode()
        {
            return _router.GetHashCode();
        }

        public IRouterPrx getRouter()
        {
            //
            // No mutex lock necessary, _router is immutable.
            //
            return _router;
        }

        public Endpoint[] getClientEndpoints()
        {
            lock (this)
            {
                if (_clientEndpoints != null) // Lazy initialization.
                {
                    return _clientEndpoints;
                }
            }

            var (proxy, hasRoutingTable) = _router.GetClientProxy();
            return setClientEndpoints(proxy, hasRoutingTable.HasValue ? hasRoutingTable.Value : true);
        }

        public void getClientEndpoints(GetClientEndpointsCallback callback)
        {
            Endpoint[]? clientEndpoints = null;
            lock (this)
            {
                clientEndpoints = _clientEndpoints;
            }

            if (clientEndpoints != null) // Lazy initialization.
            {
                callback.setEndpoints(clientEndpoints);
                return;
            }

            _router.GetClientProxyAsync().ContinueWith(
                (t) =>
                {
                    try
                    {
                        var r = t.Result;
                        callback.setEndpoints(setClientEndpoints(r.ReturnValue,
                            r.HasRoutingTable.HasValue ? r.HasRoutingTable.Value : true));
                    }
                    catch (System.AggregateException ae)
                    {
                        Debug.Assert(ae.InnerException is LocalException);
                        callback.setException((LocalException)ae.InnerException);
                    }
                },
                System.Threading.Tasks.TaskScheduler.Current);
        }

        public Endpoint[] getServerEndpoints()
        {
            Ice.IObjectPrx serverProxy = _router.GetServerProxy();
            if (serverProxy == null)
            {
                throw new Ice.NoEndpointException();
            }

            serverProxy = serverProxy.Clone(clearRouter: true); // The server proxy cannot be routed.
            return serverProxy.IceReference.getEndpoints();
        }

        public void addProxy(Ice.IObjectPrx proxy)
        {
            Debug.Assert(proxy != null);
            lock (this)
            {
                if (_identities.Contains(proxy.Identity))
                {
                    //
                    // Only add the proxy to the router if it's not already in our local map.
                    //
                    return;
                }
            }

            addAndEvictProxies(proxy, _router.AddProxies(new IObjectPrx[] { proxy }));
        }

        public bool addProxy(IObjectPrx proxy, AddProxyCallback callback)
        {
            Debug.Assert(proxy != null);
            lock (this)
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

            _router.AddProxiesAsync(new IObjectPrx[] { proxy }).ContinueWith(
                (t) =>
                {
                    try
                    {
                        addAndEvictProxies(proxy, t.Result);
                        callback.addedProxy();
                    }
                    catch (System.AggregateException ae)
                    {
                        Debug.Assert(ae.InnerException is LocalException);
                        callback.setException((LocalException)ae.InnerException);
                    }
                },
                System.Threading.Tasks.TaskScheduler.Current);
            return false;
        }

        public void setAdapter(ObjectAdapter? adapter)
        {
            lock (this)
            {
                _adapter = adapter;
            }
        }

        public ObjectAdapter? getAdapter()
        {
            lock (this)
            {
                return _adapter;
            }
        }

        public void clearCache(Reference @ref)
        {
            lock (this)
            {
                _identities.Remove(@ref.getIdentity());
            }
        }

        private Endpoint[] setClientEndpoints(Ice.IObjectPrx clientProxy, bool hasRoutingTable)
        {
            lock (this)
            {
                if (_clientEndpoints == null)
                {
                    _hasRoutingTable = hasRoutingTable;
                    if (clientProxy == null)
                    {
                        //
                        // If getClientProxy() return nil, use router endpoints.
                        //
                        _clientEndpoints = _router.IceReference.getEndpoints();
                    }
                    else
                    {
                        clientProxy = clientProxy.Clone(clearRouter: true); // The client proxy cannot be routed.

                        //
                        // In order to avoid creating a new connection to the
                        // router, we must use the same timeout as the already
                        // existing connection.
                        //
                        if (_router.GetConnection() != null)
                        {
                            clientProxy = clientProxy.Clone(connectionTimeout: _router.GetConnection().Timeout);
                        }

                        _clientEndpoints = clientProxy.IceReference.getEndpoints();
                    }
                }
                return _clientEndpoints;
            }
        }

        private void addAndEvictProxies(Ice.IObjectPrx proxy, Ice.IObjectPrx[] evictedProxies)
        {
            lock (this)
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

        private readonly IRouterPrx _router;
        private Endpoint[]? _clientEndpoints;
        private ObjectAdapter? _adapter;
        private HashSet<Identity> _identities = new HashSet<Identity>();
        private List<Identity> _evictedIdentities = new List<Identity>();
        private bool _hasRoutingTable;
    }

    public sealed class RouterManager
    {
        internal RouterManager()
        {
            _table = new Dictionary<IRouterPrx, RouterInfo>();
        }

        internal void destroy()
        {
            lock (this)
            {
                foreach (RouterInfo i in _table.Values)
                {
                    i.destroy();
                }
                _table.Clear();
            }
        }

        //
        // Returns router info for a given router. Automatically creates
        // the router info if it doesn't exist yet.
        //
        public RouterInfo get(IRouterPrx rtr)
        {
            //
            // The router cannot be routed.
            //
            IRouterPrx router = rtr.Clone(clearRouter: true);

            lock (this)
            {
                RouterInfo info;
                if (!_table.TryGetValue(router, out info))
                {
                    info = new RouterInfo(router);
                    _table.Add(router, info);
                }
                return info;
            }
        }

        //
        // Returns router info for a given router. Automatically creates
        // the router info if it doesn't exist yet.
        //
        public RouterInfo? erase(IRouterPrx? rtr)
        {
            RouterInfo? info = null;
            if (rtr != null)
            {
                //
                // The router cannot be routed.
                //
                IRouterPrx router = rtr.Clone(clearRouter: true);

                lock (this)
                {
                    if (_table.TryGetValue(router, out info))
                    {
                        _table.Remove(router);
                    }
                }
            }
            return info;
        }

        private Dictionary<IRouterPrx, RouterInfo> _table;
    }

}
