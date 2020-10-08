// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    /// <summary>The base interface for all servants.</summary>
    public interface IObject
    {
        /// <summary>Dispatches a request on this servant.</summary>
        /// <param name="request">The <see cref="IncomingRequestFrame"/> to dispatch.</param>
        /// <param name="current">Holds decoded header data and other information about the current request.</param>
        /// <param name="cancel">A cancellation token that is notified of cancellation when the dispatch is cancelled.
        /// </param>
        /// <returns>A value task that provides the <see cref="OutgoingResponseFrame"/> for the request.</returns>
        /// <exception cref="Exception">Any exception thrown by DispatchAsync will be marshaled into the response
        /// frame.</exception>
        public ValueTask<OutgoingResponseFrame> DispatchAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            // TODO: switch to abstract method (but doesn't work as of .NET 5 preview 8).
            Debug.Assert(false);
            return new ValueTask<OutgoingResponseFrame>(OutgoingResponseFrame.WithVoidReturnValue(current));
        }

        // The following are helper classes and methods for generated servants.

        /// <summary>Holds an <see cref="InputStreamReader{T}"/> for each remote operation with parameter(s) defined in
        /// the pseudo-interface Object.</summary>
        public static class Request
        {
            /// <summary>The <see cref="InputStreamReader{T}"/> for the parameter of operation ice_isA.</summary>
            public static readonly InputStreamReader<string> IceIsA = InputStream.IceReaderIntoString;
        }

        /// <summary>Provides an <see cref="OutgoingResponseFrame"/> factory method for each non-void remote operation
        /// defined in the pseudo-interface Object.</summary>
        public static class Response
        {
            /// <summary>Creates an <see cref="OutgoingResponseFrame"/> for operation ice_id.</summary>
            /// <param name="current">Holds decoded header data and other information about the current request.</param>
            /// <param name="returnValue">The return value to write into the new frame.</param>
            /// <returns>A new <see cref="OutgoingResponseFrame"/>.</returns>
            public static OutgoingResponseFrame IceId(Current current, string returnValue) =>
                OutgoingResponseFrame.WithReturnValue(
                    current,
                    compress: false,
                    format: default,
                    returnValue,
                    OutputStream.IceWriterFromString);

            /// <summary>Creates an <see cref="OutgoingResponseFrame"/> for operation ice_ids.</summary>
            /// <param name="current">Holds decoded header data and other information about the current request.</param>
            /// <param name="returnValue">The return value to write into the new frame.</param>
            /// <returns>A new <see cref="OutgoingResponseFrame"/>.</returns>
            public static OutgoingResponseFrame IceIds(Current current, IEnumerable<string> returnValue) =>
                OutgoingResponseFrame.WithReturnValue(
                    current,
                    compress: false,
                    format: default,
                    returnValue,
                    (ostr, returnValue) => ostr.WriteSequence(returnValue, OutputStream.IceWriterFromString));

            /// <summary>Creates an <see cref="OutgoingResponseFrame"/> for operation ice_isA.</summary>
            /// <param name="current">Holds decoded header data and other information about the current request.</param>
            /// <param name="returnValue">The return value to write into the new frame.</param>
            /// <returns>A new <see cref="OutgoingResponseFrame"/>.</returns>
            public static OutgoingResponseFrame IceIsA(Current current, bool returnValue) =>
                OutgoingResponseFrame.WithReturnValue(
                    current,
                    compress: false,
                    format: default,
                    returnValue,
                    OutputStream.IceWriterFromBool);
        }

        /// <summary>Returns the Slice type ID of the most-derived interface supported by this object.</summary>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <param name="cancel">A cancellation token that is notified of cancellation when the dispatch is cancelled.
        /// </param>
        /// <returns>The Slice type ID of the most-derived interface.</returns>
        public string IceId(Current current, CancellationToken cancel) => "::Ice::Object";

        /// <summary>Returns the Slice type IDs of the interfaces supported by this object.</summary>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>The Slice type IDs of the interfaces supported by this object, in alphabetical order.</returns>
        public IEnumerable<string> IceIds(Current current, CancellationToken cancel) =>
            new string[] { "::Ice::Object" };

        /// <summary>Tests whether this object supports the specified Slice interface.</summary>
        /// <param name="typeId">The type ID of the Slice interface to test against.</param>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>True if this object implements the interface specified by typeId.</returns>
        public bool IceIsA(string typeId, Current current, CancellationToken cancel) =>
            Array.BinarySearch((string[])IceIds(current, cancel), typeId, StringComparer.Ordinal) >= 0;

        /// <summary>Tests whether this object can be reached.</summary>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        public void IcePing(Current current, CancellationToken cancel)
        {
            // Does nothing
        }

        /// <summary>The generated code calls this method to ensure that when an operation is _not_ declared
        /// idempotent, the request is not marked idempotent. If the request is marked idempotent, it means the caller
        /// incorrectly believes this operation is idempotent.</summary>
        /// <param name="current">The current object for the dispatch.</param>
        protected static void IceCheckNonIdempotent(Current current)
        {
            if (current.IsIdempotent)
            {
                throw new InvalidDataException(
                        $@"idempotent mismatch for operation `{current.Operation
                        }': received request marked idempotent for a non-idempotent operation");
            }
        }

        /// <summary>Dispatches an ice_id request.</summary>
        /// <param name="request">The request frame.</param>
        /// <param name="current">The current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>The response frame.</returns>
        protected ValueTask<OutgoingResponseFrame> IceDIceIdAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            request.ReadEmptyArgs();
            string returnValue = IceId(current, cancel);
            return new ValueTask<OutgoingResponseFrame>(Response.IceId(current, returnValue));
        }

        /// <summary>Dispatches an ice_ids request.</summary>
        /// <param name="request">The request frame.</param>
        /// <param name="current">The current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>The response frame.</returns>
        protected ValueTask<OutgoingResponseFrame> IceDIceIdsAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            request.ReadEmptyArgs();
            IEnumerable<string> returnValue = IceIds(current, cancel);
            return new ValueTask<OutgoingResponseFrame>(Response.IceIds(current, returnValue));
        }

        /// <summary>Dispatches an ice_isA request.</summary>
        /// <param name="request">The request frame.</param>
        /// <param name="current">The current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>The response frame.</returns>
        protected ValueTask<OutgoingResponseFrame> IceDIceIsAAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            string id = request.ReadArgs(current.Communicator, Request.IceIsA);
            bool returnValue = IceIsA(id, current, cancel);
            return new ValueTask<OutgoingResponseFrame>(Response.IceIsA(current, returnValue));
        }

        /// <summary>Dispatches an ice_ping request.</summary>
        /// <param name="request">The request frame.</param>
        /// <param name="current">The current object for the dispatch.</param>
        /// <param name="cancel">A cancelation token that is notified of cancelation when the dispatch is cancelled.
        /// </param>
        /// <returns>The response frame.</returns>
        protected ValueTask<OutgoingResponseFrame> IceDIcePingAsync(
            IncomingRequestFrame request,
            Current current,
            CancellationToken cancel)
        {
            request.ReadEmptyArgs();
            IcePing(current, cancel);
            return new ValueTask<OutgoingResponseFrame>(OutgoingResponseFrame.WithVoidReturnValue(current));
        }
    }
}
