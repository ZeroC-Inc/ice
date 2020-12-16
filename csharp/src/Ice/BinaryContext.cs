// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Immutable;
using System.Diagnostics;

namespace ZeroC.Ice
{
    /// <summary>Constants used as keys for BinaryContext entries.</summary>
    public enum BinaryContextKey : int
    {
        /// <summary>Key used to encode the retry policy entry.</summary>
        RetryPolicy = -1
    }

    public static class BinaryContextHelper
    {
        public static ImmutableDictionary<int, ReadOnlyMemory<byte>> ReadBinaryContext(this InputStream istr)
        {
            Debug.Assert(istr.Encoding == Encoding.V20);

            int size = istr.ReadSize();
            if (size == 0)
            {
                return ImmutableDictionary<int, ReadOnlyMemory<byte>>.Empty;
            }
            else
            {
                var builder = ImmutableDictionary.CreateBuilder<int, ReadOnlyMemory<byte>>();
                for (int i = 0; i < size; ++i)
                {
                    (int key, ReadOnlyMemory<byte> value) = istr.ReadBinaryContextEntry();
                    builder.Add(key, value);
                }
                return builder.ToImmutable();
            }
        }
    }
}
