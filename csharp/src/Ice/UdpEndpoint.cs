//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Net;
using System.Text;

namespace ZeroC.Ice
{
    /// <summary>The Endpoint class for the UDP transport.</summary>
    public sealed class UdpEndpoint : IPEndpoint
    {
        // We cannot move HashCompressFlag to IPEndpoint because of the way we marshal it in TcpEndpoint
        public override bool HasCompressionFlag { get; }
        public override bool IsDatagram => true;

        /// <summary>The local network interface used to send multicast datagrams.</summary>
        public string McastInterface { get; } = "";

        /// <summary>The time-to-live of the multicast datagrams, in milliseconds.</summary>
        public int McastTtl { get; } = -1;

        public override int Timeout => -1;
        public override string Transport => "udp";
        public override EndpointType Type => EndpointType.UDP;

        private readonly bool _connect;
        private int _hashCode = 0;

        public override bool Equals(Endpoint? other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            if (other is UdpEndpoint udpEndpoint)
            {
                if (HasCompressionFlag != udpEndpoint.HasCompressionFlag)
                {
                    return false;
                }
                if (_connect != udpEndpoint._connect)
                {
                    return false;
                }
                if (McastInterface != udpEndpoint.McastInterface)
                {
                    return false;
                }
                if (McastTtl != udpEndpoint.McastTtl)
                {
                    return false;
                }
                return base.Equals(udpEndpoint);
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            // This code is thread safe because reading/writing _hashCode (an int) is atomic.
            if (_hashCode != 0)
            {
                // Return cached value
                return _hashCode;
            }
            else
            {
                var hash = new HashCode();
                hash.Add(base.GetHashCode());
                hash.Add(_connect);
                hash.Add(HasCompressionFlag);
                hash.Add(McastInterface);
                hash.Add(McastTtl);
                int hashCode = hash.ToHashCode();
                if (hashCode == 0) // 0 is not a valid value as it means "not initialized".
                {
                    hashCode = 1;
                }
                _hashCode = hashCode;
                return _hashCode;
            }
        }

        public override string OptionsToString()
        {
            var sb = new StringBuilder(base.OptionsToString());

            if (McastInterface.Length > 0)
            {
                bool addQuote = McastInterface.IndexOf(':') != -1;
                sb.Append(" --interface ");
                if (addQuote)
                {
                    sb.Append("\"");
                }
                sb.Append(McastInterface);
                if (addQuote)
                {
                    sb.Append("\"");
                }
            }

            if (McastTtl != -1)
            {
                sb.Append(" --ttl ");
                sb.Append(McastTtl.ToString(CultureInfo.InvariantCulture));
            }

            if (_connect)
            {
                sb.Append(" -c");
            }

            if (HasCompressionFlag)
            {
                sb.Append(" -z");
            }
            return sb.ToString();
        }

        public override void IceWritePayload(OutputStream ostr)
        {
            base.IceWritePayload(ostr);
            ostr.WriteBool(HasCompressionFlag);
        }

        public override Endpoint NewTimeout(int timeout) => this;

        public override IAcceptor? GetAcceptor(string adapterName) => null;

        public override ITransceiver GetTransceiver() =>
            new UdpTransceiver(this, Communicator, Transport, Host, Port, McastInterface, _connect);

        internal UdpEndpoint GetEndpoint(UdpTransceiver transceiver)
        {
            int port = transceiver.EffectivePort();
            if (port == Port)
            {
                return this;
            }
            else
            {
                return new UdpEndpoint(Communicator, Host, port, SourceAddress, McastInterface, McastTtl, _connect,
                    ConnectionId, HasCompressionFlag);
            }
        }

        internal UdpEndpoint(Communicator communicator, string host, int port, IPAddress? sourceAddress,
            string mcastInterface, int mttl, bool connect, string connectionId, bool compressionFlag)
            : base(communicator, host, port, sourceAddress, connectionId)
        {
            McastInterface = mcastInterface;
            McastTtl = mttl;
            _connect = connect;
            HasCompressionFlag = compressionFlag;
        }

        internal UdpEndpoint(Communicator communicator, InputStream istr) : base(communicator, istr)
        {
            _connect = false;
            HasCompressionFlag = istr.ReadBool();
        }

        internal UdpEndpoint(Communicator communicator, string endpointString, Dictionary<string, string?> options,
            bool oaEndpoint) : base(communicator, endpointString, options, oaEndpoint)
        {
            if (options.TryGetValue("-c", out string? argument))
            {
                if (argument != null)
                {
                    throw new FormatException(
                        $"unexpected argument `{argument}' provided for -c option in `{endpointString}'");
                }
                _connect = true;
                options.Remove("-c");
            }

            if (options.TryGetValue("-z", out argument))
            {
                if (argument != null)
                {
                    throw new FormatException(
                        $"unexpected argument `{argument}' provided for -z option in `{endpointString}'");
                }
                HasCompressionFlag = true;
                options.Remove("-z");
            }

            if (options.TryGetValue("--ttl", out argument))
            {
                if (argument == null)
                {
                    throw new FormatException($"no argument provided for --ttl option in endpoint `{endpointString}'");
                }
                try
                {
                    McastTtl = int.Parse(argument, CultureInfo.InvariantCulture);
                }
                catch (FormatException ex)
                {
                    throw new FormatException($"invalid TTL value `{argument}' in endpoint `{endpointString}'", ex);
                }

                if (McastTtl < 0)
                {
                    throw new FormatException($"TTL value `{argument}' out of range in endpoint `{endpointString}'");
                }
                options.Remove("--ttl");
            }

            if (options.TryGetValue("--interface", out argument))
            {
                McastInterface = argument ?? throw new FormatException(
                    $"no argument provided for --interface option in endpoint `{endpointString}'");

                if (McastInterface == "*")
                {
                    if (oaEndpoint)
                    {
                        McastInterface = "";
                    }
                    else
                    {
                        throw new FormatException($"`--interface *' not valid for proxy endpoint `{endpointString}'");
                    }
                }
                options.Remove("--interface");
            }
        }

        private protected override IConnector CreateConnector(EndPoint addr, INetworkProxy? proxy) =>
            new UdpConnector(Communicator, Transport, Type, addr, SourceAddress, McastInterface, McastTtl,
                ConnectionId);

        private protected override IPEndpoint CreateEndpoint(string host, int port, string connectionId,
            bool compressionFlag, int timeout) =>
            new UdpEndpoint(Communicator, host, port, SourceAddress, McastInterface, McastTtl, _connect,
                connectionId, compressionFlag);
    }

    internal sealed class UdpEndpointFactory : IEndpointFactory
    {
        public string Transport => "udp";
        public EndpointType Type => EndpointType.UDP;

        private Communicator Communicator { get; }

        public Endpoint Create(string endpointString, Dictionary<string, string?> options, bool oaEndpoint) =>
            new UdpEndpoint(Communicator, endpointString, options, oaEndpoint);

        public Endpoint Read(InputStream istr) => new UdpEndpoint(Communicator, istr);

        internal UdpEndpointFactory(Communicator communicator) => Communicator = communicator;
    }
}
