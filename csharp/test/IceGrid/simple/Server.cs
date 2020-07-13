//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using ZeroC.Ice;
using Test;
using System.Threading.Tasks;

namespace ZeroC.IceGrid.Test.Simple
{
    public class Server : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            var properties = new Dictionary<string, string>();
            properties.ParseArgs(ref args, "TestAdapter");
            properties.Add("Ice.Default.Encoding", "1.1");
            properties.Add("Ice.Default.Protocol", "ice1");

            await using Communicator communicator = Initialize(ref args, properties);
            ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            adapter.Add(communicator.GetProperty("Identity") ?? "test", new TestIntf());
            await adapter.ActivateAsync();
            await communicator.WaitForShutdownAsync();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Server>(args);
    }
}
