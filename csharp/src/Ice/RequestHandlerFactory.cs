//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using IceInternal;

namespace Ice
{
    public sealed partial class Communicator
    {
        internal IRequestHandler GetRequestHandler(RoutableReference rf, IObjectPrx proxy)
        {
            if (rf.getCollocationOptimized())
            {
                ObjectAdapter? adapter = FindObjectAdapter(proxy);
                if (adapter != null)
                {
                    return proxy.IceSetRequestHandler(new CollocatedRequestHandler(rf, adapter));
                }
            }

            bool connect = false;
            ConnectRequestHandler handler;
            if (rf.getCacheConnection())
            {
                lock (_handlers)
                {
                    if (!_handlers.TryGetValue(rf, out handler))
                    {
                        handler = new ConnectRequestHandler(rf, proxy);
                        _handlers.Add(rf, handler);
                        connect = true;
                    }
                }
            }
            else
            {
                handler = new ConnectRequestHandler(rf, proxy);
                connect = true;
            }

            if (connect)
            {
                rf.getConnection(handler);
            }
            return proxy.IceSetRequestHandler(handler.connect(proxy));
        }

        internal void RemoveRequestHandler(Reference rf, IRequestHandler handler)
        {
            if (rf.getCacheConnection())
            {
                lock (_handlers)
                {
                    ConnectRequestHandler h;
                    if (_handlers.TryGetValue(rf, out h) && h == handler)
                    {
                        _handlers.Remove(rf);
                    }
                }
            }
        }

        private readonly Dictionary<Reference, ConnectRequestHandler> _handlers =
            new Dictionary<Reference, ConnectRequestHandler>();
    }
}
