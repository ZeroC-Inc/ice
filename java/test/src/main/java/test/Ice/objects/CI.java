// Copyright (c) ZeroC, Inc.

package test.Ice.objects;

import test.Ice.objects.Test.C;

public final class CI extends C {
    @Override
    public void ice_preMarshal() {
        preMarshalInvoked = true;
    }

    @Override
    public void ice_postUnmarshal() {
        postUnmarshalInvoked = true;
    }
}
