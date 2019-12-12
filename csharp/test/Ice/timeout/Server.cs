//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Ice.timeout.Test;

namespace Ice
{
    namespace timeout
    {
        public class Server : global::Test.TestHelper
        {
            public override void run(string[] args)
            {
                var properties = createTestProperties(ref args);
                //
                // This test kills connections, so we don't want warnings.
                //
                properties.setProperty("Ice.Warn.Connections", "0");

                //
                // The client sends large messages to cause the transport
                // buffers to fill up.
                //
                properties.setProperty("Ice.MessageSizeMax", "20000");

                //
                // Limit the recv buffer size, this test relies on the socket
                // send() blocking after sending a given amount of data.
                //
                properties.setProperty("Ice.TCP.RcvSize", "50000");
                using (var communicator = initialize(properties))
                {
                    communicator.Properties.setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                    communicator.Properties.setProperty("ControllerAdapter.Endpoints", getTestEndpoint(1));
                    communicator.Properties.setProperty("ControllerAdapter.ThreadPool.Size", "1");

                    var adapter = communicator.createObjectAdapter("TestAdapter");
                    adapter.Add(new TimeoutI(), "timeout");
                    adapter.Activate();

                    var controllerAdapter = communicator.createObjectAdapter("ControllerAdapter");
                    controllerAdapter.Add(new ControllerI(adapter), "controller");
                    controllerAdapter.Activate();
                    serverReady();
                    communicator.waitForShutdown();
                }
            }

            public static int Main(string[] args)
            {
                return global::Test.TestDriver.runTest<Server>(args);
            }
        }
    }
}
