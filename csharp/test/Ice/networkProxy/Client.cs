//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.NetworkProxy
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            await using var communicator = Initialize(ref args);
            AllTests.allTests(this);
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
