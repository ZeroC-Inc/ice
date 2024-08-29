// Copyright (c) ZeroC, Inc.

using Test;

public sealed class ControllerI : ControllerDisp_
{
    public ControllerI(Ice.ObjectAdapter adapter)
    {
        _adapter = adapter;
    }

    public override void hold(Ice.Current current)
    {
        _adapter.hold();
        _adapter.waitForHold();
    }

    public override void resume(Ice.Current current)
    {
        _adapter.activate();
    }

    private readonly Ice.ObjectAdapter _adapter;
};

public sealed class MetricsI : MetricsDisp_
{
    public override Task opAsync(Ice.Current current)
    {
        return null;
    }

    public override Task failAsync(Ice.Current current)
    {
        current.con.abort();
        return Task.CompletedTask;
    }

    public override Task opWithUserExceptionAsync(Ice.Current current)
    {
        throw new UserEx();
    }

    public override Task
    opWithRequestFailedExceptionAsync(Ice.Current current)
    {
        throw new Ice.ObjectNotExistException();
    }

    public override Task
    opWithLocalExceptionAsync(Ice.Current current)
    {
        throw new Ice.SyscallException(message: null);
    }

    public override Task
    opWithUnknownExceptionAsync(Ice.Current current)
    {
        throw new ArgumentOutOfRangeException();
    }

    public override Task
    opByteSAsync(byte[] bs, Ice.Current current)
    {
        return null;
    }

    public override Ice.ObjectPrx
    getAdmin(Ice.Current current)
    {
        return current.adapter.getCommunicator().getAdmin();
    }

    public override void
    shutdown(Ice.Current current)
    {
        current.adapter.getCommunicator().shutdown();
    }
}
