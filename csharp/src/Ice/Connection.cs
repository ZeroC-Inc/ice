//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice
{
    /// <summary>Determines the behavior when manually closing a connection.</summary>
    public enum ConnectionClose
    {
        /// <summary>Close the connection immediately without sending a close connection protocol message to the peer
        /// and waiting for the peer to acknowledge it.</summary>
        Forcefully,
        /// <summary>Close the connection by notifying the peer but do not wait for pending outgoing invocations to
        /// complete.</summary>
        Gracefully,
        /// <summary>Wait for all pending invocations to complete before closing the connection.</summary>
        GracefullyWithWait
    }

    /// <summary>The state of an Ice connection</summary>
    public enum ConnectionState : byte
    {
        /// <summary>The connection is being validated.</summary>
        Validating = 0,
        /// <summary>The connection is active and can send and receive messages.</summary>
        Active,
        /// <summary>The connection is being gracefully shutdown and waits for the peer to close its end of the
        /// connection.</summary>
        Closing,
        /// <summary>The connection is closed and eventually waits for potential dispatch to be finished before being
        /// destroyed.</summary>
        Closed
    }

    /// <summary>Represents a connection used to send and receive Ice frames.</summary>
    public abstract class Connection
    {
        /// <summary>Gets or set the connection Acm (Active Connection Management) configuration.</summary>
        public Acm Acm
        {
            get
            {
                lock (_mutex)
                {
                    return _monitor.Acm;
                }
            }
            set
            {
                lock (_mutex)
                {
                    if (_state >= ConnectionState.Closing)
                    {
                        return;
                    }

                    if (_state == ConnectionState.Active)
                    {
                        _monitor.Remove(this);
                    }

                    _monitor = value == _manager.AcmMonitor.Acm ?
                        _manager.AcmMonitor : new ConnectionAcmMonitor(value, _communicator.Logger);

                    if (_monitor.Acm.IsDisabled)
                    {
                        // Disable the recording of last activity.
                        _acmLastActivity = Timeout.InfiniteTimeSpan;
                    }
                    else if (_state == ConnectionState.Active && _acmLastActivity == Timeout.InfiniteTimeSpan)
                    {
                        _acmLastActivity = Time.Elapsed;
                    }

                    if (_state == ConnectionState.Active)
                    {
                        _monitor.Add(this);
                    }
                }
            }
        }

        /// <summary>Gets or sets the object adapter that dispatches requests received over this connection.
        /// A client can invoke an operation on a server using a proxy, and then set an object adapter for the
        /// outgoing connection used by the proxy in order to receive callbacks. This is useful if the server
        /// cannot establish a connection back to the client, for example because of firewalls.</summary>
        /// <value>The object adapter that dispatches requests for the connection, or null if no adapter is set.
        /// </value>
        public ObjectAdapter? Adapter
        {
            // We don't use a volatile for _adapter to avoid extra-memory barriers when accessing _adapter with
            // the mutex locked.
            get
            {
                lock (_mutex)
                {
                    return _adapter;
                }
            }
            set
            {
                lock (_mutex)
                {
                    _adapter = value;
                }
            }
        }

        /// <summary>Get the connection ID which was used to create the connection.</summary>
        /// <value>The connection ID used to create the connection.</value>
        public string ConnectionId { get; }

        /// <summary>Get the endpoint from which the connection was created.</summary>
        /// <value>The endpoint from which the connection was created.</value>
        public Endpoint Endpoint { get; }

        /// <summary>True for incoming connections false otherwise.</summary>
        public bool IsIncoming => _connector == null;

        // The connector from which the connection was created. This is used by the outgoing connection factory.
        internal IConnector Connector => _connector!;

        // The endpoints which are associated with this connection. This is populated by the outgoing connection
        // factory when an endpoint resolves to the same connector as this connection's connector. Two endpoints
        // can be different but resolve to the same connector (e.g.: endpoints with the IPs "::1", "0:0:0:0:0:0:0:1"
        // or "localhost" are different endpoints but they all end up resolving to the same connector and can use
        // the same connection).
        internal List<Endpoint> Endpoints { get; }

        internal bool Active
        {
            get
            {
                lock (_mutex)
                {
                    return _state > ConnectionState.Validating && _state < ConnectionState.Closing;
                }
            }
        }

        protected ITransceiver Transceiver { get; }

        private bool OldProtocol => Endpoint.Protocol == Protocol.Ice1;

        private TimeSpan _acmLastActivity;
        private ObjectAdapter? _adapter;
        private EventHandler? _closed;
        private Task? _closeTask = null;
        private readonly Communicator _communicator;
        private readonly int _compressionLevel;
        private readonly IConnector? _connector;
        private int _dispatchCount;
        private TaskCompletionSource<bool>? _dispatchTaskCompletionSource;
        private Exception? _exception;
        private readonly IConnectionManager _manager;
        private readonly int _frameSizeMax;
        private IAcmMonitor _monitor;
        private readonly object _mutex = new object();
        private int _nextRequestId;
        private IConnectionObserver? _observer;
        private Task _receiveTask = Task.CompletedTask;
        private readonly Dictionary<int, (TaskCompletionSource<IncomingResponseFrame>, bool)> _requests =
            new Dictionary<int, (TaskCompletionSource<IncomingResponseFrame>, bool)>();
        private Task _sendTask = Task.CompletedTask;
        private ConnectionState _state; // The current state.
        private bool _validated = false;
        private readonly bool _warn;
        private readonly bool _warnUdp;

        private static readonly List<ArraySegment<byte>> _closeConnectionFrameIce1 =
            new List<ArraySegment<byte>> { Ice1Definitions.CloseConnectionFrame };

        private static readonly List<ArraySegment<byte>> _closeConnectionFrameIce2 =
            new List<ArraySegment<byte>> { Ice2Definitions.CloseConnectionFrame };

        private static readonly List<ArraySegment<byte>> _validateConnectionFrameIce1 =
            new List<ArraySegment<byte>> { Ice1Definitions.ValidateConnectionFrame };

        private static readonly List<ArraySegment<byte>> _validateConnectionFrameIce2 =
            new List<ArraySegment<byte>> { Ice2Definitions.ValidateConnectionFrame };

        /// <summary>Manually closes the connection using the specified closure mode.</summary>
        /// <param name="mode">Determines how the connection will be closed.</param>
        public void Close(ConnectionClose mode)
        {
            // TODO: We should consider removing this method and expose GracefulCloseAsync and CloseAsync
            // instead. This would remove the support for ConnectionClose.GracefullyWithWait. Is it
            // useful? Not waiting implies that the pending requests implies these requests will fail and
            // won't be retried. GracefulCloseAsync could wait for pending requests to complete?
            if (mode == ConnectionClose.Forcefully)
            {
                _ = CloseAsync(new ConnectionClosedLocallyException("connection closed forcefully"));
            }
            else if (mode == ConnectionClose.Gracefully)
            {
                _ = GracefulCloseAsync(new ConnectionClosedLocallyException("connection closed gracefully"));
            }
            else
            {
                Debug.Assert(mode == ConnectionClose.GracefullyWithWait);

                // Wait until all outstanding requests have been completed.
                lock (_mutex)
                {
                    while (_requests.Count > 0)
                    {
                        System.Threading.Monitor.Wait(_mutex);
                    }
                }

                _ = GracefulCloseAsync(new ConnectionClosedLocallyException("connection closed gracefully"));
            }
        }

        /// <summary>Creates a special "fixed" proxy that always uses this connection. This proxy can be used for
        /// callbacks from a server to a client if the server cannot directly establish a connection to the client,
        /// for example because of firewalls. In this case, the server would create a proxy using an already
        /// established connection from the client.</summary>
        /// <param name="identity">The identity for which a proxy is to be created.</param>
        /// <param name="factory">The proxy factory. Use INamePrx.Factory, where INamePrx is the desired proxy type.
        /// </param>
        /// <returns>A proxy that matches the given identity and uses this connection.</returns>
        public T CreateProxy<T>(Identity identity, ProxyFactory<T> factory) where T : class, IObjectPrx =>
            factory(new Reference(_communicator, this, identity));

        /// <summary>Sends a heartbeat frame.</summary>
        public void Heartbeat()
        {
            try
            {
                HeartbeatAsync().AsTask().Wait();
            }
            catch (AggregateException ex)
            {
                Debug.Assert(ex.InnerException != null);
                throw ExceptionUtil.Throw(ex.InnerException);
            }
        }

        /// <summary>Sends an asynchronous heartbeat frame.</summary>
        /// <param name="progress">Sent progress provider.</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        public async ValueTask HeartbeatAsync(IProgress<bool>? progress = null, CancellationToken cancel = default)
        {
            await SendFrameAsync(() => GetProtocolFrameData(OldProtocol ? _validateConnectionFrameIce1 :
                                                            _validateConnectionFrameIce2),
                                 cancel).ConfigureAwait(false);
            progress?.Report(true);
        }

        /// <summary>This event is raised when the connection is closed. If the subscriber needs more information about
        /// the closure, it can call Connection.ThrowException. The connection object is passed as the event sender
        /// argument.</summary>
        public event EventHandler? Closed
        {
            add
            {
                lock (_mutex)
                {
                    if (_state >= ConnectionState.Closed)
                    {
                        Task.Run(() => value?.Invoke(this, EventArgs.Empty));
                    }
                    _closed += value;
                }
            }
            remove => _closed -= value;
        }

        /// <summary>This event is raised when the connection receives a heartbeat. The connection object is passed as
        /// the event sender argument.</summary>
        public event EventHandler? HeartbeatReceived;

        /// <summary>Throws an exception indicating the reason for connection closure. For example,
        /// ConnectionClosedByPeerException is raised if the connection was closed gracefully by the peer, whereas
        /// ConnectionClosedLocallyException is raised if the connection was manually closed by the application. This
        /// operation does nothing if the connection is not yet closed.</summary>
        public void ThrowException()
        {
            lock (_mutex)
            {
                if (_exception != null)
                {
                    Debug.Assert(_state >= ConnectionState.Closing);
                    throw _exception;
                }
            }
        }

        /// <summary>Returns a description of the connection as human readable text, suitable for logging or error
        /// messages.</summary>
        /// <returns>The description of the connection as human readable text.</returns>
        public override string ToString() => Transceiver.ToString()!;

        internal Connection(
            IConnectionManager manager,
            Endpoint endpoint,
            ITransceiver transceiver,
            IConnector? connector,
            string connectionId,
            ObjectAdapter? adapter)
        {
            _communicator = endpoint.Communicator;
            _manager = manager;
            _monitor = manager.AcmMonitor;
            Transceiver = transceiver;
            _connector = connector;
            ConnectionId = connectionId;
            Endpoint = endpoint;
            Endpoints = new List<Endpoint>() { endpoint };
            _adapter = adapter;
            _warn = _communicator.GetPropertyAsBool("Ice.Warn.Connections") ?? false;
            _warnUdp = _communicator.GetPropertyAsBool("Ice.Warn.Datagrams") ?? false;
            _acmLastActivity = _monitor.Acm.IsDisabled ? Timeout.InfiniteTimeSpan : Time.Elapsed;
            _nextRequestId = 1;
            _frameSizeMax = adapter != null ? adapter.FrameSizeMax : _communicator.FrameSizeMax;
            _dispatchCount = 0;
            _state = ConnectionState.Validating;

            _compressionLevel = _communicator.GetPropertyAsInt("Ice.Compression.Level") ?? 1;
            if (_compressionLevel < 1)
            {
                _compressionLevel = 1;
            }
            else if (_compressionLevel > 9)
            {
                _compressionLevel = 9;
            }
        }

        internal void ClearAdapter(ObjectAdapter adapter)
        {
            lock (_mutex)
            {
                if (_adapter == adapter)
                {
                    _adapter = null;
                }
            }
        }

        internal async Task GracefulCloseAsync(Exception exception)
        {
            // Don't gracefully close connections for datagram endpoints
            if (!Endpoint.IsDatagram)
            {
                try
                {
                    Task? closingTask = null;
                    lock (_mutex)
                    {
                        if (_state == ConnectionState.Active)
                        {
                            SetState(ConnectionState.Closing, exception);
                            if (_dispatchCount > 0)
                            {
                                _dispatchTaskCompletionSource = new TaskCompletionSource<bool>();
                            }
                            closingTask = PerformGracefulCloseAsync();
                            if (_closeTask == null)
                            {
                                // _closeTask might already be assigned if CloseAsync() got called if the send of the
                                // closing frame failed.
                                _closeTask = closingTask;
                            }
                        }
                        else if (_state == ConnectionState.Closing)
                        {
                            closingTask = _closeTask;
                        }
                    }
                    if (closingTask != null)
                    {
                        await closingTask.ConfigureAwait(false);
                    }
                }
                catch
                {
                    // Ignore
                }
            }

            await CloseAsync(exception).ConfigureAwait(false);
        }

        internal void Monitor(TimeSpan now, Acm acm)
        {
            lock (_mutex)
            {
                if (_state != ConnectionState.Active)
                {
                    return;
                }

                // We send a heartbeat if there was no activity in the last (timeout / 4) period. Sending a heartbeat
                // sooner than really needed is safer to ensure that the receiver will receive the heartbeat in time.
                // Sending the heartbeat if there was no activity in the last (timeout / 2) period isn't enough since
                // monitor() is called only every (timeout / 2) period.
                //
                // Note that this doesn't imply that we are sending 4 heartbeats per timeout period because the
                // monitor() method is still only called every (timeout / 2) period.
                if (_state == ConnectionState.Active &&
                    (acm.Heartbeat == AcmHeartbeat.Always ||
                    (acm.Heartbeat != AcmHeartbeat.Off && now >= (_acmLastActivity + (acm.Timeout / 4)))))
                {
                    if (acm.Heartbeat != AcmHeartbeat.OnDispatch || _dispatchCount > 0)
                    {
                        Debug.Assert(_state == ConnectionState.Active);
                        if (!Endpoint.IsDatagram)
                        {
                            try
                            {
                                SendFrameAsync(() => GetProtocolFrameData(
                                    OldProtocol ? _validateConnectionFrameIce1 : _validateConnectionFrameIce2));
                            }
                            catch
                            {
                                // Ignore
                            }
                        }
                    }
                }

                if (acm.Close != AcmClose.Off && now >= _acmLastActivity + acm.Timeout)
                {
                    if (acm.Close == AcmClose.OnIdleForceful || (acm.Close != AcmClose.OnIdle && (_requests.Count > 0)))
                    {
                        // Close the connection if we didn't receive a heartbeat or if read/write didn't update the
                        // ACM activity in the last period.
                        _ = CloseAsync(new ConnectionTimeoutException());
                    }
                    else if (acm.Close != AcmClose.OnInvocation && _dispatchCount == 0 && _requests.Count == 0)
                    {
                        // The connection is idle, close it.
                        _ = GracefulCloseAsync(new ConnectionIdleException());
                    }
                }
            }
        }

        internal async ValueTask<IncomingResponseFrame> SendRequestAsync(
            OutgoingRequestFrame request,
            bool oneway,
            bool compress,
            bool synchronous,
            IInvocationObserver? observer,
            IProgress<bool> progress,
            CancellationToken cancel)
        {
            IChildInvocationObserver? childObserver = null;
            Task writeTask;
            Task<IncomingResponseFrame>? responseTask = null;
            lock (_mutex)
            {
                //
                // If the exception is thrown before we even have a chance to send our request, we always try to
                // send the request again.
                //
                if (_exception != null)
                {
                    throw new RetryException(_exception);
                }

                Debug.Assert(_state > ConnectionState.Validating);
                Debug.Assert(_state < ConnectionState.Closing);

                // Ensure the frame isn't bigger than what we can send with the transport.
                // TODO: remove?
                if (OldProtocol)
                {
                    Transceiver.CheckSendSize(request.Size + Ice1Definitions.HeaderSize + 4);
                }
                else
                {
                    Transceiver.CheckSendSize(request.Size + Ice2Definitions.HeaderSize + 4);
                }

                writeTask = SendFrameAsync(() =>
                {
                    // This is called with _mutex locked.

                    int requestId = 0;
                    if (!oneway)
                    {
                        //
                        // Create a new unique request ID.
                        //
                        requestId = _nextRequestId++;
                        if (requestId <= 0)
                        {
                            _nextRequestId = 1;
                            requestId = _nextRequestId++;
                        }

                        var responseTaskSource = new TaskCompletionSource<IncomingResponseFrame>();
                        _requests[requestId] = (responseTaskSource, synchronous);
                        responseTask = responseTaskSource.Task;
                    }

                    if (observer != null)
                    {
                        childObserver = observer.GetRemoteObserver(this, requestId, request.Size);
                        childObserver?.Attach();
                    }

                    return GetRequestFrameData(request, requestId, compress);
                }, cancel);
            }

            try
            {
                await writeTask.ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                childObserver?.Failed(ex.GetType().FullName ?? "System.Exception");
                childObserver?.Detach();
                throw;
            }

            // The request is sent
            progress.Report(false); // sentSynchronously: false

            if (responseTask == null)
            {
                childObserver?.Detach();
                return IncomingResponseFrame.WithVoidReturnValue(request.Protocol, request.Encoding);
            }
            else
            {
                try
                {
                    IncomingResponseFrame response = await responseTask.WaitAsync(cancel).ConfigureAwait(false);
                    childObserver?.Reply(response.Size);
                    return response;
                }
                catch (Exception ex)
                {
                    childObserver?.Failed(ex.GetType().FullName ?? "System.Exception");
                    throw;
                }
                finally
                {
                    childObserver?.Detach();
                }
            }
        }

        internal async Task StartAsync()
        {
            CancellationTokenSource? source = null;
            try
            {
                CancellationToken timeoutToken;
                TimeSpan timeout = _communicator.ConnectTimeout;
                if (timeout > TimeSpan.Zero)
                {
                    source = new CancellationTokenSource();
                    source.CancelAfter(timeout);
                    timeoutToken = source.Token;
                }

                // Initialize the transport
                await Transceiver.InitializeAsync(timeoutToken).ConfigureAwait(false);

                ArraySegment<byte> readBuffer = default;
                if (!Endpoint.IsDatagram) // Datagram connections are always implicitly validated.
                {
                    if (OldProtocol)
                    {
                        if (_connector == null) // The server side has the active role for connection validation.
                        {
                            int offset = 0;
                            while (offset < _validateConnectionFrameIce1.GetByteCount())
                            {
                                offset += await Transceiver.WriteAsync(_validateConnectionFrameIce1,
                                                                       offset,
                                                                       timeoutToken).ConfigureAwait(false);
                            }
                            Debug.Assert(offset == _validateConnectionFrameIce1.GetByteCount());
                        }
                        else // The client side has the passive role for connection validation.
                        {
                            readBuffer = new ArraySegment<byte>(new byte[Ice1Definitions.HeaderSize]);
                            int offset = 0;
                            while (offset < Ice1Definitions.HeaderSize)
                            {
                                offset += await Transceiver.ReadAsync(readBuffer,
                                                                      offset,
                                                                      timeoutToken).ConfigureAwait(false);
                            }

                            Ice1Definitions.CheckHeader(readBuffer.AsSpan(0, 8));
                            var frameType = (Ice1Definitions.FrameType)readBuffer[8];
                            if (frameType != Ice1Definitions.FrameType.ValidateConnection)
                            {
                                throw new InvalidDataException(@$"received ice1 frame with frame type `{frameType
                                    }' before receiving the validate connection frame");
                            }

                            int size = InputStream.ReadInt(readBuffer.AsSpan(10, 4));
                            if (size != Ice1Definitions.HeaderSize)
                            {
                                throw new InvalidDataException(
                                    @$"received an ice1 frame with validate connection type and a size of `{size
                                    }' bytes");
                            }
                        }
                    }
                    else
                    {
                        // TODO: for now ice2 is identical to ice1!
                        if (_connector == null) // The server side has the active role for connection validation.
                        {
                            int offset = 0;
                            while (offset < _validateConnectionFrameIce2.GetByteCount())
                            {
                                offset += await Transceiver.WriteAsync(_validateConnectionFrameIce2,
                                                                       offset,
                                                                       timeoutToken).ConfigureAwait(false);
                            }
                            Debug.Assert(offset == _validateConnectionFrameIce2.GetByteCount());
                        }
                        else // The client side has the passive role for connection validation.
                        {
                            readBuffer = new ArraySegment<byte>(new byte[Ice2Definitions.HeaderSize]);
                            int offset = 0;
                            while (offset < Ice2Definitions.HeaderSize)
                            {
                                offset += await Transceiver.ReadAsync(readBuffer,
                                                                      offset,
                                                                      timeoutToken).ConfigureAwait(false);
                            }

                            Ice2Definitions.CheckHeader(readBuffer.AsSpan(0, 8));
                            var frameType = (Ice2Definitions.FrameType)readBuffer[8];
                            if (frameType != Ice2Definitions.FrameType.ValidateConnection)
                            {
                                throw new InvalidDataException(@$"received ice2 frame with frame type `{frameType
                                    }' before receiving the validate connection frame");
                            }

                            // TODO: this is temporary code. With the 2.0 encoding, sizes are always variable-length
                            // with the length encoded on the first 2 bits of the size. Assuming the size is encoded
                            // on 4 bytes (like we do below) is not correct.
                            int size = InputStream.ReadFixedLengthSize(Endpoint.Protocol.GetEncoding(),
                                                                       readBuffer.AsSpan(10, 4));
                            if (size != Ice2Definitions.HeaderSize)
                            {
                                throw new InvalidDataException(
                                    @$"received an ice2 frame with validate connection type and a size of `{size
                                    }' bytes");
                            }
                        }
                    }
                }

                lock (_mutex)
                {
                    if (_state >= ConnectionState.Closed)
                    {
                        throw _exception!;
                    }

                    if (!Endpoint.IsDatagram) // Datagram connections are always implicitly validated.
                    {
                        if (_connector == null) // The server side has the active role for connection validation.
                        {
                            byte[] frame = OldProtocol ? Ice1Definitions.ValidateConnectionFrame :
                                Ice2Definitions.ValidateConnectionFrame;
                            TraceSentAndUpdateObserver(frame.Length);
                            ProtocolTrace.TraceSend(_communicator, Endpoint.Protocol, frame);
                        }
                        else
                        {
                            TraceReceivedAndUpdateObserver(readBuffer.Count);
                            ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                        }
                    }

                    if (_communicator.TraceLevels.Network >= 1)
                    {
                        var s = new StringBuilder();
                        if (Endpoint.IsDatagram)
                        {
                            s.Append("starting to ");
                            s.Append(_connector != null ? "send" : "receive");
                            s.Append(" ");
                            s.Append(Endpoint.TransportName);
                            s.Append(" datagrams\n");
                            s.Append(Transceiver.ToDetailedString());
                        }
                        else
                        {
                            s.Append(_connector != null ? "established" : "accepted");
                            s.Append(" ");
                            s.Append(Endpoint.TransportName);
                            s.Append(" connection\n");
                            s.Append(ToString());
                        }
                        _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory, s.ToString());
                    }

                    if (_acmLastActivity != Timeout.InfiniteTimeSpan)
                    {
                        _acmLastActivity = Time.Elapsed;
                    }
                    if (_connector != null)
                    {
                        _validated = true;
                    }

                    SetState(ConnectionState.Active);
                }
            }
            catch (OperationCanceledException)
            {
                _ = CloseAsync(new ConnectTimeoutException());
                throw _exception!;
            }
            catch (Exception ex)
            {
                _ = CloseAsync(ex);
                throw;
            }
            finally
            {
                source?.Dispose();
            }
        }

        internal void UpdateObserver()
        {
            lock (_mutex)
            {
                // The observer is attached once the connection is active and detached when closed and the last
                // dispatch completed.
                if (_state < ConnectionState.Active || (_state == ConnectionState.Closed && _dispatchCount == 0))
                {
                    return;
                }

                _observer = _communicator.Observer?.GetConnectionObserver(this, _state, _observer);
                _observer?.Attach();
            }
        }

        private async Task CloseAsync(Exception? exception)
        {
            lock (_mutex)
            {
                if (_state < ConnectionState.Closed)
                {
                    SetState(ConnectionState.Closed, exception ?? _exception!);
                    if (_dispatchCount > 0)
                    {
                        _dispatchTaskCompletionSource ??= new TaskCompletionSource<bool>();
                    }
                    _closeTask = PerformCloseAsync();
                }
            }
            await _closeTask!.ConfigureAwait(false);
        }

        private (List<ArraySegment<byte>>, bool) GetProtocolFrameData(List<ArraySegment<byte>> frame)
        {
            // TODO: Review the protocol tracing? We print out the trace when the frame is about to be sent. It would
            // be simpler to trace the frame before it's queued. This would avoid having these GetXxxData methods.
            // This would also allow to compress the frame from the user thread.
            if (_communicator.TraceLevels.Protocol > 0)
            {
                ProtocolTrace.TraceSend(_communicator, Endpoint.Protocol, frame[0]);
            }
            return (frame, false);
        }

        private (List<ArraySegment<byte>>, bool) GetRequestFrameData(
            OutgoingRequestFrame request,
            int requestId,
            bool compress)
        {
            // TODO: Review the protocol tracing? We print out the trace when the frame is about to be sent. It would
            // be simpler to trace the frame before it's queued. This would avoid having these GetXxxData methods.
            // This would also allow to compress the frame from the user thread.
            List<ArraySegment<byte>> writeBuffer = OldProtocol ?
                Ice1Definitions.GetRequestData(request, requestId) : Ice2Definitions.GetRequestData(request, requestId);

            if (_communicator.TraceLevels.Protocol >= 1)
            {
                ProtocolTrace.TraceFrame(_communicator, writeBuffer[0], request);
            }
            return (writeBuffer, compress);
        }

        private (List<ArraySegment<byte>>, bool) GetResponseFrameData(
            OutgoingResponseFrame response,
            int requestId,
            bool compress)
        {
            // TODO: Review the protocol tracing? We print out the trace when the frame is about to be sent. It would
            // be simpler to trace the frame before it's queued. This would avoid having these GetXxxData methods.
            // This would also allow to compress the frame from the user thread.
            List<ArraySegment<byte>> writeBuffer = OldProtocol ? Ice1Definitions.GetResponseData(response, requestId) :
                Ice2Definitions.GetResponseData(response, requestId);
            if (_communicator.TraceLevels.Protocol > 0)
            {
                ProtocolTrace.TraceFrame(_communicator, writeBuffer[0], response);
            }
            return (writeBuffer, compress);
        }

        private async ValueTask InvokeAsync(
            IncomingRequestFrame request,
            Current current,
            int requestId,
            byte compressionStatus)
        {
            IDispatchObserver? dispatchObserver = null;
            OutgoingResponseFrame? response = null;
            try
            {
                // Notify and set dispatch observer, if any.
                ICommunicatorObserver? communicatorObserver = _communicator.Observer;
                if (communicatorObserver != null)
                {
                    dispatchObserver = communicatorObserver.GetDispatchObserver(current, requestId, request.Size);
                    dispatchObserver?.Attach();
                }

                try
                {
                    IObject? servant = current.Adapter.Find(current.Identity, current.Facet);
                    if (servant == null)
                    {
                        throw new ObjectNotExistException(current.Identity, current.Facet, current.Operation);
                    }

                    ValueTask<OutgoingResponseFrame> vt = servant.DispatchAsync(request, current);
                    if (!current.IsOneway)
                    {
                        response = await vt.ConfigureAwait(false);
                    }
                }
                catch (Exception ex)
                {
                    if (!current.IsOneway)
                    {
                        RemoteException actualEx;
                        if (ex is RemoteException remoteEx && !remoteEx.ConvertToUnhandled)
                        {
                            actualEx = remoteEx;
                        }
                        else
                        {
                            actualEx = new UnhandledException(current.Identity, current.Facet, current.Operation, ex);
                        }
                        Incoming.ReportException(actualEx, dispatchObserver, current);
                        response = new OutgoingResponseFrame(current, actualEx);
                    }
                }

                if (response != null)
                {
                    dispatchObserver?.Reply(response.Size);
                }
            }
            finally
            {
                lock (_mutex)
                {
                    // Send the response if there's a response
                    if (_state < ConnectionState.Closed && response != null)
                    {
                        try
                        {
                            SendFrameAsync(() => GetResponseFrameData(response, requestId, compressionStatus > 0));
                        }
                        catch
                        {
                            // Ignore
                        }
                    }

                    // Decrease the dispatch count
                    Debug.Assert(_dispatchCount > 0);
                    if (--_dispatchCount == 0 && _dispatchTaskCompletionSource != null)
                    {
                        Debug.Assert(_state > ConnectionState.Active);
                        _dispatchTaskCompletionSource.SetResult(true);
                    }
                }

                dispatchObserver?.Detach();
            }
        }

        private (Func<ValueTask>?, ObjectAdapter?) ParseFrameIce1(ArraySegment<byte> readBuffer)
        {
            Debug.Assert(OldProtocol);
            Func<ValueTask>? incoming = null;
            ObjectAdapter? adapter = null;
            lock (_mutex)
            {
                if (_state >= ConnectionState.Closed)
                {
                    throw _exception!;
                }

                // The magic and version fields have already been checked.
                var frameType = (Ice1Definitions.FrameType)readBuffer[8];
                byte compressionStatus = readBuffer[9];
                if (compressionStatus == 2)
                {
                    if (BZip2.IsLoaded)
                    {
                        readBuffer = BZip2.Decompress(readBuffer, Ice1Definitions.HeaderSize, _frameSizeMax);
                    }
                    else
                    {
                        throw new LoadException("compression not supported, bzip2 library not found");
                    }
                }

                switch (frameType)
                {
                    case Ice1Definitions.FrameType.CloseConnection:
                    {
                        ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                        if (Endpoint.IsDatagram)
                        {
                            if (_warn)
                            {
                                _communicator.Logger.Warning(
                                    $"ignoring close connection frame for datagram connection:\n{this}");
                            }
                        }
                        else
                        {
                            throw new ConnectionClosedByPeerException();
                        }
                        break;
                    }

                    case Ice1Definitions.FrameType.Request:
                    {
                        if (_state >= ConnectionState.Closing)
                        {
                            ProtocolTrace.Trace(
                                "received request during closing\n(ignored by server, client will retry)",
                                _communicator,
                                Endpoint.Protocol,
                                readBuffer);
                        }
                        else
                        {
                            var request = new IncomingRequestFrame(Endpoint.Protocol,
                                                                   readBuffer.Slice(Ice1Definitions.HeaderSize + 4));
                            ProtocolTrace.TraceFrame(_communicator, readBuffer, request);
                            if (_adapter == null)
                            {
                                throw new ObjectNotExistException(request.Identity, request.Facet,
                                    request.Operation);
                            }
                            else
                            {
                                adapter = _adapter;
                                int requestId = InputStream.ReadInt(readBuffer.AsSpan(Ice1Definitions.HeaderSize, 4));
                                // TODO: instead of a default cancellation token, we'll have to create a cancellation
                                // token source here and keep track of them in a dictionnary for each dispatch. When a
                                // stream is cancelled with ice2, we'll request cancellation on the cached token source.
                                var current = new Current(_adapter,
                                                          request,
                                                          oneway: requestId == 0,
                                                          cancel: default,
                                                          this);
                                incoming = () => InvokeAsync(request, current, requestId, compressionStatus);
                                ++_dispatchCount;
                            }
                        }
                        break;
                    }

                    case Ice1Definitions.FrameType.RequestBatch:
                    {
                        if (_state >= ConnectionState.Closing)
                        {
                            ProtocolTrace.Trace(
                                "received batch request during closing\n(ignored by server, client will retry)",
                                _communicator,
                                Endpoint.Protocol,
                                readBuffer);
                        }
                        else
                        {
                            ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                            int invokeNum = InputStream.ReadInt(readBuffer.AsSpan(Ice1Definitions.HeaderSize, 4));
                            if (invokeNum < 0)
                            {
                                throw new InvalidDataException(
                                    $"received ice1 RequestBatchMessage with {invokeNum} batch requests");
                            }
                            Debug.Assert(false); // TODO: deal with batch requests
                        }
                        break;
                    }

                    case Ice1Definitions.FrameType.Reply:
                    {
                        int requestId = InputStream.ReadInt(readBuffer.AsSpan(14, 4));
                        var responseFrame = new IncomingResponseFrame(Endpoint.Protocol,
                                                                      readBuffer.Slice(Ice1Definitions.HeaderSize + 4));
                        ProtocolTrace.TraceFrame(_communicator, readBuffer, responseFrame);

                        if (_requests.Remove(requestId,
                                out (TaskCompletionSource<IncomingResponseFrame> TaskCompletionSource,
                                     bool Synchronous) request))
                        {
                            // We can't call SetResult directly from here as it might be trigger the continuations
                            // to run synchronously and it wouldn't be safe to run a continuation with the mutex
                            // locked.
                            //
                            if (request.Synchronous)
                            {
                                request.TaskCompletionSource.SetResult(responseFrame);
                            }
                            else
                            {
                                incoming = () =>
                                {
                                    request.TaskCompletionSource.SetResult(responseFrame);
                                    return new ValueTask(Task.CompletedTask);
                                };
                            }
                            if (_requests.Count == 0)
                            {
                                System.Threading.Monitor.PulseAll(_mutex); // Notify threads blocked in Close()
                            }
                        }
                        break;
                    }

                    case Ice1Definitions.FrameType.ValidateConnection:
                    {
                        ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                        incoming = () =>
                        {
                            try
                            {
                                HeartbeatReceived?.Invoke(this, EventArgs.Empty);
                            }
                            catch (Exception ex)
                            {
                                _communicator.Logger.Error($"connection callback exception:\n{ex}\n{this}");
                            }
                            return default;
                        };
                        break;
                    }

                    default:
                    {
                        ProtocolTrace.Trace(
                            "received unknown frame\n(invalid, closing connection)",
                            _communicator,
                            Endpoint.Protocol,
                            readBuffer);
                        throw new InvalidDataException(
                            $"received ice1 frame with unknown frame type `{frameType}'");
                    }
                }
            }
            return (incoming, adapter);
        }

        private (Func<ValueTask>?, ObjectAdapter?) ParseFrameIce2(ArraySegment<byte> readBuffer)
        {
            // TODO: for now, it's just a slightly simplified version of ParseFrameIce1, with no compression or
            // batch.

            Debug.Assert(!OldProtocol);
            Func<ValueTask>? incoming = null;
            ObjectAdapter? adapter = null;
            lock (_mutex)
            {
                if (_state >= ConnectionState.Closed)
                {
                    throw _exception!;
                }

                // The magic and version fields have already been checked.
                var frameType = (Ice2Definitions.FrameType)readBuffer[8];

                switch (frameType)
                {
                    case Ice2Definitions.FrameType.CloseConnection:
                    {
                        ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                        if (Endpoint.IsDatagram)
                        {
                            if (_warn)
                            {
                                _communicator.Logger.Warning(
                                    $"ignoring close connection frame for datagram connection:\n{this}");
                            }
                        }
                        else
                        {
                            throw new ConnectionClosedByPeerException();
                        }
                        break;
                    }

                    case Ice2Definitions.FrameType.Request:
                    {
                        if (_state >= ConnectionState.Closing)
                        {
                            ProtocolTrace.Trace(
                                "received request during closing\n(ignored by server, client will retry)",
                                _communicator,
                                Endpoint.Protocol,
                                readBuffer);
                        }
                        else
                        {
                            var request = new IncomingRequestFrame(Endpoint.Protocol,
                                                                   readBuffer.Slice(Ice2Definitions.HeaderSize + 4));
                            ProtocolTrace.TraceFrame(_communicator, readBuffer, request);
                            if (_adapter == null)
                            {
                                throw new ObjectNotExistException(request.Identity, request.Facet,
                                    request.Operation);
                            }
                            else
                            {
                                adapter = _adapter;
                                int requestId = InputStream.ReadInt(readBuffer.AsSpan(Ice2Definitions.HeaderSize, 4));
                                // TODO: instead of a default cancellation token, we'll have to create a cancellation
                                // token source here and keep track of them in a dictionnary for each dispatch. When a
                                // stream is cancelled with ice2, we'll request cancellation on the cached token source.
                                var current = new Current(_adapter,
                                                          request,
                                                          oneway: requestId == 0,
                                                          cancel: default,
                                                          this);
                                incoming = () => InvokeAsync(request, current, requestId, compressionStatus: 0);
                                ++_dispatchCount;
                            }
                        }
                        break;
                    }

                    case Ice2Definitions.FrameType.Reply:
                    {
                        int requestId = InputStream.ReadInt(readBuffer.AsSpan(14, 4));
                        var responseFrame = new IncomingResponseFrame(Endpoint.Protocol,
                                                                      readBuffer.Slice(Ice2Definitions.HeaderSize + 4));
                        ProtocolTrace.TraceFrame(_communicator, readBuffer, responseFrame);

                        if (_requests.Remove(requestId,
                                out (TaskCompletionSource<IncomingResponseFrame> TaskCompletionSource,
                                     bool Synchronous) request))
                        {
                            // We can't call SetResult directly from here as it might be trigger the continuations
                            // to run synchronously and it wouldn't be safe to run a continuation with the mutex
                            // locked.
                            //
                            if (request.Synchronous)
                            {
                                request.TaskCompletionSource.SetResult(responseFrame);
                            }
                            else
                            {
                                incoming = () =>
                                {
                                    request.TaskCompletionSource.SetResult(responseFrame);
                                    return new ValueTask(Task.CompletedTask);
                                };
                            }
                            if (_requests.Count == 0)
                            {
                                System.Threading.Monitor.PulseAll(_mutex); // Notify threads blocked in Close()
                            }
                        }
                        break;
                    }

                    case Ice2Definitions.FrameType.ValidateConnection:
                    {
                        ProtocolTrace.TraceReceived(_communicator, Endpoint.Protocol, readBuffer);
                        incoming = () =>
                        {
                            try
                            {
                                HeartbeatReceived?.Invoke(this, EventArgs.Empty);
                            }
                            catch (Exception ex)
                            {
                                _communicator.Logger.Error($"connection callback exception:\n{ex}\n{this}");
                            }
                            return default;
                        };
                        break;
                    }

                    default:
                    {
                        ProtocolTrace.Trace(
                            "received unknown frame\n(invalid, closing connection)",
                            _communicator,
                            Endpoint.Protocol,
                            readBuffer);
                        throw new InvalidDataException(
                            $"received ice2 frame with unknown frame type `{frameType}'");
                    }
                }
            }
            return (incoming, adapter);
        }

        private async Task PerformCloseAsync()
        {
            // Close the transceiver, this should cause pending IO async calls to return.
            try
            {
                Transceiver.ThreadSafeClose();
            }
            catch (Exception ex)
            {
                _communicator.Logger.Error($"unexpected connection exception:\n{ex}\n{Transceiver}");
            }

            if (_state > ConnectionState.Validating && _communicator.TraceLevels.Network >= 1)
            {
                var s = new StringBuilder();
                s.Append("closed ");
                s.Append(Endpoint.TransportName);
                s.Append(" connection\n");
                s.Append(ToString());

                //
                // Trace the cause of unexpected connection closures
                //
                if (!(_exception is ConnectionClosedException ||
                      _exception is ConnectionIdleException ||
                      _exception is ObjectDisposedException))
                {
                    s.Append("\n");
                    s.Append(_exception);
                }

                _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory, s.ToString());
            }

            // Wait for pending receives and sends to complete
            try
            {
                await _sendTask.ConfigureAwait(false);
            }
            catch
            {
            }
            try
            {
                await _receiveTask.ConfigureAwait(false);
            }
            catch
            {
            }

            // Destroy the transport
            try
            {
                Transceiver.Destroy();
            }
            catch (Exception ex)
            {
                _communicator.Logger.Error($"unexpected connection exception:\n{ex}\n{Transceiver}");
            }

            // Notify pending requests of the failure and the close callback. We use the thread pool to ensure the
            // continuations or the callback are not run from this thread which might still lock the connection's mutex.
            await Task.Run(() =>
            {
                foreach ((TaskCompletionSource<IncomingResponseFrame> TaskCompletionSource, bool _) request in
                    _requests.Values)
                {
                    request.TaskCompletionSource.SetException(_exception!);
                }

                // Raise the Closed event
                try
                {
                    _closed?.Invoke(this, EventArgs.Empty);
                }
                catch (Exception ex)
                {
                    _communicator.Logger.Error($"connection callback exception:\n{ex}\n{this}");
                }
            });

            // Wait for all the dispatch to complete before reaping the connection and notifying the observer
            if (_dispatchTaskCompletionSource != null)
            {
                await _dispatchTaskCompletionSource.Task.ConfigureAwait(false);
            }

            _manager.Remove(this);
            _observer?.Detach();
        }

        private async Task PerformGracefulCloseAsync()
        {
            if (!(_exception is ConnectionClosedByPeerException))
            {
                // Wait for the all the dispatch to be completed to ensure the responses are sent.
                if (_dispatchTaskCompletionSource != null)
                {
                    await _dispatchTaskCompletionSource.Task.ConfigureAwait(false);
                }
            }

            CancellationToken timeoutToken;
            CancellationTokenSource? source = null;
            TimeSpan timeout = _communicator.CloseTimeout;
            if (timeout > TimeSpan.Zero)
            {
                source = new CancellationTokenSource();
                source.CancelAfter(timeout);
                timeoutToken = source.Token;
            }

            if (!(_exception is ConnectionClosedByPeerException))
            {
                // Write and wait for the close connection frame to be written
                try
                {
                    await SendFrameAsync(() => GetProtocolFrameData(
                        OldProtocol ? _closeConnectionFrameIce1 : _closeConnectionFrameIce2),
                        timeoutToken).ConfigureAwait(false);
                }
                catch
                {
                    // Ignore
                }
            }

            // Notify the transport of the graceful connection closure.
            try
            {
                await Transceiver.ClosingAsync(_exception!, timeoutToken).ConfigureAwait(false);
            }
            catch
            {
                // Ignore
            }

            // Wait for the connection closure from the peer
            try
            {
                await _receiveTask.WaitAsync(timeoutToken).ConfigureAwait(false);
            }
            catch
            {
                // Ignore
            }

            source?.Dispose();
        }

        private async ValueTask<ArraySegment<byte>> PerformReceiveFrameAsync()
        {
            // Read header
            ArraySegment<byte> readBuffer;
            if (Endpoint.IsDatagram)
            {
                readBuffer = await Transceiver.ReadAsync().ConfigureAwait(false);
            }
            else if (OldProtocol)
            {
                readBuffer = new ArraySegment<byte>(new byte[256], 0, Ice1Definitions.HeaderSize);
                int offset = 0;
                while (offset < Ice1Definitions.HeaderSize)
                {
                    offset += await Transceiver.ReadAsync(readBuffer, offset).ConfigureAwait(false);
                }
            }
            else
            {
                readBuffer = new ArraySegment<byte>(new byte[256], 0, Ice2Definitions.HeaderSize);
                int offset = 0;
                while (offset < Ice2Definitions.HeaderSize)
                {
                    offset += await Transceiver.ReadAsync(readBuffer, offset).ConfigureAwait(false);
                }
            }

            // Check header
            int size;
            if (OldProtocol)
            {
                Ice1Definitions.CheckHeader(readBuffer.AsSpan(0, 8));
                size = InputStream.ReadInt(readBuffer.Slice(10, 4));
                if (size < Ice1Definitions.HeaderSize)
                {
                    throw new InvalidDataException($"received ice1 frame with only {size} bytes");
                }
            }
            else
            {
                Ice2Definitions.CheckHeader(readBuffer.AsSpan(0, 8));
                size = InputStream.ReadFixedLengthSize(Endpoint.Protocol.GetEncoding(), readBuffer.Slice(10, 4));
                if (size < Ice2Definitions.HeaderSize)
                {
                    throw new InvalidDataException($"received ice1 frame with only {size} bytes");
                }
            }

            if (size > _frameSizeMax)
            {
                throw new InvalidDataException($"frame with {size} bytes exceeds Ice.MessageSizeMax value");
            }

            lock (_mutex)
            {
                if (_state >= ConnectionState.Closed)
                {
                    Debug.Assert(_exception != null);
                    throw _exception;
                }

                TraceReceivedAndUpdateObserver(readBuffer.Count);
                if (_acmLastActivity != Timeout.InfiniteTimeSpan)
                {
                    _acmLastActivity = Time.Elapsed;
                }

                // Connection is validated on the first frame. This is only used by setState() to check whether or
                // not we can print a connection warning (a client might close the connection forcefully if the
                // connection isn't validated, we don't want to print a warning in this case).
                _validated = true;
            }

            // Read the remainder of the frame if needed
            if (!Endpoint.IsDatagram)
            {
                if (size > readBuffer.Array!.Length)
                {
                    // Allocate a new array and copy the header over
                    var buffer = new ArraySegment<byte>(new byte[size], 0, size);
                    readBuffer.AsSpan().CopyTo(buffer.AsSpan(0,
                        OldProtocol ? Ice1Definitions.HeaderSize : Ice2Definitions.HeaderSize));
                    readBuffer = buffer;
                }
                else if (size > readBuffer.Count)
                {
                    readBuffer = new ArraySegment<byte>(readBuffer.Array!, 0, size);
                }
                Debug.Assert(size == readBuffer.Count);

                int offset = OldProtocol ? Ice1Definitions.HeaderSize : Ice2Definitions.HeaderSize;
                while (offset < readBuffer.Count)
                {
                    int bytesReceived = await Transceiver.ReadAsync(readBuffer, offset).ConfigureAwait(false);
                    offset += bytesReceived;

                    // Trace the receive progress within the loop as we might be receiving significant amount
                    // of data here.
                    lock (_mutex)
                    {
                        if (_state >= ConnectionState.Closed)
                        {
                            Debug.Assert(_exception != null);
                            throw _exception;
                        }

                        TraceReceivedAndUpdateObserver(bytesReceived);
                        if (_acmLastActivity != Timeout.InfiniteTimeSpan)
                        {
                            _acmLastActivity = Time.Elapsed;
                        }
                    }
                }
            }
            else if (size > readBuffer.Count)
            {
                if (_warnUdp)
                {
                    _communicator.Logger.Warning($"maximum datagram size of {readBuffer.Count} exceeded");
                }
                return default;
            }
            return readBuffer;
        }

        private async ValueTask PerformSendFrameAsync(Func<(List<ArraySegment<byte>>, bool)> getFrameData)
        {
            List<ArraySegment<byte>> writeBuffer;
            bool compress;
            lock (_mutex)
            {
                if (_state >= ConnectionState.Closed)
                {
                    throw _exception!;
                }
                (writeBuffer, compress) = getFrameData();
            }

            // Compress the frame if needed and possible
            int size = writeBuffer.GetByteCount();
            if (OldProtocol && BZip2.IsLoaded && compress)
            {
                List<ArraySegment<byte>>? compressed = null;
                if (size >= 100)
                {
                    compressed = BZip2.Compress(writeBuffer, size, Ice1Definitions.HeaderSize, _compressionLevel);
                }

                if (compressed != null)
                {
                    writeBuffer = compressed!;
                    size = writeBuffer.GetByteCount();
                }
                else // Message not compressed, request compressed response, if any.
                {
                    ArraySegment<byte> header = writeBuffer[0];
                    header[9] = 1; // Write the compression status
                }
            }

            // Write the frame
            int offset = 0;
            while (offset < size)
            {
                int bytesSent = await Transceiver.WriteAsync(writeBuffer, offset).ConfigureAwait(false);
                offset += bytesSent;
                lock (_mutex)
                {
                    if (_state >= ConnectionState.Closed)
                    {
                        throw _exception!;
                    }

                    TraceSentAndUpdateObserver(bytesSent);
                    if (_acmLastActivity != Timeout.InfiniteTimeSpan)
                    {
                        _acmLastActivity = Time.Elapsed;
                    }
                }
            }
        }

        private async ValueTask ReceiveAndDispatchFrameAsync()
        {
            try
            {
                while (true)
                {
                    ArraySegment<byte> readBuffer = await ReceiveFrameAsync().ConfigureAwait(false);
                    if (readBuffer.Count == 0)
                    {
                        // If received without reading, start another receive. This can occur with datagram transports
                        // if the datagram was truncated.
                        continue;
                    }

                    (Func<ValueTask>? incoming, ObjectAdapter? adapter) =
                        OldProtocol ? ParseFrameIce1(readBuffer) : ParseFrameIce2(readBuffer);
                    if (incoming != null)
                    {
                        bool serialize = adapter?.SerializeDispatch ?? false;
                        if (!serialize)
                        {
                            // Start a new receive task before running the incoming dispatch. We start the new receive
                            // task from a separate task because ReadAsync could complete synchronously and we don't
                            // want the dispatch from this read to run before we actually ran the dispatch from this
                            // block. An alternative could be to start a task to run the incoming dispatch and continue
                            // reading with this loop. It would have a negative impact on latency however since
                            // execution of the incoming dispatch would potentially require a thread context switch.
                            if (adapter?.TaskScheduler != null)
                            {
                                _ = TaskRun(ReceiveAndDispatchFrameAsync, adapter.TaskScheduler);
                            }
                            else
                            {
                                _ = Task.Run(ReceiveAndDispatchFrameAsync);
                            }
                        }

                        // Run the received incoming frame
                        if (adapter?.TaskScheduler != null)
                        {
                            await TaskRun(incoming, adapter.TaskScheduler).ConfigureAwait(false);
                        }
                        else
                        {
                            await incoming().ConfigureAwait(false);
                        }

                        // Don't continue reading from this task if we're not using serialization, we started
                        // another receive task above.
                        if (!serialize)
                        {
                            return;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                await CloseAsync(ex);
            }

            static async Task TaskRun(Func<ValueTask> func, TaskScheduler scheduler)
            {
                // First await for the dispatch to be ran on the task scheduler.
                ValueTask task = await Task.Factory.StartNew(func, default, TaskCreationOptions.None,
                    scheduler).ConfigureAwait(false);

                // Now wait for the async dispatch to complete.
                await task.ConfigureAwait(false);
            }
        }

        private async ValueTask<ArraySegment<byte>> ReceiveFrameAsync()
        {
            Task<ArraySegment<byte>> task;
            lock (_mutex)
            {
                if (_state == ConnectionState.Closed)
                {
                    throw _exception!;
                }
                ValueTask<ArraySegment<byte>> readTask = PerformAsync(this);
                if (readTask.IsCompletedSuccessfully)
                {
                    _receiveTask = Task.CompletedTask;
                    return readTask.Result;
                }
                else
                {
                    _receiveTask = task = readTask.AsTask();
                }
            }
            return await task.ConfigureAwait(false);

            static async ValueTask<ArraySegment<byte>> PerformAsync(Connection self)
            {
                try
                {
                    return await self.PerformReceiveFrameAsync().ConfigureAwait(false);
                }
                catch (ConnectionClosedByPeerException ex)
                {
                    _ = self.GracefulCloseAsync(ex);
                    throw;
                }
                catch (Exception ex)
                {
                    _ = self.CloseAsync(ex);
                    throw;
                }
            }
        }

        private Task SendFrameAsync(
            Func<(List<ArraySegment<byte>>, bool)> getFrameData,
            CancellationToken cancel = default)
        {
            lock (_mutex)
            {
                if (_state >= ConnectionState.Closed)
                {
                    throw _exception!;
                }
                cancel.ThrowIfCancellationRequested();
                ValueTask sendTask = QueueAsync(this, _sendTask, getFrameData, cancel);
                _sendTask = sendTask.IsCompletedSuccessfully ? Task.CompletedTask : sendTask.AsTask();
                return _sendTask;
            }

            static async ValueTask QueueAsync(
                Connection self,
                Task previous,
                Func<(List<ArraySegment<byte>>, bool)> getFrameData,
                CancellationToken cancel)
            {
                // Wait for the previous send to complete
                try
                {
                    await previous.ConfigureAwait(false);
                }
                catch (OperationCanceledException)
                {
                    // If the previous send was canceled, ignore and continue sending.
                }

                // If the send got cancelled, throw now. This isn't a fatal connection error, the next pending
                // outgoing will be sent because we ignore the cancelation exception above.
                cancel.ThrowIfCancellationRequested();

                // Perform the write
                try
                {
                    await self.PerformSendFrameAsync(getFrameData).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    _ = self.CloseAsync(ex);
                    throw;
                }
            }
        }

        private void SetState(ConnectionState state, Exception? exception = null)
        {
            // If SetState() is called with an exception, then only closed and closing states are permissible.
            Debug.Assert((exception == null && state < ConnectionState.Closing) ||
                         (exception != null && state >= ConnectionState.Closing));

            if (_exception == null && exception != null)
            {
                // If we are in closed state, an exception must be set.
                Debug.Assert(_state != ConnectionState.Closed);

                _exception = exception;

                // We don't warn if we are not validated.
                if (_warn && _validated)
                {
                    // Don't warn about certain expected exceptions.
                    if (!(_exception is ConnectionClosedException ||
                         _exception is ConnectionIdleException ||
                         _exception is ObjectDisposedException ||
                         (_exception is ConnectionLostException && _state >= ConnectionState.Closing)))
                    {
                        _communicator.Logger.Warning($"connection exception:\n{_exception}\n{this}");
                    }
                }
            }

            Debug.Assert(_state != state); // Don't switch twice.
            switch (state)
            {
                case ConnectionState.Validating:
                {
                    Debug.Assert(false);
                    break;
                }

                case ConnectionState.Active:
                {
                    Debug.Assert(_state == ConnectionState.Validating);
                    // Start the asynchronous operation from the thread pool to prevent eventually reading
                    // synchronously new frames from this thread.
                    _ = Task.Run(ReceiveAndDispatchFrameAsync);
                    break;
                }

                case ConnectionState.Closing:
                {
                    Debug.Assert(_state == ConnectionState.Active);
                    break;
                }

                case ConnectionState.Closed:
                {
                    Debug.Assert(_state < ConnectionState.Closed);
                    break;
                }
            }

            // We register with the connection monitor if our new state is State.Active. ACM monitors the connection
            // once it's initialized and validated and until it's closed. Timeouts for connection establishment and
            // validation are implemented with a timer instead and setup in the outgoing connection factory.
            if (_monitor != null)
            {
                if (state == ConnectionState.Active)
                {
                    if (_acmLastActivity != Timeout.InfiniteTimeSpan)
                    {
                        _acmLastActivity = Time.Elapsed;
                    }
                    _monitor.Add(this);
                }
                else if (_state == ConnectionState.Active)
                {
                    Debug.Assert(state > ConnectionState.Active);
                    _monitor.Remove(this);
                }
            }

            if (_communicator.Observer != null)
            {
                if (_state != state)
                {
                    _observer = _communicator.Observer!.GetConnectionObserver(this, state, _observer);
                    if (_observer != null)
                    {
                        _observer.Attach();
                    }
                }
                if (_observer != null && state == ConnectionState.Closed && _exception != null)
                {
                    if (!(_exception is ConnectionClosedException ||
                          _exception is ConnectionIdleException ||
                          _exception is ObjectDisposedException ||
                         (_exception is ConnectionLostException && _state >= ConnectionState.Closing)))
                    {
                        _observer.Failed(_exception.GetType().FullName!);
                    }
                }
            }
            _state = state;
        }

        private void TraceReceivedAndUpdateObserver(int length)
        {
            if (_communicator.TraceLevels.Network >= 3 && length > 0)
            {
                _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory,
                    $"received {length} bytes via {Endpoint.TransportName}\n{this}");
            }

            if (_observer != null && length > 0)
            {
                _observer.ReceivedBytes(length);
            }
        }

        private void TraceSentAndUpdateObserver(int length)
        {
            if (_communicator.TraceLevels.Network >= 3 && length > 0)
            {
                _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory,
                    $"sent {length} bytes via {Endpoint.TransportName}\n{this}");
            }

            if (_observer != null && length > 0)
            {
                _observer.SentBytes(length);
            }
        }
    }

    /// <summary>Represents a connection to an IP-endpoint.</summary>
    public abstract class IPConnection : Connection
    {
        /// <summary>The socket local IP-endpoint or null if it is not available.</summary>
        public System.Net.IPEndPoint? LocalEndpoint
        {
            get
            {
                try
                {
                    return Transceiver.Fd()?.LocalEndPoint as System.Net.IPEndPoint;
                }
                catch
                {
                    return null;
                }
            }
        }

        /// <summary>The socket remote IP-endpoint or null if it is not available.</summary>
        public System.Net.IPEndPoint? RemoteEndpoint
        {
            get
            {
                try
                {
                    return Transceiver.Fd()?.RemoteEndPoint as System.Net.IPEndPoint;
                }
                catch
                {
                    return null;
                }
            }
        }

        protected IPConnection(
            IConnectionManager manager,
            Endpoint endpoint,
            ITransceiver transceiver,
            IConnector? connector,
            string connectionId,
            ObjectAdapter? adapter)
            : base(manager, endpoint, transceiver, connector, connectionId, adapter)
        {
        }
    }

    /// <summary>Represents a connection to a TCP-endpoint.</summary>
    public class TcpConnection : IPConnection
    {
        /// <summary>Gets a Boolean value that indicates whether the certificate revocation list is checked during the
        /// certificate validation process.</summary>
        public bool CheckCertRevocationStatus => SslStream?.CheckCertRevocationStatus ?? false;
        /// <summary>Gets a Boolean value that indicates whether this SslStream uses data encryption.</summary>
        public bool IsEncrypted => SslStream?.IsEncrypted ?? false;
        /// <summary>Gets a Boolean value that indicates whether both server and client have been authenticated.
        /// </summary>
        public bool IsMutuallyAuthenticated => SslStream?.IsMutuallyAuthenticated ?? false;
        /// <summary>Gets a Boolean value that indicates whether the data sent using this stream is signed.</summary>
        public bool IsSigned => SslStream?.IsSigned ?? false;

        /// <summary>Gets the certificate used to authenticate the local endpoint or null if no certificate was
        /// supplied.</summary>
        public X509Certificate? LocalCertificate => SslStream?.LocalCertificate;

        /// <summary>The negotiated application protocol in TLS handshake.</summary>
        public SslApplicationProtocol? NegotiatedApplicationProtocol => SslStream?.NegotiatedApplicationProtocol;

        /// <summary>Gets the cipher suite which was negotiated for this connection.</summary>
        public TlsCipherSuite? NegotiatedCipherSuite => SslStream?.NegotiatedCipherSuite;
        /// <summary>Gets the certificate used to authenticate the remote endpoint or null if no certificate was
        /// supplied.</summary>
        public X509Certificate? RemoteCertificate => SslStream?.RemoteCertificate;

        /// <summary>Gets a value that indicates the security protocol used to authenticate this connection or
        /// null if the connection is not secure.</summary>
        public SslProtocols? SslProtocol => SslStream?.SslProtocol;

        private SslStream? SslStream => (Transceiver as SslTransceiver)?.SslStream ??
            (Transceiver as WSTransceiver)?.SslStream;

        protected internal TcpConnection(
            IConnectionManager manager,
            Endpoint endpoint,
            ITransceiver transceiver,
            IConnector? connector,
            string connectionId,
            ObjectAdapter? adapter)
            : base(manager, endpoint, transceiver, connector, connectionId, adapter)
        {
        }
    }

    /// <summary>Represents a connection to a UDP-endpoint.</summary>
    public class UdpConnection : IPConnection
    {
        /// <summary>The multicast IP-endpoint for a multicast connection otherwise null.</summary>
        public System.Net.IPEndPoint? McastEndpoint => (Transceiver as UdpTransceiver)?.McastAddress;

        protected internal UdpConnection(
            IConnectionManager manager,
            Endpoint endpoint,
            ITransceiver transceiver,
            IConnector? connector,
            string connectionId,
            ObjectAdapter? adapter)
            : base(manager, endpoint, transceiver, connector, connectionId, adapter)
        {
        }
    }

    /// <summary>Represents a connection to a WS-endpoint.</summary>
    public class WSConnection : TcpConnection
    {
        /// <summary>The HTTP headers in the WebSocket upgrade request.</summary>
        public IReadOnlyDictionary<string, string> Headers => ((WSTransceiver)Transceiver).Headers;

        protected internal WSConnection(
            IConnectionManager manager,
            Endpoint endpoint,
            ITransceiver transceiver,
            IConnector? connector,
            string connectionId,
            ObjectAdapter? adapter)
            : base(manager, endpoint, transceiver, connector, connectionId, adapter)
        {
        }
    }
}
