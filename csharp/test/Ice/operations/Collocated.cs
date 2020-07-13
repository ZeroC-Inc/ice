//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Operations
{
    public class Collocated : TestHelper
    {
        public override Task RunAsync(string[] args)
        {
            using var communicator = Initialize(ref args);
            communicator.SetProperty("TestAdapter.AdapterId", "test");
            communicator.SetProperty("TestAdapter.Endpoints", GetTestEndpoint(0));
            ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            var prx = adapter.Add("test", new MyDerivedClass(), IMyDerivedClassPrx.Factory);
            //adapter.activate(); // Don't activate OA to ensure collocation is used.

            if (prx.GetConnection() != null)
            {
                Assert(false);
                throw new System.Exception();
            }
            AllTests.allTests(this);
            return Task.CompletedTask;
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Collocated>(args);
    }
}
