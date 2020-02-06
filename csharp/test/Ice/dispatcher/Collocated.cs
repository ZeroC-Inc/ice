//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Reflection;
using Test;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Collocated : Test.TestHelper
{
    public override void run(string[] args)
    {
        try
        {
            var properties = createTestProperties(ref args);
            properties["Ice.Warn.AMICallback"] = "0";
            var dispatcher = new Dispatcher();

            using var communicator = initialize(properties, dispatcher.dispatch);

            communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
            communicator.SetProperty("ControllerAdapter.Endpoints", getTestEndpoint(1));
            communicator.SetProperty("ControllerAdapter.ThreadPool.Size", "1");

            Ice.ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            Ice.ObjectAdapter adapter2 = communicator.CreateObjectAdapter("ControllerAdapter");

            adapter.Add("test", new TestIntf());
            //adapter.activate(); // Don't activate OA to ensure collocation is used.
            adapter2.Add("testController", new TestController(adapter));
            //adapter2.activate(); // Don't activate OA to ensure collocation is used.

            AllTests.allTests(this);
        }
        finally
        {
            Dispatcher.terminate();
        }
    }

    public static int Main(string[] args)
    {
        return Test.TestDriver.runTest<Collocated>(args);
    }
}
