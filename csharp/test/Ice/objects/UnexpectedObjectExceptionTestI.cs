// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Threading.Tasks;

namespace ZeroC.Ice.Test.Objects
{
    public sealed class UnexpectedObjectExceptionTest : IObject
    {
        public ValueTask<OutgoingResponseFrame> DispatchAsync(IncomingRequestFrame request, Current current)
        {
            var ae = new AlsoEmpty();
            var responseFrame =
                OutgoingResponseFrame.WithReturnValue(current,
                                                      compress: false,
                                                      format: default,
                                                      ae,
                                                      (OutputStream ostr, AlsoEmpty ae) => ostr.WriteClass(ae, null));
            return new ValueTask<OutgoingResponseFrame>(responseFrame);
        }
    }
}
