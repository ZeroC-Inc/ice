//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Net;

namespace ZeroC.Ice
{
    internal sealed class TcpConnector : IConnector
    {
        public ITransceiver Connect() =>
            _endpoint.CreateTransceiver(new StreamSocket(_communicator, _proxy, _addr, _sourceAddr), null);

        // TODO: why are we copying all these readonly fields of the endpoint?
        internal TcpConnector(
            TcpEndpoint endpoint,
            Communicator communicator,
            EndPoint addr,
            INetworkProxy? proxy,
            IPAddress? sourceAddr,
            int timeout)
        {
            _endpoint = endpoint;
            _communicator = communicator;
            _addr = addr;
            _proxy = proxy;
            _sourceAddr = sourceAddr;
            _timeout = timeout;

            var hash = new System.HashCode();
            hash.Add(_addr);
            if (_sourceAddr != null)
            {
                hash.Add(_sourceAddr);
            }
            hash.Add(_timeout);
            _hashCode = hash.ToHashCode();
        }

        public override bool Equals(object? obj)
        {
            if (!(obj is TcpConnector))
            {
                return false;
            }

            if (this == obj)
            {
                return true;
            }

            var p = (TcpConnector)obj;
            if (_timeout != p._timeout)
            {
                return false;
            }

            if (!Equals(_sourceAddr, p._sourceAddr))
            {
                return false;
            }

            if (!_endpoint.Equals(p._endpoint))
            {
                return false;
            }

            return _addr.Equals(p._addr);
        }

        public override string ToString() => Network.AddrToString(_proxy == null ? _addr : _proxy.GetAddress());

        public override int GetHashCode() => _hashCode;

        private readonly TcpEndpoint _endpoint;
        private readonly Communicator _communicator;
        private readonly EndPoint _addr;
        private readonly INetworkProxy? _proxy;
        private readonly IPAddress? _sourceAddr;
        private readonly int _timeout;
        private readonly int _hashCode;
    }
}
