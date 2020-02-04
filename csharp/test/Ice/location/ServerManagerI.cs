//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections;
using Ice.location.Test;

namespace Ice.location
{
    public class ServerManager : IServerManager
    {
        internal ServerManager(ServerLocatorRegistry registry, global::Test.TestHelper helper)
        {
            _registry = registry;
            _communicators = new ArrayList();
            _helper = helper;
        }

        public void startServer(Current current)
        {
            foreach (Communicator c in _communicators)
            {
                c.WaitForShutdown();
                c.Destroy();
            }
            _communicators.Clear();

            //
            // Simulate a server: create a new communicator and object
            // adapter. The object adapter is started on a system allocated
            // port. The configuration used here contains the Ice.Locator
            // configuration variable. The new object adapter will register
            // its endpoints with the locator and create references containing
            // the adapter id instead of the endpoints.
            //
            var properties = _helper.communicator().GetProperties();
            properties["TestAdapter.AdapterId"] = "TestAdapter";
            properties["TestAdapter.ReplicaGroupId"] = "ReplicatedAdapter";
            properties["TestAdapter2.AdapterId"] = "TestAdapter2";

            Communicator serverCommunicator = _helper.initialize(properties);
            _communicators.Add(serverCommunicator);

            //
            // Use fixed port to ensure that OA re-activation doesn't re-use previous port from
            // another OA(e.g.: TestAdapter2 is re-activated using port of TestAdapter).
            //
            int nRetry = 10;
            while (--nRetry > 0)
            {
                ObjectAdapter? adapter = null;
                ObjectAdapter? adapter2 = null;
                try
                {
                    serverCommunicator.SetProperty("TestAdapter.Endpoints",
                                                                _helper.getTestEndpoint(_nextPort++));
                    serverCommunicator.SetProperty("TestAdapter2.Endpoints",
                                                                _helper.getTestEndpoint(_nextPort++));

                    adapter = serverCommunicator.CreateObjectAdapter("TestAdapter");
                    adapter2 = serverCommunicator.CreateObjectAdapter("TestAdapter2");

                    var locator = ILocatorPrx.Parse($"locator:{_helper.getTestEndpoint(0)}", serverCommunicator);
                    adapter.SetLocator(locator);
                    adapter2.SetLocator(locator);

                    var testI = new TestIntf(adapter, adapter2, _registry);
                    _registry.addObject(adapter.Add("test", testI, Ice.IObjectPrx.Factory));
                    _registry.addObject(adapter.Add("test2", testI, Ice.IObjectPrx.Factory));
                    adapter.Add("test3", testI);

                    adapter.Activate();
                    adapter2.Activate();
                    break;
                }
                catch (SocketException ex)
                {
                    if (nRetry == 0)
                    {
                        throw ex;
                    }

                    // Retry, if OA creation fails with EADDRINUSE(this can occur when running with JS web
                    // browser clients if the driver uses ports in the same range as this test, ICE-8148)
                    if (adapter != null)
                    {
                        adapter.Destroy();
                    }
                    if (adapter2 != null)
                    {
                        adapter2.Destroy();
                    }
                }
            }
        }

        public void shutdown(Current current)
        {
            foreach (Communicator c in _communicators)
            {
                c.Destroy();
            }
            _communicators.Clear();
            current.Adapter.Communicator.Shutdown();
        }

        private ServerLocatorRegistry _registry;
        private ArrayList _communicators;
        private global::Test.TestHelper _helper;
        private int _nextPort = 1;
    }
}
