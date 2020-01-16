//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.inheritance.Test;

namespace Ice.inheritance
{
    public class Collocated : TestHelper
    {
        public override void run(string[] args)
        {
            using var communicator = initialize(ref args);
            communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
            var adapter = communicator.createObjectAdapter("TestAdapter");
            adapter.Add(new InitialI(adapter), "initial");
            AllTests.allTests(this);
        }

        public static int Main(string[] args) => TestDriver.runTest<Collocated>(args);
    }
}
