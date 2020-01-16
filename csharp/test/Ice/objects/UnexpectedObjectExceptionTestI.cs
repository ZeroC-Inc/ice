//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace Ice.objects
{
    public sealed class UnexpectedObjectExceptionTest : Blobject
    {
        public override bool ice_invoke(byte[] inParams, out byte[] outParams, Current current)
        {
            var communicator = current.Adapter.Communicator;
            var ostr = new OutputStream(communicator);
            ostr.StartEncapsulation(current.Encoding, FormatType.DefaultFormat);
            var ae = new Test.AlsoEmpty();
            ostr.WriteValue(ae);
            ostr.WritePendingValues();
            ostr.EndEncapsulation();
            outParams = ostr.Finished();
            return true;
        }
    }
}
