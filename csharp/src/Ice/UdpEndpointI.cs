//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Net;

namespace IceInternal
{
    internal sealed class UdpEndpointI : IPEndpoint
    {
        public UdpEndpointI(ProtocolInstance instance, string ho, int po, EndPoint? sourceAddr, string mcastInterface,
                            int mttl, bool conn, string conId, bool co) :
            base(instance, ho, po, sourceAddr, conId)
        {
            _mcastInterface = mcastInterface;
            _mcastTtl = mttl;
            _connect = conn;
            _compress = co;
        }

        public UdpEndpointI(ProtocolInstance instance) :
            base(instance)
        {
            _connect = false;
            _compress = false;
        }

        public UdpEndpointI(ProtocolInstance instance, Ice.InputStream s) :
            base(instance, s)
        {
            if (s.Encoding.Equals(Ice.Util.Encoding_1_0))
            {
                s.ReadByte();
                s.ReadByte();
                s.ReadByte();
                s.ReadByte();
            }
            // Not transmitted.
            //_connect = s.readBool();
            _connect = false;
            _compress = s.ReadBool();
        }

        private sealed class InfoI : Ice.UDPEndpointInfo
        {
            public InfoI(UdpEndpointI e) => _endpoint = e;

            public override short type() => _endpoint.type();

            public override bool datagram() => _endpoint.datagram();

            public override bool secure() => _endpoint.secure();

            private UdpEndpointI _endpoint;
        }

        //
        // Return the endpoint information.
        //
        public override Ice.EndpointInfo getInfo()
        {
            InfoI info = new InfoI(this);
            fillEndpointInfo(info);
            return info;
        }

        //
        // Return the timeout for the endpoint in milliseconds. 0 means
        // non-blocking, -1 means no timeout.
        //
        public override int timeout() => -1;

        //
        // Return a new endpoint with a different timeout value, provided
        // that timeouts are supported by the endpoint. Otherwise the same
        // endpoint is returned.
        //
        public override Endpoint timeout(int timeout) => this;

        //
        // Return true if the endpoints support bzip2 compress, or false
        // otherwise.
        //
        public override bool compress() => _compress;

        //
        // Return a new endpoint with a different compression value,
        // provided that compression is supported by the
        // endpoint. Otherwise the same endpoint is returned.
        //
        public override Endpoint compress(bool compress)
        {
            if (compress == _compress)
            {
                return this;
            }
            else
            {
                return new UdpEndpointI(instance_, host_, port_, sourceAddr_, _mcastInterface, _mcastTtl, _connect,
                                        connectionId_, compress);
            }
        }

        //
        // Return true if the endpoint is datagram-based.
        //
        public override bool datagram() => true;

        //
        // Return a server side transceiver for this endpoint, or null if a
        // transceiver can only be created by an acceptor.
        //
        public override ITransceiver transceiver() =>
            new UdpTransceiver(this, instance_, host_!, port_, _mcastInterface, _connect);

        //
        // Return an acceptor for this endpoint, or null if no acceptors
        // is available.
        //
        public override IAcceptor? acceptor(string adapterName) => null;

        public override void initWithOptions(List<string> args, bool oaEndpoint)
        {
            base.initWithOptions(args, oaEndpoint);

            if (_mcastInterface.Equals("*"))
            {
                if (oaEndpoint)
                {
                    _mcastInterface = "";
                }
                else
                {
                    throw new FormatException($"`--interface *' not valid for proxy endpoint `{this}'");
                }
            }
        }

        public UdpEndpointI endpoint(UdpTransceiver transceiver)
        {
            int port = transceiver.effectivePort();
            if (port == port_)
            {
                return this;
            }
            else
            {
                return new UdpEndpointI(instance_, host_, port, sourceAddr_, _mcastInterface, _mcastTtl, _connect,
                                        connectionId_, _compress);
            }
        }

        public override string options()
        {
            //
            // WARNING: Certain features, such as proxy validation in Glacier2,
            // depend on the format of proxy strings. Changes to toString() and
            // methods called to generate parts of the reference string could break
            // these features. Please review for all features that depend on the
            // format of proxyToString() before changing this and related code.
            //
            string s = base.options();

            if (_mcastInterface.Length != 0)
            {
                bool addQuote = _mcastInterface.IndexOf(':') != -1;
                s += " --interface ";
                if (addQuote)
                {
                    s += "\"";
                }
                s += _mcastInterface;
                if (addQuote)
                {
                    s += "\"";
                }
            }

            if (_mcastTtl != -1)
            {
                s += " --ttl " + _mcastTtl;
            }

            if (_connect)
            {
                s += " -c";
            }

            if (_compress)
            {
                s += " -z";
            }

            return s;
        }

        //
        // Compare endpoints for sorting purposes
        //
        public override int CompareTo(Endpoint obj)
        {
            if (!(obj is UdpEndpointI))
            {
                return type() < obj.type() ? -1 : 1;
            }

            UdpEndpointI p = (UdpEndpointI)obj;
            if (this == p)
            {
                return 0;
            }

            if (!_connect && p._connect)
            {
                return -1;
            }
            else if (!p._connect && _connect)
            {
                return 1;
            }

            if (!_compress && p._compress)
            {
                return -1;
            }
            else if (!p._compress && _compress)
            {
                return 1;
            }

            int rc = string.Compare(_mcastInterface, p._mcastInterface, StringComparison.Ordinal);
            if (rc != 0)
            {
                return rc;
            }

            if (_mcastTtl < p._mcastTtl)
            {
                return -1;
            }
            else if (p._mcastTtl < _mcastTtl)
            {
                return 1;
            }

            return base.CompareTo(p);
        }

        //
        // Marshal the endpoint
        //
        public override void streamWriteImpl(Ice.OutputStream s)
        {
            base.streamWriteImpl(s);
            if (s.GetEncoding().Equals(Ice.Util.Encoding_1_0))
            {
                s.WriteByte(Ice.Util.Protocol_1_0.major);
                s.WriteByte(Ice.Util.Protocol_1_0.minor);
                s.WriteByte(Ice.Util.Encoding_1_0.major);
                s.WriteByte(Ice.Util.Encoding_1_0.minor);
            }
            // Not transmitted.
            //s.writeBool(_connect);
            s.WriteBool(_compress);
        }

        public override void hashInit(ref int h)
        {
            base.hashInit(ref h);
            HashUtil.hashAdd(ref h, _mcastInterface);
            HashUtil.hashAdd(ref h, _mcastTtl);
            HashUtil.hashAdd(ref h, _connect);
            HashUtil.hashAdd(ref h, _compress);
        }

        public override void fillEndpointInfo(Ice.IPEndpointInfo info)
        {
            base.fillEndpointInfo(info);
            if (info is Ice.UDPEndpointInfo)
            {
                Ice.UDPEndpointInfo udpInfo = (Ice.UDPEndpointInfo)info;
                udpInfo.timeout = -1;
                udpInfo.compress = _compress;
                udpInfo.mcastInterface = _mcastInterface;
                udpInfo.mcastTtl = _mcastTtl;
            }
        }

        protected override bool checkOption(string option, string argument, string endpoint)
        {
            if (base.checkOption(option, argument, endpoint))
            {
                return true;
            }

            if (option.Equals("-c"))
            {
                if (argument != null)
                {
                    throw new FormatException($"unexpected argument `{argument} ' provided for -c option in {endpoint}");
                }

                _connect = true;
            }
            else if (option.Equals("-z"))
            {
                if (argument != null)
                {
                    throw new FormatException($"unexpected argument `{argument}' provided for -z option in {endpoint}");
                }

                _compress = true;
            }
            else if (option.Equals("-v") || option.Equals("-e"))
            {
                if (argument == null)
                {
                    throw new FormatException($"no argument provided for {option} option in endpoint {endpoint}");
                }

                try
                {
                    Ice.EncodingVersion v = Ice.Util.stringToEncodingVersion(argument);
                    if (v.major != 1 || v.minor != 0)
                    {
                        instance_.Logger.warning($"deprecated udp endpoint option: {option}");
                    }
                }
                catch (FormatException ex)
                {
                    throw new FormatException("invalid version `{argument}' in endpoint {endpoint}", ex);
                }
            }
            else if (option.Equals("--ttl"))
            {
                if (argument == null)
                {
                    throw new FormatException($"no argument provided for --ttl option in endpoint {endpoint}");
                }

                try
                {
                    _mcastTtl = int.Parse(argument, CultureInfo.InvariantCulture);
                }
                catch (FormatException ex)
                {
                    throw new FormatException($"invalid TTL value `{argument}' in endpoint {endpoint}", ex);
                }

                if (_mcastTtl < 0)
                {
                    throw new FormatException("TTL value `{argument}' out of range in endpoint {endpoint}");
                }
            }
            else if (option.Equals("--interface"))
            {
                if (argument == null)
                {
                    throw new FormatException("no argument provided for --interface option in endpoint {endpoint}");
                }
                _mcastInterface = argument;
            }
            else
            {
                return false;
            }

            return true;
        }

        protected override IConnector CreateConnector(EndPoint addr, INetworkProxy? proxy) =>
            new UdpConnector(instance_, addr, sourceAddr_, _mcastInterface, _mcastTtl, connectionId_);

        protected override IPEndpoint CreateEndpoint(string host, int port, string connectionId) =>
            new UdpEndpointI(instance_, host, port, sourceAddr_, _mcastInterface, _mcastTtl, _connect, connectionId,
                             _compress);

        private string _mcastInterface = "";
        private int _mcastTtl = -1;
        private bool _connect;
        private bool _compress;
    }

    internal sealed class UdpEndpointFactory : IEndpointFactory
    {
        internal UdpEndpointFactory(ProtocolInstance instance) => _instance = instance;

        public void initialize()
        {
        }

        public short type() => _instance!.Type;

        public string protocol() => _instance!.Protocol;

        public Endpoint create(List<string> args, bool oaEndpoint)
        {
            IPEndpoint endpt = new UdpEndpointI(_instance!);
            endpt.initWithOptions(args, oaEndpoint);
            return endpt;
        }

        public Endpoint read(Ice.InputStream s) => new UdpEndpointI(_instance!, s);

        public void destroy() => _instance = null;

        public IEndpointFactory clone(ProtocolInstance instance) => new UdpEndpointFactory(instance);

        private ProtocolInstance? _instance;
    }

}
