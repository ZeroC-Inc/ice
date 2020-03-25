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
        properties["Ice.Default.SlicedFormat"] = "1";
        using var communicator = initialize(properties);
        communicator.SetProperty("TestAdapter.Endpoints", $"{getTestEndpoint(0)} -t 2000");
        Ice.ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
        adapter.Add("Test", new TestIntf());
        adapter.Add("Test2", new TestIntf2());
        adapter.Activate();
        communicator.WaitForShutdown();
    }

    public static int Main(string[] args) => TestDriver.runTest<Server>(args);
}
