// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    internal class TcpAcceptor : IAcceptor
    {
        public Endpoint Endpoint { get; }

        private readonly ObjectAdapter _adapter;
        private readonly IConnectionManager _manager;
        private readonly Socket _socket;
        private readonly IPEndPoint _addr;

        // See https://tools.ietf.org/html/rfc5246#appendix-A.4
        private const byte TlsHandshakeRecord = 0x16;

        public async ValueTask<Connection> AcceptAsync()
        {
            Socket fd = await _socket.AcceptAsync().ConfigureAwait(false);

            bool secure = Endpoint.IsAlwaysSecure || !_adapter.AcceptNonSecure;

            if (_adapter.Protocol == Protocol.Ice2 && _adapter.AcceptNonSecure)
            {
                Debug.Assert(_adapter.Communicator.ConnectTimeout != TimeSpan.Zero);
                // TODO: we are using reusing ConnectTimeout here so that peeking cannot block forever.
                // However, this means that it's possible to end up waiting 2 * ConnectTime if reading the
                // first byte is slow and then the actual connection initialization is also slow.

                using var source = new CancellationTokenSource(_adapter.Communicator.ConnectTimeout);
                CancellationToken cancel = source.Token;

                // Peek one byte into the tcp stream to see if it contains the TLS handshake record
                var buffer = new ArraySegment<byte>(new byte[1]);

                int received;
                try
                {
                    received = await fd.ReceiveAsync(buffer, SocketFlags.Peek, cancel).ConfigureAwait(false);
                }
                catch (SocketException) when (cancel.IsCancellationRequested)
                {
                    throw new OperationCanceledException(cancel);
                }
                catch (SocketException ex) when (ex.IsConnectionLost())
                {
                    throw new ConnectionLostException(ex);
                }
                catch (SocketException ex)
                {
                    throw new TransportException(ex);
                }
                if (received == 0)
                {
                    throw new ConnectionLostException();
                }

                Debug.Assert(received == 1);
                secure = buffer.Array![0] == TlsHandshakeRecord;
            }

            ITransceiver transceiver = ((TcpEndpoint)Endpoint).CreateTransceiver(fd, _adapter.Name, secure);

            MultiStreamTransceiverWithUnderlyingTransceiver multiStreamTranceiver = Endpoint.Protocol switch
            {
                Protocol.Ice1 => new LegacyTransceiver(transceiver, Endpoint, _adapter),
                _ => new SlicTransceiver(transceiver, Endpoint, _adapter)
            };

            return ((TcpEndpoint)Endpoint).CreateConnection(_manager, multiStreamTranceiver, null, "", _adapter);
        }

        public void Dispose() => _socket.CloseNoThrow();

        public string ToDetailedString()
        {
            var s = new StringBuilder("local address = ");
            s.Append(ToString());

            List<string> interfaces =
                Network.GetHostsForEndpointExpand(_addr.Address.ToString(), Network.EnableBoth, true);
            if (interfaces.Count != 0)
            {
                s.Append("\nlocal interfaces = ");
                s.Append(string.Join(", ", interfaces));
            }
            return s.ToString();
        }

        public override string ToString() => _addr.ToString();

        internal TcpAcceptor(TcpEndpoint endpoint, IConnectionManager manager, ObjectAdapter adapter)
        {
            _manager = manager;
            _adapter = adapter;

            _addr = Network.GetAddressForServerEndpoint(endpoint.Host,
                                                        endpoint.Port,
                                                        Network.EnableBoth);

            _socket = Network.CreateServerSocket(endpoint, _addr.AddressFamily);

            try
            {
                _socket.Bind(_addr);
                _addr = (IPEndPoint)_socket.LocalEndPoint!;
                _socket.Listen(endpoint.Communicator.GetPropertyAsInt("Ice.TCP.Backlog") ?? 511);
            }
            catch (SocketException ex)
            {
                _socket.CloseNoThrow();
                throw new TransportException(ex);
            }

            Endpoint = endpoint.Clone((ushort)_addr.Port);
        }
    }
}
