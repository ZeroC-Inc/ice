//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using Test;

public class Client : TestHelper
{
    public override void Run(string[] args)
    {
        Console.Out.Write("testing Slice predefined macros... ");
        Console.Out.Flush();
        Default d = new Default();
        Assert(d.x == 10);
        Assert(d.y == 10);

        NoDefault nd = new NoDefault();
        Assert(nd.x != 10);
        Assert(nd.y != 10);

        CsOnly c = new CsOnly();
        Assert(c.lang.Equals("cs"));
        Assert(c.version == ZeroC.Ice.Communicator.IntVersion);
        Console.Out.WriteLine("ok");
    }

    public static int Main(string[] args) => TestDriver.RunTest<Client>(args);
}
