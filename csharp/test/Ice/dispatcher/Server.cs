//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Reflection;
using Test;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Server : Test.TestHelper
{
    public override void run(string[] args)
    {
        Ice.InitializationData initData = new Ice.InitializationData();
        initData.properties = createTestProperties(ref args);
        initData.properties.setProperty("Ice.ServerIdleTime", "30");
        //
        // Limit the recv buffer size, this test relies on the socket
        // send() blocking after sending a given amount of data.
        //
        initData.properties.setProperty("Ice.TCP.RcvSize", "50000");
        try
        {
            initData.dispatcher = new Dispatcher().dispatch;

            using (var communicator = initialize(initData))
            {
                communicator.Properties.setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                communicator.Properties.setProperty("ControllerAdapter.Endpoints", getTestEndpoint(1));
                communicator.Properties.setProperty("ControllerAdapter.ThreadPool.Size", "1");

                Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                Ice.ObjectAdapter adapter2 = communicator.createObjectAdapter("ControllerAdapter");

                adapter.Add(new TestI(), "test");
                adapter.Activate();
                adapter2.Add(new TestControllerI(adapter), "testController");
                adapter2.Activate();

                communicator.waitForShutdown();
            }
        }
        finally
        {
            Dispatcher.terminate();
        }
    }

    public static int Main(string[] args)
    {
        return Test.TestDriver.runTest<Server>(args);
    }
}
