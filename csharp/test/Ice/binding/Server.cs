// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

using Test;

namespace Ice
{
    namespace binding
    {
        public class Server : TestHelper
        {
            public override void run(string[] args)
            {
                Ice.Properties properties = createTestProperties(ref args);
                properties.setProperty("Ice.ServerIdleTime", "30");
                using(var communicator = initialize(properties))
                {
                    communicator.getProperties().setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                    Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                    Ice.Identity id = Ice.Util.stringToIdentity("communicator");
                    adapter.add(new RemoteCommunicatorI(), id);
                    adapter.activate();
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
