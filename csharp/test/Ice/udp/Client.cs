//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;

using Test;

namespace ZeroC.Ice.Test.UDP
{
    public class Client : TestHelper
    {
        public override void Run(string[] args)
        {
            var properties = CreateTestProperties(ref args);
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.UDP.SndSize"] = "16384";
            using var communicator = Initialize(properties);
            AllTests.allTests(this);

            int num;
            try
            {
                num = args.Length == 1 ? int.Parse(args[0]) : 1;
            }
            catch (FormatException)
            {
                num = 1;
            }

            for (int i = 0; i < num; ++i)
            {
                ITestIntfPrx.Parse("control:" + GetTestEndpoint(i, "tcp"), communicator).shutdown();
            }
        }

        public static int Main(string[] args) => TestDriver.RunTest<Client>(args);
    }
}
