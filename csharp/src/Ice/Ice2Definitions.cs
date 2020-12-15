// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace ZeroC.Ice
{
    // Definitions for the ice2 protocol.

    internal static class Ice2Definitions
    {
        internal static readonly Encoding Encoding = Encoding.V20;

        /// <summary>Writes a request header body This implementation is slightly more efficient than the generated code
        /// because it avoids the allocation of a string[] to write the location.</summary>
        internal static void WriteIce2RequestHeaderBody(
            this OutputStream ostr,
            Identity identity,
            string facet,
            IReadOnlyList<string> location,
            string operation,
            bool idempotent,
            DateTime deadline,
            IReadOnlyDictionary<string, string> context)
        {
            Debug.Assert(ostr.Encoding == Encoding);
            BitSequence bitSequence = ostr.WriteBitSequence(5); // bit set to true (set) by default

            identity.IceWrite(ostr);
            if (facet.Length > 0)
            {
                ostr.WriteString(facet);
            }
            else
            {
                bitSequence[0] = false;
            }

            if (location.Count > 0)
            {
                ostr.WriteSequence(location, OutputStream.IceWriterFromString);
            }
            else
            {
                bitSequence[1] = false;
            }

            ostr.WriteString(operation);

            if (idempotent)
            {
                ostr.WriteBool(true);
            }
            else
            {
                bitSequence[2] = false;
            }

            bitSequence[3] = false; // TODO: source for priority.

            // DateTime.MaxValue represents an infinite deadline and it is encoded as -1
            ostr.WriteVarLong(
                deadline == DateTime.MaxValue ? -1 : (long)(deadline - DateTime.UnixEpoch).TotalMilliseconds);

            if (context.Count > 0)
            {
                ostr.WriteDictionary(context, OutputStream.IceWriterFromString, OutputStream.IceWriterFromString);
            }
            else
            {
                bitSequence[4] = false;
            }
        }
    }
}
