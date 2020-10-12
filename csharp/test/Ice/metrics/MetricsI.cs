// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Threading;

namespace ZeroC.Ice.Test.Metrics
{
    public sealed class Metrics : IMetrics
    {
        public void Op(Current current, CancellationToken cancel)
        {
        }

        public void Fail(Current current, CancellationToken cancel) =>
            current.Connection!.Close(ConnectionClose.Forcefully);

        public void OpWithUserException(Current current, CancellationToken cancel) =>
            throw new UserEx("custom UserEx message");

        public void OpWithRequestFailedException(Current current, CancellationToken cancel) =>
            throw new ObjectNotExistException(current);

        public void OpWithLocalException(Current current, CancellationToken cancel) =>
            throw new InvalidConfigurationException("fake");

        public void OpWithUnknownException(Current current, CancellationToken cancel) =>
            throw new ArgumentOutOfRangeException();

        public void OpByteS(byte[] bs, Current current, CancellationToken cancel)
        {
        }

        public IObjectPrx? GetAdmin(Current current, CancellationToken cancel) =>
            current.Adapter.Communicator.GetAdmin();

        public void Shutdown(Current current, CancellationToken cancel) =>
            current.Adapter.Communicator.ShutdownAsync();
    }
}
