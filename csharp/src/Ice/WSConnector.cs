//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace IceInternal
{
    internal sealed class WSConnector : IConnector
    {
        public ITransceiver connect() => new WSTransceiver(_instance, _delegate.connect(), _host, _resource);

        public short type()
        {
            return _delegate.type();
        }

        internal WSConnector(ProtocolInstance instance, IConnector del, string host, string resource)
        {
            _instance = instance;
            _delegate = del;
            _host = host;
            _resource = resource;
        }

        public override bool Equals(object obj)
        {
            if (!(obj is WSConnector))
            {
                return false;
            }

            if (this == obj)
            {
                return true;
            }

            WSConnector p = (WSConnector)obj;
            if (!_delegate.Equals(p._delegate))
            {
                return false;
            }

            if (!_resource.Equals(p._resource))
            {
                return false;
            }

            return true;
        }

        public override string ToString() => _delegate.ToString();

        public override int GetHashCode() => _delegate.GetHashCode();

        private ProtocolInstance _instance;
        private IConnector _delegate;
        private string _host;
        private string _resource;
    }
}
