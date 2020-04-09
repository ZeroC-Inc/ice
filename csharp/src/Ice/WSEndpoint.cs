//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using IceInternal;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;

namespace Ice
{
    public sealed class WSEndpoint : Endpoint
    {
        public override string ConnectionId => _delegate.ConnectionId;
        public override bool HasCompressionFlag => _delegate.HasCompressionFlag;
        public override bool IsDatagram => _delegate.IsDatagram;
        public override bool IsSecure => _delegate.IsSecure;

        /// <summary> The resource of the WebSocket endpoint.</summary>
        // TODO: better description
        public string Resource { get; }
        public override int Timeout => _delegate.Timeout;
        public override EndpointType Type => _delegate.Type;
        public override Endpoint? Underlying => _delegate;

        private readonly TransportInstance _instance;
        private readonly Endpoint _delegate;

        public override bool Equals(Endpoint? other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            if (other is WSEndpoint wsEndpoint)
            {
                if (Type != wsEndpoint.Type)
                {
                    return false;
                }
                if (Resource != wsEndpoint.Resource)
                {
                    return false;
                }
                return _delegate.Equals(wsEndpoint._delegate);
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode() => HashCode.Combine(_delegate, Resource);

        public override string OptionsToString()
        {
            string s = _delegate.OptionsToString();

            if (Resource.Length > 0)
            {
                s += " -r ";
                bool addQuote = Resource.IndexOf(':') != -1;
                if (addQuote)
                {
                    s += "\"";
                }
                s += Resource;
                if (addQuote)
                {
                    s += "\"";
                }
            }
            return s;
        }

        public override bool Equivalent(Endpoint endpoint)
        {
            if (endpoint is WSEndpoint wsEndpoint)
            {
                return _delegate.Equivalent(wsEndpoint._delegate);
            }
            else
            {
                return false;
            }
        }

        public override void IceWriteImpl(Ice.OutputStream s)
        {
            _delegate.IceWriteImpl(s);
            s.WriteString(Resource);
        }

        public override Endpoint NewTimeout(int timeout)
            => timeout == _delegate.Timeout ? this : new WSEndpoint(_instance, _delegate.NewTimeout(timeout), Resource);

        public override Endpoint NewConnectionId(string connectionId)
            => connectionId == _delegate.ConnectionId ? this :
                new WSEndpoint(_instance, _delegate.NewConnectionId(connectionId), Resource);

        public override Endpoint NewCompressionFlag(bool compressionFlag)
            => compressionFlag == _delegate.HasCompressionFlag ? this :
                new WSEndpoint(_instance, _delegate.NewCompressionFlag(compressionFlag), Resource);

        public override IAcceptor Acceptor(string adapterName)
        {
            IAcceptor? acceptor = _delegate.Acceptor(adapterName);
            Debug.Assert(acceptor != null);
            return new WSAcceptor(this, _instance, acceptor);
        }

        public override void ConnectorsAsync(Ice.EndpointSelectionType endpointSelection, IEndpointConnectors callback)
        {
            string host = "";
            for (Ice.Endpoint? p = _delegate; p != null; p = p.Underlying)
            {
                if (p is Ice.IPEndpoint ipEndpoint)
                {
                    host = $"{ipEndpoint.Host}:{ipEndpoint.Port.ToString(CultureInfo.InvariantCulture)}";
                    break;
                }
            }
            _delegate.ConnectorsAsync(endpointSelection, new EndpointConnectors(_instance, host, Resource, callback));
        }

        public WSEndpoint GetEndpoint(Endpoint del)
            => del == _delegate ? this : new WSEndpoint(_instance, del, Resource);

        public override List<Endpoint> ExpandIfWildcard()
        {
            var l = new List<Endpoint>();
            foreach (Endpoint e in _delegate.ExpandIfWildcard())
            {
                l.Add(e == _delegate ? this : new WSEndpoint(_instance, e, Resource));
            }
            return l;
        }

        public override List<Endpoint> ExpandHost(out Endpoint? publish)
        {
            var endpoints = new List<Endpoint>();
            foreach (Endpoint e in _delegate.ExpandHost(out publish))
            {
                endpoints.Add(e == _delegate ? this : new WSEndpoint(_instance, e, Resource));
            }
            if (publish != null)
            {
                publish = publish == _delegate ? this : new WSEndpoint(_instance, publish, Resource);
            }
            return endpoints;
        }

        public override ITransceiver? GetTransceiver() => null;

        internal WSEndpoint(TransportInstance instance, Endpoint del, string res)
        {
            _instance = instance;
            _delegate = del;
            Resource = res;
        }

        internal WSEndpoint(TransportInstance instance, Endpoint del, string endpointString,
                            Dictionary<string, string?> options)
        {
            _instance = instance;
            _delegate = del;

            string? argument = null;
            if (options.TryGetValue("-r", out argument))
            {
                Resource = argument ?? throw new FormatException(
                        $"no argument provided for -r option in endpoint `{endpointString}'");

                options.Remove("-r");
            }
            else
            {
                Resource = "/";
            }
        }

        internal WSEndpoint(TransportInstance instance, Endpoint del, InputStream istr)
        {
            _instance = instance;
            _delegate = del;

            Resource = istr.ReadString();
        }

        private sealed class EndpointConnectors : IEndpointConnectors
        {
            private readonly TransportInstance _instance;
            private readonly string _host;
            private readonly string _resource;
            private readonly IEndpointConnectors _callback;

            public void Connectors(List<IConnector> connectors)
            {
                var newConnectors = new List<IConnector>();
                foreach (IConnector c in connectors)
                {
                    newConnectors.Add(new WSConnector(_instance, c, _host, _resource));
                }
                _callback.Connectors(newConnectors);
            }

            public void Exception(System.Exception ex) => _callback.Exception(ex);

            internal EndpointConnectors(TransportInstance instance, string host, string res, IEndpointConnectors cb)
            {
                _instance = instance;
                _host = host;
                _resource = res;
                _callback = cb;
            }
        }
    }

    internal class WSEndpointFactory : EndpointFactoryWithUnderlying
    {
        public override IEndpointFactory CloneWithUnderlying(TransportInstance instance, EndpointType underlying)
            => new WSEndpointFactory(instance, underlying);

        protected override Endpoint CreateWithUnderlying(Endpoint underlying, string endpointString,
            Dictionary<string, string?> options, bool oaEndpoint)
                => new WSEndpoint(Instance, underlying, endpointString, options);

        protected override Endpoint ReadWithUnderlying(Endpoint underlying, Ice.InputStream s)
            => new WSEndpoint(Instance, underlying, s);

        internal WSEndpointFactory(TransportInstance instance, EndpointType type)
            : base(instance, type)
        {
        }
    }
}
