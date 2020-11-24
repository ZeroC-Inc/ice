// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Collections.Generic;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Exceptions
{
    public class ServerAMD : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            Dictionary<string, string> properties = CreateTestProperties(ref args);
            properties["Ice.Warn.Dispatch"] = "0";
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.IncomingFrameMaxSize"] = "10K";
            using Communicator communicator = Initialize(properties);
            communicator.SetProperty("TestAdapter.Endpoints", GetTestEndpoint(0));
            communicator.SetProperty("TestAdapter2.Endpoints", GetTestEndpoint(1));
            communicator.SetProperty("TestAdapter2.IncomingFrameMaxSize", "0");
            communicator.SetProperty("TestAdapter3.Endpoints", GetTestEndpoint(2));
            communicator.SetProperty("TestAdapter3.IncomingFrameMaxSize", "1K");

            ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            ObjectAdapter adapter2 = communicator.CreateObjectAdapter("TestAdapter2");
            ObjectAdapter adapter3 = communicator.CreateObjectAdapter("TestAdapter3");
            var obj = new AsyncThrower();
            ZeroC.Ice.IObjectPrx prx = adapter.Add("thrower", obj, ZeroC.Ice.IObjectPrx.Factory);
            adapter2.Add("thrower", obj);
            adapter3.Add("thrower", obj);
            await adapter.ActivateAsync();
            await adapter2.ActivateAsync();
            await adapter3.ActivateAsync();

            using var communicator2 = new Communicator(properties);
            communicator2.SetProperty("ForwarderAdapter.Endpoints", GetTestEndpoint(3));
            communicator2.SetProperty("ForwarderAdapter.IncomingFrameMaxSize", "0");
            ObjectAdapter forwarderAdapter = communicator2.CreateObjectAdapter("ForwarderAdapter");
            forwarderAdapter.Add("forwarder", new Forwarder(IObjectPrx.Parse(GetTestProxy("thrower"), communicator2)));
            await forwarderAdapter.ActivateAsync();

            ServerReady();
            await communicator.WaitForShutdownAsync();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<ServerAMD>(args);
    }
}
