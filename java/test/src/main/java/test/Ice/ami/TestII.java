// Copyright (c) ZeroC, Inc.

package test.Ice.ami;

import test.Ice.ami.Test.Outer.Inner.TestIntf;

public class TestII implements TestIntf {
    public OpResult op(int i, com.zeroc.Ice.Current current) {
        OpResult result = new OpResult();
        result.returnValue = i;
        result.j = i;
        return result;
    }
}
