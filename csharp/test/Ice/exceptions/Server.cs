//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.exceptions.Test;

namespace Ice.exceptions
{
    public sealed class DummyLogger : ILogger
    {
        public void print(string message)
        {
        }

        public void trace(string category, string message)
        {
        }

        public void warning(string message)
        {
        }

        public void error(string message)
        {
        }

        public string getPrefix() => "";

        public ILogger cloneWithPrefix(string prefix) => new DummyLogger();
    }

    public class Server : TestHelper
    {
        public override void run(string[] args)
        {
            var properties = createTestProperties(ref args);
            properties["Ice.Warn.Dispatch"] = "0";
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.MessageSizeMax"] = "10"; // 10KB max
            using var communicator = initialize(properties);
            communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
            communicator.SetProperty("TestAdapter2.Endpoints", getTestEndpoint(1));
            communicator.SetProperty("TestAdapter2.MessageSizeMax", "0");
            communicator.SetProperty("TestAdapter3.Endpoints", getTestEndpoint(2));
            communicator.SetProperty("TestAdapter3.MessageSizeMax", "1");

            ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
            ObjectAdapter adapter2 = communicator.createObjectAdapter("TestAdapter2");
            ObjectAdapter adapter3 = communicator.createObjectAdapter("TestAdapter3");
            var obj = new Thrower();
            adapter.Add(obj, "thrower");
            adapter2.Add(obj, "thrower");
            adapter3.Add(obj, "thrower");
            adapter.Activate();
            adapter2.Activate();
            adapter3.Activate();
            serverReady();
            communicator.waitForShutdown();
        }

        public static int Main(string[] args) => TestDriver.runTest<Server>(args);
    }
}
