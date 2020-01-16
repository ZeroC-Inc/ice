//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

public abstract class BasePlugin : Ice.IPlugin
{
    public BasePlugin(Ice.Communicator communicator) => _communicator = communicator;

    public bool isInitialized() => _initialized;

    public bool isDestroyed() => _destroyed;

    protected static void test(bool b)
    {
        if (!b)
        {
            throw new System.Exception();
        }
    }

    public abstract void initialize();
    public abstract void destroy();

    protected Ice.Communicator _communicator;
    protected bool _initialized = false;
    protected bool _destroyed = false;
    protected BasePlugin _other = null;
}
