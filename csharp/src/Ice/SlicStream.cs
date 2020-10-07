// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    // The stream implementation for Slic. It implements IValueTaskSource<> directly instead of using
    // ManualResetValueTaskCompletionSource<T> to minimize the number of heap allocations.
    internal class SlicStream : SignaledTransceiverStream<(int, bool)>
    {
        protected override ReadOnlyMemory<byte> Header => SlicDefinitions.FrameHeader;
        private volatile Exception? _exception;
        private readonly SlicTransceiver _transceiver;
        private int _receivedOffset;
        private int _receivedSize;
        private bool _receivedEndOfStream;

        public override void Abort(Exception ex)
        {
            base.Abort(ex);
            _exception = ex;
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
            if (disposing)
            {
                if (IsSignaled && _exception == null)
                {
                    // If the stream is signaled and wasn't aborted, it was canceled or discarded. We get the
                    // information for the frame to receive in order to pass it back to the transceiver below.
                    ValueTask<(int, bool)> valueTask = WaitSignalAsync(CancellationToken.None);
                    Debug.Assert(valueTask.IsCompleted);
                    (_receivedSize, _receivedEndOfStream) = valueTask.Result;
                    _receivedOffset = 0;
                }

                // If there's still data pending to be receive for the stream, we notify the transceiver that
                // we're abandoning the reading. It will finish to read the stream's frame data in order to
                // continue receiving frames for other streams.
                if (_receivedOffset < _receivedSize)
                {
                    _transceiver.FinishedReceivedStreamData(Id, _receivedOffset, _receivedSize, _receivedEndOfStream);
                }

                if (IsIncoming && !IsBidirectional && !IsControl)
                {
                    Interlocked.Decrement(ref _transceiver.UnidirectionalStreamCount);
                    _transceiver.PrepareAndSendFrameAsync(SlicDefinitions.FrameType.StreamUnidirectionalFin, null);
                }
                else if (!IsIncoming && IsBidirectional && IsStarted)
                {
                    Debug.Assert(IsBidirectional);
                    _transceiver.BidirectionalStreamSemaphore!.Release();
                }
            }
        }

        protected override async ValueTask<bool> ReceiveAsync(ArraySegment<byte> buffer, CancellationToken cancel)
        {
            int offset = 0;
            while (offset < buffer.Count)
            {
                if (_receivedSize == _receivedOffset)
                {
                    // Wait to be signaled for the reception of a new stream frame for this stream.
                    (_receivedSize, _receivedEndOfStream) = await WaitSignalAsync(cancel).ConfigureAwait(false);
                    _receivedOffset = 0;
                    if (_receivedEndOfStream && offset + _receivedSize < buffer.Count)
                    {
                        throw new InvalidDataException("received less data than expected with last frame");
                    }
                }

                if (_exception != null)
                {
                    throw _exception;
                }

                // Read and append the received stream frame data into the given buffer.
                int size = Math.Min(_receivedSize - _receivedOffset, buffer.Slice(offset).Count);
                await _transceiver.ReceiveDataAsync(buffer.Slice(offset, size),
                                                    CancellationToken.None).ConfigureAwait(false);
                offset += size;
                _receivedOffset += size;

                // If we've consumed the whole Slic frame, notify the transceiver that it can start receiving
                // a new frame.
                if (_receivedOffset == _receivedSize)
                {
                    _transceiver.FinishedReceivedStreamData(Id, _receivedOffset, _receivedSize, _receivedEndOfStream);
                }
            }
            return _receivedEndOfStream;
        }

        protected override async ValueTask ResetAsync() =>
            await _transceiver.PrepareAndSendFrameAsync(SlicDefinitions.FrameType.StreamReset, ostr =>
                {
                    ostr.WriteVarLong(Id);
                    ostr.WriteVarLong(0); // TODO: reason code?
                    return Id;
                }, CancellationToken.None).ConfigureAwait(false);

        protected override async ValueTask SendAsync(
            IList<ArraySegment<byte>> buffer,
            bool fin,
            CancellationToken cancel)
        {
            // Ensure the caller reserved space for the Slic header by checking for the sentinel header.
            Debug.Assert(Header.Span.SequenceEqual(buffer[0].Slice(0, Header.Length)));

            int size = buffer.GetByteCount();

            int maxFrameSize = _transceiver.Options.PacketSize;
            if (size > maxFrameSize)
            {
                var sendBuffer = new List<ArraySegment<byte>>(buffer.Count);
                int offset = 0;
                var start = new OutputStream.Position();
                while (offset < size)
                {
                    int sendSize = 0;
                    sendBuffer.Clear();

                    if (offset > 0)
                    {
                        sendBuffer.Add(buffer[0].Slice(0, Header.Length));
                        sendSize += sendBuffer[0].Count;
                    }

                    bool lastBuffer = false;
                    for (int i = start.Segment; i < buffer.Count; ++i)
                    {
                        int segmentOffset = i == start.Segment ? start.Offset : 0;
                        if (sendSize + buffer[i].Slice(segmentOffset).Count > maxFrameSize)
                        {
                            sendBuffer.Add(buffer[i].Slice(segmentOffset, maxFrameSize - sendSize));
                            start = new OutputStream.Position(i, segmentOffset + sendBuffer[^1].Count);
                            sendSize += sendBuffer[^1].Count;
                            break;
                        }
                        else
                        {
                            sendBuffer.Add(buffer[i].Slice(segmentOffset));
                            sendSize += sendBuffer[^1].Count;
                            lastBuffer = i + 1 == buffer.Count;
                        }
                    }

                    offset += sendSize;
                    await SendFrameAsync(sendSize, lastBuffer && fin, sendBuffer).ConfigureAwait(false);
                }
            }
            else
            {
                await SendFrameAsync(size, fin, buffer).ConfigureAwait(false);
            }

            async Task SendFrameAsync(int frameSize, bool fin, IList<ArraySegment<byte>> buffer)
            {
                if (_exception != null)
                {
                    throw _exception;
                }
                else if (!IsStarted)
                {
                    Debug.Assert(!IsIncoming);

                    // Ensure we don't open more streams than the peer allows.
                    if (IsBidirectional)
                    {
                        await _transceiver.BidirectionalStreamSemaphore!.WaitAsync(cancel).ConfigureAwait(false);
                    }
                    else
                    {
                        await _transceiver.UnidirectionalStreamSemaphore!.WaitAsync(cancel).ConfigureAwait(false);
                    }

                    // Ensure we allocate and queue the first stream frame atomically to ensure the receiver won't
                    // receive stream frames with out-of-order stream IDs.
                    Task task;
                    lock (_transceiver.Mutex)
                    {
                        Id = _transceiver.AllocateId(IsBidirectional);
                        task = SendFrameAsync(frameSize, fin, buffer);
                    }
                    Debug.Assert(!IsControl);
                    await task.ConfigureAwait(false);
                    return;
                }

                // The given buffer includes space for the Slic header, we substract the header size from the given
                // frame size.
                Debug.Assert(frameSize > Header.Length);
                frameSize -= Header.Length;

                int sizeLength = OutputStream.GetVarLongLength(frameSize);
                int streamIdLength = OutputStream.GetVarLongLength(Id);
                frameSize += streamIdLength;

                SlicDefinitions.FrameType frameType =
                    fin ? SlicDefinitions.FrameType.StreamLast : SlicDefinitions.FrameType.Stream;

                // Write the Slic frame header (frameType - byte, frameSize - varint, streamId - varlong). Since
                // we might not need the full space reserved for the header, we modify the send buffer to ensure
                // the first element points at the start of the Slic header. We'll restore the send buffer once
                // the send is complete (it's important for the tracing code which might rely on the encoded
                // data).
                ArraySegment<byte> previous = buffer[0];
                ArraySegment<byte> headerData = buffer[0].Slice(Header.Length - sizeLength - streamIdLength - 1);
                headerData[0] = (byte)frameType;
                headerData.AsSpan(1, sizeLength).WriteFixedLengthSize20(frameSize);
                headerData.AsSpan(1 + sizeLength, streamIdLength).WriteFixedLengthVarLong(Id);
                buffer[0] = headerData;

                if (_transceiver.Endpoint.Communicator.TraceLevels.Transport > 2)
                {
                    _transceiver.TraceTransportFrame("sending ", frameType, frameSize, Id);
                }

                if (IsIncoming && fin)
                {
                    Debug.Assert(IsBidirectional);
                    Interlocked.Decrement(ref _transceiver.BidirectionalStreamCount);
                }

                try
                {
                    await _transceiver.SendFrameAsync(buffer, cancel).ConfigureAwait(false);
                }
                finally
                {
                    // Restore the original value of the send buffer.
                    buffer[0] = previous;
                }
            }
        }

        internal SlicStream(long streamId, SlicTransceiver transceiver)
            : base(streamId, transceiver) => _transceiver = transceiver;

        internal SlicStream(bool bidirectional, SlicTransceiver transceiver)
            : base(bidirectional, transceiver) => _transceiver = transceiver;

        internal void ReceivedFrame(int size, bool fin) =>
            // Ensure to run the continuation asynchronously in case the continuation ends up calling user-code.
            SignalCompletion((size, fin), runContinuationAsynchronously: true);
    }
}
