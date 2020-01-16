//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Reflection;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        var observer = new CommunicatorObserver();

        var properties = createTestProperties(ref args);
        properties["Ice.Admin.Endpoints"] = "tcp";
        properties["Ice.Admin.InstanceName"] = "client";
        properties["Ice.Admin.DelayCreation"] = "1";
        properties["Ice.Warn.Connections"] = "0";
        properties["Ice.Default.Host"] = "127.0.0.1";

        using var communicator = initialize(properties, observer: observer);
        Test.IMetricsPrx metrics = AllTests.allTests(this, observer);
        metrics.shutdown();
    }

    public static int Main(string[] args) => Test.TestDriver.runTest<Client>(args);
}
