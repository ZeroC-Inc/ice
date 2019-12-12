//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.ami.Test;
using Ice.ami.Test.Outer;
using Ice.ami.Test.Outer.Inner;

namespace Ice
{
    namespace ami
    {
        public class Server : TestHelper
        {
            public override void run(string[] args)
            {
                Ice.Properties properties = createTestProperties(ref args);

                //
                // Disable collocation optimization to test async/await dispatch.
                //
                properties.setProperty("Ice.Default.CollocationOptimized", "0");

                //
                // This test kills connections, so we don't want warnings.
                //
                properties.setProperty("Ice.Warn.Connections", "0");

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

                    Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                    Ice.ObjectAdapter adapter2 = communicator.createObjectAdapter("ControllerAdapter");

                    adapter.Add(new TestI(), "test");
                    adapter.Add(new TestII(), "test2");
                    adapter.Activate();
                    adapter2.Add(new TestControllerI(adapter), "testController");
                    adapter2.Activate();
                    serverReady();
                    communicator.waitForShutdown();
                }
            }

            public static int Main(string[] args)
            {
                return TestDriver.runTest<Server>(args);
            }
        }
    }
}
