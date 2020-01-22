//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace Ice
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using Protocol = IceInternal.Protocol;

    /// <summary>
    /// Interface for output streams used to write Slice types to a sequence
    /// of bytes.
    /// </summary>
    public class OutputStream
    {

        /// <summary>
        /// Constructing an OutputStream without providing a communicator means the stream will
        /// use the default encoding version and the default format for class encoding.
        /// You can supply a communicator later by calling initialize().
        /// </summary>
        public OutputStream()
        {
            _buf = new IceInternal.Buffer();
            _communicator = null;
            _closure = null;
            _encoding = Util.currentEncoding;
            _format = FormatType.CompactFormat;
        }

        /// <summary>
        /// This constructor uses the communicator's default encoding version.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        public OutputStream(Communicator communicator)
        {
            Debug.Assert(communicator != null);
            Initialize(communicator, communicator.DefaultsAndOverrides.defaultEncoding, new IceInternal.Buffer());
        }

        /// <summary>
        /// This constructor uses the given communicator and encoding version.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        /// <param name="encoding">The desired encoding version.</param>
        public OutputStream(Communicator communicator, EncodingVersion encoding)
        {
            Debug.Assert(communicator != null);
            Initialize(communicator, encoding);
        }

        public OutputStream(Ice.Communicator communicator, EncodingVersion encoding, IceInternal.Buffer buf, bool adopt)
        {
            Initialize(communicator, encoding, new IceInternal.Buffer(buf, adopt));
        }

        /// <summary>
        /// Initializes the stream to use the communicator's default encoding version and class
        /// encoding format.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        public void Initialize(Communicator communicator)
        {
            Debug.Assert(communicator != null);
            Initialize(communicator, communicator.DefaultsAndOverrides.defaultEncoding);
        }

        public void Initialize(Communicator communicator, EncodingVersion encoding)
        {
            Debug.Assert(communicator != null);
            Initialize(communicator, encoding, new IceInternal.Buffer());
        }

        /// <summary>
        /// Initializes the stream to use the given encoding version and the communicator's
        /// default class encoding format.
        /// </summary>
        /// <param name="communicator">The communicator to use when initializing the stream.</param>
        /// <param name="encoding">The desired encoding version.</param>
        /// <param name="buf">The desired encoding version.</param>
        private void Initialize(Ice.Communicator communicator, EncodingVersion encoding, IceInternal.Buffer buf)
        {
            Debug.Assert(communicator != null);

            _communicator = communicator;
            _buf = buf;
            _closure = null;
            _encoding = encoding;

            _format = _communicator.DefaultsAndOverrides.defaultFormat;

            _encapsStack = null;
            _encapsCache = null;
        }

        /// <summary>
        /// Resets this output stream. This method allows the stream to be reused, to avoid creating
        /// unnecessary garbage.
        /// </summary>
        public void Reset()
        {
            _buf.reset();
            Clear();
        }

        /// <summary>
        /// Releases any data retained by encapsulations. The reset() method internally calls clear().
        /// </summary>
        public void Clear()
        {
            if (_encapsStack != null)
            {
                Debug.Assert(_encapsStack.next == null);
                _encapsStack.next = _encapsCache;
                _encapsCache = _encapsStack;
                _encapsStack = null;
                _encapsCache.reset();
            }
        }

        public Communicator communicator()
        {
            return _communicator;
        }

        /// <summary>
        /// Sets the encoding format for class and exception instances.
        /// </summary>
        /// <param name="fmt">The encoding format.</param>
        public void SetFormat(FormatType fmt)
        {
            _format = fmt;
        }

        /// <summary>
        /// Retrieves the closure object associated with this stream.
        /// </summary>
        /// <returns>The closure object.</returns>
        public object GetClosure()
        {
            return _closure;
        }

        /// <summary>
        /// Associates a closure object with this stream.
        /// </summary>
        /// <param name="p">The new closure object.</param>
        /// <returns>The previous closure object, or null.</returns>
        public object SetClosure(object p)
        {
            object prev = _closure;
            _closure = p;
            return prev;
        }

        /// <summary>
        /// Indicates that the marshaling of a request or reply is finished.
        /// </summary>
        /// <returns>The byte sequence containing the encoded request or reply.</returns>
        public byte[] Finished()
        {
            IceInternal.Buffer buf = PrepareWrite();
            byte[] result = new byte[buf.b.limit()];
            buf.b.get(result);
            return result;
        }

        /// <summary>
        /// Swaps the contents of one stream with another.
        /// </summary>
        /// <param name="other">The other stream.</param>
        public void Swap(OutputStream other)
        {
            Debug.Assert(_communicator == other._communicator);

            IceInternal.Buffer tmpBuf = other._buf;
            other._buf = _buf;
            _buf = tmpBuf;

            EncodingVersion tmpEncoding = other._encoding;
            other._encoding = _encoding;
            _encoding = tmpEncoding;

            object tmpClosure = other._closure;
            other._closure = _closure;
            _closure = tmpClosure;

            //
            // Swap is never called for streams that have encapsulations being written. However,
            // encapsulations might still be set in case marshalling failed. We just
            // reset the encapsulations if there are still some set.
            //
            ResetEncapsulation();
            other.ResetEncapsulation();
        }

        private void ResetEncapsulation()
        {
            _encapsStack = null;
        }

        /// <summary>
        /// Resizes the stream to a new size.
        /// </summary>
        /// <param name="sz">The new size.</param>
        public void Resize(int sz)
        {
            _buf.resize(sz, false);
            _buf.b.position(sz);
        }

        /// <summary>
        /// Prepares the internal data buffer to be written to a socket.
        /// </summary>
        public IceInternal.Buffer PrepareWrite()
        {
            _buf.b.limit(_buf.size());
            _buf.b.position(0);
            return _buf;
        }

        /// <summary>
        /// Retrieves the internal data buffer.
        /// </summary>
        /// <returns>The buffer.</returns>
        public IceInternal.Buffer GetBuffer()
        {
            return _buf;
        }

        /// <summary>
        /// Marks the start of a class instance.
        /// </summary>
        /// <param name="data">Preserved slices for this instance, or null.</param>
        public void StartClass(SlicedData? data)
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.StartInstance(SliceType.ClassSlice, data);
        }

        /// <summary>
        /// Marks the end of a class instance.
        /// </summary>
        public void EndClass()
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.EndInstance();
        }

        /// <summary>
        /// Marks the start of a user exception.
        /// </summary>
        /// <param name="data">Preserved slices for this exception, or null.</param>
        public void StartException(SlicedData? data)
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.StartInstance(SliceType.ExceptionSlice, data);
        }

        /// <summary>
        /// Marks the end of a user exception.
        /// </summary>
        public void EndException()
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.EndInstance();
        }

        /// <summary>
        /// Writes the start of an encapsulation to the stream.
        /// </summary>
        public void StartEncapsulation()
        {
            //
            // If no encoding version is specified, use the current write
            // encapsulation encoding version if there's a current write
            // encapsulation, otherwise, use the stream encoding version.
            //

            if (_encapsStack != null)
            {
                StartEncapsulation(_encapsStack.encoding, _encapsStack.format);
            }
            else
            {
                StartEncapsulation(_encoding, FormatType.DefaultFormat);
            }
        }

        /// <summary>
        /// Writes the start of an encapsulation to the stream.
        /// </summary>
        /// <param name="encoding">The encoding version of the encapsulation.</param>
        /// <param name="format">Specify the compact or sliced format.</param>
        public void StartEncapsulation(EncodingVersion encoding, FormatType format)
        {
            Protocol.checkSupportedEncoding(encoding);

            Encaps curr = _encapsCache;
            if (curr != null)
            {
                curr.reset();
                _encapsCache = _encapsCache.next;
            }
            else
            {
                curr = new Encaps();
            }
            curr.next = _encapsStack;
            _encapsStack = curr;

            _encapsStack.format = format;
            _encapsStack.setEncoding(encoding);
            _encapsStack.start = _buf.b.position();

            WriteInt(0); // Placeholder for the encapsulation length.
            WriteByte(_encapsStack.encoding.major);
            WriteByte(_encapsStack.encoding.minor);
        }

        /// <summary>
        /// Ends the previous encapsulation.
        /// </summary>
        public void EndEncapsulation()
        {
            Debug.Assert(_encapsStack != null);

            // Size includes size and version.
            int start = _encapsStack.start;
            int sz = _buf.size() - start;
            _buf.b.putInt(start, sz);

            Encaps curr = _encapsStack;
            _encapsStack = curr.next;
            curr.next = _encapsCache;
            _encapsCache = curr;
            _encapsCache.reset();
        }

        /// <summary>
        /// Writes an empty encapsulation using the given encoding version.
        /// </summary>
        /// <param name="encoding">The encoding version of the encapsulation.</param>
        public void WriteEmptyEncapsulation(EncodingVersion encoding)
        {
            Protocol.checkSupportedEncoding(encoding);
            WriteInt(6); // Size
            WriteByte(encoding.major);
            WriteByte(encoding.minor);
        }

        /// <summary>
        /// Writes a pre-encoded encapsulation.
        /// </summary>
        /// <param name="v">The encapsulation data.</param>
        public void WriteEncapsulation(byte[] v)
        {
            if (v.Length < 6)
            {
                throw new EncapsulationException();
            }
            expand(v.Length);
            _buf.b.put(v);
        }

        /// <summary>
        /// Determines the current encoding version.
        /// </summary>
        /// <returns>The encoding version.</returns>
        public EncodingVersion GetEncoding()
        {
            return _encapsStack != null ? _encapsStack.encoding : _encoding;
        }

        /// <summary>
        /// Marks the start of a new slice for a class instance or user exception.
        /// </summary>
        /// <param name="typeId">The Slice type ID corresponding to this slice.</param>
        /// <param name="compactId">The Slice compact type ID corresponding to this slice or -1 if no compact ID
        /// is defined for the type ID.</param>
        /// <param name="last">True if this is the last slice, false otherwise.</param>
        public void StartSlice(string typeId, int compactId, bool last)
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.StartSlice(typeId, compactId, last);
        }

        /// <summary>
        /// Marks the end of a slice for a class instance or user exception.
        /// </summary>
        public void EndSlice()
        {
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.endSlice();
        }

        /// <summary>
        /// Writes a size to the stream.
        /// </summary>
        /// <param name="v">The size to write.</param>
        public void WriteSize(int v)
        {
            if (v > 254)
            {
                expand(5);
                _buf.b.put(255);
                _buf.b.putInt(v);
            }
            else
            {
                expand(1);
                _buf.b.put((byte)v);
            }
        }

        /// <summary>
        /// Returns the current position and allocates four bytes for a fixed-length (32-bit) size value.
        /// </summary>
        public int StartSize()
        {
            int pos = _buf.b.position();
            WriteInt(0); // Placeholder for 32-bit size
            return pos;
        }

        /// <summary>
        /// Computes the amount of data written since the previous call to startSize and writes that value
        /// at the saved position.
        /// </summary>
        /// <param name="pos">The saved position.</param>
        public void EndSize(int pos)
        {
            Debug.Assert(pos >= 0);
            RewriteInt(_buf.b.position() - pos - 4, pos);
        }

        /// <summary>
        /// Writes a blob of bytes to the stream.
        /// </summary>
        /// <param name="v">The byte array to be written. All of the bytes in the array are written.</param>
        public void WriteBlob(byte[] v)
        {
            if (v == null)
            {
                return;
            }
            expand(v.Length);
            _buf.b.put(v);
        }

        /// <summary>
        /// Writes a blob of bytes to the stream.
        /// </summary>
        /// <param name="v">The byte array to be written. All of the bytes in the array are written.</param>
        /// <param name="off">The offset into the byte array from which to copy.</param>
        /// <param name="len">The number of bytes from the byte array to copy.</param>
        public void WriteBlob(byte[] v, int off, int len)
        {
            if (v == null)
            {
                return;
            }
            expand(len);
            _buf.b.put(v, off, len);
        }

        /// <summary>
        /// Write the header information for an optional value.
        /// </summary>
        /// <param name="tag">The numeric tag associated with the value.</param>
        /// <param name="format">The optional format of the value.</param>
        public bool WriteOptional(int tag, OptionalFormat format)
        {
            Debug.Assert(_encapsStack != null);
            if (_encapsStack.encoder != null)
            {
                return _encapsStack.encoder.writeOptional(tag, format);
            }
            else
            {
                return writeOptionalImpl(tag, format);
            }
        }

        /// <summary>
        /// Writes a byte to the stream.
        /// </summary>
        /// <param name="v">The byte to write to the stream.</param>
        public void WriteByte(byte v)
        {
            expand(1);
            _buf.b.put(v);
        }

        /// <summary>
        /// Writes an optional byte to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The byte to write to the stream.</param>
        public void WriteByte(int tag, byte? v)
        {
            if (v is byte value && WriteOptional(tag, OptionalFormat.F1))
            {
                WriteByte(value);
            }
        }

        /// <summary>
        /// Writes a byte to the stream at the given position. The current position of the stream is not modified.
        /// </summary>
        /// <param name="v">The byte to write to the stream.</param>
        /// <param name="dest">The position at which to store the byte in the buffer.</param>
        public void RewriteByte(byte v, int dest)
        {
            _buf.b.put(dest, v);
        }

        /// <summary>
        /// Writes a byte sequence to the stream.
        /// </summary>
        /// <param name="v">The byte sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteByteSeq(byte[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length);
                _buf.b.put(v);
            }
        }

        /// <summary>
        /// Writes a byte sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteByteSeq(int count, IEnumerable<byte> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<byte>? value = v as List<byte>;
                if (value != null)
                {
                    WriteByteSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<byte>? value = v as LinkedList<byte>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count);
                    IEnumerator<byte> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.put(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<byte>? value = v as Queue<byte>;
                if (value != null)
                {
                    WriteByteSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<byte>? value = v as Stack<byte>;
                if (value != null)
                {
                    WriteByteSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count);
            foreach (byte b in v)
            {
                _buf.b.put(b);
            }
        }

        /// <summary>
        /// Writes an optional byte sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The byte sequence to write to the stream.</param>
        public void WriteByteSeq(int tag, byte[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteByteSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional byte sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the byte sequence.</param>
        public void WriteByteSeq(int tag, int count, IEnumerable<byte> v)
        {
            if (WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteByteSeq(count, v);
            }
        }

        /// <summary>
        /// Writes a serializable object to the stream.
        /// </summary>
        /// <param name="o">The serializable object to write.</param>
        public void WriteSerializable(object o)
        {
            if (o == null)
            {
                WriteSize(0);
                return;
            }
            try
            {
                IceInternal.OutputStreamWrapper w = new IceInternal.OutputStreamWrapper(this);
                IFormatter f = new BinaryFormatter();
                f.Serialize(w, o);
                w.Close();
            }
            catch (System.Exception ex)
            {
                throw new MarshalException("cannot serialize object:", ex);
            }
        }

        /// <summary>
        /// Writes a boolean to the stream.
        /// </summary>
        /// <param name="v">The boolean to write to the stream.</param>
        public void WriteBool(bool v)
        {
            expand(1);
            _buf.b.put(v ? (byte)1 : (byte)0);
        }

        /// <summary>
        /// Writes an optional boolean to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The boolean to write to the stream.</param>
        public void WriteBool(int tag, bool? v)
        {
            if (v is bool value && WriteOptional(tag, OptionalFormat.F1))
            {
                WriteBool(value);
            }
        }

        /// <summary>
        /// Writes a boolean to the stream at the given position. The current position of the stream is not modified.
        /// </summary>
        /// <param name="v">The boolean to write to the stream.</param>
        /// <param name="dest">The position at which to store the boolean in the buffer.</param>
        public void RewriteBool(bool v, int dest)
        {
            _buf.b.put(dest, v ? (byte)1 : (byte)0);
        }

        /// <summary>
        /// Writes a boolean sequence to the stream.
        /// </summary>
        /// <param name="v">The boolean sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteBoolSeq(bool[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length);
                _buf.b.putBoolSeq(v);
            }
        }

        /// <summary>
        /// Writes a boolean sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteBoolSeq(int count, IEnumerable<bool> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<bool>? value = v as List<bool>;
                if (value != null)
                {
                    WriteBoolSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<bool>? value = v as LinkedList<bool>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count);
                    IEnumerator<bool> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putBool(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<bool>? value = v as Queue<bool>;
                if (value != null)
                {
                    WriteBoolSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<bool>? value = v as Stack<bool>;
                if (value != null)
                {
                    WriteBoolSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count);
            foreach (bool b in v)
            {
                _buf.b.putBool(b);
            }
        }

        /// <summary>
        /// Writes an optional boolean sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The optional boolean sequence to write to the stream.</param>
        public void WriteBoolSeq(int tag, bool[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteBoolSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional boolean sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the optional boolean sequence.</param>
        public void WriteBoolSeq<T>(int tag, int count, T? v) where T : class, IEnumerable<bool>
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteBoolSeq(count, v);
            }
        }

        /// <summary>
        /// Writes a short to the stream.
        /// </summary>
        /// <param name="v">The short to write to the stream.</param>
        public void WriteShort(short v)
        {
            expand(2);
            _buf.b.putShort(v);
        }

        /// <summary>
        /// Writes an optional short to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The short to write to the stream.</param>
        public void WriteShort(int tag, short? v)
        {
            if (v is short value && WriteOptional(tag, OptionalFormat.F2))
            {
                WriteShort(value);
            }
        }

        /// <summary>
        /// Writes a short sequence to the stream.
        /// </summary>
        /// <param name="v">The short sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteShortSeq(short[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length * 2);
                _buf.b.putShortSeq(v);
            }
        }

        /// <summary>
        /// Writes a short sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteShortSeq(int count, IEnumerable<short> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<short>? value = v as List<short>;
                if (value != null)
                {
                    WriteShortSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<short>? value = v as LinkedList<short>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count * 2);
                    IEnumerator<short> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putShort(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<short>? value = v as Queue<short>;
                if (value != null)
                {
                    WriteShortSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<short>? value = v as Stack<short>;
                if (value != null)
                {
                    WriteShortSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count * 2);
            foreach (short s in v)
            {
                _buf.b.putShort(s);
            }
        }

        /// <summary>
        /// Writes an optional short sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The short sequence to write to the stream.</param>
        public void WriteShortSeq(int tag, short[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(v.Length == 0 ? 1 : v.Length * 2 + (v.Length > 254 ? 5 : 1));
                WriteShortSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional short sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the short sequence.</param>
        public void WriteShortSeq(int tag, int count, IEnumerable<short>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(count == 0 ? 1 : count * 2 + (count > 254 ? 5 : 1));
                WriteShortSeq(count, v);
            }
        }

        /// <summary>
        /// Writes an int to the stream.
        /// </summary>
        /// <param name="v">The int to write to the stream.</param>
        public void WriteInt(int v)
        {
            expand(4);
            _buf.b.putInt(v);
        }

        /// <summary>
        /// Writes an optional int to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The int to write to the stream.</param>
        public void WriteInt(int tag, int? v)
        {
            if (v is int value && WriteOptional(tag, OptionalFormat.F4))
            {
                WriteInt(value);
            }
        }

        /// <summary>
        /// Writes an int to the stream at the given position. The current position of the stream is not modified.
        /// </summary>
        /// <param name="v">The int to write to the stream.</param>
        /// <param name="dest">The position at which to store the int in the buffer.</param>
        public void RewriteInt(int v, int dest)
        {
            _buf.b.putInt(dest, v);
        }

        /// <summary>
        /// Writes an int sequence to the stream.
        /// </summary>
        /// <param name="v">The int sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteIntSeq(int[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length * 4);
                _buf.b.putIntSeq(v);
            }
        }

        /// <summary>
        /// Writes an int sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteIntSeq(int count, IEnumerable<int> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<int>? value = v as List<int>;
                if (value != null)
                {
                    WriteIntSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<int>? value = v as LinkedList<int>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count * 4);
                    IEnumerator<int> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putInt(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<int>? value = v as Queue<int>;
                if (value != null)
                {
                    WriteIntSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<int>? value = v as Stack<int>;
                if (value != null)
                {
                    WriteIntSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count * 4);
            foreach (int i in v)
            {
                _buf.b.putInt(i);
            }
        }

        /// <summary>
        /// Writes an optional int sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The int sequence to write to the stream.</param>
        public void WriteIntSeq(int tag, int[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(v.Length == 0 ? 1 : v.Length * 4 + (v.Length > 254 ? 5 : 1));
                WriteIntSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional int sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the int sequence.</param>
        public void WriteIntSeq(int tag, int count, IEnumerable<int>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(count == 0 ? 1 : count * 4 + (count > 254 ? 5 : 1));
                WriteIntSeq(count, v);
            }
        }

        /// <summary>
        /// Writes a long to the stream.
        /// </summary>
        /// <param name="v">The long to write to the stream.</param>
        public void WriteLong(long v)
        {
            expand(8);
            _buf.b.putLong(v);
        }

        /// <summary>
        /// Writes an optional long to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The long to write to the stream.</param>
        public void WriteLong(int tag, long? v)
        {
            if (v is long value && WriteOptional(tag, OptionalFormat.F8))
            {
                WriteLong(value);
            }
        }

        /// <summary>
        /// Writes a long sequence to the stream.
        /// </summary>
        /// <param name="v">The long sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteLongSeq(long[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length * 8);
                _buf.b.putLongSeq(v);
            }
        }

        /// <summary>
        /// Writes a long sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteLongSeq(int count, IEnumerable<long> v)
        {
            if (count == 0 || v == null)
            {
                WriteSize(0);
                return;
            }

            {
                List<long>? value = v as List<long>;
                if (value != null)
                {
                    WriteLongSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<long>? value = v as LinkedList<long>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count * 8);
                    IEnumerator<long> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putLong(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<long>? value = v as Queue<long>;
                if (value != null)
                {
                    WriteLongSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<long>? value = v as Stack<long>;
                if (value != null)
                {
                    WriteLongSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count * 8);
            foreach (long l in v)
            {
                _buf.b.putLong(l);
            }
        }

        /// <summary>
        /// Writes an optional long sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The long sequence to write to the stream.</param>
        public void WriteLongSeq(int tag, long[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(v.Length == 0 ? 1 : v.Length * 8 + (v.Length > 254 ? 5 : 1));
                WriteLongSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional long sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the long sequence.</param>
        public void WriteLongSeq(int tag, int count, IEnumerable<long>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(count == 0 ? 1 : count * 8 + (count > 254 ? 5 : 1));
                WriteLongSeq(count, v);
            }
        }

        /// <summary>
        /// Writes a float to the stream.
        /// </summary>
        /// <param name="v">The float to write to the stream.</param>
        public void WriteFloat(float v)
        {
            expand(4);
            _buf.b.putFloat(v);
        }

        /// <summary>
        /// Writes an optional float to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The float to write to the stream.</param>
        public void WriteFloat(int tag, float? v)
        {
            if (v is float value && WriteOptional(tag, OptionalFormat.F4))
            {
                WriteFloat(value);
            }
        }

        /// <summary>
        /// Writes a float sequence to the stream.
        /// </summary>
        /// <param name="v">The float sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteFloatSeq(float[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length * 4);
                _buf.b.putFloatSeq(v);
            }
        }

        /// <summary>
        /// Writes a float sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteFloatSeq(int count, IEnumerable<float> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<float>? value = v as List<float>;
                if (value != null)
                {
                    WriteFloatSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<float>? value = v as LinkedList<float>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count * 4);
                    IEnumerator<float> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putFloat(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<float>? value = v as Queue<float>;
                if (value != null)
                {
                    WriteFloatSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<float>? value = v as Stack<float>;
                if (value != null)
                {
                    WriteFloatSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count * 4);
            foreach (float f in v)
            {
                _buf.b.putFloat(f);
            }
        }

        /// <summary>
        /// Writes an optional float sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The float sequence to write to the stream.</param>
        public void WriteFloatSeq(int tag, float[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(v.Length == 0 ? 1 : v.Length * 4 + (v.Length > 254 ? 5 : 1));
                WriteFloatSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional float sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the float sequence.</param>
        public void WriteFloatSeq(int tag, int count, IEnumerable<float>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(count == 0 ? 1 : count * 4 + (count > 254 ? 5 : 1));
                WriteFloatSeq(count, v);
            }
        }

        /// <summary>
        /// Writes a double to the stream.
        /// </summary>
        /// <param name="v">The double to write to the stream.</param>
        public void WriteDouble(double v)
        {
            expand(8);
            _buf.b.putDouble(v);
        }

        /// <summary>
        /// Writes an optional double to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The double to write to the stream.</param>
        public void WriteDouble(int tag, double? v)
        {
            if (v is double value && WriteOptional(tag, OptionalFormat.F8))
            {
                WriteDouble(value);
            }
        }

        /// <summary>
        /// Writes a double sequence to the stream.
        /// </summary>
        /// <param name="v">The double sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteDoubleSeq(double[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                expand(v.Length * 8);
                _buf.b.putDoubleSeq(v);
            }
        }

        /// <summary>
        /// Writes a double sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteDoubleSeq(int count, IEnumerable<double> v)
        {
            if (count == 0)
            {
                WriteSize(0);
                return;
            }

            {
                List<double>? value = v as List<double>;
                if (value != null)
                {
                    WriteDoubleSeq(value.ToArray());
                    return;
                }
            }

            {
                LinkedList<double>? value = v as LinkedList<double>;
                if (value != null)
                {
                    WriteSize(count);
                    expand(count * 8);
                    IEnumerator<double> i = v.GetEnumerator();
                    while (i.MoveNext())
                    {
                        _buf.b.putDouble(i.Current);
                    }
                    return;
                }
            }

            {
                Queue<double>? value = v as Queue<double>;
                if (value != null)
                {
                    WriteDoubleSeq(value.ToArray());
                    return;
                }
            }

            {
                Stack<double>? value = v as Stack<double>;
                if (value != null)
                {
                    WriteDoubleSeq(value.ToArray());
                    return;
                }
            }

            WriteSize(count);
            expand(count * 8);
            foreach (double d in v)
            {
                _buf.b.putDouble(d);
            }
        }

        /// <summary>
        /// Writes an optional double sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The double sequence to write to the stream.</param>
        public void WriteDoubleSeq(int tag, double[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(v.Length == 0 ? 1 : v.Length * 8 + (v.Length > 254 ? 5 : 1));
                WriteDoubleSeq(v);
            }
        }

        /// <summary>
        /// Writes an optional double sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the double sequence.</param>
        public void WriteDoubleSeq(int tag, int count, IEnumerable<double>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteSize(count == 0 ? 1 : count * 8 + (count > 254 ? 5 : 1));
                WriteDoubleSeq(count, v);
            }
        }

        private static System.Text.UTF8Encoding utf8 = new System.Text.UTF8Encoding(false, true);

        /// <summary>
        /// Writes a string to the stream.
        /// </summary>
        /// <param name="v">The string to write to the stream. Passing null causes
        /// an empty string to be written to the stream.</param>
        public void WriteString(string? v)
        {
            if (v == null || v.Length == 0)
            {
                WriteSize(0);
                return;
            }
            byte[] arr = utf8.GetBytes(v);
            WriteSize(arr.Length);
            expand(arr.Length);
            _buf.b.put(arr);
        }

        /// <summary>
        /// Writes an optional string to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The string to write to the stream.</param>
        public void WriteString(int tag, string? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.VSize))
            {
                WriteString(v);
            }
        }

        /// <summary>
        /// Writes a string sequence to the stream.
        /// </summary>
        /// <param name="v">The string sequence to write to the stream.
        /// Passing null causes an empty sequence to be written to the stream.</param>
        public void WriteStringSeq(string[]? v)
        {
            if (v == null)
            {
                WriteSize(0);
            }
            else
            {
                WriteSize(v.Length);
                for (int i = 0; i < v.Length; i++)
                {
                    WriteString(v[i]);
                }
            }
        }

        /// <summary>
        /// Writes a string sequence to the stream.
        /// </summary>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the container holding the sequence.</param>
        public void WriteStringSeq(int count, IEnumerable<string> v)
        {
            WriteSize(count);
            foreach (string s in v)
            {
                WriteString(s);
            }
        }

        /// <summary>
        /// Writes an optional string sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The string sequence to write to the stream.</param>
        public void WriteStringSeq(int tag, string[]? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.FSize))
            {
                int pos = StartSize();
                WriteStringSeq(v);
                EndSize(pos);
            }
        }

        /// <summary>
        /// Writes an optional string sequence to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="count">The number of elements in the sequence.</param>
        /// <param name="v">An enumerator for the string sequence.</param>
        public void WriteStringSeq(int tag, int count, IEnumerable<string>? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.FSize))
            {
                int pos = StartSize();
                WriteStringSeq(count, v);
                EndSize(pos);
            }
        }

        /// <summary>
        /// Writes a proxy to the stream.
        /// </summary>
        /// <param name="v">The proxy to write.</param>
        public void WriteProxy(IObjectPrx? v)
        {
            if (v != null)
            {
                v.IceWrite(this);
            }
            else
            {
                Identity ident = new Identity();
                ident.ice_writeMembers(this);
            }
        }

        /// <summary>
        /// Writes an optional proxy to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The proxy to write.</param>
        public void WriteProxy(int tag, IObjectPrx? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.FSize))
            {
                int pos = StartSize();
                WriteProxy(v);
                EndSize(pos);
            }
        }

        /// <summary>
        /// Writes an enumerated value.
        /// </summary>
        /// <param name="v">The enumerator.</param>
        /// <param name="maxValue">The maximum enumerator value in the definition.</param>
        public void WriteEnum(int v, int maxValue)
        {
            if (isEncoding_1_0())
            {
                if (maxValue < 127)
                {
                    WriteByte((byte)v);
                }
                else if (maxValue < 32767)
                {
                    WriteShort((short)v);
                }
                else
                {
                    WriteInt(v);
                }
            }
            else
            {
                WriteSize(v);
            }
        }

        /// <summary>
        /// Writes an optional enumerator to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The enumerator.</param>
        /// <param name="maxValue">The maximum enumerator value in the definition.</param>
        public void WriteEnum(int tag, int? v, int maxValue)
        {
            if (v is int value && WriteOptional(tag, OptionalFormat.Size))
            {
                WriteEnum(value, maxValue);
            }
        }

        /// <summary>
        /// Writes a class instance to the stream.
        /// </summary>
        /// <param name="v">The value to write.</param>
        public void WriteClass(AnyClass? v)
        {
            initEncaps();
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.WriteClass(v);
        }

        /// <summary>
        /// Writes an optional class instance to the stream.
        /// </summary>
        /// <param name="tag">The optional tag.</param>
        /// <param name="v">The value to write.</param>
        public void WriteClass(int tag, AnyClass? v)
        {
            if (v != null && WriteOptional(tag, OptionalFormat.Class))
            {
                WriteClass(v);
            }
        }

        /// <summary>
        /// Writes a user exception to the stream.
        /// </summary>
        /// <param name="v">The user exception to write.</param>
        public void WriteException(UserException v)
        {
            initEncaps();
            Debug.Assert(_encapsStack != null && _encapsStack.encoder != null);
            _encapsStack.encoder.WriteException(v);
        }

        private bool writeOptionalImpl(int tag, OptionalFormat format)
        {
            if (isEncoding_1_0())
            {
                return false; // Optional members aren't supported with the 1.0 encoding.
            }

            int v = (int)format;
            if (tag < 30)
            {
                v |= tag << 3;
                WriteByte((byte)v);
            }
            else
            {
                v |= 0x0F0; // tag = 30
                WriteByte((byte)v);
                WriteSize(tag);
            }
            return true;
        }

        /// <summary>
        /// Determines the current position in the stream.
        /// </summary>
        /// <returns>The current position.</returns>
        public int pos()
        {
            return _buf.b.position();
        }

        /// <summary>
        /// Sets the current position in the stream.
        /// </summary>
        /// <param name="n">The new position.</param>
        public void pos(int n)
        {
            _buf.b.position(n);
        }

        /// <summary>
        /// Determines the current size of the stream.
        /// </summary>
        /// <returns>The current size.</returns>
        public int size()
        {
            return _buf.size();
        }

        /// <summary>
        /// Determines whether the stream is empty.
        /// </summary>
        /// <returns>True if no data has been written yet, false otherwise.</returns>
        public bool isEmpty()
        {
            return _buf.empty();
        }

        /// <summary>
        /// Expand the stream to accept more data.
        /// </summary>
        /// <param name="n">The number of bytes to accommodate in the stream.</param>
        public void expand(int n)
        {
            _buf.expand(n);
        }

        private Ice.Communicator _communicator;
        private IceInternal.Buffer _buf;
        private object? _closure;
        private FormatType _format;

        private enum SliceType { NoSlice, ClassSlice, ExceptionSlice }

        private abstract class EncapsEncoder
        {
            protected EncapsEncoder(OutputStream stream, Encaps encaps)
            {
                _stream = stream;
                _encaps = encaps;
                _typeIdIndex = 0;
                _marshaledMap = new Dictionary<AnyClass, int>();
            }

            internal abstract void WriteClass(AnyClass? v);
            internal abstract void WriteException(UserException v);

            internal abstract void StartInstance(SliceType type, SlicedData? data);
            internal abstract void EndInstance();
            internal abstract void StartSlice(string typeId, int compactId, bool last);
            internal abstract void endSlice();

            internal virtual bool writeOptional(int tag, OptionalFormat format)
            {
                return false;
            }

            protected int registerTypeId(string typeId)
            {
                if (_typeIdMap == null)
                {
                    _typeIdMap = new Dictionary<string, int>();
                }

                int p;
                if (_typeIdMap.TryGetValue(typeId, out p))
                {
                    return p;
                }
                else
                {
                    _typeIdMap.Add(typeId, ++_typeIdIndex);
                    return -1;
                }
            }

            protected readonly OutputStream _stream;
            protected readonly Encaps _encaps;

            // Encapsulation attributes for instance marshaling.
            protected readonly Dictionary<AnyClass, int> _marshaledMap;

            // Encapsulation attributes for instance marshaling.
            private Dictionary<string, int>? _typeIdMap;
            private int _typeIdIndex;
        }

        private sealed class EncapsEncoder11 : EncapsEncoder
        {
            internal EncapsEncoder11(OutputStream stream, Encaps encaps) : base(stream, encaps)
            {
                _current = null;
                _valueIdIndex = 1;
            }

            internal override void WriteClass(AnyClass? v)
            {
                if (v == null)
                {
                    _stream.WriteSize(0);
                }
                else if (_current != null && _encaps.format == FormatType.SlicedFormat)
                {
                    if (_current.indirectionTable == null)
                    {
                        _current.indirectionTable = new List<AnyClass>();
                        _current.indirectionMap = new Dictionary<AnyClass, int>();
                    }

                    Debug.Assert(_current.indirectionMap != null);
                    //
                    // If writing an instance within a slice and using the sliced
                    // format, write an index from the instance indirection table.
                    //
                    int index;
                    if (!_current.indirectionMap.TryGetValue(v, out index))
                    {
                        _current.indirectionTable.Add(v);
                        int idx = _current.indirectionTable.Count; // Position + 1 (0 is reserved for nil)
                        _current.indirectionMap.Add(v, idx);
                        _stream.WriteSize(idx);
                    }
                    else
                    {
                        _stream.WriteSize(index);
                    }
                }
                else
                {
                    writeInstance(v); // Write the instance or a reference if already marshaled.
                }
            }

            internal override void WriteException(UserException v)
            {
                v.iceWrite(_stream);
            }

            internal override void StartInstance(SliceType sliceType, SlicedData? data)
            {
                if (_current == null)
                {
                    _current = new InstanceData(null);
                }
                else
                {
                    _current = _current.next == null ? new InstanceData(_current) : _current.next;
                }
                _current.sliceType = sliceType;
                _current.firstSlice = true;

                if (data != null)
                {
                    writeSlicedData(data);
                }
            }

            internal override void EndInstance()
            {
                Debug.Assert(_current != null);
                _current = _current.previous;
            }

            internal override void StartSlice(string typeId, int compactId, bool last)
            {
                Debug.Assert(_current != null);
                Debug.Assert(_current.indirectionTable == null || _current.indirectionTable.Count == 0);
                Debug.Assert(_current.indirectionMap == null || _current.indirectionMap.Count == 0);

                _current.sliceFlagsPos = _stream.pos();

                _current.sliceFlags = 0;
                if (_encaps.format == FormatType.SlicedFormat)
                {
                    //
                    // Encode the slice size if using the sliced format.
                    //
                    _current.sliceFlags |= Protocol.FLAG_HAS_SLICE_SIZE;
                }
                if (last)
                {
                    _current.sliceFlags |= Protocol.FLAG_IS_LAST_SLICE; // This is the last slice.
                }

                _stream.WriteByte(0); // Placeholder for the slice flags

                //
                // For instance slices, encode the flag and the type ID either as a
                // string or index. For exception slices, always encode the type
                // ID a string.
                //
                if (_current.sliceType == SliceType.ClassSlice)
                {
                    //
                    // Encode the type ID (only in the first slice for the compact
                    // encoding).
                    //
                    if (_encaps.format == FormatType.SlicedFormat || _current.firstSlice)
                    {
                        if (compactId >= 0)
                        {
                            _current.sliceFlags |= Protocol.FLAG_HAS_TYPE_ID_COMPACT;
                            _stream.WriteSize(compactId);
                        }
                        else
                        {
                            int index = registerTypeId(typeId);
                            if (index < 0)
                            {
                                _current.sliceFlags |= Protocol.FLAG_HAS_TYPE_ID_STRING;
                                _stream.WriteString(typeId);
                            }
                            else
                            {
                                _current.sliceFlags |= Protocol.FLAG_HAS_TYPE_ID_INDEX;
                                _stream.WriteSize(index);
                            }
                        }
                    }
                }
                else
                {
                    _stream.WriteString(typeId);
                }

                if ((_current.sliceFlags & Protocol.FLAG_HAS_SLICE_SIZE) != 0)
                {
                    _stream.WriteInt(0); // Placeholder for the slice length.
                }

                _current.writeSlice = _stream.pos();
                _current.firstSlice = false;
            }

            internal override void endSlice()
            {
                Debug.Assert(_current != null);
                //
                // Write the optional member end marker if some optional members
                // were encoded. Note that the optional members are encoded before
                // the indirection table and are included in the slice size.
                //
                if ((_current.sliceFlags & Protocol.FLAG_HAS_OPTIONAL_MEMBERS) != 0)
                {
                    _stream.WriteByte(Protocol.OPTIONAL_END_MARKER);
                }

                //
                // Write the slice length if necessary.
                //
                if ((_current.sliceFlags & Protocol.FLAG_HAS_SLICE_SIZE) != 0)
                {
                    int sz = _stream.pos() - _current.writeSlice + 4;
                    _stream.RewriteInt(sz, _current.writeSlice - 4);
                }

                //
                // Only write the indirection table if it contains entries.
                //
                if (_current.indirectionTable != null && _current.indirectionTable.Count > 0)
                {
                    Debug.Assert(_encaps.format == FormatType.SlicedFormat);
                    _current.sliceFlags |= Protocol.FLAG_HAS_INDIRECTION_TABLE;

                    //
                    // Write the indirect instance table.
                    //
                    _stream.WriteSize(_current.indirectionTable.Count);
                    foreach (var v in _current.indirectionTable)
                    {
                        writeInstance(v);
                    }
                    _current.indirectionTable.Clear();
                    Debug.Assert(_current.indirectionMap != null);
                    _current.indirectionMap.Clear();
                }

                //
                // Finally, update the slice flags.
                //
                _stream.RewriteByte(_current.sliceFlags, _current.sliceFlagsPos);
            }

            internal override bool writeOptional(int tag, OptionalFormat format)
            {
                if (_current == null)
                {
                    return _stream.writeOptionalImpl(tag, format);
                }
                else
                {
                    if (_stream.writeOptionalImpl(tag, format))
                    {
                        _current.sliceFlags |= Protocol.FLAG_HAS_OPTIONAL_MEMBERS;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            private void writeSlicedData(SlicedData? slicedData)
            {
                Debug.Assert(slicedData != null);
                Debug.Assert(_current != null);

                //
                // We only remarshal preserved slices if we are using the sliced
                // format. Otherwise, we ignore the preserved slices, which
                // essentially "slices" the instance into the most-derived type
                // known by the sender.
                //
                if (_encaps.format != FormatType.SlicedFormat)
                {
                    return;
                }

                foreach (var info in slicedData.Value.Slices)
                {
                    StartSlice(info.TypeId ?? "", info.CompactId ?? -1, info.IsLastSlice);

                    //
                    // Write the bytes associated with this slice.
                    //
                    // Temporary:
                    var sliceBytes = new byte[info.Bytes.Count];
                    info.Bytes.CopyTo(sliceBytes, 0);
                    _stream.WriteBlob(sliceBytes);

                    if (info.HasOptionalMembers)
                    {
                        _current.sliceFlags |= Protocol.FLAG_HAS_OPTIONAL_MEMBERS;
                    }

                    //
                    // Make sure to also re-write the instance indirection table.
                    //
                    if (info.Instances.Count > 0)
                    {
                        if (_current.indirectionTable == null)
                        {
                            _current.indirectionTable = new List<AnyClass>();
                            _current.indirectionMap = new Dictionary<AnyClass, int>();
                        }
                        foreach (var o in info.Instances)
                        {
                            _current.indirectionTable.Add(o);
                        }
                    }

                    endSlice();
                }
            }

            private void writeInstance(AnyClass v)
            {
                Debug.Assert(v != null);

                //
                // If the instance was already marshaled, just write it's ID.
                //
                int p;
                if (_marshaledMap.TryGetValue(v, out p))
                {
                    _stream.WriteSize(p);
                    return;
                }

                //
                // We haven't seen this instance previously, create a new ID,
                // insert it into the marshaled map, and write the instance.
                //
                _marshaledMap.Add(v, ++_valueIdIndex);

                _stream.WriteSize(1); // IObject instance marker.
                v.iceWrite(_stream);
            }

            private sealed class InstanceData
            {
                internal InstanceData(InstanceData? previous)
                {
                    if (previous != null)
                    {
                        previous.next = this;
                    }
                    this.previous = previous;
                    next = null;
                }

                // Instance attributes
                internal SliceType sliceType;
                internal bool firstSlice;

                // Slice attributes
                internal byte sliceFlags;
                internal int writeSlice;    // Position of the slice data members
                internal int sliceFlagsPos; // Position of the slice flags
                internal List<AnyClass>? indirectionTable;
                internal Dictionary<AnyClass, int>? indirectionMap;

                internal InstanceData? previous;
                internal InstanceData? next;
            }

            private InstanceData? _current;

            private int _valueIdIndex; // The ID of the next instance to marhsal
        }

        private sealed class Encaps
        {
            internal void reset()
            {
                encoder = null;
            }

            internal void setEncoding(EncodingVersion encoding)
            {
                this.encoding = encoding;
                encoding_1_0 = encoding.Equals(Util.Encoding_1_0);
            }

            internal int start;
            internal EncodingVersion encoding;
            internal bool encoding_1_0;
            internal FormatType format = FormatType.DefaultFormat;

            internal EncapsEncoder? encoder;

            internal Encaps? next;
        }

        //
        // The encoding version to use when there's no encapsulation to
        // read from or write to. This is for example used to read message
        // headers or when the user is using the streaming API with no
        // encapsulation.
        //
        private EncodingVersion _encoding;

        private bool isEncoding_1_0()
        {
            return _encapsStack != null ? _encapsStack.encoding_1_0 : _encoding.Equals(Util.Encoding_1_0);
        }

        private Encaps? _encapsStack;
        private Encaps? _encapsCache;

        private void initEncaps()
        {
            if (_encapsStack == null) // Lazy initialization
            {
                _encapsStack = _encapsCache;
                if (_encapsStack != null)
                {
                    _encapsCache = _encapsCache!.next;
                }
                else
                {
                    _encapsStack = new Encaps();
                }
                _encapsStack.setEncoding(_encoding);
            }

            if (_encapsStack.format == FormatType.DefaultFormat)
            {
                _encapsStack.format = _communicator.DefaultsAndOverrides.defaultFormat;
            }

            if (_encapsStack.encoder == null) // Lazy initialization.
            {
                if (_encapsStack.encoding_1_0)
                {
                    // TODO: temporary until larger refactoring
                    Debug.Assert(false);
                }
                else
                {
                    _encapsStack.encoder = new EncapsEncoder11(this, _encapsStack);
                }
            }
        }
    }

    /// <summary>
    /// Base class for writing class instances to an output stream.
    /// </summary>
    public abstract class ClassWriter : AnyClass
    {
        /// <summary>
        /// Writes the state of this Slice class instance to an output stream.
        /// </summary>
        /// <param name="outStream">The stream to write to.</param>
        public abstract void write(OutputStream outStream);

        public override void iceWrite(OutputStream os)
        {
            write(os);
        }

        protected override void IceRead(InputStream istr, bool firstSlice)
        {
            Debug.Assert(false);
        }
    }
}
