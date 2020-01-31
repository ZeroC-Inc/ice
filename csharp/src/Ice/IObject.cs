//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using IceInternal;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Threading.Tasks;

namespace Ice
{
    /// <summary>
    /// Interface for incoming requests.
    /// </summary>
    public interface IRequest
    {
        /// <summary>
        /// Returns the {@link Current} object for this the request.
        /// </summary>
        /// <returns>The Current object for this request.</returns>
        Current? GetCurrent();
    }

    ///<summary>The base interface for all servants.</summary>
    public interface IObject
    {
        // See Dispatcher.
        public Task<OutputStream?>? Dispatch(Incoming inS, Current current)
        {
            // TODO: switch to abstract method
            Debug.Assert(false);
            return null;
        }

        protected static void IceCheckMode(OperationMode expected, OperationMode received)
        {
            if (expected != received)
            {
                if (expected == OperationMode.Idempotent && received == OperationMode.Nonmutating)
                {
                    // Fine: typically an old client still using the deprecated nonmutating keyword or metadata.
                }
                else
                {
                    var expectedString = expected.ToString("G");
                    var receivedString = received.ToString("G");
                    throw new MarshalException(
                        $"unexpected operation mode. expected = {expectedString} received = {receivedString}");
                }
            }
        }
    }

    /// <summary>
    /// Base class for dynamic dispatch servants. A server application
    /// derives a concrete servant class from Blobject that
    /// implements the Blobject.ice_invoke method.
    /// </summary>
    public abstract class Blobject : IObject
    {
        /// <summary>
        /// Dispatch an incoming request.
        /// </summary>
        /// <param name="inParams">The encoded in-parameters for the operation.</param>
        /// <param name="outParams">The encoded out-paramaters and return value
        /// for the operation. The return value follows any out-parameters.</param>
        /// <param name="current">The Current object to pass to the operation.</param>
        /// <returns>If the operation completed successfully, the return value
        /// is true. If the operation raises a user exception,
        /// the return value is false; in this case, outParams
        /// must contain the encoded user exception. If the operation raises an
        /// Ice run-time exception, it must throw it directly.</returns>
        public abstract bool IceInvoke(byte[] inParams, out byte[] outParams, Current current);

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? Dispatch(Incoming incoming, Current current)
        {
            incoming.StartOver();
            byte[] inEncaps = incoming.ReadParamEncaps();
            bool ok = IceInvoke(inEncaps, out byte[] outEncaps, current);
            incoming.SetResult(incoming.WriteParamEncaps(incoming.GetAndClearCachedOutputStream(), outEncaps, ok));
            return null;
        }
    }

    public abstract class BlobjectAsync : IObject
    {
        public abstract Task<Ice.Object_Ice_invokeResult> IceInvokeAsync(byte[] inEncaps, Current current);

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? Dispatch(Incoming incoming, Current current)
        {
            incoming.StartOver();
            byte[] inEncaps = incoming.ReadParamEncaps();
            Task<Object_Ice_invokeResult> task = IceInvokeAsync(inEncaps, current);
            OutputStream? cached = incoming.GetAndClearCachedOutputStream();
            return task.ContinueWith((Task<Object_Ice_invokeResult> t) =>
                {
                    Object_Ice_invokeResult ret = t.GetAwaiter().GetResult();
                    return Task.FromResult(incoming.WriteParamEncaps(cached, ret.OutEncaps, ret.ReturnValue));
                },
                TaskScheduler.Current).Unwrap();
        }
    }

    /// <summary>
    /// Provides the 4 built-in operations that all Ice objects implement.
    /// </summary>
    public interface IObjectOperations
    {
        /// <summary>
        /// Tests whether this object supports a specific Slice interface.
        /// </summary>
        ///
        /// <param name="s">The type ID of the Slice interface to test against.</param>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <returns>True if this object has the interface
        /// specified by s or derives from the interface specified by s.</returns>
        public bool IceIsA(string s, Current current);

        /// <summary>
        /// Tests whether this object can be reached.
        /// </summary>
        /// <param name="current">The Current object for the dispatch.</param>
        public void IcePing(Current current);

        /// <summary>
        /// Returns the Slice type IDs of the interfaces supported by this object.
        /// </summary>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <returns>The Slice type IDs of the interfaces supported by this object, in base-to-derived
        /// order. The first element of the returned array is always ::Ice::IObject.</returns>
        public string[] IceIds(Current current);

        /// <summary>
        /// Returns the Slice type ID of the most-derived interface supported by this object.
        /// </summary>
        /// <param name="current">The Current object for the dispatch.</param>
        /// <returns>The Slice type ID of the most-derived interface.</returns>
        public string IceId(Current current);

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? IceD_ice_isA(IceInternal.Incoming inS, Current current)
        {
            InputStream istr = inS.StartReadParams();
            string id = istr.ReadString();
            inS.EndReadParams();
            bool ret = IceIsA(id, current);
            OutputStream ostr = inS.StartWriteParams();
            ostr.WriteBool(ret);
            inS.EndWriteParams(ostr);
            return inS.SetResult(ostr)!;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? IceD_ice_ping(IceInternal.Incoming inS, Current current)
        {
            inS.ReadEmptyParams();
            IcePing(current);
            inS.SetResult(inS.WriteEmptyParams());
            return null;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? IceD_ice_ids(IceInternal.Incoming inS, Current current)
        {
            inS.ReadEmptyParams();
            string[] ret = IceIds(current);
            OutputStream ostr = inS.StartWriteParams();
            ostr.WriteStringSeq(ret);
            inS.EndWriteParams(ostr);
            inS.SetResult(ostr);
            return null;
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        public Task<OutputStream?>? IceD_ice_id(IceInternal.Incoming inS, Current current)
        {
            inS.ReadEmptyParams();
            string ret = IceId(current);
            OutputStream ostr = inS.StartWriteParams();
            ostr.WriteString(ret);
            inS.EndWriteParams(ostr);
            inS.SetResult(ostr);
            return null;
        }
    }

    public class ObjectOperations<T> : IObjectOperations
    {
        private static readonly string _typeId = typeof(T).GetIceTypeId()!;
        private static readonly string[] _allTypeIds = typeof(T).GetAllIceTypeIds();

        public virtual string IceId(Current current) => _typeId;
        public virtual string[] IceIds(Current current) => _allTypeIds;

        public virtual bool IceIsA(string s, Current current)
            => Array.BinarySearch(_allTypeIds, s, StringComparer.Ordinal) >= 0;

        public virtual void IcePing(Current current)
        {
            // Nothing to do
        }
    }
}
