// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

using System;
using System.Reflection;

[assembly: CLSCompliant(true)]

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        Ice.Properties properties = createTestProperties(ref args);
        properties.setProperty("Ice.Warn.AMICallback", "0");
        properties.setProperty("Ice.Warn.Connections", "0");

        //
        // We use a client thread pool with more than one thread to test
        // that task inlining works.
        //
        properties.setProperty("Ice.ThreadPool.Client.Size", "5");

        //
        // Limit the send buffer size, this test relies on the socket
        // send() blocking after sending a given amount of data.
        //
        properties.setProperty("Ice.TCP.SndSize", "50000");
        using(var communicator = initialize(properties))
        {
            AllTests.allTests(this, false);
        }
    }

    public static int Main(string[] args)
    {
        return Test.TestDriver.runTest<Client>(args);
    }
}
