//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.UDP
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            Dictionary<string, string>? properties = CreateTestProperties(ref args);
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.UDP.SndSize"] = "16K";
            await using Communicator communicator = Initialize(properties);
            AllTests.Run(this);

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
                ITestIntfPrx.Parse(GetTestProxy("control", i, "tcp"), communicator).Shutdown();
            }
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
