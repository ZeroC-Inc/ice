//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using ZeroC.Ice;
using Test;
using System.Threading.Tasks;

namespace ZeroC.IceBox.Test.Admin
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            var properties = CreateTestProperties(ref args);
            properties["Ice.Default.Host"] = "127.0.0.1";
            properties["Ice.Default.Protocol"] = "ice1";
            await using var communicator = Initialize(properties);
            AllTests.allTests(this);
            // Shutdown the IceBox server.
            IProcessPrx.Parse("DemoIceBox/admin -f Process:default -p 9996", communicator).Shutdown();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
