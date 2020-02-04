//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Ice.proxy.AMD.Test;

namespace Ice.proxy.AMD
{
    public class Server : global::Test.TestHelper
    {
        public override void run(string[] args)
        {
            var properties = createTestProperties(ref args);
            //
            // We don't want connection warnings because of the timeout test.
            //
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.Warn.Dispatch"] = "0";
            using var communicator = initialize(properties);
            communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
            var adapter = communicator.CreateObjectAdapter("TestAdapter");
            adapter.Add("test", new MyDerivedClass());
            adapter.Activate();
            serverReady();
            communicator.WaitForShutdown();
        }

        public static int Main(string[] args) => global::Test.TestDriver.runTest<Server>(args);
    }
}
