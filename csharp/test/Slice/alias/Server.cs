// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Threading.Tasks;
using Test;
using ZeroC.Ice;

namespace ZeroC.Slice.Test.Alias
{
    public class Server : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            await using Communicator communicator = Initialize(ref args);
            communicator.SetProperty("TestAdapter.Endpoints", GetTestEndpoint(0));

            ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            adapter.Add("test", new Interface2());
            await adapter.ActivateAsync();
            ServerReady();

            await communicator.WaitForShutdownAsync();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Server>(args);
    }
}
