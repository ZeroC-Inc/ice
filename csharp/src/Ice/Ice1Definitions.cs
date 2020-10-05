// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace ZeroC.Ice
{
    // Definitions for the ice1 protocol.
    internal static class Ice1Definitions
    {
        // The encoding of the header for ice1 frames. It is nominally 1.0, but in practice it is identical to 1.1
        // for the subset of the encoding used by the ice1 headers.
        internal static readonly Encoding Encoding = Encoding.V11;

        // Size of an ice1 frame header:
        // Magic number (4 bytes)
        // Protocol bytes (4 bytes)
        // Frame type (Byte)
        // Compression status (Byte)
        // Frame size (Int - 4 bytes)
        internal const int HeaderSize = 14;

        // The magic number at the front of each frame.
        internal static readonly byte[] Magic = new byte[] { 0x49, 0x63, 0x65, 0x50 }; // 'I', 'c', 'e', 'P'

        // 4-bytes after magic that provide the protocol version (always 1.0 for an ice1 frame) and the
        // encoding of the frame header (always set to 1.0 with the an ice1 frame, even though we use 1.1).
        internal static readonly byte[] ProtocolBytes = new byte[] { 1, 0, 1, 0 };

        // ice1 frame types:
        internal enum FrameType : byte
        {
            Request = 0,
            RequestBatch = 1,
            Reply = 2,
            ValidateConnection = 3,
            CloseConnection = 4
        }

        internal static readonly byte[] ValidateConnectionFrame = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.ValidateConnection,
            0, // Compression status.
            HeaderSize, 0, 0, 0 // Frame size.
        };

        internal static readonly byte[] CloseConnectionFrame = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.CloseConnection,
            0, // Compression status.
            HeaderSize, 0, 0, 0 // Frame size.
        };

        /*
        private static readonly byte[] _batchRequestHeaderPrologue = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.RequestBatch,
            0, // Compression status.
            0, 0, 0, 0, // Frame size (placeholder).
            0, 0, 0, 0 // Number of requests in batch (placeholder).
        };
        */

        private static readonly byte[] _requestHeaderPrologue = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.Request,
            0, // Compression status.
            0, 0, 0, 0, // Frame size (placeholder).
            0, 0, 0, 0 // Request ID (placeholder).
        };

        private static readonly byte[] _responseHeaderPrologue = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.Reply,
            0, // Compression status.
            0, 0, 0, 0 // Frame size (placeholder).
        };

        // Verify that the first 8 bytes correspond to Magic + ProtocolBytes
        internal static void CheckHeader(Span<byte> header)
        {
            Debug.Assert(header.Length >= 8);
            if (header[0] != Magic[0] || header[1] != Magic[1] || header[2] != Magic[2] || header[3] != Magic[3])
            {
                throw new InvalidDataException(
                    $"received incorrect magic bytes in header of ice1 frame: {BytesToString(header.Slice(0, 4))}");
            }

            header = header[4..];

            if (header[0] != ProtocolBytes[0] || header[1] != ProtocolBytes[1])
            {
                throw new InvalidDataException(
                    $"received ice1 protocol frame with protocol set to {header[0]}.{header[1]}");
            }

            if (header[2] != ProtocolBytes[2] || header[3] != ProtocolBytes[3])
            {
                throw new InvalidDataException(
                    $"received ice1 protocol frame with protocol encoding set to {header[2]}.{header[3]}");
            }
        }

        internal static string GetFacet(string[] facetPath)
        {
            if (facetPath.Length > 1)
            {
                throw new InvalidDataException($"read ice1 facet path with {facetPath.Length} elements");
            }
            return facetPath.Length == 1 ? facetPath[0] : "";
        }

        internal static List<ArraySegment<byte>> GetRequestData(OutgoingRequestFrame frame, int requestId)
        {
            byte[] headerData = new byte[HeaderSize + 4];
            _requestHeaderPrologue.CopyTo(headerData.AsSpan());

            OutputStream.WriteInt(frame.Size + HeaderSize + 4, headerData.AsSpan(10, 4));
            OutputStream.WriteInt(requestId, headerData.AsSpan(HeaderSize, 4));

            var data = new List<ArraySegment<byte>>() { headerData };
            data.AddRange(frame.Data);
            return data;
        }

        internal static List<ArraySegment<byte>> GetResponseData(OutgoingResponseFrame frame, int requestId)
        {
            byte[] headerData = new byte[HeaderSize + 4];
            _responseHeaderPrologue.CopyTo(headerData.AsSpan());

            OutputStream.WriteInt(frame.Size + HeaderSize + 4, headerData.AsSpan(10, 4));
            OutputStream.WriteInt(requestId, headerData.AsSpan(HeaderSize, 4));

            var data = new List<ArraySegment<byte>>() { headerData };
            data.AddRange(frame.Data);
            return data;
        }

        /// <summary>Reads a facet in the old ice1 format from the stream.</summary>
        /// <param name="istr">The stream to read from.</param>
        /// <returns>The facet read from the stream.</returns>
        internal static string ReadIce1Facet(this InputStream istr)
        {
            Debug.Assert(istr.Encoding == Encoding);
            return GetFacet(istr.ReadArray(1, InputStream.IceReaderIntoString));
        }

        /// <summary>Reads an ice1 system exception encoded based on the provided reply status.</summary>
        /// <param name="istr">The stream to read from.</param>
        /// <param name="replyStatus">The reply status.</param>
        /// <returns>The exception read from the stream.</returns>
        internal static DispatchException ReadIce1SystemException(this InputStream istr, ReplyStatus replyStatus)
        {
            Debug.Assert(istr.Encoding == Encoding);
            Debug.Assert((byte)replyStatus > (byte)ReplyStatus.UserException);

            DispatchException systemException;

            switch (replyStatus)
            {
                case ReplyStatus.FacetNotExistException:
                case ReplyStatus.ObjectNotExistException:
                case ReplyStatus.OperationNotExistException:
                    var identity = new Identity(istr);
                    string facet = istr.ReadIce1Facet();
                    string operation = istr.ReadString();

                    if (replyStatus == ReplyStatus.OperationNotExistException)
                    {
                        systemException = new OperationNotExistException(identity, facet, operation);
                    }
                    else
                    {
                        systemException = new ObjectNotExistException(identity, facet, operation);
                    }
                    break;

                default:
                    systemException = new UnhandledException(istr.ReadString(), Identity.Empty, "", "");
                    break;
            }

            systemException.ConvertToUnhandled = true;
            return systemException;
        }

        /// <summary>Writes a facet as a facet path.</summary>
        /// <param name="ostr">The stream.</param>
        /// <param name="facet">The facet to write to the stream.</param>
        internal static void WriteIce1Facet(this OutputStream ostr, string facet)
        {
            Debug.Assert(ostr.Encoding == Encoding);

            // The old facet-path style used by the ice1 protocol.
            if (facet.Length == 0)
            {
                ostr.WriteSize(0);
            }
            else
            {
                ostr.WriteSize(1);
                ostr.WriteString(facet);
            }
        }

        /// <summary>Writes a request header body without constructing an Ice1RequestHeaderBody instance. This
        /// implementation is slightly more efficient than the generated code because it avoids the allocation of a
        /// string[] to write the facet and the allocation of a Dictionary{string, string} to write the context.
        /// </summary>
        internal static void WriteIce1RequestHeaderBody(
            this OutputStream ostr,
            Identity identity,
            string facet,
            string operation,
            bool idempotent,
            IReadOnlyDictionary<string, string> context)
        {
            Debug.Assert(ostr.Encoding == Encoding);
            identity.IceWrite(ostr);
            ostr.WriteIce1Facet(facet);
            ostr.WriteString(operation);
            ostr.Write(idempotent ? OperationMode.Idempotent : OperationMode.Normal);
            ostr.WriteDictionary(context, OutputStream.IceWriterFromString, OutputStream.IceWriterFromString);
        }

        private static string BytesToString(Span<byte> bytes) => BitConverter.ToString(bytes.ToArray());
    }
}
