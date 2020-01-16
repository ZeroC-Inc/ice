//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;

namespace Ice.servantLocator
{
    public class Client : TestHelper
    {
        public override void run(string[] args)
        {
            using var communicator = initialize(createTestProperties(ref args),
                typeIdNamespaces: new string[] { "Ice.servantLocator.TypeId" });
            AllTests.allTests(this).shutdown();
        }

        public static int Main(string[] args) => TestDriver.runTest<Client>(args);
    }
}
