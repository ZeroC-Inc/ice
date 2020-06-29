//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;

namespace ZeroC.Ice
{
    /// <summary>Represents a request protocol frame received by the application.</summary>
    public sealed class IncomingRequestFrame
    {
        /// <summary>The request context. Its initial value is computed when the request frame is created.</summary>
        public IReadOnlyDictionary<string, string> Context { get; }
        /// <summary>The encoding of the frame payload.</summary>
        public Encoding Encoding { get; }
        /// <summary>The facet of the target Ice object.</summary>
        public string Facet { get; }
        /// <summary>The identity of the target Ice object.</summary>
        public Identity Identity { get; }
        /// <summary>When true, the operation is idempotent.</summary>
        public bool IsIdempotent { get; }
        /// <summary>The operation called on the Ice object.</summary>
        public string Operation { get; }

        /// <summary>The payload of this request frame. The bytes inside the payload should not be written to;
        /// they are writable because of the <see cref="System.Net.Sockets.Socket"/> methods for sending.</summary>
        // TODO: describe how long this payload remains valid once we add memory pooling.
        public ArraySegment<byte> Payload { get; }

        /// <summary>The Ice protocol of this frame.</summary>
        public Protocol Protocol { get; }

        /// <summary>The frame byte count</summary>
        public int Size => Data.Count;

        internal ArraySegment<byte> Data { get; }
        private readonly Communicator _communicator;

        /// <summary>Creates a new IncomingRequestFrame.</summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        /// <param name="protocol">The Ice protocol.</param>
        /// <param name="data">The frame data as an array segment.</param>
        public IncomingRequestFrame(Communicator communicator, Protocol protocol, ArraySegment<byte> data)
        {
            _communicator = communicator;
            Data = data;
            Protocol = protocol;

            var istr = new InputStream(communicator, Protocol.GetEncoding(), data);
            Identity = new Identity(istr);
            Facet = istr.ReadFacet();
            Operation = istr.ReadString();
            IsIdempotent = istr.ReadOperationMode() != OperationMode.Normal;
            Context = istr.ReadContext();
            Payload = Data.Slice(istr.Pos);
            (int size, Encoding encoding) = istr.ReadEncapsulationHeader();
            if (protocol == Protocol.Ice1 && size + 4 != Payload.Count)
            {
                // The payload holds an encapsulation and the encapsulation must use up the full buffer with ice1.
                // "4" corresponds to fixed-length size with the 1.1 encoding.
                throw new InvalidDataException($"invalid request encapsulation size: {size}");
            }
            else
            {
                // TODO: with ice2, the payload is followed by a context, and the size is not fixed-length.
            }
            Encoding = encoding;
        }

        // TODO avoid copy payload (ToArray) creates a copy, that should be possible when
        // the frame has a single segment.
        internal IncomingRequestFrame(Communicator communicator, OutgoingRequestFrame frame)
            : this(communicator, frame.Protocol, VectoredBufferExtensions.ToArray(frame.Data))
        {
        }

        /// <summary>Reads the empty parameter list, calling this methods ensure that the frame payload
        /// correspond to the empty parameter list.</summary>
        public void ReadEmptyParamList() =>
            InputStream.ReadEmptyEncapsulation(_communicator, Protocol.GetEncoding(), Payload);

        /// <summary>Reads the request frame parameter list.</summary>
        /// <param name="reader">An InputStreamReader delegate used to read the request frame
        /// parameters.</param>
        /// <returns>The request parameters, when the frame parameter list contains multiple parameters
        /// they must be return as a tuple.</returns>
        public T ReadParamList<T>(InputStreamReader<T> reader) =>
            InputStream.ReadEncapsulation(_communicator, Protocol.GetEncoding(), Payload, reader);
    }
}
