// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Threading;
using Test;

namespace ZeroC.IceGrid.Test.Simple
{
    public sealed class TestIntf : ITestIntf
    {
        public void Shutdown(Ice.Current current, CancellationToken cancel) =>
            current.Adapter.Communicator.ShutdownAsync();
    }
}
