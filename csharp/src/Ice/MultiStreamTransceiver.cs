//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Threading.Tasks.Sources;
using System.Collections.Generic;
using System.Collections.Concurrent;

using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice
{
    public abstract class TransceiverStream : IDisposable
    {
        public long Id
        {
            get
            {
                if (_id == -1)
                {
                    throw new InvalidOperationException("stream ID isn't allocated yet");
                }
                return _id;
            }
            set
            {
                Debug.Assert(_id == -1);
                _id = value;
                _transceiver.Streams.TryAdd(_id, this);
            }
        }
        public bool IsIncoming => _id % 2 == (_transceiver.IsIncoming ? 0 : 1);
        public bool IsBidirectional { get; }
        public bool IsControl => _id == 2 || _id == 3;
        protected virtual ReadOnlyMemory<byte> Header => ArraySegment<byte>.Empty;
        private protected bool IsStarted => _id != -1;
        private long _id = -1;

        internal IObserver? Observer
        {
            get => _observer;

            set
            {
                _observer = value;
                _observer?.Attach();
            }
        }

        private IObserver? _observer;
        private readonly MultiStreamTransceiver _transceiver;

        public abstract void Abort(Exception ex);

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected abstract ValueTask<bool> ReceiveAsync(ArraySegment<byte> buffer, CancellationToken cancel);
        protected abstract ValueTask ResetAsync();
        protected abstract ValueTask SendAsync(IList<ArraySegment<byte>> buffer, bool fin, CancellationToken cancel);

        protected TransceiverStream(long streamId, MultiStreamTransceiver transceiver)
        {
            _transceiver = transceiver;
            _id = streamId;
            _transceiver.Streams.TryAdd(_id, this);
            IsBidirectional = _id % 4 < 2;
        }

        protected TransceiverStream(bool bidirectional, MultiStreamTransceiver transceiver)
        {
            IsBidirectional = bidirectional;
            _transceiver = transceiver;
        }

        protected virtual void Dispose(bool disposing)
        {
            _observer?.Detach();
            if (_transceiver.Streams.Remove(Id, out TransceiverStream? _))
            {
                _transceiver.CheckStreamsEmpty();
            }
        }

        internal virtual void CancelSourceIfStreamReset(CancellationTokenSource source)
        {
            async void CancelSourceIfStreamReset()
            {
                try
                {
                    await ReceiveAsync(new byte[1], CancellationToken.None).ConfigureAwait(false);
                }
                catch (StreamResetByPeerException)
                {
                    source.Cancel();
                }
                catch
                {
                    // Ignore
                }
            }
            CancelSourceIfStreamReset();
        }

        internal virtual async ValueTask<(long, string message)> ReceiveCloseFrameAsync()
        {
            byte frameType = _transceiver.Endpoint.Protocol == Protocol.Ice1 ?
                (byte)Ice1Definitions.FrameType.CloseConnection : (byte)Ice2Definitions.FrameType.Close;

            (ArraySegment<byte> data, bool fin) = await ReceiveFrameAsync(frameType, CancellationToken.None);
            if (!fin)
            {
                throw new InvalidDataException($"expected end of stream after Close frame");
            }

            if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
            {
                ProtocolTrace.TraceFrame(_transceiver.Endpoint, Id, data, frameType);
            }

            if (_transceiver.Endpoint.Protocol == Protocol.Ice1)
            {
                return (0, "connection gracefully closed by peer");
            }
            else
            {
                var istr = new InputStream(data, Ice2Definitions.Encoding);
                return (istr.ReadVarLong(), istr.ReadString());
            }
        }

        internal virtual async ValueTask ReceiveInitializeFrameAsync(CancellationToken cancel)
        {
            byte frameType = _transceiver.Endpoint.Protocol == Protocol.Ice1 ?
                (byte)Ice1Definitions.FrameType.ValidateConnection : (byte)Ice2Definitions.FrameType.Initialize;

            (ArraySegment<byte> data, bool fin) = await ReceiveFrameAsync(frameType, cancel);
            if (fin)
            {
                throw new InvalidDataException($"received unexpected end of stream after initialize frame");
            }

            if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
            {
                ProtocolTrace.TraceFrame(_transceiver.Endpoint, Id, data, (byte)frameType);
            }

            if (_transceiver.Endpoint.Protocol == Protocol.Ice1)
            {
                if (data.Count > 0)
                {
                    throw new InvalidDataException(
                        @$"received an ice1 frame with validate connection type and a size of `{data.Count}' bytes");
                }
            }
            else
            {
                // TODO: read initialize settings?
            }
        }

        internal async ValueTask<(IncomingRequestFrame, bool)> ReceiveRequestFrameAsync(CancellationToken cancel)
        {
            byte frameType = _transceiver.Endpoint.Protocol == Protocol.Ice1 ?
                (byte)Ice1Definitions.FrameType.Request : (byte)Ice2Definitions.FrameType.Request;

            (ArraySegment<byte> data, bool fin) = await ReceiveFrameAsync(frameType, cancel);

            var request = new IncomingRequestFrame(_transceiver.Endpoint.Protocol,
                                                   data,
                                                   _transceiver.IncomingFrameSizeMax);

            if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
            {
                ProtocolTrace.TraceFrame(_transceiver.Endpoint, Id, request);
            }

            return (request, fin);
        }

        internal async ValueTask<(IncomingResponseFrame, bool)> ReceiveResponseFrameAsync(CancellationToken cancel)
        {
            try
            {
                byte frameType = _transceiver.Endpoint.Protocol == Protocol.Ice1 ?
                    (byte)Ice1Definitions.FrameType.Reply : (byte)Ice2Definitions.FrameType.Response;

                (ArraySegment<byte> data, bool fin) = await ReceiveFrameAsync(frameType, cancel);

                var response = new IncomingResponseFrame(_transceiver.Endpoint.Protocol,
                                                         data,
                                                         _transceiver.IncomingFrameSizeMax);

                if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
                {
                    ProtocolTrace.TraceFrame(_transceiver.Endpoint, Id, response);
                }

                (Observer as IRemoteObserver)?.Reply(response.Size);
                return (response, fin);
            }
            catch (OperationCanceledException)
            {
                await ResetAsync().ConfigureAwait(false);
                throw;
            }
        }

        internal virtual async ValueTask SendCloseFrameAsync(long streamId, string reason, CancellationToken cancel)
        {
            if (_transceiver.Endpoint.Protocol == Protocol.Ice1)
            {
                await SendAsync(Ice1Definitions.CloseConnectionFrame, true, cancel).ConfigureAwait(false);

                if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
                {
                    ProtocolTrace.TraceFrame(_transceiver.Endpoint,
                                             Id,
                                             new List<ArraySegment<byte>>(),
                                             (byte)Ice1Definitions.FrameType.CloseConnection);
                }
            }
            else
            {
                var data = new List<ArraySegment<byte>>() { new byte[256] };
                var ostr = new OutputStream(Ice2Definitions.Encoding, data);
                if (!Header.IsEmpty)
                {
                    ostr.WriteByteSpan(Header.Span);
                }
                ostr.WriteByte((byte)Ice2Definitions.FrameType.Close);
                OutputStream.Position sizePos = ostr.StartFixedLengthSize();
                OutputStream.Position pos = ostr.Tail;
                ostr.WriteVarLong(streamId);
                ostr.WriteString(reason);
                ostr.EndFixedLengthSize(sizePos);
                data[^1] = data[^1].Slice(0, ostr.Finish().Offset);
                data[0] = new ArraySegment<byte>(data[0].Array!, 0, data[0].Count);

                await SendAsync(data, true, cancel).ConfigureAwait(false);

                if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
                {
                    ProtocolTrace.TraceFrame(_transceiver.Endpoint,
                                             Id,
                                             data.Slice(pos, ostr.Tail),
                                             (byte)Ice2Definitions.FrameType.Close);
                }
            }
        }

        internal virtual async ValueTask SendInitializeFrameAsync(CancellationToken cancel)
        {
            if (_transceiver.Endpoint.Protocol == Protocol.Ice1)
            {
                await SendAsync(Ice1Definitions.ValidateConnectionFrame, false, cancel).ConfigureAwait(false);

                if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
                {
                    ProtocolTrace.TraceFrame(_transceiver.Endpoint,
                                             Id,
                                             new List<ArraySegment<byte>>(),
                                             (byte)Ice1Definitions.FrameType.ValidateConnection);
                }
            }
            else
            {
                var data = new List<ArraySegment<byte>>() { new byte[Header.Length + 2] };
                Header.CopyTo(data[0]);
                data[0].Slice(Header.Length)[0] = (byte)Ice2Definitions.FrameType.Initialize;
                data[0].Slice(Header.Length + 1).AsSpan().WriteFixedLengthSize20(0);

                // TODO: send protocol specific settings from the frame?

                await SendAsync(data, false, cancel).ConfigureAwait(false);

                if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
                {
                    ProtocolTrace.TraceFrame(_transceiver.Endpoint,
                                             Id,
                                             new List<ArraySegment<byte>>(),
                                             (byte)Ice2Definitions.FrameType.Initialize);
                }
            }
        }

        internal async ValueTask SendRequestFrameAsync(OutgoingRequestFrame request, bool fin, CancellationToken cancel)
        {
            try
            {
                await SendFrameAsync(request, fin, cancel).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                await ResetAsync().ConfigureAwait(false);
                throw;
            }
        }

        internal ValueTask SendResponseFrameAsync(OutgoingResponseFrame response, bool fin, CancellationToken cancel)
        {
            (Observer as IDispatchObserver)?.Reply(response.Size);
            return SendFrameAsync(response, fin, cancel);
        }

        private protected virtual async ValueTask<(ArraySegment<byte>, bool)> ReceiveFrameAsync(
            byte expectedFrameType,
            CancellationToken cancel)
        {
            // The default implementation only supports the Ice2 protocol
            Debug.Assert(_transceiver.Endpoint.Protocol == Protocol.Ice2);

            // Read the Ice2 protocol header (byte frameType, varuint size)
            ArraySegment<byte> buffer = new byte[256];
            bool fin = await ReceiveAsync(buffer.Slice(0, 2), cancel).ConfigureAwait(false);
            var frameType = (Ice2Definitions.FrameType)buffer[0];
            if ((byte)frameType != expectedFrameType)
            {
                throw new InvalidDataException($"received frame type {frameType} but expected {expectedFrameType}");
            }

            int sizeLength = buffer[1].ReadSizeLength20();
            if (sizeLength > 1)
            {
                fin = await ReceiveAsync(buffer.Slice(2, sizeLength - 1), cancel).ConfigureAwait(false);
            }
            int size = buffer.Slice(1).AsReadOnlySpan().ReadSize20().Size;

            // Read the frame data
            if (size > 0)
            {
                buffer = size > buffer.Array!.Length ? new ArraySegment<byte>(new byte[size]) : buffer.Slice(0, size);
                fin = await ReceiveAsync(buffer, cancel).ConfigureAwait(false);
            }
            return (buffer, fin);
        }

        private protected virtual async ValueTask SendFrameAsync(
            OutgoingFrame frame,
            bool fin,
            CancellationToken cancel)
        {
            // The default implementation only supports the Ice2 protocol
            Debug.Assert(_transceiver.Endpoint.Protocol == Protocol.Ice2);

            var data = new List<ArraySegment<byte>>(frame.Data.Count + 1);
            int headerLength = Header.Length;
            byte[] headerData = new byte[headerLength + 1 + OutputStream.GetVarLongLength(frame.Size)];
            Header.CopyTo(headerData);
            if (frame is OutgoingRequestFrame)
            {
                headerData[headerLength] = (byte)Ice2Definitions.FrameType.Request;
            }
            else
            {
                headerData[headerLength] = (byte)Ice2Definitions.FrameType.Response;
            }
            headerData.AsSpan(headerLength + 1).WriteFixedLengthSize20(frame.Size);
            data.Add(headerData);
            data.AddRange(frame.Data);

            await SendAsync(data, fin, cancel).ConfigureAwait(false);

            if (_transceiver.Endpoint.Communicator.TraceLevels.Protocol >= 1)
            {
                ProtocolTrace.TraceFrame(_transceiver.Endpoint, Id, frame);
            }
        }
    }

    // The signaled transceiver stream class provide signaling functionality using the IValueTaskSource interface.
    // It useful to implement streams that depend on the transceiver for receiving data: the transceiver signals
    // the stream when new data is available.
    internal abstract class SignaledTransceiverStream<T> : TransceiverStream, IValueTaskSource<T>
    {
        private ManualResetValueTaskSourceCore<T> _source;

        public override void Abort(Exception ex)
        {
            try
            {
                _source.RunContinuationsAsynchronously = true;
                _source.SetException(ex);
            }
            catch
            {
            }
        }

        protected SignaledTransceiverStream(long streamId, MultiStreamTransceiver transceiver) :
            base(streamId, transceiver)
        {
        }

        protected SignaledTransceiverStream(bool bidirectional, MultiStreamTransceiver transceiver) :
            base(bidirectional, transceiver)
        {
        }

        protected void SignalCompletion(T result, bool runContinuationAsynchronously)
        {
            _source.RunContinuationsAsynchronously = runContinuationAsynchronously;
            _source.SetResult(result);
        }

        protected ValueTask<T> WaitSignalAsync(CancellationToken cancel = default)
        {
            if (cancel.CanBeCanceled)
            {
                return new ValueTask<T>(this, _source.Version).WaitAsync(cancel);
            }
            else
            {
                return new ValueTask<T>(this, _source.Version);
            }
        }

        T IValueTaskSource<T>.GetResult(short token)
        {
            Debug.Assert(token == _source.Version);
            try
            {
                return _source.GetResult(token);
            }
            finally
            {
                _source.Reset();
            }
        }

        ValueTaskSourceStatus IValueTaskSource<T>.GetStatus(short token) => _source.GetStatus(token);

        void IValueTaskSource<T>.OnCompleted(
            Action<object?> continuation,
            object? state,
            short token,
            ValueTaskSourceOnCompletedFlags flags) => _source.OnCompleted(continuation, state, token, flags);
    }

    public abstract class MultiStreamTransceiver : IDisposable
    {
        public Endpoint Endpoint { get; }
        public bool IsIncoming { get; }

        internal int IncomingFrameSizeMax { get; }
        internal TimeSpan LastActivity { get; set; }
        internal IConnectionObserver? Observer
        {
            get
            {
                lock (Mutex)
                {
                    return _observer;
                }
            }
            set
            {
                lock (Mutex)
                {
                    _observer = value;
                    _observer?.Attach();
                }
            }
        }
        internal event EventHandler? Ping;
        internal readonly ConcurrentDictionary<long, TransceiverStream> Streams =
            new ConcurrentDictionary<long, TransceiverStream>();

        private IConnectionObserver? _observer;
        // The mutex provides thread-safety for the _observer and LastActivity data members.
        private readonly object Mutex = new object();
        private volatile TaskCompletionSource? _streamsEmptySource;

        /// <summary>Abort the transport.</summary>
        public abstract void Abort();

        /// <summary>Receives a new frame.</summary>
        public abstract ValueTask<TransceiverStream> AcceptStreamAsync(CancellationToken cancel);

        public abstract ValueTask CloseAsync(Exception ex, CancellationToken cancel);

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>Sends ping data to defer the idle timeout.</summary>
        public abstract ValueTask PingAsync(CancellationToken cancel);

        /// <summary>Initializes the transport.</summary>
        public abstract ValueTask InitializeAsync(CancellationToken cancel);

        /// <summary>Creates a new stream.</summary>
        public abstract TransceiverStream CreateStream(bool bidirectional);

        protected MultiStreamTransceiver(Endpoint endpoint, ObjectAdapter? adapter)
        {
            Endpoint = endpoint;
            IsIncoming = adapter != null;
            IncomingFrameSizeMax = adapter?.IncomingFrameSizeMax ?? Endpoint.Communicator.IncomingFrameSizeMax;
            LastActivity = Time.Elapsed;
            Mutex = new object();
        }

        protected virtual void Dispose(bool disposing)
        {
            foreach (TransceiverStream stream in Streams.Values)
            {
                Debug.Assert(stream.IsControl);
                stream.Dispose();
            }
        }

        protected void PingReceived()
        {
            // Capture the event handler which can be modified anytime by the user code.
            EventHandler? callback = Ping;
            if (callback != null)
            {
                Task.Run(() =>
                {
                    try
                    {
                        callback.Invoke(this, EventArgs.Empty);
                    }
                    catch (Exception ex)
                    {
                        Endpoint.Communicator.Logger.Error($"ping event handler raised an exception:\n{ex}\n{this}");
                    }
                });
            }
        }

        protected void Received(int length)
        {
            lock (Mutex)
            {
                Debug.Assert(length > 0);
                _observer?.ReceivedBytes(length);

                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 3)
            {
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory,
                    $"received {length} bytes via {Endpoint.TransportName}\n{this}");
            }
        }

        protected void Sent(int length)
        {
            lock (Mutex)
            {
                Debug.Assert(length > 0);
                _observer?.SentBytes(length);

                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 3 && length > 0)
            {
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory,
                    $"sent {length} bytes via {Endpoint.TransportName}\n{this}");
            }
        }

        protected bool TryGetStream<T>(long streamId, [NotNullWhen(returnValue: true)] out T? value)
            where T : TransceiverStream
        {
            if (Streams.TryGetValue(streamId, out TransceiverStream? stream))
            {
                value = (T)stream;
                return true;
            }
            value = null;
            return false;
        }

        internal async ValueTask AbortAsync(Exception exception)
        {
            // Abort the transport
            Abort();

            // Abort the streams and Wait for all all the streams to be completed
            AbortStreams(exception);

            await WaitForEmptyStreamsAsync().ConfigureAwait(false);

            lock (Mutex)
            {
                _observer?.Detach();
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 1)
            {
                var s = new StringBuilder();
                s.Append("closed ");
                s.Append(Endpoint.TransportName);
                s.Append(" connection\n");
                s.Append(ToString());

                //
                // Trace the cause of unexpected connection closures
                //
                if (!(exception is ConnectionClosedException ||
                      exception is ConnectionIdleException ||
                      exception is ObjectDisposedException))
                {
                    s.Append('\n');
                    s.Append(exception);
                }

                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory, s.ToString());
            }
        }

        internal void Initialized()
        {
            lock (Mutex)
            {
                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 1)
            {
                var s = new StringBuilder();
                if (Endpoint.IsDatagram)
                {
                    s.Append("starting to ");
                    s.Append(IsIncoming ? "receive" : "send");
                    s.Append(' ');
                    s.Append(Endpoint.TransportName);
                    s.Append(" datagrams\n");
                }
                else
                {
                    s.Append(IsIncoming ? "accepted" : "established");
                    s.Append(' ');
                    s.Append(Endpoint.TransportName);
                    s.Append(" connection\n");
                }
                s.Append(ToString());
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory, s.ToString());
            }
        }

        internal void CheckStreamsEmpty()
        {
            if (Streams.Count <= 2)
            {
                _streamsEmptySource?.TrySetResult();
            }
        }

        internal virtual async ValueTask<TransceiverStream> ReceiveInitializeFrameAsync(CancellationToken cancel)
        {
            TransceiverStream stream = await AcceptStreamAsync(cancel).ConfigureAwait(false);
            await stream.ReceiveInitializeFrameAsync(cancel).ConfigureAwait(false);
            return stream;
        }

        internal virtual async ValueTask<TransceiverStream> SendInitializeFrameAsync(CancellationToken cancel)
        {
            TransceiverStream stream = CreateStream(false);
            await stream.SendInitializeFrameAsync(cancel).ConfigureAwait(false);
            return stream;
        }

        internal long AbortStreams(Exception exception, Func<TransceiverStream, bool>? predicate = null)
        {
            long largestStreamId = 0;
            foreach (TransceiverStream stream in Streams.Values)
            {
                if (!stream.IsControl && (predicate?.Invoke(stream) ?? true))
                {
                    stream.Abort(exception);
                }
                else if (stream.Id > largestStreamId)
                {
                    largestStreamId = stream.Id;
                }
            }
            return largestStreamId;
        }

        internal async ValueTask WaitForEmptyStreamsAsync()
        {
            if (Streams.Count > 2)
            {
                // Wait for all the streams to complete.
                _streamsEmptySource ??= new TaskCompletionSource();
                CheckStreamsEmpty();
                await _streamsEmptySource.Task.ConfigureAwait(false);
            }
        }
    }

    // A multi-stream transceiver based on a single stream transceiver.
    public abstract class MultiStreamTransceiverWithUnderlyingTransceiver : MultiStreamTransceiver
    {
        public ITransceiver Underlying { get; }

        public override string ToString() => Underlying.ToString()!;

        public override void Abort() => Underlying.Dispose();

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing)
            {
                Underlying.Dispose();
            }
        }

        protected MultiStreamTransceiverWithUnderlyingTransceiver(
            Endpoint endpoint,
            ObjectAdapter? adapter,
            ITransceiver transceiver) :
            base(endpoint, adapter) => Underlying = transceiver;
    }
}
