//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Threading.Tasks;
using Test;

public sealed class Controller : IController
{
    public Controller(Ice.ObjectAdapter adapter) => _adapter = adapter;

    public void hold(Ice.Current current)
    {
        _adapter.Hold();
        _adapter.WaitForHold();
    }

    public void resume(Ice.Current current) => _adapter.Activate();

    readonly private Ice.ObjectAdapter _adapter;
};

public sealed class Metrics : IMetrics
{
    public Task opAsync(Ice.Current current) => null;

    public Task failAsync(Ice.Current current)
    {
        Debug.Assert(current != null);
        current.Connection.Close(Ice.ConnectionClose.Forcefully);
        return null;
    }

    public Task opWithUserExceptionAsync(Ice.Current current) => throw new UserEx();

    public Task
    opWithRequestFailedExceptionAsync(Ice.Current current) => throw new Ice.ObjectNotExistException();

    public Task
    opWithLocalExceptionAsync(Ice.Current current) => throw new Ice.SyscallException();

    public Task
    opWithUnknownExceptionAsync(Ice.Current current) => throw new ArgumentOutOfRangeException();

    public Task
    opByteSAsync(byte[] bs, Ice.Current current) => null;

    public Ice.IObjectPrx
    getAdmin(Ice.Current current)
    {
        Debug.Assert(current != null);
        return current.Adapter.Communicator.GetAdmin();
    }

    public void
    shutdown(Ice.Current current) => current.Adapter.Communicator.Shutdown();
}
