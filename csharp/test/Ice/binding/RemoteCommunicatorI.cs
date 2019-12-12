//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Ice.binding.Test;

namespace Ice
{
    namespace binding
    {
        public class RemoteCommunicatorI : Test.RemoteCommunicator
        {
            public Test.RemoteObjectAdapterPrx
            createObjectAdapter(string name, string endpts, Current current)
            {
                int retry = 5;
                while (true)
                {
                    try
                    {
                        Communicator communicator = current.Adapter.Communicator;
                        string endpoints = endpts;
                        if (endpoints.IndexOf("-p") < 0)
                        {
                            endpoints = global::Test.TestHelper.getTestEndpoint(communicator.Properties, _nextPort++, endpoints);
                        }

                        communicator.Properties.setProperty(name + ".ThreadPool.Size", "1");
                        ObjectAdapter adapter = communicator.createObjectAdapterWithEndpoints(name, endpoints);
                        return current.Adapter.Add(new RemoteObjectAdapterI(adapter));
                    }
                    catch (SocketException)
                    {
                        if (--retry == 0)
                        {
                            throw;
                        }
                    }
                }
            }

            public void
            deactivateObjectAdapter(Test.RemoteObjectAdapterPrx adapter, Ice.Current current)
            {
                adapter.deactivate(); // Collocated call.
            }

            public void
            shutdown(Ice.Current current)
            {
                current.Adapter.Communicator.shutdown();
            }

            private int _nextPort = 10;
        }
    }
}
