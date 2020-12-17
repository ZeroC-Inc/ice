// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Threading;

namespace ZeroC.Ice
{
    /// <summary>Represents an ice1 or ice2 request frame sent by the application.</summary>
    public sealed class OutgoingRequestFrame : OutgoingFrame, IDisposable
    {
        /// <summary>The context of this request frame as a read-only dictionary.</summary>
        public IReadOnlyDictionary<string, string> Context => _writableContext ?? _initialContext;

        /// <summary>A cancellation token that receives the cancellation requests. The cancellation token takes into
        /// account the invocation timeout and the cancellation token provided by the application.</summary>
        public CancellationToken CancellationToken => _linkedCancellationSource.Token;

        /// <summary>The deadline corresponds to the request's expiration time. Once the deadline is reached, the
        /// caller is no longer interested in the response and discards the request. The server-side runtime does not
        /// enforce this deadline - it's provided "for information" to the application. The Ice client runtime sets
        /// this deadline automatically using the proxy's invocation timeout and sends it with ice2 requests but not
        /// with ice1 requests. As a result, the deadline for an ice1 request is always <see cref="DateTime.MaxValue"/>
        /// on the server-side even though the invocation timeout is usually not infinite.</summary>
        public DateTime Deadline { get; }

        /// <summary>The encoding of the request payload.</summary>
        public override Encoding Encoding { get; }

        /// <summary>The facet of the target Ice object.</summary>
        public string Facet { get; }

        /// <summary>The identity of the target Ice object.</summary>
        public Identity Identity { get; }

        public override IReadOnlyDictionary<int, ReadOnlyMemory<byte>> InitialBinaryContext { get; }

        /// <summary>When true, the operation is idempotent.</summary>
        public bool IsIdempotent { get; }

        /// <summary>The location of the target Ice object. With ice1, it is always empty.</summary>
        public IReadOnlyList<string> Location { get; }

        /// <summary>The operation called on the Ice object.</summary>
        public string Operation { get; }

        /// <summary>WritableContext is writable version of Context. Its entries are always the same as Context's
        /// entries.</summary>
        public SortedDictionary<string, string> WritableContext
        {
            get
            {
                _writableContext ??= new SortedDictionary<string, string>((IDictionary<string, string>)_initialContext);
                return _writableContext;
            }
        }

        private SortedDictionary<string, string>? _writableContext;
        private readonly IReadOnlyDictionary<string, string> _initialContext;
        private readonly CancellationTokenSource? _invocationTimeoutCancellationSource;
        private readonly CancellationTokenSource _linkedCancellationSource;

        /// <inheritdoc/>
        public void Dispose()
        {
            _invocationTimeoutCancellationSource?.Dispose();
            _linkedCancellationSource.Dispose();
        }

        /// <summary>Creates a new <see cref="OutgoingRequestFrame"/> for an operation with a single non-struct
        /// parameter.</summary>
        /// <typeparam name="T">The type of the operation's parameter.</typeparam>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet,
        /// encoding and context of this proxy to create the request frame.</param>
        /// <param name="operation">The operation to invoke on the target Ice object.</param>
        /// <param name="idempotent">True when operation is idempotent, otherwise false.</param>
        /// <param name="compress">True if the request should be compressed, false otherwise.</param>
        /// <param name="format">The format to use when writing class instances in case <c>args</c> contains class
        /// instances.</param>
        /// <param name="context">An optional explicit context. When non null, it overrides both the context of the
        /// proxy and the communicator's current context (if any).</param>
        /// <param name="args">The argument(s) to write into the frame.</param>
        /// <param name="writer">The <see cref="OutputStreamWriter{T}"/> that writes the arguments into the frame.
        /// </param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A new OutgoingRequestFrame.</returns>
        public static OutgoingRequestFrame WithArgs<T>(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            bool compress,
            FormatType format,
            IReadOnlyDictionary<string, string>? context,
            T args,
            OutputStreamWriter<T> writer,
            CancellationToken cancel = default)
        {
            var request = new OutgoingRequestFrame(proxy, operation, idempotent, compress, context, cancel);
            var ostr = new OutputStream(proxy.Protocol.GetEncoding(),
                                        request.Payload,
                                        startAt: default,
                                        request.Encoding,
                                        format);
            writer(ostr, args);
            _ = ostr.Finish();
            if (compress && proxy.Encoding == Encoding.V20)
            {
                request.CompressPayload();
            }
            return request;
        }

        /// <summary>Creates a new <see cref="OutgoingRequestFrame"/> for an operation with a single stream
        /// parameter.</summary>
        /// <typeparam name="T">The type of the operation's parameter.</typeparam>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet,
        /// encoding and context of this proxy to create the request frame.</param>
        /// <param name="operation">The operation to invoke on the target Ice object.</param>
        /// <param name="idempotent">True when operation is idempotent, otherwise false.</param>
        /// <param name="compress">True if the request should be compressed, false otherwise.</param>
        /// <param name="format">The format to use when writing class instances in case <c>args</c> contains class
        /// instances.</param>
        /// <param name="context">An optional explicit context. When non null, it overrides both the context of the
        /// proxy and the communicator's current context (if any).</param>
        /// <param name="args">The argument(s) to write into the frame.</param>
        /// <param name="writer">The delegate that will send the streamable.</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A new OutgoingRequestFrame.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage(
            "Microsoft.Performance",
            "CA1801: Review unused parameters",
            Justification = "TODO")]
        public static OutgoingRequestFrame WithArgs<T>(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            bool compress,
            FormatType format,
            IReadOnlyDictionary<string, string>? context,
            T args,
            Action<SocketStream, T, CancellationToken> writer,
            CancellationToken cancel = default)
        {
            OutgoingRequestFrame request = WithEmptyArgs(proxy, operation, idempotent, context, cancel);
            // TODO: deal with compress, format, and cancel paramters
            request.StreamDataWriter = socketStream => writer(socketStream, args, cancel);
            return request;
        }

        /// <summary>Creates a new <see cref="OutgoingRequestFrame"/> for an operation with multiple parameters or a
        /// single struct parameter.</summary>
        /// <typeparam name="T">The type of the operation's parameters; it's a tuple type for an operation with multiple
        /// parameters.</typeparam>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet,
        /// encoding and context of this proxy to create the request frame.</param>
        /// <param name="operation">The operation to invoke on the target Ice object.</param>
        /// <param name="idempotent">True when operation is idempotent, otherwise false.</param>
        /// <param name="compress">True if the request should be compressed, false otherwise.</param>
        /// <param name="format">The format to use when writing class instances in case <c>args</c> contains class
        /// instances.</param>
        /// <param name="context">An optional explicit context. When non null, it overrides both the context of the
        /// proxy and the communicator's current context (if any).</param>
        /// <param name="args">The argument(s) to write into the frame.</param>
        /// <param name="writer">The <see cref="OutputStreamWriter{T}"/> that writes the arguments into the frame.
        /// </param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A new OutgoingRequestFrame.</returns>
        public static OutgoingRequestFrame WithArgs<T>(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            bool compress,
            FormatType format,
            IReadOnlyDictionary<string, string>? context,
            in T args,
            OutputStreamValueWriter<T> writer,
            CancellationToken cancel = default) where T : struct
        {
            var request = new OutgoingRequestFrame(proxy, operation, idempotent, compress, context, cancel);
            var ostr = new OutputStream(proxy.Protocol.GetEncoding(),
                                        request.Payload,
                                        startAt: default,
                                        request.Encoding,
                                        format);
            writer(ostr, in args);
            _ = ostr.Finish();
            if (compress && proxy.Encoding == Encoding.V20)
            {
                request.CompressPayload();
            }
            return request;
        }

        /// <summary>Creates a new <see cref="OutgoingRequestFrame"/> for an operation with multiple parameters where
        /// one of the parameter is a stream parameter.</summary>
        /// <typeparam name="T">The type of the operation's parameters; it's a tuple type for an operation with multiple
        /// parameters.</typeparam>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet,
        /// encoding and context of this proxy to create the request frame.</param>
        /// <param name="operation">The operation to invoke on the target Ice object.</param>
        /// <param name="idempotent">True when operation is idempotent, otherwise false.</param>
        /// <param name="compress">True if the request should be compressed, false otherwise.</param>
        /// <param name="format">The format to use when writing class instances in case <c>args</c> contains class
        /// instances.</param>
        /// <param name="context">An optional explicit context. When non null, it overrides both the context of the
        /// proxy and the communicator's current context (if any).</param>
        /// <param name="args">The argument(s) to write into the frame.</param>
        /// <param name="writer">The delegate that writes the arguments into the frame.</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A new OutgoingRequestFrame.</returns>
        public static OutgoingRequestFrame WithArgs<T>(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            bool compress,
            FormatType format,
            IReadOnlyDictionary<string, string>? context,
            in T args,
            OutputStreamValueWriterWithStreamable<T> writer,
            CancellationToken cancel = default) where T : struct
        {
            var request = new OutgoingRequestFrame(proxy, operation, idempotent, compress, context, cancel);
            var ostr = new OutputStream(proxy.Protocol.GetEncoding(),
                                        request.Payload,
                                        startAt: default,
                                        request.Encoding,
                                        format);
            // TODO: deal with compress, format, and cancel paramters
            request.StreamDataWriter = writer(ostr, in args, cancel);
            _ = ostr.Finish();
            if (compress && proxy.Encoding == Encoding.V20)
            {
                request.CompressPayload();
            }
            return request;
        }

        /// <summary>Creates a new <see cref="OutgoingRequestFrame"/> for an operation with no parameter.</summary>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet,
        /// encoding and context of this proxy to create the request frame.</param>
        /// <param name="operation">The operation to invoke on the target Ice object.</param>
        /// <param name="idempotent">True when operation is idempotent, otherwise false.</param>
        /// <param name="context">An optional explicit context. When non null, it overrides both the context of the
        /// proxy and the communicator's current context (if any).</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        /// <returns>A new OutgoingRequestFrame.</returns>
        public static OutgoingRequestFrame WithEmptyArgs(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            IReadOnlyDictionary<string, string>? context = null,
            CancellationToken cancel = default)
        {
            var emptyArgsFrame = new OutgoingRequestFrame(proxy,
                                                          operation,
                                                          idempotent,
                                                          compress: false,
                                                          context,
                                                          cancel);
            emptyArgsFrame.Payload.Add(proxy.Protocol.GetEmptyArgsPayload(proxy.Encoding));
            emptyArgsFrame.Size = emptyArgsFrame.Payload.GetByteCount(); // TODO
            return emptyArgsFrame;
        }

        /// <summary>Constructs an outgoing request frame from the given incoming request frame.</summary>
        /// <param name="proxy">A proxy to the target Ice object. This method uses the communicator, identity, facet
        /// and context of this proxy to create the request frame.</param>
        /// <param name="request">The incoming request from which to create an outgoing request.</param>
        /// <param name="forwardBinaryContext">When true (the default), the new frame uses the incoming request frame's
        /// binary context as a fallback - all the entries in this binary context are added before the frame is sent,
        /// except for entries previously added by invocation interceptors.</param>
        /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
        public OutgoingRequestFrame(
            IObjectPrx proxy,
            IncomingRequestFrame request,
            bool forwardBinaryContext = true,
            CancellationToken cancel = default)
            : this(proxy, request.Operation, request.IsIdempotent, compress: false, request.Context, cancel)
        {
            Encoding = request.Encoding;

            if (request.Protocol == Protocol)
            {
                // We only include the encapsulation.
                Payload.Add(request.Payload);

                if (Protocol == Protocol.Ice2 && forwardBinaryContext)
                {
                    InitialBinaryContext = request.BinaryContext;
                }
            }
            else
            {
                // We forward the encapsulation and the string-string context. The context was marshaled by the
                // constructor (when Protocol == Ice1) or will be written by Finish (when Protocol == Ice2).
                // The payload encoding must remain the same since we cannot transcode the encoded bytes.

                int sizeLength = request.Protocol == Protocol.Ice1 ? 4 : request.Payload[0].ReadSizeLength20();

                var ostr = new OutputStream(Protocol.GetEncoding(), Payload);
                ostr.WriteEncapsulationHeader(request.Payload.Count - sizeLength, request.Encoding);
                _ = ostr.Finish();

                // "2" below corresponds to the encoded length of the encoding.
                if (request.Payload.Count > sizeLength + 2)
                {
                    // Add encoded bytes, not including the header or binary context.
                    Payload.Add(request.Payload.Slice(sizeLength + 2));
                }
            }

            Size = Payload.GetByteCount();
            IsSealed = Protocol == Protocol.Ice1;
        }

        /// <inheritdoc/>
        internal override void WriteHeader(OutputStream ostr)
        {
            if (ostr.Encoding != Protocol.GetEncoding())
            {
                throw new ArgumentException(
                    @$"cannot write header of {Protocol.GetName()} frame to output stream using the {ostr.Encoding
                    } encoding",
                    nameof(ostr));
            }

            if (Protocol == Protocol.Ice2)
            {
                OutputStream.Position start = ostr.StartFixedLengthSize(2);
                ostr.WriteIce2RequestHeaderBody(Identity,
                                                Facet,
                                                Location,
                                                Operation,
                                                IsIdempotent,
                                                Deadline,
                                                Context);

                WriteBinaryContext(ostr);
                ostr.EndFixedLengthSize(start, 2);
            }
            else
            {
                Debug.Assert(Protocol == Protocol.Ice1);
                ostr.WriteIce1RequestHeaderBody(Identity, Facet, Operation, IsIdempotent, Context);
            }
        }

        private OutgoingRequestFrame(
            IObjectPrx proxy,
            string operation,
            bool idempotent,
            bool compress,
            IReadOnlyDictionary<string, string>? context,
            CancellationToken cancel)
            : base(proxy.Protocol,
                   compress,
                   proxy.Communicator.CompressionLevel,
                   proxy.Communicator.CompressionMinSize)
        {
            Encoding = proxy.Encoding;
            Identity = proxy.Identity;
            Facet = proxy.Facet;
            Location = proxy.Location;
            Operation = operation;
            IsIdempotent = idempotent;
            InitialBinaryContext = ImmutableDictionary<int, ReadOnlyMemory<byte>>.Empty;

            Debug.Assert(proxy.InvocationTimeout != TimeSpan.Zero);
            Deadline = Protocol == Protocol.Ice1 || proxy.InvocationTimeout == Timeout.InfiniteTimeSpan ?
                DateTime.MaxValue : DateTime.UtcNow + proxy.InvocationTimeout;

            if (proxy.InvocationTimeout != Timeout.InfiniteTimeSpan)
            {
                _invocationTimeoutCancellationSource = new CancellationTokenSource(proxy.InvocationTimeout);
            }

            _linkedCancellationSource = CancellationTokenSource.CreateLinkedTokenSource(
                _invocationTimeoutCancellationSource?.Token ?? default,
                proxy.Communicator.CancellationToken,
                cancel);

            if (context != null)
            {
                // This makes a copy if context is not immutable.
                _initialContext = context.ToImmutableSortedDictionary();
            }
            else
            {
                IReadOnlyDictionary<string, string> currentContext = proxy.Communicator.CurrentContext;

                if (proxy.Context.Count == 0)
                {
                    _initialContext = currentContext;
                }
                else if (currentContext.Count == 0)
                {
                    _initialContext = proxy.Context;
                }
                else
                {
                    var combinedContext = new SortedDictionary<string, string>(
                        (IDictionary<string, string>)currentContext);

                    foreach ((string key, string value) in proxy.Context)
                    {
                        combinedContext[key] = value; // the proxy Context entry prevails.
                    }
                    _initialContext = combinedContext;
                    _writableContext = combinedContext;
                }
            }
        }
    }
}
