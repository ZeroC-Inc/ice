//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Reflection;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        var properties = createTestProperties(ref args);
        properties["Ice.Default.Host"] = "127.0.0.1";
        using var communicator = initialize(properties);
        AllTests.allTests(this);
        //
        // Shutdown the IceBox server.
        //
        Ice.IProcessPrx.Parse("DemoIceBox/admin -f Process:default -p 9996", communicator).Shutdown();
    }

    public static int Main(string[] args) => Test.TestDriver.runTest<Client>(args);
}
