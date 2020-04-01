//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Ice.timeout.Test;

namespace Ice.timeout
{
    public class Server : global::Test.TestHelper
    {
        public override void run(string[] args)
        {
            var properties = createTestProperties(ref args);
            //
            // This test kills connections, so we don't want warnings.
            //
            properties["Ice.Warn.Connections"] = "0";

            //
            // The client sends large messages to cause the transport
            // buffers to fill up.
            //
            properties["Ice.MessageSizeMax"] = "20000";

            //
            // Limit the recv buffer size, this test relies on the socket
            // send() blocking after sending a given amount of data.
            //
            properties["Ice.TCP.RcvSize"] = "50000";
            using var communicator = initialize(properties);
            communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
            communicator.SetProperty("ControllerAdapter.Endpoints", getTestEndpoint(1));
            communicator.SetProperty("ControllerAdapter.ThreadPool.Size", "1");

            var controllerAdapter = communicator.CreateObjectAdapter("ControllerAdapter");
            controllerAdapter.Add("controller", new Controller(communicator));
            controllerAdapter.Activate();

            serverReady();
            communicator.WaitForShutdown();
        }

        public static int Main(string[] args) => global::Test.TestDriver.runTest<Server>(args);
    }
}
