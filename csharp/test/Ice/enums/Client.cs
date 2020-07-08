//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Enums
{
    public class Client : TestHelper
    {
        public override Task Run(string[] args)
        {
            using Communicator communicator = Initialize(ref args);
            ITestIntfPrx proxy = AllTests.allTests(this);
            proxy.Shutdown();
            return Task.CompletedTask;
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTest<Client>(args);
    }
}
