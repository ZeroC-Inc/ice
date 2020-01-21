//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using Ice.dictMapping.Test;

namespace Ice.dictMapping
{
    public class Collocated : TestHelper
    {
        public override void run(string[] args)
        {
            using (var communicator = initialize(ref args))
            {
                communicator.SetProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                var adapter = communicator.CreateObjectAdapter("TestAdapter");
                adapter.Add(new MyClass(), "test");
                //adapter.activate(); // Don't activate OA to ensure collocation is used.
                AllTests.allTests(this, true);
            }
        }

        public static int Main(string[] args)
        {
            return TestDriver.runTest<Collocated>(args);
        }
    }
}
