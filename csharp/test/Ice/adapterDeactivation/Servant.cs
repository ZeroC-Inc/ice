//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.AdapterDeactivation
{
    public sealed class Router : IRouter
    {
        private int _nextPort = 23456;

        public (IObjectPrx?, bool?) GetClientProxy(Current current) => (null, false);

        public IObjectPrx GetServerProxy(Current current) =>
            IObjectPrx.Parse(TestHelper.GetTestProtocol(current.Communicator.GetProperties()) == Protocol.Ice1 ?
                $"dummy:tcp -h localhost -p {_nextPort++}" : $"ice+tcp://localhost:{_nextPort++}/dummy",
                current.Communicator);

        public IEnumerable<IObjectPrx?> AddProxies(IObjectPrx?[] proxies, Current current) =>
            Array.Empty<IObjectPrx?>();
    }

    public sealed class Servant : IObject
    {
        private readonly IRouter _router = new Router();

        public ValueTask<OutgoingResponseFrame> DispatchAsync(IncomingRequestFrame request, Current current)
        {
            IObject? servant;
            if (current.Identity.Name == "router")
            {
                servant = _router;
            }
            else
            {
                TestHelper.Assert(current.Identity.Category.Length == 0);
                TestHelper.Assert(current.Identity.Name == "test");
                servant = new TestIntf();
            }
            return servant.DispatchAsync(request, current);
        }
    }
}
