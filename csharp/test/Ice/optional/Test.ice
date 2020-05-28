//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[normalize-case]]
[[ice-prefix]]

module ZeroC::Ice::Test::Optional
{
    class C
    {
        int x;
    }

    interface Test
    {
        void shutdown();

        void opSingleInInt(int? i1);
        void opSingleInString(string? i1);
        void opSingleOutInt(out int? o1);
        void opSingleOutString(out string? o1);
        int? opSingleReturnInt();
        string? opSingleReturnString();

        void opBasicIn(int i1, int? i2, string? i3, string i4);
        int? opBasicInOut(int i1, int? i2, string? i3, out int o1, out int? o2, out string? o3);

        Object? opObject(Object i1, Object? i2);
        Test? opTest(Test i1, Test? i2);

        Value? opAnyClass(Value i1, Value? i2);
        C? opC(C i1, C? i2);
    }
}
