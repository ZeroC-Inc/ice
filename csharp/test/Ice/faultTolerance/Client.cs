//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Reflection;
using System.Linq;
using System.Collections.Generic;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        var properties = createTestProperties(ref args);
        properties["Ice.Warn.Connections"] = "0";
        using var communicator = initialize(properties);
        List<int> ports = args.Select(v => int.Parse(v)).ToList();
        if (ports.Count == 0)
        {
            throw new ArgumentException("Client: no ports specified");
        }
        AllTests.allTests(this, ports);
    }

    public static int Main(string[] args) => Test.TestDriver.runTest<Client>(args);
}
