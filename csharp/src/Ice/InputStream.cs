//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace Ice
{
    public delegate T InputStreamReader<T>(InputStream ins);

    /// <summary>
    /// Throws a UserException corresponding to the given Slice type Id, such as "::Module::MyException".
    /// If the implementation does not throw an exception, the Ice run time will fall back
    /// to using its default behavior for instantiating the user exception.
    /// </summary>
    /// <param name="id">A Slice type Id corresponding to a Slice user exception.</param>
    public delegate void UserExceptionFactory(string id);

    /// <summary>
    /// Interface for input streams used to extract Slice types from a sequence of bytes.
    /// </summary>
    public sealed class InputStream
    {
        public static readonly InputStreamReader<bool> IceReaderIntoBool = (istr) => istr.ReadBool();
        public static readonly InputStreamReader<byte> IceReaderIntoByte = (istr) => istr.ReadByte();
        public static readonly InputStreamReader<short> IceReaderIntoShort = (istr) => istr.ReadShort();
        public static readonly InputStreamReader<int> IceReaderIntoInt = (istr) => istr.ReadInt();
        public static readonly InputStreamReader<long> IceReaderIntoLong = (istr) => istr.ReadLong();
        public static readonly InputStreamReader<float> IceReaderIntoFloat = (istr) => istr.ReadFloat();
        public static readonly InputStreamReader<double> IceReaderIntoDouble = (istr) => istr.ReadDouble();
        public static readonly InputStreamReader<string> IceReaderIntoString = (istr) => istr.ReadString();

        /// <summary>
        /// Returns the current size of the stream.
        /// </summary>
        /// <value>The size of the stream.</value>
        public int Size => _buf.Size();

        /// <summary>
        /// The communicator associated with this stream.
        /// </summary>
        /// <value>The communicator.</value>
        public Communicator Communicator { get; }

        /// <summary>
        /// The encoding used when reading from this stream.
        /// </summary>
        /// <value>The encoding.</value>
        public Encoding Encoding { get; private set; }

        // The position (offset) in the underlying buffer.
        internal int Pos
        {
            get => _buf.B.Position();
            set => _buf.B.Position(value);
        }

        // True if the internal buffer has no data, false otherwise.
        internal bool IsEmpty => _buf.Empty();

        // Returns the sliced data held by the current instance.
        internal SlicedData? SlicedData
        {
            get
            {
                Debug.Assert(_current != null);
                if (_current.Slices == null)
                {
                    return null;
                }
                else
                {
                    return new SlicedData(Encoding, _current.Slices);
                }
            }
        }

        // Number of bytes remaining in the underlying buffer.
        private int Remaining => _limit - Pos ?? _buf.B.Remaining();

        // When set, we are in reading a top-level encapsulation.
        private Encaps? _mainEncaps;

        // See StartEncapsulation/RestartEncapsulation
        private MainEncapsBackup? _mainEncapsBackup;

        // When set, we are reading an endpoint encapsulation. An endpoint encaps is a lightweight encaps that cannot
        // contain classes, exceptions, tagged members/parameters, or another endpoint. It is often but not always set
        // when _mainEncaps is set (so nested inside _mainEncaps).
        private Encaps? _endpointEncaps;

        // Temporary upper limit set by an encapsulation. See Remaining.
        private int? _limit;

        // The sum of all the mininum sizes (in bytes) of the sequences read in this buffer. Must not exceed the buffer
        // size.
        private int _minTotalSeqSize = 0;

        private IceInternal.Buffer _buf;

        private byte[]? _stringBytes; // Reusable array for reading strings.

        // TODO: should we cache those per InputStream or per communicator?
        //       should we clear the caches in ResetEncapsulation?
        private Dictionary<string, Type?>? _typeIdCache;
        private Dictionary<int, Type?>? _compactIdCache;

        // Map of type ID index to type ID string.
        // When reading a top-level encapsulation, we assign a type ID index (starting with 1) to each type ID we
        // read, in order. Since this map is a list, we lookup a previously assigned type ID string with
        // _typeIdMap[index - 1].
        private List<string>? _typeIdMap;
        private int _posAfterLatestInsertedTypeId = 0;

        // The remaining fields are used for class/exception unmarshaling.
        // Class/exception unmarshaling is allowed only when _mainEncaps != null and _endpointEncaps == null.

        // Map of class instance ID to class instance.
        // When reading a top-level encapsulation:
        //  - Instance ID = 0 means null
        //  - Instance ID = 1 means the instance is encoded inline afterwards
        //  - Instance ID > 1 means a reference to a previously read instance, found in this map.
        // Since the map is actually a list, we use instance ID - 2 to lookup an instance.
        private List<AnyClass>? _instanceMap;
        private int _classGraphDepth = 0;

         // Data for the class or exception instance that is currently getting unmarshaled.
        private InstanceData? _current;

        /// <summary>
        /// This constructor uses the communicator's default encoding version.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        /// <param name="data">The byte array containing encoded Slice types.</param>
        public InputStream(Communicator communicator, byte[] data)
            : this(communicator, null, new IceInternal.Buffer(data))
        {
        }

        /// <summary>
        /// This constructor uses the given encoding version.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        /// <param name="encoding">The desired encoding version.</param>
        public InputStream(Communicator communicator, Encoding encoding)
            : this(communicator, encoding, new IceInternal.Buffer())
        {
        }

        /// <summary>
        /// Releases any data retained by encapsulations.
        /// </summary>
        public void Clear() => ResetEncapsulation();

        /// <summary>Reads the start of an encapsulation.</summary>
        /// <returns>The encoding of the encapsulation.</returns>
        public Encoding StartEncapsulation()
        {
            Debug.Assert(_mainEncaps == null && _endpointEncaps == null);
            (Encoding Encoding, int Size) encapsHeader = ReadEncapsulationHeader();
            Debug.Assert(encapsHeader.Encoding == Encoding.V1_1); // TODO: temporary
            _mainEncaps = new Encaps(_limit, Encoding, encapsHeader.Size);
            Encoding = encapsHeader.Encoding;
            _limit = Pos + encapsHeader.Size - 6;

            // _mainEncapsBackup is usually null here, but can be set in the event we are reading a batch request
            // message/frame with multiple main encaps (one per request in the batch).
            _mainEncapsBackup = new MainEncapsBackup(_mainEncaps.Value, Pos, Encoding, _minTotalSeqSize);
            return encapsHeader.Encoding;
        }

        /// <summary>Ends an encapsulation started with StartEncpasulation or RestartEncapsulation.</summary>
        public void EndEncapsulation()
        {
            Debug.Assert(_mainEncaps != null && _endpointEncaps == null);
            SkipTaggedMembers();

            if (Remaining != 0)
            {
                throw new EncapsulationException();
            }
            _limit = _mainEncaps.Value.OldLimit;
            Encoding = _mainEncaps.Value.OldEncoding;
            ResetEncapsulation();
        }

        /// <summary>Restarts the most recently started encapsulation.</summary>
        public void RestartEncapsulation()
        {
            if (_mainEncapsBackup == null)
            {
                throw new EncapsulationException();
            }

            if (_mainEncaps != null || _endpointEncaps != null)
            {
                ResetEncapsulation();
            }

            // Restore backup:
            _mainEncaps = _mainEncapsBackup.Value.Encaps;
            Pos = _mainEncapsBackup.Value.Pos;
            Encoding = _mainEncapsBackup.Value.Encoding;
            _minTotalSeqSize = _mainEncapsBackup.Value.MinTotalSeqSize;
            _limit = Pos + _mainEncaps.Value.Size - 6;
        }

        /// <summary>Verifies if this InputStream can read data encoded using its current encoding.
        /// Throws Ice.UnsupportedEncodingException if it cannot.</summary>
        public void CheckIsReadable() => Encoding.CheckSupported();

        /// <summary>Go to the end of the current main encapsulation, if we are in one.</summary>
        public void SkipCurrentEncapsulation()
        {
            Debug.Assert(_endpointEncaps == null);
            if (_mainEncaps != null)
            {
                Pos = _limit!.Value;
                EndEncapsulation();
            }
        }

        /// <summary>
        /// Skips an empty encapsulation.
        /// </summary>
        /// <returns>The encapsulation's encoding version.</returns>
        public Encoding SkipEmptyEncapsulation()
        {
            (Encoding Encoding, int Size) encapsHeader = ReadEncapsulationHeader();
            Debug.Assert(encapsHeader.Encoding == Encoding.V1_1); // TODO: temporary
            // Skip the optional content of the encapsulation if we are expecting an
            // empty encapsulation.
            _buf.B.Position(_buf.B.Position() + encapsHeader.Size - 6);
            return encapsHeader.Encoding;
        }

        /// <summary>
        /// Returns a blob of bytes representing an encapsulation. The encapsulation's encoding version
        /// is returned in the argument.
        /// </summary>
        /// <param name="encoding">The encapsulation's encoding version.</param>
        /// <returns>The encoded encapsulation.</returns>
        public byte[] ReadEncapsulation(out Encoding encoding)
        {
            (Encoding Encoding, int Size) encapsHeader = ReadEncapsulationHeader();
            _buf.B.Position(_buf.B.Position() - 6);
            encoding = encapsHeader.Encoding;

            byte[] v = new byte[encapsHeader.Size];
            try
            {
                _buf.B.Get(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Determines the size of the current encapsulation, excluding the encapsulation header.
        /// </summary>
        /// <returns>The size of the encapsulated data.</returns>
        public int GetEncapsulationSize()
        {
            Debug.Assert(_endpointEncaps != null || _mainEncaps != null);
            int size = _endpointEncaps?.Size ?? _mainEncaps?.Size ?? 0;
            return size - 6;
        }

        /// <summary>
        /// Skips over an encapsulation.
        /// </summary>
        /// <returns>The encoding version of the skipped encapsulation.</returns>
        public Encoding SkipEncapsulation()
        {
            (Encoding Encoding, int Size) encapsHeader = ReadEncapsulationHeader();
            try
            {
                _buf.B.Position(_buf.B.Position() + encapsHeader.Size - 6);
            }
            catch (ArgumentOutOfRangeException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
            return encapsHeader.Encoding;
        }

        // Start reading a slice of a class or exception instance.
        // This is an Ice-internal method marked public because it's called by the generated code.
        // typeId is the expected type ID of this slice.
        // firstSlice is true when reading the first (most derived) slice of an instance.
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void IceStartSlice(string typeId, bool firstSlice)
        {
            Debug.Assert(_mainEncaps != null && _endpointEncaps == null);
            if (firstSlice)
            {
                Debug.Assert(_current != null && (_current.SliceTypeId == null || _current.SliceTypeId == typeId));
                if (_current.InstanceType == InstanceType.Class)
                {
                    // For exceptions, we read it for the first slice in ThrowException.
                    ReadIndirectionTableIntoCurrent();
                }

                // We can discard all the unknown slices: the generated code calls IceStartSliceAndGetSlicedData to
                // preserve them and it just called IceStartSlice instead.
                _current.Slices = null;
            }
            else
            {
                string? headerTypeId = ReadSliceHeaderIntoCurrent();
                Debug.Assert(headerTypeId == null || headerTypeId == typeId);
                ReadIndirectionTableIntoCurrent();
            }
        }

        // Start reading the first slice of an instance and get the unknown slices for this instances that were
        // previously saved (if any).
        // This is an Ice-internal method marked public because it's called by the generated code.
        // typeId is the expected typeId of this slice.
        [EditorBrowsable(EditorBrowsableState.Never)]
        public SlicedData? IceStartSliceAndGetSlicedData(string typeId)
        {
            Debug.Assert(_mainEncaps != null && _endpointEncaps == null);
            // Called by generated code for first slice instead of IceStartSlice
            Debug.Assert(_current != null && (_current.SliceTypeId == null || _current.SliceTypeId == typeId));
            if (_current.InstanceType == InstanceType.Class)
            {
                    // For exceptions, we read it for the first slice in ThrowException.
                    ReadIndirectionTableIntoCurrent();
            }
            return SlicedData;
        }

        // Tells the InputStream the end of a class or exception slice was reached.
        // This is an Ice-internal method marked public because it's called by the generated code.
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void IceEndSlice()
        {
            // Note that IceEndSlice is not called when we call SkipSlice.
            Debug.Assert(_mainEncaps != null && _endpointEncaps == null && _current != null);
            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_OPTIONAL_MEMBERS) != 0)
            {
                SkipTaggedMembers();
            }
            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_INDIRECTION_TABLE) != 0)
            {
                Debug.Assert(_current.PosAfterIndirectionTable.HasValue && _current.IndirectionTable != null);
                Pos = _current.PosAfterIndirectionTable.Value;
                _current.PosAfterIndirectionTable = null;
                _current.IndirectionTable = null;
            }
        }

        /// <summary>
        /// Extracts a size from the stream.
        /// </summary>
        /// <returns>The extracted size.</returns>
        public int ReadSize()
        {
            try
            {
                byte b = _buf.B.Get();
                if (b == 255)
                {
                    int v = _buf.B.GetInt();
                    if (v < 0)
                    {
                        throw new UnmarshalOutOfBoundsException();
                    }
                    return v;
                }
                else
                {
                    return b; // byte is unsigned
                }
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Reads a sequence size and make sure there is enough space in the underlying buffer to read the sequence.
        /// This validation is performed to make sure we do not allocate a large container based on an invalid encoded
        /// size.
        /// </summary>
        /// <param name="minElementSize">The minimum encoded size of an element of the sequence, in bytes.</param>
        /// <returns>The number of elements in the sequence.</returns>
        public int ReadAndCheckSeqSize(int minElementSize)
        {
            int sz = ReadSize();

            if (sz == 0)
            {
                return 0;
            }

            int minSize = sz * minElementSize;

            // With _minTotalSeqSize, we make sure that multiple sequences within an InpuStream can't trigger
            // maliciously the allocation of a large amount of memory before we read these sequences from the buffer.
            _minTotalSeqSize += minSize;

            if (_buf.B.Position() + minSize > _buf.Size() || _minTotalSeqSize > _buf.Size())
            {
                throw new UnmarshalOutOfBoundsException();
            }
            return sz;
        }

        /// <summary>
        /// Reads a blob of bytes from the stream. The length of the given array determines how many bytes are read.
        /// </summary>
        /// <param name="v">Bytes from the stream.</param>
        public void ReadBlob(byte[] v)
        {
            try
            {
                _buf.B.Get(v);
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Reads a blob of bytes from the stream.
        /// </summary>
        /// <param name="sz">The number of bytes to read.</param>
        /// <returns>The requested bytes as a byte array.</returns>
        public byte[] ReadBlob(int sz)
        {
            if (Remaining < sz)
            {
                throw new UnmarshalOutOfBoundsException();
            }
            byte[] v = new byte[sz];
            try
            {
                _buf.B.Get(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Determine if an optional value is available for reading.
        /// </summary>
        /// <param name="tag">The tag associated with the value.</param>
        /// <param name="expectedFormat">The optional format for the value.</param>
        /// <returns>True if the value is present, false otherwise.</returns>
        public bool ReadOptional(int tag, OptionalFormat expectedFormat)
        {
            // Tagged members/parameters can only be in the main encaps
            Debug.Assert(_mainEncaps != null && _endpointEncaps == null);

            // The current slice has no tagged member
            if (_current != null && (_current.SliceFlags & EncodingDefinitions.FLAG_HAS_OPTIONAL_MEMBERS) == 0)
            {
                return false;
            }

            int requestedTag = tag;

            while (true)
            {
                if (Remaining <= 0)
                {
                    return false; // End of encapsulation also indicates end of optionals.
                }

                int v = ReadByte();
                if (v == EncodingDefinitions.OPTIONAL_END_MARKER)
                {
                    _buf.B.Position(_buf.B.Position() - 1); // Rewind.
                    return false;
                }

                var format = (OptionalFormat)(v & 0x07); // First 3 bits.
                tag = v >> 3;
                if (tag == 30)
                {
                    tag = ReadSize();
                }

                if (tag > requestedTag)
                {
                    int offset = tag < 30 ? 1 : (tag < 255 ? 2 : 6); // Rewind
                    _buf.B.Position(_buf.B.Position() - offset);
                    return false; // No tagged member with the requested tag.
                }
                else if (tag < requestedTag)
                {
                    SkipTagged(format);
                }
                else
                {
                    if (format != expectedFormat)
                    {
                        throw new MarshalException("invalid tagged data member `" + tag + "': unexpected format");
                    }
                    return true;
                }
            }
        }

        /// <summary>
        /// Extracts a byte value from the stream.
        /// </summary>
        /// <returns>The extracted byte.</returns>
        public byte ReadByte()
        {
            try
            {
                return _buf.B.Get();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional byte value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public byte? ReadByte(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F1))
            {
                return ReadByte();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of byte values from the stream.
        /// </summary>
        /// <returns>The extracted byte sequence.</returns>
        public byte[] ReadByteArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(1);
                byte[] v = new byte[sz];
                _buf.B.Get(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional byte sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public byte[]? ReadByteArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                return ReadByteArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a serializable object from the stream.
        /// </summary>
        /// <returns>The serializable object.</returns>
        public object? ReadSerializable()
        {
            int sz = ReadAndCheckSeqSize(1);
            if (sz == 0)
            {
                return null;
            }
            try
            {
                var f = new BinaryFormatter(null, new StreamingContext(StreamingContextStates.All, Communicator));
                return f.Deserialize(new IceInternal.InputStreamWrapper(sz, this));
            }
            catch (System.Exception ex)
            {
                throw new MarshalException("cannot deserialize object:", ex);
            }
        }

        /// <summary>
        /// Extracts a boolean value from the stream.
        /// </summary>
        /// <returns>The extracted boolean.</returns>
        public bool ReadBool()
        {
            try
            {
                return _buf.B.Get() == 1;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional boolean value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public bool? ReadBool(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F1))
            {
                return ReadBool();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of boolean values from the stream.
        /// </summary>
        /// <returns>The extracted boolean sequence.</returns>
        public bool[] ReadBoolArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(1);
                bool[] v = new bool[sz];
                _buf.B.GetBoolSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional boolean sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public bool[]? ReadBoolArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                return ReadBoolArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a short value from the stream.
        /// </summary>
        /// <returns>The extracted short.</returns>
        public short ReadShort()
        {
            try
            {
                return _buf.B.GetShort();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional short value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public short? ReadShort(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F2))
            {
                return ReadShort();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of short values from the stream.
        /// </summary>
        /// <returns>The extracted short sequence.</returns>
        public short[] ReadShortArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(2);
                short[] v = new short[sz];
                _buf.B.GetShortSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional short sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public short[]? ReadShortArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                SkipSize();
                return ReadShortArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts an int value from the stream.
        /// </summary>
        /// <returns>The extracted int.</returns>
        public int ReadInt()
        {
            try
            {
                return _buf.B.GetInt();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional int value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public int? ReadInt(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F4))
            {
                return ReadInt();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of int values from the stream.
        /// </summary>
        /// <returns>The extracted int sequence.</returns>
        public int[] ReadIntArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(4);
                int[] v = new int[sz];
                _buf.B.GetIntSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional int sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public int[]? ReadIntArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                SkipSize();
                return ReadIntArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a long value from the stream.
        /// </summary>
        /// <returns>The extracted long.</returns>
        public long ReadLong()
        {
            try
            {
                return _buf.B.GetLong();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional long value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public long? ReadLong(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F8))
            {
                return ReadLong();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of long values from the stream.
        /// </summary>
        /// <returns>The extracted long sequence.</returns>
        public long[] ReadLongArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(8);
                long[] v = new long[sz];
                _buf.B.GetLongSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional long sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public long[]? ReadLongArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                SkipSize();
                return ReadLongArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a float value from the stream.
        /// </summary>
        /// <returns>The extracted float.</returns>
        public float ReadFloat()
        {
            try
            {
                return _buf.B.GetFloat();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional float value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public float? ReadFloat(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F4))
            {
                return ReadFloat();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of float values from the stream.
        /// </summary>
        /// <returns>The extracted float sequence.</returns>
        public float[] ReadFloatArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(4);
                float[] v = new float[sz];
                _buf.B.GetFloatSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional float sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public float[]? ReadFloatArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                SkipSize();
                return ReadFloatArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a double value from the stream.
        /// </summary>
        /// <returns>The extracted double.</returns>
        public double ReadDouble()
        {
            try
            {
                return _buf.B.GetDouble();
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional double value from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public double? ReadDouble(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.F8))
            {
                return ReadDouble();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a sequence of double values from the stream.
        /// </summary>
        /// <returns>The extracted double sequence.</returns>
        public double[] ReadDoubleArray()
        {
            try
            {
                int sz = ReadAndCheckSeqSize(8);
                double[] v = new double[sz];
                _buf.B.GetDoubleSeq(v);
                return v;
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
        }

        /// <summary>
        /// Extracts an optional double sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public double[]? ReadDoubleArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                SkipSize();
                return ReadDoubleArray();
            }
            else
            {
                return null;
            }
        }

        private static readonly System.Text.UTF8Encoding _utf8 = new System.Text.UTF8Encoding(false, true);

        /// <summary>
        /// Extracts a string from the stream.
        /// </summary>
        /// <returns>The extracted string.</returns>
        public string ReadString()
        {
            int len = ReadSize();

            if (len == 0)
            {
                return "";
            }

            //
            // Check the buffer has enough bytes to read.
            //
            if (Remaining < len)
            {
                throw new UnmarshalOutOfBoundsException();
            }

            try
            {
                //
                // We reuse the _stringBytes array to avoid creating
                // excessive garbage
                //
                if (_stringBytes == null || len > _stringBytes.Length)
                {
                    _stringBytes = new byte[len];
                }
                _buf.B.Get(_stringBytes, 0, len);
                return _utf8.GetString(_stringBytes, 0, len);
            }
            catch (InvalidOperationException ex)
            {
                throw new UnmarshalOutOfBoundsException(ex);
            }
            catch (ArgumentException ex)
            {
                throw new MarshalException("Invalid UTF8 string", ex);
            }
        }

        /// <summary>
        /// Extracts an optional string from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public string? ReadString(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.VSize))
            {
                return ReadString();
            }
            else
            {
                return null;
            }
        }

        public T[] ReadArray<T>(InputStreamReader<T> reader, int minSize)
        {
            var enumerable = new Collection<T>(this, reader, minSize);
            var arr = new T[enumerable.Count];
            int pos = 0;
            foreach (T item in enumerable)
            {
                arr[pos++] = item;
            }
            return arr;
        }

        public IEnumerable<T> ReadCollection<T>(InputStreamReader<T> reader, int minSize) =>
            new Collection<T>(this, reader, minSize);

        public Dictionary<TKey, TValue> ReadDict<TKey, TValue>(InputStreamReader<TKey> keyReader,
            InputStreamReader<TValue> valueReader, int minWireSize = 1)
        {
            int sz = ReadAndCheckSeqSize(minWireSize);
            var dict = new Dictionary<TKey, TValue>(sz);
            for (int i = 0; i < sz; ++i)
            {
                TKey key = keyReader(this);
                TValue value = valueReader(this);
                dict.Add(key, value);
            }
            return dict;
        }

        public SortedDictionary<TKey, TValue> ReadSortedDict<TKey, TValue>(InputStreamReader<TKey> keyReader,
            InputStreamReader<TValue> valueReader)
        {
            int sz = ReadSize();
            var dict = new SortedDictionary<TKey, TValue>();
            for (int i = 0; i < sz; ++i)
            {
                TKey key = keyReader(this);
                TValue value = valueReader(this);
                dict.Add(key, value);
            }
            return dict;
        }

        /// <summary>
        /// Extracts a sequence of strings from the stream.
        /// </summary>
        /// <returns>The extracted string sequence.</returns>
        public string[] ReadStringArray() => ReadArray(IceReaderIntoString, 1);

        public IEnumerable<string> ReadStringCollection() => ReadCollection(IceReaderIntoString, 1);

        /// <summary>
        /// Extracts an optional string sequence from the stream.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <returns>The optional value.</returns>
        public string[]? ReadStringArray(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.FSize))
            {
                Skip(4);
                return ReadStringArray();
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Extracts a proxy from the stream. The stream must have been initialized with a communicator.
        /// </summary>
        /// <returns>The extracted proxy.</returns>
        public T? ReadProxy<T>(ProxyFactory<T> factory) where T : class, IObjectPrx
        {
            var ident = new Identity(this);
            if (ident.Name.Length == 0)
            {
                return null;
            }
            else
            {
                return factory(Communicator.CreateReference(ident, this));
            }
        }

        /// <summary>
        /// Extracts an optional proxy from the stream. The stream must have been initialized with a communicator.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <param name="factory">The proxy factory used to create the typed proxy.</param>
        /// <returns>The optional value.</returns>
        public T? ReadProxy<T>(int tag, ProxyFactory<T> factory) where T : class, IObjectPrx
        {
            if (ReadOptional(tag, OptionalFormat.FSize))
            {
                Skip(4);
                return ReadProxy(factory);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Read an enumerated value.
        /// </summary>
        /// <param name="maxValue">The maximum enumerator value in the definition.</param>
        /// <returns>The enumerator.</returns>
        public int ReadEnum(int maxValue)
        {
            // TODO: eliminate maxValue
            Debug.Assert(Encoding == Encoding.V1_1); // TODO: temporary
            return ReadSize();
        }

        /// <summary>
        /// Read an instance of class T.
        /// </summary>
        /// <returns>The class instance, or null.</returns>
        public T? ReadClass<T>() where T : AnyClass
        {
            AnyClass? obj = ReadAnyClass();
            if (obj == null)
            {
                return null;
            }
            else if (obj is T)
            {
                return (T)obj;
            }
            else
            {
                IceInternal.Ex.ThrowUOE(typeof(T), obj);
                return null;
            }
        }

        /// <summary>
        /// Read a tagged parameter or data member of type class T.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the class parameter or data member.</param>
        /// <returns>The class instance, or null.</returns>
        public T? ReadClass<T>(int tag) where T : AnyClass
        {
            AnyClass? obj = ReadAnyClass(tag);
            if (obj == null)
            {
                return null;
            }
            else if (obj is T)
            {
                return (T)obj;
            }
            else
            {
                IceInternal.Ex.ThrowUOE(typeof(T), obj);
                return null;
            }
        }

        /// <summary>
        /// Extracts a user exception from the stream and throws it.
        /// </summary>
        public void ThrowException()
        {
            Push(InstanceType.Exception);
            Debug.Assert(_current != null);

            // Read the first slice header, and exception's type ID cannot be null.
            string typeId = ReadSliceHeaderIntoCurrent()!;
            string mostDerivedId = typeId;
            ReadIndirectionTableIntoCurrent(); // we read the indirection table immediately

            while (true)
            {
                UserException? userEx = null;

                try
                {
                    Type? type = Communicator.ResolveClass(typeId);
                    if (type != null)
                    {
                        userEx = (UserException?)IceInternal.AssemblyUtil.CreateInstance(type);
                    }
                }
                catch (Exception ex)
                {
                    throw new MarshalException(ex);
                }

                // We found the exception.
                if (userEx != null)
                {
                    userEx.Read(this);
                    Pop(null);
                    throw userEx;
                    // Never reached.
                }

                // Slice off what we don't understand.
                SkipSlice();

                if ((_current.SliceFlags & EncodingDefinitions.FLAG_IS_LAST_SLICE) != 0)
                {
                    if (mostDerivedId.StartsWith("::", StringComparison.Ordinal) == true)
                    {
                        throw new UnknownUserException(mostDerivedId.Substring(2));
                    }
                    else
                    {
                        throw new UnknownUserException(mostDerivedId);
                    }
                }

                typeId = ReadSliceHeaderIntoCurrent()!;
                ReadIndirectionTableIntoCurrent();
            }
        }

        /// <summary>
        /// Skip the given number of bytes.
        /// </summary>
        /// <param name="size">The number of bytes to skip</param>
        public void Skip(int size)
        {
            if (size < 0 || size > Remaining)
            {
                throw new UnmarshalOutOfBoundsException();
            }
            _buf.B.Position(_buf.B.Position() + size);
        }

        /// <summary>
        /// Skip over a size value.
        /// </summary>
        public void SkipSize()
        {
            byte b = ReadByte();
            if (b == 255)
            {
                Skip(4);
            }
        }
        internal InputStream(Communicator communicator, Encoding encoding, IceInternal.Buffer buf, bool adopt)
            : this(communicator, encoding, new IceInternal.Buffer(buf, adopt))
        {
        }

        internal Encoding StartEndpointEncapsulation()
        {
            Debug.Assert(_endpointEncaps == null);
            (Encoding Encoding, int Size) encapsHeader = ReadEncapsulationHeader();
            _endpointEncaps = new Encaps(_limit, Encoding, encapsHeader.Size);
            Encoding = encapsHeader.Encoding;
            _limit = Pos + encapsHeader.Size - 6;
            return encapsHeader.Encoding;
        }

        internal void EndEndpointEncapsulation()
        {
            Debug.Assert(_endpointEncaps != null);

            if (Remaining != 0)
            {
                throw new EncapsulationException();
            }

            _limit = _endpointEncaps.Value.OldLimit;
            Encoding = _endpointEncaps.Value.OldEncoding;
            _endpointEncaps = null;
        }

       // Swaps the contents of one stream with another.
        internal void Swap(InputStream other)
        {
            Debug.Assert(Communicator == other.Communicator);

            IceInternal.Buffer tmpBuf = other._buf;
            other._buf = _buf;
            _buf = tmpBuf;

            Encoding tmpEncoding = other.Encoding;
            other.Encoding = Encoding;
            Encoding = tmpEncoding;

            // Swap is never called for InputStreams that have encapsulations being read. However,
            // encapsulations might still be set in case un-marshalling failed. We just
            // reset the encapsulations if there are still some set.
            ResetEncapsulation();
            other.ResetEncapsulation();

            int tmpMinTotalSeqSize = other._minTotalSeqSize;
            other._minTotalSeqSize = _minTotalSeqSize;
            _minTotalSeqSize = tmpMinTotalSeqSize;
        }

        // Resizes the stream to a new size.
        internal void Resize(int sz)
        {
            _buf.Resize(sz, true);
            _buf.B.Position(sz);
        }

        internal IceInternal.Buffer GetBuffer() => _buf;

        // Helper constructor used by the other constructors.
        private InputStream(Communicator communicator, Encoding? encoding, IceInternal.Buffer buf)
        {
            Encoding = encoding ?? communicator.DefaultsAndOverrides.DefaultEncoding;
            Communicator = communicator;
            _buf = buf;
        }

        private void ResetEncapsulation()
        {
            _mainEncaps = null;
            _endpointEncaps = null;
            _instanceMap?.Clear();
            _classGraphDepth = 0;
            _typeIdMap?.Clear();
            _posAfterLatestInsertedTypeId = 0;
            _current = null;
            _limit = null;
        }

        private (Encoding Encoding, int Size) ReadEncapsulationHeader()
        {
            // With the 1.1 encoding, the encaps size is encoded on a 4-bytes int and not on a variable-length size,
            // for ease of marshaling.
            int sz = ReadInt();
            Debug.Assert(sz >= 6);
            if (sz < 6)
            {
                throw new UnmarshalOutOfBoundsException();
            }
            if (sz - 4 > Remaining)
            {
                throw new UnmarshalOutOfBoundsException();
            }
            byte major = ReadByte();
            byte minor = ReadByte();
            var encoding = new Encoding(major, minor);
            return (encoding, sz);
        }

        private void SkipTagged(OptionalFormat format)
        {
            switch (format)
            {
                case OptionalFormat.F1:
                    {
                        Skip(1);
                        break;
                    }
                case OptionalFormat.F2:
                    {
                        Skip(2);
                        break;
                    }
                case OptionalFormat.F4:
                    {
                        Skip(4);
                        break;
                    }
                case OptionalFormat.F8:
                    {
                        Skip(8);
                        break;
                    }
                case OptionalFormat.Size:
                    {
                        SkipSize();
                        break;
                    }
                case OptionalFormat.VSize:
                    {
                        Skip(ReadSize());
                        break;
                    }
                case OptionalFormat.FSize:
                    {
                        Skip(ReadInt());
                        break;
                    }
                case OptionalFormat.Class:
                    {
                        ReadAnyClass();
                        break;
                    }
            }
        }

        private bool SkipTaggedMembers()
        {
            // Skip remaining unread tagged members.
            while (true)
            {
                if (Remaining <= 0)
                {
                    return false; // End of encapsulation also indicates end of tagged members.
                }

                int v = ReadByte();
                if (v == EncodingDefinitions.OPTIONAL_END_MARKER)
                {
                    return true;
                }

                var format = (OptionalFormat)(v & 0x07); // Read first 3 bits.
                if ((v >> 3) == 30)
                {
                    SkipSize();
                }
                SkipTagged(format);
            }
        }

        private string ReadTypeId(bool isIndex)
        {
            _typeIdMap ??= new List<string>();

            if (isIndex)
            {
                int index = ReadSize();
                if (index > 0 && index - 1 < _typeIdMap.Count)
                {
                    // The encoded type-id indices start at 1, not 0.
                    return _typeIdMap[index - 1];
                }
                throw new MarshalException($"read invalid typeId index {index}");
            }
            else
            {
                string typeId = ReadString();

                // The typeIds of slices in indirection tables can be read several times: when we skip the
                // indirection table and later on when we read it. We only want to add this typeId to the list
                // and assign it an index when it's the first time we read it, so we save the largest pos we
                // read to figure out when to add to the list.
                if (Pos > _posAfterLatestInsertedTypeId)
                {
                    _posAfterLatestInsertedTypeId = Pos;
                    _typeIdMap.Add(typeId);
                }

                return typeId;
            }
        }

        private Type? ResolveClass(string typeId)
        {
            if (_typeIdCache == null || !_typeIdCache.TryGetValue(typeId, out Type? cls))
            {
                // Not found in typeIdCache
                try
                {
                    cls = Communicator.ResolveClass(typeId);
                    _typeIdCache ??= new Dictionary<string, Type?>(); // Lazy initialization
                    _typeIdCache.Add(typeId, cls);
                }
                catch (Exception ex)
                {
                    throw new NoClassFactoryException("no class factory", typeId, ex);
                }
            }
            return cls;
        }

        private Type? ResolveClass(int compactId)
        {
            Type? cls = null;
            if (_compactIdCache == null || !_compactIdCache.TryGetValue(compactId, out cls))
            {
                // Not found in compactIdCache
                string? typeId = Communicator.ResolveCompactId(compactId);
                if (typeId != null)
                {
                    cls = ResolveClass(typeId);
                    _compactIdCache ??= new Dictionary<int, Type?>();
                    _compactIdCache.Add(compactId, cls);
                }
            }
            return cls;
        }

        private AnyClass? ReadAnyClass()
        {
            int index = ReadSize();
            if (index < 0)
            {
                throw new MarshalException("invalid object id");
            }
            else if (index == 0)
            {
                return null;
            }
            else if (_current != null && (_current.SliceFlags & EncodingDefinitions.FLAG_HAS_INDIRECTION_TABLE) != 0)
            {
                // When reading an instance within a slice and there is an
                // indirection table, we have an index within this indirection table.
                //
                // We need to decrement index since position 0 in the indirection table
                // corresponds to index 1.
                index--;
                if (index < _current.IndirectionTable?.Length)
                {
                    return _current.IndirectionTable[index];
                }
                else
                {
                    throw new MarshalException("index too big for indirection table");
                }
            }
            else
            {
                return ReadInstance(index);
            }
        }

        // Read a tagged parameter or data member of type class.
        private AnyClass? ReadAnyClass(int tag)
        {
            if (ReadOptional(tag, OptionalFormat.Class))
            {
                return ReadAnyClass();
            }
            else
            {
                return null;
            }
        }

        // Read a slice header into _current.
        // Returns the type ID of that slice. Null means it's a slice in compact format without a type ID,
        // or a slice with a compact ID we could not resolve.
        private string? ReadSliceHeaderIntoCurrent()
        {
            Debug.Assert(_current != null);

            _current.SliceFlags = ReadByte();

            // Read the type ID. For class slices, the type ID is encoded as a
            // string or as an index or as a compact ID, for exceptions it's always encoded as a
            // string.
            if (_current.InstanceType == InstanceType.Class)
            {
                // TYPE_ID_COMPACT must be checked first!
                if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_TYPE_ID_COMPACT) ==
                    EncodingDefinitions.FLAG_HAS_TYPE_ID_COMPACT)
                {
                    _current.SliceCompactId = ReadSize();
                    _current.SliceTypeId = null;
                }
                else if ((_current.SliceFlags &
                        (EncodingDefinitions.FLAG_HAS_TYPE_ID_INDEX | EncodingDefinitions.FLAG_HAS_TYPE_ID_STRING)) != 0)
                {
                    _current.SliceTypeId = ReadTypeId((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_TYPE_ID_INDEX) != 0);
                    _current.SliceCompactId = null;
                }
                else
                {
                    // Slice in compact format, without a type ID or compact ID.
                    Debug.Assert((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_SLICE_SIZE) == 0);
                    _current.SliceTypeId = null;
                    _current.SliceCompactId = null;
                }
            }
            else
            {
                _current.SliceTypeId = ReadString();
                Debug.Assert(_current.SliceCompactId == null); // no compact ID for exceptions
            }

            // Read the slice size if necessary.
            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_SLICE_SIZE) != 0)
            {
                _current.SliceSize = ReadInt();
                if (_current.SliceSize < 4)
                {
                    throw new MarshalException("invalid slice size");
                }
            }
            else
            {
                _current.SliceSize = 0;
            }

            // Clear other per-slice fields:
            _current.IndirectionTable = null;
            _current.PosAfterIndirectionTable = null;

            return _current.SliceTypeId;
        }

        // Read the indirection table into _current's fields if there is an indirection table.
        // Precondition: called after reading the slice's header.
        // This method does not change Pos.
        private void ReadIndirectionTableIntoCurrent()
        {
            Debug.Assert(_current != null && _current.IndirectionTable == null);
            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_INDIRECTION_TABLE) != 0)
            {
                int savedPos = Pos;
                if (_current.SliceSize < 4)
                {
                    throw new MarshalException("invalid slice size");
                }
                Pos = savedPos + _current.SliceSize - 4;
                _current.IndirectionTable = ReadIndirectionTable();
                _current.PosAfterIndirectionTable = Pos;
                Pos = savedPos;
            }
        }

        // Skip the body of the current slice and it indirection table (if any).
        // When it's a class instance and there is an indirection table, it returns the starting position of that
        // indirection table; otherwise, it return 0.
        private int SkipSlice()
        {
            Debug.Assert(_current != null);
            if (Communicator.TraceLevels.Slicing > 0)
            {
                ILogger logger = Communicator.Logger;
                string slicingCat = Communicator.TraceLevels.SlicingCat;
                if (_current.InstanceType == InstanceType.Exception)
                {
                    IceInternal.TraceUtil.TraceSlicing("exception", _current.SliceTypeId ?? "", slicingCat, logger);
                }
                else
                {
                    IceInternal.TraceUtil.TraceSlicing("object", _current.SliceTypeId ?? "", slicingCat, logger);
                }
            }

            int start = Pos;

            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_SLICE_SIZE) != 0)
            {
                Debug.Assert(_current.SliceSize >= 4);
                Skip(_current.SliceSize - 4);
            }
            else
            {
                if (_current.InstanceType == InstanceType.Class)
                {
                    throw new NoClassFactoryException("no class factory found and compact format prevents " +
                                                      "slicing (the sender should use the sliced format " +
                                                      "instead)", _current.SliceTypeId ?? "");
                }
                else
                {
                    if (_current.SliceTypeId?.StartsWith("::", StringComparison.Ordinal) == true)
                    {
                        throw new UnknownUserException(_current.SliceTypeId!.Substring(2));
                    }
                    else
                    {
                        throw new UnknownUserException(_current.SliceTypeId ?? "");
                    }
                }
            }

            // Preserve this slice.
            bool hasOptionalMembers = (_current.SliceFlags & EncodingDefinitions.FLAG_HAS_OPTIONAL_MEMBERS) != 0;
            IceInternal.ByteBuffer b = GetBuffer().B;
            int end = b.Position();
            int dataEnd = end;
            if (hasOptionalMembers)
            {
                // Don't include the tagged end marker. It will be re-written by IceEndSlice when the sliced data
                // is re-written.
                --dataEnd;
            }
            byte[] bytes = new byte[dataEnd - start];
            b.Position(start);
            b.Get(bytes);
            b.Position(end);

            int startOfIndirectionTable = 0;

            if ((_current.SliceFlags & EncodingDefinitions.FLAG_HAS_INDIRECTION_TABLE) != 0)
            {
                if (_current.InstanceType == InstanceType.Class)
                {
                    startOfIndirectionTable = Pos;
                    SkipIndirectionTable();
                }
                else
                {
                    Debug.Assert(_current.PosAfterIndirectionTable != null);
                    // Move past indirection table
                    Pos = _current.PosAfterIndirectionTable.Value;
                    _current.PosAfterIndirectionTable = null;
                }
            }
            _current.Slices ??= new List<SliceInfo>();
            var info = new SliceInfo(_current.SliceTypeId,
                                     _current.SliceCompactId,
                                     new ReadOnlyMemory<byte>(bytes),
                                     Array.AsReadOnly(_current.IndirectionTable ?? Array.Empty<AnyClass>()),
                                     hasOptionalMembers,
                                     (_current.SliceFlags & EncodingDefinitions.FLAG_IS_LAST_SLICE) != 0);
            _current.Slices.Add(info);

            // An exception slice may have an indirection table (saved above). We don't need it anymore
            // since we're skipping this slice.
            _current.IndirectionTable = null;
            return startOfIndirectionTable;
        }

        // Skip the indirection table. The caller must save the current stream position before calling
        // SkipIndirectionTable (to read the indirection table at a later point) except when the caller
        // is SkipIndirectionTable itself.
        private void SkipIndirectionTable()
        {
            Debug.Assert(_current != null);
            // We should never skip an exception's indirection table
            Debug.Assert(_current.InstanceType == InstanceType.Class);

            // We use ReadSize and not ReadAndCheckSeqSize here because we don't allocate memory for this
            // sequence, and since we are skipping this sequence to read it later, we don't want to double-count
            // its contribution to _minTotalSeqSize.
            int tableSize = ReadSize();
            for (int i = 0; i < tableSize; ++i)
            {
                int index = ReadSize();
                if (index <= 0)
                {
                    throw new MarshalException($"read invalid index {index} in indirection table");
                }
                if (index == 1)
                {
                    if (++_classGraphDepth > Communicator.ClassGraphDepthMax)
                    {
                        throw new MarshalException("maximum class graph depth reached");
                    }

                    // Read/skip this instance
                    byte sliceFlags;
                    do
                    {
                        sliceFlags = ReadByte();
                        if ((sliceFlags & EncodingDefinitions.FLAG_HAS_TYPE_ID_COMPACT) == EncodingDefinitions.FLAG_HAS_TYPE_ID_COMPACT)
                        {
                            ReadSize(); // compact type-id
                        }
                        else if ((sliceFlags &
                            (EncodingDefinitions.FLAG_HAS_TYPE_ID_INDEX | EncodingDefinitions.FLAG_HAS_TYPE_ID_STRING)) != 0)
                        {
                            // This can update the typeIdMap
                            ReadTypeId((sliceFlags & EncodingDefinitions.FLAG_HAS_TYPE_ID_INDEX) != 0);
                        }
                        else
                        {
                            throw new MarshalException(
                                "indirection table cannot hold an instance without a type-id");
                        }

                        // Read the slice size, then skip the slice
                        if ((sliceFlags & EncodingDefinitions.FLAG_HAS_SLICE_SIZE) == 0)
                        {
                            throw new MarshalException("size of slice missing");
                        }
                        int sliceSize = ReadInt();
                        if (sliceSize < 4)
                        {
                            throw new MarshalException("invalid slice size");
                        }
                        Pos = Pos + sliceSize - 4;

                        // If this slice has an indirection table, skip it too
                        if ((sliceFlags & EncodingDefinitions.FLAG_HAS_INDIRECTION_TABLE) != 0)
                        {
                            SkipIndirectionTable();
                        }
                    } while ((sliceFlags & EncodingDefinitions.FLAG_IS_LAST_SLICE) == 0);
                    _classGraphDepth--;
                }
            }
        }

        private AnyClass[] ReadIndirectionTable()
        {
            int size = ReadAndCheckSeqSize(1);
            if (size == 0)
            {
                throw new MarshalException("invalid empty indirection table");
            }
            var indirectionTable = new AnyClass[size];
            for (int i = 0; i < indirectionTable.Length; ++i)
            {
                int index = ReadSize();
                if (index < 1)
                {
                    throw new MarshalException($"read invalid index {index} in indirection table");
                }
                indirectionTable[i] = ReadInstance(index);
            }
            return indirectionTable;
        }

        private AnyClass ReadInstance(int index)
        {
            Debug.Assert(index > 0);

            if (index > 1)
            {
                if (_instanceMap != null && _instanceMap.Count > index - 2)
                {
                    return _instanceMap[index - 2];
                }
                throw new MarshalException($"could not find index {index} in {nameof(_instanceMap)}");
            }

            InstanceData? previousCurrent = Push(InstanceType.Class);
            Debug.Assert(_current != null);

            // Read the first slice header.
            string? mostDerivedId = ReadSliceHeaderIntoCurrent();
            string? typeId = mostDerivedId;
            // We cannot read the indirection table at this point as it may reference the new instance that is not
            // created yet.

            AnyClass? v = null;
            List<int>? deferredIndirectionTableList = null;

            while (true)
            {
                Type? cls = null;
                if (typeId != null)
                {
                    Debug.Assert(_current.SliceCompactId == null);
                    cls = ResolveClass(typeId);
                }
                else if (_current.SliceCompactId.HasValue)
                {
                    cls = ResolveClass(_current.SliceCompactId.Value);
                }

                if (cls != null)
                {
                    try
                    {
                        Debug.Assert(!cls.IsAbstract && !cls.IsInterface);
                        v = (AnyClass?)IceInternal.AssemblyUtil.CreateInstance(cls);
                    }
                    catch (Exception ex)
                    {
                        throw new NoClassFactoryException("no class factory", typeId ?? "", ex);
                    }
                }

                if (v != null)
                {
                    // We have an instance, get out of this loop.
                    break;
                }

                // Slice off what we don't understand, and save the indirection table (if any) in
                // deferredIndirectionTableList.
                deferredIndirectionTableList ??= new List<int>();
                deferredIndirectionTableList.Add(SkipSlice());

                // If this is the last slice, keep the instance as an opaque UnknownSlicedClass object.
                if ((_current.SliceFlags & EncodingDefinitions.FLAG_IS_LAST_SLICE) != 0)
                {
                    v = new UnknownSlicedClass();
                    break;
                }

                typeId = ReadSliceHeaderIntoCurrent(); // Read next Slice header for next iteration.
            }

            if (++_classGraphDepth > Communicator.ClassGraphDepthMax)
            {
                throw new MarshalException("maximum class graph depth reached");
            }

            // Add the instance to the map/list of instances. This must be done before reading the instances (for
            // circular references).
            _instanceMap ??= new List<AnyClass>();
            _instanceMap.Add(v);

            // Read all the deferred indirection tables now that the instance is inserted in _instanceMap.
            if (deferredIndirectionTableList?.Count > 0)
            {
                int savedPos = Pos;

                Debug.Assert(_current.Slices?.Count == deferredIndirectionTableList.Count);
                for (int i = 0; i < deferredIndirectionTableList.Count; ++i)
                {
                    int pos = deferredIndirectionTableList[i];
                    if (pos > 0)
                    {
                        Pos = pos;
                        _current.Slices[i].Instances = Array.AsReadOnly(ReadIndirectionTable());
                    }
                    // else remains empty
                }
                Pos = savedPos;
            }

            // Read the instance.
            v.Read(this);
            Pop(previousCurrent);

            --_classGraphDepth;
            return v;
        }

        // Create a new current instance of the specified slice type
        // and return the previous current instance, if any.
        private InstanceData? Push(InstanceType instanceType)
        {
            // Can't have a current instance already if we are reading an exception
            Debug.Assert(instanceType == InstanceType.Class || _current == null);
            InstanceData? oldInstance = _current;
            _current = new InstanceData(instanceType);
            return oldInstance;
        }

        // Replace the current instance by savedInstance
        private void Pop(InstanceData? savedInstance)
        {
            Debug.Assert(_current != null);
            _current = savedInstance;
        }

        private enum InstanceType { Class, Exception }

        private sealed class InstanceData
        {
            internal InstanceData(InstanceType instanceType) => InstanceType = instanceType;

            // Instance attributes
            internal readonly InstanceType InstanceType;
            internal List<SliceInfo>? Slices; // Preserved slices.

            // Slice attributes
            internal byte SliceFlags = 0;
            internal int SliceSize = 0;
            internal string? SliceTypeId;
            internal int? SliceCompactId;
            // Indirection table of the current slice
            internal AnyClass[]? IndirectionTable;
            internal int? PosAfterIndirectionTable;
        }

        private readonly struct Encaps
        {
            // Previous upper limit of the buffer, if set
            internal readonly int? OldLimit;

            // Old Encoding
            internal readonly Encoding OldEncoding;

            // Size of the encaps, as read from the stream
            internal readonly int Size;

            internal Encaps(int? oldLimit, Encoding oldEncoding, int size)
            {
                OldLimit = oldLimit;
                OldEncoding = oldEncoding;
                Size = size;
            }
        }

        private readonly struct MainEncapsBackup
        {
            internal readonly Encaps Encaps;
            internal readonly int Pos;

            internal readonly Encoding Encoding;
            internal readonly int MinTotalSeqSize;

            internal MainEncapsBackup(Encaps encaps, int pos, Encoding encoding, int minTotalSeqSize)
            {
                Encaps = encaps;
                Pos = pos;
                Encoding = encoding;
                MinTotalSeqSize = minTotalSeqSize;
            }
        }

        private sealed class Collection<T> : ICollection<T>
        {
            private readonly InputStream _ins;
            private readonly InputStreamReader<T> _read;

            public int Count { get; }

            public bool IsReadOnly => true;

            public Collection(InputStream ins, InputStreamReader<T> read, int minSize)
            {
                _ins = ins;
                _read = read;
                Count = ins.ReadAndCheckSeqSize(minSize);
            }

            // TODO: Ideally this should use a InputStream view and cache the input stream start
            // position, so that succesivelly enumerators yield same valid results. In practice
            // GetEnuerator should only be called once when the collection is unmarshal.
            public IEnumerator<T> GetEnumerator() => new Enumerator<T>(_ins, _read, Count);

            IEnumerator IEnumerable.GetEnumerator() => new Enumerator<T>(_ins, _read, Count);
            public void Add(T item) => throw new NotSupportedException();
            public void Clear() => throw new NotSupportedException();
            public bool Contains(T item) => throw new NotSupportedException();
            public void CopyTo(T[] array, int arrayIndex)
            {
                foreach (T value in this)
                {
                    array[arrayIndex++] = value;
                }
            }
            public bool Remove(T item) => throw new NotSupportedException();
        }

        private sealed class Enumerator<T> : IEnumerator<T>
        {
            public T Current
            {
                get
                {
                    if (_pos == 0 || _pos > _size)
                    {
                        throw new InvalidOperationException();
                    }
                    return _current;
                }
            }

            object IEnumerator.Current => Current!;

            private T _current;
            private readonly InputStream _ins;
            private readonly InputStreamReader<T> _read;
            private int _pos;
            private readonly int _size;

#pragma warning disable CS8618 // Non-nullable field is uninitialized. Consider declaring as nullable.
            // Disabled this warning as the _current field is never read until it is initialized
            // in MoveNext. The Current property accessor warrants it. Declared the field as nullable
            // is not an option for a generic that can be used with reference and value types.
            public Enumerator(InputStream ins, InputStreamReader<T> read, int size)
#pragma warning restore CS8618 // Non-nullable field is uninitialized. Consider declaring as nullable.
            {
                _ins = ins;
                _read = read;
                _size = size;
                _pos = 0;
            }

            public bool MoveNext()
            {
                if (++_pos > _size)
                {
                    _pos = _size + 1;
                    return false;
                }
                else
                {
                    _current = _read(_ins);
                    return true;
                }
            }

            public void Reset() => throw new NotSupportedException();
            public void Dispose()
            {
            }
        }
    }
}
