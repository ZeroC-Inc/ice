//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Text;
using ZeroC.Ice.Instrumentation;
using ZeroC.IceMX;

namespace ZeroC.IceMX
{
    // Extends generated class that requires a public parameterless constructor in the code below.
    public partial class InvocationMetrics
    {
        public InvocationMetrics()
            : this(remotes: Array.Empty<Metrics>(), collocated: Array.Empty<Metrics>())
        {
        }
    }
}

namespace ZeroC.Ice
{
    internal class CollocatedInvocationHelper : MetricsHelper<CollocatedMetrics>
    {
        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();
        private readonly int _requestId;
        private readonly string _id;
        private readonly int _size;

        public override void InitMetrics(CollocatedMetrics v) => v.Size += _size;

        internal CollocatedInvocationHelper(ObjectAdapter adapter, int requestId, int size)
            : base(_attributeResolver)
        {
            _id = adapter.Name;
            _requestId = requestId;
            _size = size;
        }

        private class AttributeResolverI : AttributeResolver
        {
            public AttributeResolverI()
            {
                Add("parent", obj => "Communicator");
                Add("id", obj => (obj as CollocatedInvocationHelper)?._id);
                Add("requestId", obj => (obj as CollocatedInvocationHelper)?._requestId);
            }
        }
    }

    internal class CollocatedObserver : ObserverWithDelegate<CollocatedMetrics, ICollocatedObserver>,
        ICollocatedObserver
    {
        public void Reply(int size)
        {
            ForEach(v => v.ReplySize += size);
            Delegate?.Reply(size);
        }
    }

    internal class CommunicatorObserver : ICommunicatorObserver
    {
        public MetricsAdmin AdminFacet { get; }

        private readonly ObserverFactoryWithDelegate<ConnectionMetrics, ConnectionObserver,
            IConnectionObserver> _connections;
        private readonly ObserverFactoryWithDelegate<Metrics, ObserverWithDelegate,
            IObserver> _connects;
        private readonly ICommunicatorObserver? _delegate;
        private readonly ObserverFactoryWithDelegate<DispatchMetrics, DispatchObserver,
            IDispatchObserver> _dispatch;
        private readonly ObserverFactoryWithDelegate<Metrics, ObserverWithDelegate,
            IObserver> _endpointLookups;
        private readonly ObserverFactoryWithDelegate<InvocationMetrics, InvocationObserver,
            IInvocationObserver> _invocations;

        internal CommunicatorObserver(Communicator communicator, ILogger logger)
        {
            AdminFacet = new MetricsAdmin(communicator, logger);
            _delegate = communicator.Observer;
            _connections = new ObserverFactoryWithDelegate<ConnectionMetrics, ConnectionObserver,
                IConnectionObserver>(AdminFacet, "Connection");
            _dispatch = new ObserverFactoryWithDelegate<DispatchMetrics, DispatchObserver, IDispatchObserver>(
                AdminFacet, "Dispatch");
            _invocations = new ObserverFactoryWithDelegate<InvocationMetrics, InvocationObserver,
                IInvocationObserver>(AdminFacet, "Invocation");
            _connects = new ObserverFactoryWithDelegate<Metrics, ObserverWithDelegate,
                IObserver>(AdminFacet, "ConnectionEstablishment");
            _endpointLookups = new ObserverFactoryWithDelegate<Metrics, ObserverWithDelegate,
                IObserver>(AdminFacet, "EndpointLookup");
            _invocations.RegisterSubMap<RemoteMetrics>("Remote",
                (obj, metrics) => (obj as InvocationMetrics)!.Remotes = metrics);
            _invocations.RegisterSubMap<CollocatedMetrics>("Collocated",
                (obj, metrics) => (obj as InvocationMetrics)!.Collocated = metrics);
        }

        public IObserver? GetConnectionEstablishmentObserver(Endpoint endpoint, string connector)
        {
            if (_connects.IsEnabled)
            {
                try
                {
                    return _connects.GetObserver(new EndpointHelper(endpoint, connector),
                        _delegate?.GetConnectionEstablishmentObserver(endpoint, connector));
                }
                catch (Exception ex)
                {
                    AdminFacet.Logger.Error($"unexpected exception trying to obtain observer:\n{ex}");
                }
            }
            return null;
        }

        public IConnectionObserver? GetConnectionObserver(
            Connection connection,
            ConnectionState connectionState,
            IConnectionObserver? observer)
        {
            if (_connections.IsEnabled)
            {
                try
                {
                    return _connections.GetObserver(
                        new ConnectionHelper(connection, connectionState), observer,
                        _delegate?.GetConnectionObserver(connection, connectionState,
                            (observer as ConnectionObserver)?.Delegate ?? observer));
                }
                catch (Exception ex)
                {
                    AdminFacet.Logger.Error($"unexpected exception trying to obtain observer:\n{ex}");
                }
            }
            return null;
        }

        public IDispatchObserver? GetDispatchObserver(Current current, int requestId, int size)
        {
            if (_dispatch.IsEnabled)
            {
                try
                {
                    return _dispatch.GetObserver(new DispatchHelper(current, requestId, size),
                        _delegate?.GetDispatchObserver(current, requestId, size));
                }
                catch (Exception ex)
                {
                    AdminFacet.Logger.Error($"unexpected exception trying to obtain observer:\n{ex}");
                }
            }
            return null;
        }

        public IObserver? GetEndpointLookupObserver(Endpoint endpoint)
        {
            if (_endpointLookups.IsEnabled)
            {
                try
                {
                    return _endpointLookups.GetObserver(new EndpointHelper(endpoint),
                        _delegate?.GetEndpointLookupObserver(endpoint));
                }
                catch (Exception ex)
                {
                    AdminFacet.Logger.Error($"unexpected exception trying to obtain observer:\n{ex}");
                }
            }
            return null;
        }

        public IInvocationObserver? GetInvocationObserver(
            IObjectPrx prx,
            string operation,
            IReadOnlyDictionary<string, string> context)
        {
            if (_invocations.IsEnabled)
            {
                try
                {
                    return _invocations.GetObserver(new InvocationHelper(prx, operation, context),
                        _delegate?.GetInvocationObserver(prx, operation, context));
                }
                catch (Exception ex)
                {
                    AdminFacet.Logger.Error($"unexpected exception trying to obtain observer:\n{ex}");
                }
            }
            return null;
        }

        public void SetObserverUpdater(IObserverUpdater? updater)
        {
            if (updater == null)
            {
                _connections.SetUpdater(null);
            }
            else
            {
                _connections.SetUpdater(updater.UpdateConnectionObservers);
            }

            _delegate?.SetObserverUpdater(updater);
        }
    }

    internal class ConnectionHelper : MetricsHelper<ConnectionMetrics>
    {
        private string Id
        {
            get
            {
                if (_id == null)
                {
                    var os = new StringBuilder();
                    if ((_connection as IPConnection)?.LocalEndpoint?.ToString() is string localAddress)
                    {
                        os.Append(localAddress);
                        if ((_connection as IPConnection)?.RemoteEndpoint?.ToString() is string remoteAddress)
                        {
                            os.Append(" -> ");
                            os.Append(remoteAddress);
                        }
                    }

                    if (_connection.ConnectionId.Length > 0)
                    {
                        os.Append(" [").Append(_connection.ConnectionId).Append("]");
                    }
                    _id = os.ToString();
                }
                return _id;
            }
        }

        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();

        private readonly Connection _connection;
        private string? _id;
        private readonly ConnectionState _state;

        internal ConnectionHelper(Connection connection, ConnectionState state)
            : base(_attributeResolver)
        {
            _connection = connection;
            _state = state;
        }

        private class AttributeResolverI : AttributeResolver
        {
            internal AttributeResolverI()
            {
                Add("parent", obj =>
                    {
                        return (obj as ConnectionHelper)?._connection?.Adapter?.Name ?? "Communicator";
                    });
                Add("id", obj => (obj as ConnectionHelper)?.Id);
                Add("state", obj => (obj as ConnectionHelper)?._state.ToString().ToLowerInvariant());
                Add("incoming", obj => (obj as ConnectionHelper)?._connection.IsIncoming);
                Add("adapterName", obj => (obj as ConnectionHelper)?._connection.Adapter?.Name);
                Add("connectionId", obj => (obj as ConnectionHelper)?._connection.ConnectionId);

                Add("localHost", obj =>
                    ((obj as ConnectionHelper)?._connection as IPConnection)?.LocalEndpoint?.Address);

                Add("localPort", obj =>
                    ((obj as ConnectionHelper)?._connection as IPConnection)?.LocalEndpoint?.Port);

                Add("remoteHost", obj =>
                    ((obj as ConnectionHelper)?._connection as IPConnection)?.RemoteEndpoint?.Address);

                Add("remotePort", obj =>
                    ((obj as ConnectionHelper)?._connection as IPConnection)?.RemoteEndpoint?.Port);

                Add("mcastHost", obj =>
                    ((obj as ConnectionHelper)?._connection as UdpConnection)?.McastEndpoint?.Address);

                Add("mcastPort", obj =>
                    ((obj as ConnectionHelper)?._connection as UdpConnection)?.McastEndpoint?.Port);

                Add("endpoint", obj => (obj as ConnectionHelper)?._connection.Endpoint);
                Add("endpointTransport", obj => (obj as ConnectionHelper)?._connection.Endpoint?.Transport);
                Add("endpointIsDatagram", obj => (obj as ConnectionHelper)?._connection.Endpoint?.IsDatagram);
                Add("endpointIsSecure", obj => (obj as ConnectionHelper)?._connection.Endpoint?.IsSecure);
                Add("endpointTimeout", obj =>
                    {
                        Endpoint? endpoint = (obj as ConnectionHelper)?._connection.Endpoint;
                        return endpoint != null ? (int?)endpoint.Timeout.TotalMilliseconds : null;
                    });
                Add("endpointCompress", obj => (obj as ConnectionHelper)?._connection.Endpoint?.HasCompressionFlag);
                Add("endpointHost", obj => (obj as ConnectionHelper)?._connection.Endpoint.Host);
                Add("endpointPort", obj => (obj as ConnectionHelper)?._connection.Endpoint.Port);
            }
        }
    }

    internal class ConnectionObserver
        : ObserverWithDelegate<ConnectionMetrics, IConnectionObserver>, IConnectionObserver
    {
        private int _receivedBytes;
        private int _sentBytes;

        public void ReceivedBytes(int num)
        {
            _receivedBytes = num;
            ForEach(v => v.ReceivedBytes += _receivedBytes);
            Delegate?.ReceivedBytes(num);
        }

        public void SentBytes(int num)
        {
            _sentBytes = num;
            ForEach(v => v.SentBytes += _sentBytes);
            Delegate?.SentBytes(num);
        }
    }

    internal class DispatchHelper : MetricsHelper<DispatchMetrics>
    {
        // It is important to throw here when there isn't a connection, so that the filters doesn't use the
        // connection attributes for a collocated dispatch.
        private Connection Connection => _current.Connection ?? throw new MissingFieldException();

        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();

        private readonly Current _current;
        private readonly int _requestId;

        private string? _id;
        private readonly int _size;

        public override void InitMetrics(DispatchMetrics v) => v.Size += _size;

        internal DispatchHelper(Current current, int requestId, int size)
            : base(_attributeResolver)
        {
            _current = current;
            _requestId = requestId;
            _size = size;
        }

        protected override string DefaultResolve(string attribute) =>
            attribute.StartsWith("context.") && _current.Context.TryGetValue(attribute.Substring(8), out string? v) ?
                v : throw new MissingFieldException(attribute);

        private class AttributeResolverI : AttributeResolver
        {
            internal AttributeResolverI()
            {
                Add("parent", obj => (obj as DispatchHelper)?._current.Adapter.Name);
                Add("id", obj =>
                    {
                        DispatchHelper? helper = (obj as DispatchHelper)!;
                        return helper._id ??= $"{helper._current.Identity} [{helper._current.Operation}]";
                    });

                Add("incoming", obj => (obj as DispatchHelper)?.Connection.IsIncoming);
                Add("adapterName", obj => (obj as DispatchHelper)?.Connection.Adapter?.Name);
                Add("connectionId", obj => (obj as DispatchHelper)?.Connection.ConnectionId);

                Add("localHost", obj =>
                    ((obj as DispatchHelper)?.Connection as IPConnection)?.LocalEndpoint?.Address);

                Add("localPort", obj =>
                    ((obj as DispatchHelper)?.Connection as IPConnection)?.LocalEndpoint?.Port);

                Add("remoteHost", obj =>
                    ((obj as DispatchHelper)?.Connection as IPConnection)?.RemoteEndpoint?.Address);

                Add("remotePort", obj =>
                    ((obj as DispatchHelper)?.Connection as IPConnection)?.RemoteEndpoint?.Port);

                Add("mcastHost", obj =>
                    ((obj as DispatchHelper)?.Connection as UdpConnection)?.McastEndpoint?.Address);

                Add("mcastPort", obj =>
                    ((obj as DispatchHelper)?.Connection as UdpConnection)?.McastEndpoint?.Port);

                Add("endpoint", obj => (obj as DispatchHelper)?.Connection.Endpoint);
                Add("endpointTransport", obj => (obj as DispatchHelper)?.Connection.Endpoint.Transport);
                Add("endpointIsDatagram", obj => (obj as DispatchHelper)?.Connection.Endpoint.IsDatagram);
                Add("endpointIsSecure", obj => (obj as DispatchHelper)?.Connection.Endpoint.IsSecure);
                Add("endpointTimeout", obj =>
                    {
                        Endpoint? endpoint = (obj as DispatchHelper)?.Connection.Endpoint;
                        return endpoint != null ? (int?)endpoint.Timeout.TotalMilliseconds : null;
                    });
                Add("endpointCompress", obj => (obj as DispatchHelper)?.Connection.Endpoint.HasCompressionFlag);
                Add("endpointHost", obj => (obj as DispatchHelper)?.Connection.Endpoint.Host);
                Add("endpointPort", obj => (obj as DispatchHelper)?.Connection.Endpoint.Port);

                Add("operation", obj => (obj as DispatchHelper)?._current.Operation);
                Add("identity", obj =>
                    {
                        Current? current = (obj as DispatchHelper)?._current;
                        return current?.Identity.ToString(current.Adapter!.Communicator.ToStringMode);
                    });
                Add("facet", obj => (obj as DispatchHelper)?._current.Facet);
                Add("requestId", obj => (obj as DispatchHelper)?._requestId);
                Add("mode", obj => (obj as DispatchHelper)?._current.IsOneway == true ? "oneway" : "twoway");
            }
        }
    }

    internal class DispatchObserver : ObserverWithDelegate<DispatchMetrics, IDispatchObserver>, IDispatchObserver
    {
        public void RemoteException()
        {
            ForEach(v => ++v.UserException);
            Delegate?.RemoteException();
        }

        public void Reply(int size)
        {
            ForEach(v => v.ReplySize += size);
            Delegate?.Reply(size);
        }
    }

    internal class EndpointHelper : MetricsHelper<Metrics>
    {
        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();

        private readonly Endpoint _endpoint;
        private string? _id;

        internal EndpointHelper(Endpoint endpoint, string id) : base(_attributeResolver)
        {
            _endpoint = endpoint;
            _id = id;
        }

        internal EndpointHelper(Endpoint endpoint) : base(_attributeResolver) => _endpoint = endpoint;

        private class AttributeResolverI : AttributeResolver
        {
            internal AttributeResolverI()
            {
                Add("parent", obj => "Communicator");
                Add("id", obj =>
                    {
                        EndpointHelper helper = (obj as EndpointHelper)!;
                        return helper._id ??= helper._endpoint.ToString();
                    });
                Add("endpoint", obj => (obj as EndpointHelper)?._endpoint);
                Add("endpointTransport", obj => (obj as EndpointHelper)?._endpoint?.Transport);
                Add("endpointIsDatagram", obj => (obj as EndpointHelper)?._endpoint?.IsDatagram);
                Add("endpointIsSecure", obj => (obj as EndpointHelper)?._endpoint?.IsSecure);
                Add("endpointTimeout", obj =>
                    {
                        Endpoint? endpoint = (obj as EndpointHelper)?._endpoint;
                        return endpoint != null ? (int?)endpoint.Timeout.TotalMilliseconds : null;
                    });
                Add("endpointCompress", obj => (obj as EndpointHelper)?._endpoint?.HasCompressionFlag);
                Add("endpointHost", obj => (obj as EndpointHelper)?._endpoint.Host);
                Add("endpointPort", obj => (obj as EndpointHelper)?._endpoint.Port);
            }
        }
    }

    internal class InvocationHelper : MetricsHelper<InvocationMetrics>
    {
        private string Id
        {
            get
            {
                if (_id == null)
                {
                    var sb = new StringBuilder();
                    try
                    {
                        sb.Append(_proxy.Clone(endpoints: Array.Empty<Endpoint>()));
                        sb.Append(" [").Append(_operation).Append(']');
                    }
                    catch (Exception)
                    {
                        // Either a fixed proxy or the communicator is destroyed.
                        sb.Append(_proxy.Identity.ToString(_proxy.Communicator.ToStringMode));
                        sb.Append(" [").Append(_operation).Append(']');
                    }
                    _id = sb.ToString();
                }
                return _id;
            }
        }

        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();
        private readonly IReadOnlyDictionary<string, string> _context;
        private string? _id;
        private readonly string _operation;
        private readonly IObjectPrx _proxy;

        internal InvocationHelper(IObjectPrx proxy, string operation, IReadOnlyDictionary<string, string> context)
            : base(_attributeResolver)
        {
            _proxy = proxy;
            _operation = operation;
            _context = context;
        }

        protected override string DefaultResolve(string attribute) =>
            attribute.StartsWith("context.") && _context.TryGetValue(attribute.Substring(8), out string? v) ?
                v : throw new MissingFieldException(attribute);

        private class AttributeResolverI : AttributeResolver
        {
            public AttributeResolverI()
            {
                Add("parent", obj => "Communicator");
                Add("id", obj => (obj as InvocationHelper)?.Id);

                Add("operation", obj => (obj as InvocationHelper)?._operation);
                Add("identity", obj =>
                {
                    IObjectPrx? proxy = (obj as InvocationHelper)?._proxy;
                    return proxy?.Identity.ToString(proxy.Communicator.ToStringMode);
                });

                Add("facet", obj => (obj as InvocationHelper)?._proxy.Facet);
                Add("encoding", obj => (obj as InvocationHelper)?._proxy.Encoding);
                Add("mode", obj => (obj as InvocationHelper)?._proxy.InvocationMode.ToString().ToLowerInvariant());
                Add("proxy", obj => (obj as InvocationHelper)?._proxy);
            }
        }
    }

    internal class InvocationObserver : ObserverWithDelegate<InvocationMetrics, IInvocationObserver>,
        IInvocationObserver
    {
        public ICollocatedObserver? GetCollocatedObserver(ObjectAdapter adapter, int requestId, int size) =>
            GetObserver<CollocatedMetrics, CollocatedObserver, ICollocatedObserver>(
                "Collocated",
                new CollocatedInvocationHelper(adapter, requestId, size),
                Delegate?.GetCollocatedObserver(adapter, requestId, size));

        public IRemoteObserver? GetRemoteObserver(
            Connection connection,
            int requestId,
            int size) =>
            GetObserver<RemoteMetrics, RemoteObserver, IRemoteObserver>(
                "Remote",
                new RemoteInvocationHelper(connection, requestId, size),
                Delegate?.GetRemoteObserver(connection, requestId, size));

        public void RemoteException()
        {
            ForEach(v => ++v.UserException);
            Delegate?.RemoteException();
        }

        public void Retried()
        {
            ForEach(v => ++v.Retry);
            Delegate?.Retried();
        }
    }

    internal class ObserverFactoryWithDelegate<T, OImpl, O> : ObserverFactory<T, OImpl>
        where T : Metrics, new()
        where OImpl : ObserverWithDelegate<T, O>, O, new()
        where O : class, IObserver
    {
        public ObserverFactoryWithDelegate(MetricsAdmin metrics, string name)
            : base(metrics, name)
        {
        }

        public O? GetObserver(MetricsHelper<T> helper, O? del)
        {
            OImpl? o = GetObserver(helper);
            if (o != null)
            {
                o.Delegate = del;
                return o;
            }
            return del;
        }

        public O? GetObserver(MetricsHelper<T> helper, object? observer, O? del)
        {
            OImpl? o = GetObserver(helper, observer);
            if (o != null)
            {
                o.Delegate = del;
                return o;
            }
            return del;
        }
    }

    internal class ObserverWithDelegate<T, O> : Observer<T>
        where T : Metrics, new()
        where O : class, IObserver
    {
        public O? Delegate { get; set; }

        public override void Attach()
        {
            base.Attach();
            Delegate?.Attach();
        }

        public override void Detach()
        {
            base.Detach();
            Delegate?.Detach();
        }

        public override void Failed(string exceptionName)
        {
            base.Failed(exceptionName);
            Delegate?.Failed(exceptionName);
        }

        public Observer? GetObserver<S, ObserverImpl, Observer>(string mapName, MetricsHelper<S> helper, Observer? del)
            where S : Metrics, new()
            where ObserverImpl : ObserverWithDelegate<S, Observer>, Observer, new()
            where Observer : class, IObserver
        {
            ObserverImpl? obsv = GetObserver<S, ObserverImpl>(mapName, helper);
            if (obsv != null)
            {
                obsv.Delegate = del;
                return obsv;
            }
            return del;
        }
    }

    internal class ObserverWithDelegate : ObserverWithDelegate<Metrics, IObserver>
    {
    }

    internal class RemoteInvocationHelper : MetricsHelper<RemoteMetrics>
    {
        private string Id
        {
            get
            {
                _id ??= string.IsNullOrEmpty(_connection.ConnectionId) ?
                    _connection.Endpoint.ToString() : $"{_connection.Endpoint} [{_connection.ConnectionId}]";
                return _id;
            }
        }

        private static readonly AttributeResolver _attributeResolver = new AttributeResolverI();

        private readonly Connection _connection;
        private readonly int _requestId;
        private readonly int _size;
        private string? _id;

        public override void InitMetrics(RemoteMetrics v) => v.Size += _size;

        internal RemoteInvocationHelper(Connection connection, int requestId, int size)
            : base(_attributeResolver)
        {
            _connection = connection;
            _requestId = requestId;
            _size = size;
        }

        private class AttributeResolverI : AttributeResolver
        {
            internal AttributeResolverI()
            {
                Add("parent", obj =>
                    {
                        Connection connection = ((RemoteInvocationHelper)obj)._connection;
                        return string.IsNullOrEmpty(connection.Adapter?.Name) ? "Communicator" : connection.Adapter?.Name;
                    });
                Add("id", obj => (obj as RemoteInvocationHelper)?.Id);
                Add("requestId", obj => (obj as RemoteInvocationHelper)?._requestId);
                Add("incoming", obj => (obj as RemoteInvocationHelper)?._connection.IsIncoming);
                Add("adapterName", obj => (obj as RemoteInvocationHelper)?._connection.Adapter?.Name);
                Add("connectionId", obj => (obj as RemoteInvocationHelper)?._connection.ConnectionId);

                Add("localHost", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as IPConnection)?.LocalEndpoint?.Address);

                Add("localPort", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as IPConnection)?.LocalEndpoint?.Port);

                Add("remoteHost", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as IPConnection)?.RemoteEndpoint?.Address);

                Add("remotePort", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as IPConnection)?.RemoteEndpoint?.Port);

                Add("mcastHost", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as UdpConnection)?.McastEndpoint?.Address);

                Add("mcastPort", obj =>
                    ((obj as RemoteInvocationHelper)?._connection as UdpConnection)?.McastEndpoint?.Port);

                Add("endpoint", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint);
                Add("endpointTransport", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.Transport);
                Add("endpointIsDatagram", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.IsDatagram);
                Add("endpointIsSecure", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.IsSecure);
                Add("endpointTimeout", obj =>
                    {
                        Endpoint? endpoint = (obj as RemoteInvocationHelper)?._connection.Endpoint;
                        return endpoint != null ? (int?)endpoint.Timeout.TotalMilliseconds : null;
                    });
                Add("endpointCompress", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.HasCompressionFlag);
                Add("endpointHost", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.Host);
                Add("endpointPort", obj => (obj as RemoteInvocationHelper)?._connection.Endpoint.Port);
            }
        }
    }

    internal class RemoteObserver : ObserverWithDelegate<RemoteMetrics, IRemoteObserver>, IRemoteObserver
    {
        public void Reply(int size)
        {
            ForEach(v => v.ReplySize += size);
            Delegate?.Reply(size);
        }
    }
}
