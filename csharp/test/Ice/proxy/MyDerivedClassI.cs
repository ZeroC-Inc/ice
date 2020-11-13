// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using System.Threading;

using Test;

namespace ZeroC.Ice.Test.Proxy
{
    internal sealed class MyDerivedClass : IMyDerivedClass
    {
        private Dictionary<string, string>? _ctx;

        public IObjectPrx? Echo(IObjectPrx? obj, Current c, CancellationToken cancel) => obj;

        public IEnumerable<string> GetLocation(Current current, CancellationToken cancel) => current.Location;

        public void Shutdown(Current current, CancellationToken cancel) =>
            current.Adapter.Communicator.ShutdownAsync();

        public IReadOnlyDictionary<string, string> GetContext(Current current, CancellationToken cancel) => _ctx!;

        public bool IceIsA(string typeId, Current current, CancellationToken cancel)
        {
            _ctx = current.Context;
            return typeof(IMyDerivedClass).GetAllIceTypeIds().Contains(typeId);
        }

        public IRelativeTestPrx OpRelative(ICallbackPrx callback, Current current, CancellationToken cancel)
        {
            TestHelper.Assert(callback.IsFixed);
            IRelativeTestPrx relativeTest =
                current.Adapter.AddWithUUID(new RelativeTest(), IRelativeTestPrx.Factory).Clone(relative: true);

            TestHelper.Assert(callback.Op(relativeTest, cancel: cancel) == 1);
            return relativeTest;
        }
    }

    internal sealed class RelativeTest : IRelativeTest
    {
        private int _count;

        public int DoIt(Current current, CancellationToken cancel) => ++_count;
    }
}
