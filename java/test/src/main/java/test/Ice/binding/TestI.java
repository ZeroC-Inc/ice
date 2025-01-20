// Copyright (c) ZeroC, Inc.

package test.Ice.binding;

import test.Ice.binding.Test.TestIntf;

public class TestI implements TestIntf {
    TestI() {}

    @Override
    public String getAdapterName(com.zeroc.Ice.Current current) {
        return current.adapter.getName();
    }
}
