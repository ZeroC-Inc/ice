//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Metrics
{
    public class Client : TestHelper
    {
        public override Task Run(string[] args)
        {
            var observer = new CommunicatorObserver();

            Dictionary<string, string>? properties = CreateTestProperties(ref args);
            properties["Ice.Admin.Endpoints"] = "tcp";
            properties["Ice.Admin.InstanceName"] = "client";
            properties["Ice.Admin.DelayCreation"] = "1";
            properties["Ice.Warn.Connections"] = "0";
            properties["Ice.Default.Host"] = "127.0.0.1";

            using var communicator = Initialize(properties, observer: observer);
            IMetricsPrx metrics = AllTests.allTests(this, observer);
            metrics.shutdown();
            return Task.CompletedTask;
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTest<Client>(args);
    }
}
