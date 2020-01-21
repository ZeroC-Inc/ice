//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;

namespace IceInternal
{
    public class RequestHandlerFactory
    {
        internal RequestHandlerFactory(Ice.Communicator communicator) => _communicator = communicator;

        internal IRequestHandler
        getRequestHandler(RoutableReference rf, Ice.IObjectPrx proxy)
        {
            if (rf.getCollocationOptimized())
            {
                Ice.ObjectAdapter adapter = _communicator.ObjectAdapterFactory().findObjectAdapter(proxy);
                if (adapter != null)
                {
                    return proxy.IceSetRequestHandler(new CollocatedRequestHandler(rf, adapter));
                }
            }

            bool connect = false;
            ConnectRequestHandler handler;
            if (rf.getCacheConnection())
            {
                lock (this)
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

        internal void
        removeRequestHandler(Reference rf, IRequestHandler handler)
        {
            if (rf.getCacheConnection())
            {
                lock (this)
                {
                    ConnectRequestHandler h;
                    if (_handlers.TryGetValue(rf, out h) && h == handler)
                    {
                        _handlers.Remove(rf);
                    }
                }
            }
        }

        private readonly Ice.Communicator _communicator;
        private readonly Dictionary<Reference, ConnectRequestHandler> _handlers =
            new Dictionary<Reference, ConnectRequestHandler>();
    }
}
