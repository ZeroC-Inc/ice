//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;

public class Server : TestHelper
{
    public override void run(string[] args)
    {
        var properties = createTestProperties(ref args);
        properties["Ice.Warn.Dispatch"] = "0";
        using var communicator = initialize(properties);
        communicator.SetProperty("TestAdapter.Endpoints", $"{getTestEndpoint(0)} -t 2000");
        Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
        adapter.Add(new TestIntf(), "Test");
        adapter.Activate();
        communicator.waitForShutdown();
    }

    public static int Main(string[] args) => TestDriver.runTest<Server>(args);
}
