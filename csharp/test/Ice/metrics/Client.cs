// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Threading.Tasks;
using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Metrics
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            var observer = new CommunicatorObserver();

            Dictionary<string, string> properties = CreateTestProperties(ref args);
            bool ice1 = GetTestProtocol(properties) == Protocol.Ice1;

            properties["Ice.Admin.Endpoints"] = ice1 ? "tcp -h 127.0.0.1" : "ice+tcp://127.0.0.1:0";
            properties["Ice.Admin.InstanceName"] = "client";
            properties["Ice.Admin.DelayCreation"] = "1";
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.ConnectTimeout"] = "500ms";

            await using Communicator? communicator = Initialize(properties, observer: observer);
            IMetricsPrx metrics = AllTests.Run(this, observer, colocated: false);
            await metrics.ShutdownAsync();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
