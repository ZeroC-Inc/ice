//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

using ZeroC.Ice;

public static class TestHelper
{
    public static void Assert([DoesNotReturnIf(false)] bool b)
    {
        if (!b)
        {
            Debug.Assert(false);
            throw new Exception();
        }
    }
}
public abstract class BasePlugin : IPlugin
{
    public BasePlugin(Communicator communicator) => _communicator = communicator;

    public bool isInitialized() => _initialized;

    public bool isDestroyed() => _destroyed;

    public abstract void Initialize();
    public abstract void Destroy();

    protected Communicator _communicator;
    protected bool _initialized = false;
    protected bool _destroyed = false;
    protected BasePlugin? _other = null;
}
