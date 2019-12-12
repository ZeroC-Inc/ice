//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Reflection;
using Test;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Server : TestHelper
{
    public override void run(string[] args)
    {
        using (var communicator = initialize(ref args))
        {
            communicator.Properties.parseCommandLineOptions("TestAdapter", args);
            Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
            adapter.Add(new TestI(), communicator.Properties.getPropertyWithDefault("Identity", "test"));
            try
            {
                adapter.Activate();
            }
            catch (Ice.ObjectAdapterDeactivatedException)
            {
            }
            communicator.waitForShutdown();
        }
    }

    public static int Main(string[] args)
    {
        return Test.TestDriver.runTest<Server>(args);
    }
}
