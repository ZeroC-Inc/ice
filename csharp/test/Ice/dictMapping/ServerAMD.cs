//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.dictMapping.AMD.Test;

namespace Ice
{
    namespace dictMapping
    {
        namespace AMD
        {
            public class Server : TestHelper
            {
                public override void run(string[] args)
                {
                    using (var communicator = initialize(ref args))
                    {
                        communicator.Properties.setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                        Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                        adapter.Add(new MyClassI(), "test");
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
}
