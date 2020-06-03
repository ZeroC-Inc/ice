//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;

namespace ZeroC.Ice.Test.Metrics
{
    public sealed class Controller : IController
    {
        public Controller(Func<ObjectAdapter> factory)
        {
            _factory = factory;
            _adapter = factory();
            _adapter.Activate();
        }

        public void hold(Current current)
        {
            _adapter.Destroy();
            _adapter = _factory(); // Recreate the adapter without activating it
        }

        public void resume(Current current) => _adapter.Activate();

        private readonly Func<ObjectAdapter> _factory;
        private ObjectAdapter _adapter;
    };

    public sealed class Metrics : IMetrics
    {
        public void op(Current current)
        {
        }

        public void fail(Current current) => current.Connection!.Close(ConnectionClose.Forcefully);

        public void opWithUserException(Current current) => throw new UserEx();

        public void opWithRequestFailedException(Current current) => throw new ObjectNotExistException(current);

        public void opWithLocalException(Current current) => throw new InvalidConfigurationException("fake");

        public void opWithUnknownException(Current current) => throw new ArgumentOutOfRangeException();

        public void opByteS(byte[] bs, Current current)
        {
        }

        public IObjectPrx? getAdmin(Current current) => current.Adapter.Communicator.GetAdmin();

        public void shutdown(Current current) => current.Adapter.Communicator.Shutdown();
    }
}
