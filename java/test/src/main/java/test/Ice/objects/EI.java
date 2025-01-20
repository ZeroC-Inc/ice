// Copyright (c) ZeroC, Inc.

package test.Ice.objects;

import test.Ice.objects.Test.E;

public final class EI extends E {
    public EI() {
        super(1, "hello");
    }

    public boolean checkValues() {
        return i == 1 && s.equals("hello");
    }
}
