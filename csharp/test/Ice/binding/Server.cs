//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.binding.Test;

namespace Ice
{
    namespace binding
    {
        public class Server : TestHelper
        {
            public override void run(string[] args)
            {
                Properties properties = createTestProperties(ref args);
                properties.setProperty("Ice.ServerIdleTime", "30");
                using (var communicator = initialize(properties))
                {
                    communicator.Properties.setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                    ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                    adapter.Add(new RemoteCommunicatorI(), "communicator");
                    adapter.Activate();
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
