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
        using var communicator = initialize(ref args);
        if (args.Length < 1)
        {
            throw new ArgumentException("Usage: server testdir");
        }

        communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0, "tcp"));
        Ice.ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
        adapter.Add(new ServerFactory(args[0] + "/../certs"), "factory");
        adapter.Activate();

        communicator.WaitForShutdown();
    }

    public static int Main(string[] args) => TestDriver.runTest<Server>(args);
}
