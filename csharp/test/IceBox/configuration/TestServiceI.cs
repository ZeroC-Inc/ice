//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using IceBox;
using Test;

class TestService : IService
{
    public void
    start(string name, Ice.Communicator communicator, string[] args)
    {
        Ice.ObjectAdapter adapter = communicator.CreateObjectAdapter(name + "OA");
        adapter.Add("test", new TestIntf(args));
        adapter.Activate();
    }

    public void
    stop()
    {
    }
}
