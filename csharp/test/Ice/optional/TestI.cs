//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;

namespace ZeroC.Ice.Test.Optional
{
    internal class Test : ITest
    {
        public void Shutdown(Current current) => current.Adapter.Communicator.Shutdown();

        public void OpSingleInInt(int? i1, Current current) => TestHelper.Assert(i1 == null || i1.Value == 42);

        public void OpSingleInString(string? i1, Current current) => TestHelper.Assert(i1 == null || i1 == "42");

        public int? OpSingleOutInt(Current current) => 42;
        public string? OpSingleOutString(Current current) => "42";

        public int? OpSingleReturnInt(Current current) => 42;

        public string? OpSingleReturnString(Current current) => "42";

        public void OpBasicIn(int i1, int? i2, string? i3, string i4, Current current)
        {
            TestHelper.Assert(i2 == null || i2.Value == i1);
            TestHelper.Assert(i3 == null || i3 == i4);
        }

        public (int? ReturnValue, int o1, int? o2, string? o3) OpBasicInOut(int i1, int? i2, string? i3,
            Current current) => (i2, i1, i2, i3);

        public IObjectPrx? OpObject(IObjectPrx i1, IObjectPrx? i2, Current current)
        {
            TestHelper.Assert(i2 == null || i2.Equals(i1));
            return i2;
        }

        public ITestPrx? OpTest(ITestPrx i1, ITestPrx? i2, Current current)
        {
            TestHelper.Assert(i2 == null || i2.Equals(i1));
            return i2;
        }

        public AnyClass? OpAnyClass(AnyClass i1, AnyClass? i2, Current current)
        {
            TestHelper.Assert(i2 == null || i2 == i1);
            return i2;
        }

        public C? OpC(C i1, C? i2, Current current)
        {
            TestHelper.Assert(i2 == null || i2 == i1);
            return i2;
        }
    }
}
