//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.NetworkProxy
{
    public class Client : TestHelper
    {
        public override Task Run(string[] args)
        {
            using var communicator = Initialize(ref args);
            AllTests.allTests(this);
            return Task.CompletedTask;
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTest<Client>(args);
    }
}
