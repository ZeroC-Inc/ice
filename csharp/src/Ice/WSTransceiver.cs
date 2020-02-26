//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Ice;
using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace IceInternal
{
    internal sealed class WSTransceiver : ITransceiver
    {
        public Socket? Fd() => _delegate.Fd();

        public int Initialize(Buffer readBuffer, IList<ArraySegment<byte>> writeBuffer)
        {
            //
            // Delegate logs exceptions that occur during initialize(), so there's no need to trap them here.
            //
            if (_state == StateInitializeDelegate)
            {
                int op = _delegate.Initialize(readBuffer, writeBuffer);
                if (op != 0)
                {
                    return op;
                }
                _state = StateConnected;
            }

            try
            {
                if (_state == StateConnected)
                {
                    //
                    // We don't know how much we'll need to read.
                    //
                    _readBuffer.Resize(1024, true);
                    _readBuffer.B.Position(0);
                    _readBufferPos = 0;

                    //
                    // The server waits for the client's upgrade request, the
                    // client sends the upgrade request.
                    //
                    _state = StateUpgradeRequestPending;
                    if (!_incoming)
                    {
                        //
                        // Compose the upgrade request.
                        //
                        var sb = new StringBuilder();
                        sb.Append("GET " + _resource + " HTTP/1.1\r\n");
                        sb.Append("Host: " + _host + "\r\n");
                        sb.Append("Upgrade: websocket\r\n");
                        sb.Append("Connection: Upgrade\r\n");
                        sb.Append("Sec-WebSocket-Protocol: " + IceProtocol + "\r\n");
                        sb.Append("Sec-WebSocket-Version: 13\r\n");
                        sb.Append("Sec-WebSocket-Key: ");

                        //
                        // The value for Sec-WebSocket-Key is a 16-byte random number,
                        // encoded with Base64.
                        //
                        byte[] key = new byte[16];
                        _rand.NextBytes(key);
                        _key = Convert.ToBase64String(key);
                        sb.Append(_key + "\r\n\r\n"); // EOM
                        Debug.Assert(_writeBufferSize == 0);
                        var data = _utf8.GetBytes(sb.ToString());
                        _writeBuffer.Add(data);
                        _writeBufferSize = data.Length;
                        _writeBufferOffset = 0;
                    }
                }

                //
                // Try to write the client's upgrade request.
                //
                if (_state == StateUpgradeRequestPending && !_incoming)
                {
                    if (_writeBufferOffset < _writeBufferSize)
                    {
                        int socketOperation = _delegate.Write(_writeBuffer, ref _writeBufferOffset);
                        if (socketOperation != 0)
                        {
                            return socketOperation;
                        }
                    }
                    Debug.Assert(_writeBufferOffset == _writeBufferSize);
                    _state = StateUpgradeResponsePending;
                }

                bool hasMoreData = false; // TODO is this required?
                while (true)
                {
                    if (_readBuffer.B.HasRemaining())
                    {
                        int s = _delegate.Read(_readBuffer, ref hasMoreData);
                        if (s == SocketOperation.Write || _readBuffer.B.Position() == 0)
                        {
                            return s;
                        }
                    }

                    //
                    // Try to read the client's upgrade request or the server's response.
                    //
                    if ((_state == StateUpgradeRequestPending && _incoming) ||
                       (_state == StateUpgradeResponsePending && !_incoming))
                    {
                        //
                        // Check if we have enough data for a complete message.
                        //
                        int p = _parser.IsCompleteMessage(_readBuffer.B, 0, _readBuffer.B.Position());
                        if (p == -1)
                        {
                            if (_readBuffer.B.HasRemaining())
                            {
                                return SocketOperation.Read;
                            }

                            //
                            // Enlarge the buffer and try to read more.
                            //
                            int oldSize = _readBuffer.B.Position();
                            if (oldSize + 1024 > _instance.MessageSizeMax)
                            {
                                throw new Ice.MemoryLimitException();
                            }
                            _readBuffer.Resize(oldSize + 1024, true);
                            _readBuffer.B.Position(oldSize);
                            continue; // Try again to read the response/request
                        }

                        //
                        // Set _readBufferPos at the end of the response/request message.
                        //
                        _readBufferPos = p;
                    }

                    //
                    // We're done, the client's upgrade request or server's response is read.
                    //
                    break;
                }

                try
                {
                    //
                    // Parse the client's upgrade request.
                    //
                    if (_state == StateUpgradeRequestPending && _incoming)
                    {
                        if (_parser.Parse(_readBuffer.B, 0, _readBufferPos))
                        {
                            HandleRequest();
                            _state = StateUpgradeResponsePending;
                        }
                        else
                        {
                            throw new Ice.ProtocolException("incomplete request message");
                        }
                    }

                    if (_state == StateUpgradeResponsePending)
                    {
                        if (_incoming)
                        {
                            if (_writeBufferOffset < _writeBufferSize)
                            {
                                int socketOperation = _delegate.Write(_writeBuffer, ref _writeBufferOffset);
                                if (socketOperation != 0)
                                {
                                    return socketOperation;
                                }
                            }
                        }
                        else
                        {
                            //
                            // Parse the server's response
                            //
                            if (_parser.Parse(_readBuffer.B, 0, _readBufferPos))
                            {
                                HandleResponse();
                            }
                            else
                            {
                                throw new Ice.ProtocolException("incomplete response message");
                            }
                        }
                    }
                }
                catch (WebSocketException ex)
                {
                    throw new Ice.ProtocolException(ex.Message);
                }

                _state = StateOpened;
                _nextState = StateOpened;
            }
            catch (Ice.LocalException ex)
            {
                if (_instance.TraceLevel >= 2)
                {
                    _instance.Logger.Trace(_instance.TraceCategory,
                        $"{Protocol()} connection HTTP upgrade request failed\n{this}\n{ex}");
                }
                throw;
            }

            if (_instance.TraceLevel >= 1)
            {
                if (_incoming)
                {
                    _instance.Logger.Trace(_instance.TraceCategory,
                        $"accepted {Protocol()} connection HTTP upgrade request\n{this}");
                }
                else
                {
                    _instance.Logger.Trace(_instance.TraceCategory, $"{Protocol()} connection HTTP upgrade request accepted\n{this}");
                }
            }

            return SocketOperation.None;
        }

        public int Closing(bool initiator, Ice.LocalException? reason)
        {
            if (_instance.TraceLevel >= 1)
            {
                _instance.Logger.Trace(_instance.TraceCategory, $"gracefully closing {Protocol()} connection\n{this}");
            }

            int s = _nextState == StateOpened ? _state : _nextState;

            if (s == StateClosingRequestPending && _closingInitiator)
            {
                //
                // If we initiated a close connection but also received a
                // close connection, we assume we didn't initiated the
                // connection and we send the close frame now. This is to
                // ensure that if both peers close the connection at the same
                // time we don't hang having both peer waiting for the close
                // frame of the other.
                //
                Debug.Assert(!initiator);
                _closingInitiator = false;
                return SocketOperation.Write;
            }
            else if (s >= StateClosingRequestPending)
            {
                return SocketOperation.None;
            }

            _closingInitiator = initiator;
            if (reason is Ice.CloseConnectionException)
            {
                _closingReason = CLOSURE_NORMAL;
            }
            else if (reason is Ice.ObjectAdapterDeactivatedException ||
                    reason is Ice.CommunicatorDestroyedException)
            {
                _closingReason = CLOSURE_SHUTDOWN;
            }
            else if (reason is Ice.ProtocolException)
            {
                _closingReason = CLOSURE_PROTOCOL_ERROR;
            }
            else if (reason is Ice.MemoryLimitException)
            {
                _closingReason = CLOSURE_TOO_BIG;
            }

            if (_state == StateOpened)
            {
                _state = StateClosingRequestPending;
                return initiator ? SocketOperation.Read : SocketOperation.Write;
            }
            else
            {
                _nextState = StateClosingRequestPending;
                return SocketOperation.None;
            }
        }

        public void Close()
        {
            _delegate.Close();
            _state = StateClosed;

            //
            // Clear the buffers now instead of waiting for destruction.
            //
            if (!_readPending)
            {
                _readBuffer.Clear();
            }
            if (!_writePending)
            {
                _writeBuffer.Clear();
            }
        }

        public Endpoint Bind()
        {
            Debug.Assert(false);
            return null;
        }

        public void Destroy() => _delegate.Destroy();

        public int Write(IList<ArraySegment<byte>> buffer, ref int offset)
        {
            if (_writePending)
            {
                return SocketOperation.Write;
            }

            if (_state < StateOpened)
            {
                if (_state < StateConnected)
                {
                    return _delegate.Write(buffer, ref offset);
                }
                else
                {
                    return _delegate.Write(_writeBuffer, ref _writeBufferOffset);
                }
            }

            int socketOperation = SocketOperation.None;
            int size = buffer.GetBytesCount();
            do
            {
                if (PreWrite(buffer, size, offset))
                {
                    if (_writeState == WriteStateFlush)
                    {
                        //
                        // Invoke write() even though there's nothing to write.
                        //
                        Debug.Assert(size == offset);
                        socketOperation = _delegate.Write(buffer, ref offset);
                    }

                    if (socketOperation == SocketOperation.None && _writeBufferOffset < _writeBufferSize)
                    {
                        socketOperation = _delegate.Write(_writeBuffer, ref _writeBufferOffset);
                    }
                    else if (socketOperation == SocketOperation.None && _incoming && size > 0 && _writeState == WriteStatePayload)
                    {
                        socketOperation = _delegate.Write(buffer, ref offset);
                    }
                }
            }
            while (PostWrite(size, ref offset, socketOperation));

            if (socketOperation != SocketOperation.None)
            {
                return socketOperation;
            }
            if (_state == StateClosingResponsePending && !_closingInitiator)
            {
                return SocketOperation.Read;
            }
            return SocketOperation.None;
        }

        public int Read(Buffer buf, ref bool hasMoreData)
        {
            if (_readPending)
            {
                return SocketOperation.Read;
            }

            if (_state < StateOpened)
            {
                if (_state < StateConnected)
                {
                    return _delegate.Read(buf, ref hasMoreData);
                }
                else
                {
                    if (_delegate.Read(_readBuffer, ref hasMoreData) == SocketOperation.Write)
                    {
                        return SocketOperation.Write;
                    }
                    else
                    {
                        return SocketOperation.None;
                    }
                }
            }

            if (!buf.B.HasRemaining())
            {
                hasMoreData |= _readBufferPos < _readBuffer.B.Position();
                return SocketOperation.None;
            }

            int s;
            do
            {
                if (PreRead(buf))
                {
                    if (_readState == ReadStatePayload)
                    {
                        //
                        // If the payload length is smaller than what remains to be read, we read
                        // no more than the payload length. The remaining of the buffer will be
                        // sent over in another frame.
                        //
                        int readSz = _readPayloadLength - (buf.B.Position() - _readStart);
                        if (buf.B.Remaining() > readSz)
                        {
                            int size = buf.Size();
                            buf.Resize(buf.B.Position() + readSz, true);
                            s = _delegate.Read(buf, ref hasMoreData);
                            buf.Resize(size, true);
                        }
                        else
                        {
                            s = _delegate.Read(buf, ref hasMoreData);
                        }
                    }
                    else
                    {
                        s = _delegate.Read(_readBuffer, ref hasMoreData);
                    }

                    if (s == SocketOperation.Write)
                    {
                        PostRead(buf);
                        return s;
                    }
                }
            }
            while (PostRead(buf));

            if (!buf.B.HasRemaining())
            {
                hasMoreData |= _readBufferPos < _readBuffer.B.Position();
                s = SocketOperation.None;
            }
            else
            {
                hasMoreData = false;
                s = SocketOperation.Read;
            }

            if (((_state == StateClosingRequestPending && !_closingInitiator) ||
                (_state == StateClosingResponsePending && _closingInitiator) ||
                _state == StatePingPending ||
                _state == StatePongPending) &&
               _writeState == WriteStateHeader)
            {
                // We have things to write, ask to be notified when writes are ready.
                s |= SocketOperation.Write;
            }

            return s;
        }

        public bool StartRead(Buffer buf, AsyncCallback callback, object state)
        {
            _readPending = true;
            if (_state < StateOpened)
            {
                _finishRead = true;
                if (_state < StateConnected)
                {
                    return _delegate.StartRead(buf, callback, state);
                }
                else
                {
                    return _delegate.StartRead(_readBuffer, callback, state);
                }
            }

            if (PreRead(buf))
            {
                _finishRead = true;
                if (_readState == ReadStatePayload)
                {
                    //
                    // If the payload length is smaller than what remains to be read, we read
                    // no more than the payload length. The remaining of the buffer will be
                    // sent over in another frame.
                    //
                    int readSz = _readPayloadLength - (buf.B.Position() - _readStart);
                    if (buf.B.Remaining() > readSz)
                    {
                        int size = buf.Size();
                        buf.Resize(buf.B.Position() + readSz, true);
                        bool completedSynchronously = _delegate.StartRead(buf, callback, state);
                        buf.Resize(size, true);
                        return completedSynchronously;
                    }
                    else
                    {
                        return _delegate.StartRead(buf, callback, state);
                    }
                }
                else
                {
                    return _delegate.StartRead(_readBuffer, callback, state);
                }
            }
            else
            {
                return true;
            }
        }

        public void FinishRead(Buffer buf)
        {
            Debug.Assert(_readPending);
            _readPending = false;

            if (_state < StateOpened)
            {
                Debug.Assert(_finishRead);
                _finishRead = false;
                if (_state < StateConnected)
                {
                    _delegate.FinishRead(buf);
                }
                else
                {
                    _delegate.FinishRead(_readBuffer);
                }
                return;
            }

            if (!_finishRead)
            {
                // Nothing to do.
            }
            else if (_readState == ReadStatePayload)
            {
                Debug.Assert(_finishRead);
                _finishRead = false;
                _delegate.FinishRead(buf);
            }
            else
            {
                Debug.Assert(_finishRead);
                _finishRead = false;
                _delegate.FinishRead(_readBuffer);
            }

            if (_state == StateClosed)
            {
                _readBuffer.Clear();
                return;
            }

            PostRead(buf);
        }

        public bool
        StartWrite(IList<ArraySegment<byte>> buf, int offset, AsyncCallback callback, object state, out bool completed)
        {
            _writePending = true;
            if (_state < StateOpened)
            {
                if (_state < StateConnected)
                {
                    return _delegate.StartWrite(buf, offset, callback, state, out completed);
                }
                else
                {
                    return _delegate.StartWrite(_writeBuffer, _writeBufferOffset, callback, state, out completed);
                }
            }

            int size = buf.GetBytesCount();
            if (PreWrite(buf, size, offset))
            {
                if (_writeBufferOffset < _writeBufferSize)
                {
                    return _delegate.StartWrite(_writeBuffer, _writeBufferOffset, callback, state, out completed);
                }
                else
                {
                    Debug.Assert(_incoming);
                    return _delegate.StartWrite(buf, offset, callback, state, out completed);
                }
            }
            else
            {
                completed = true;
                return false;
            }
        }

        public void FinishWrite(IList<ArraySegment<byte>> buffer, ref int offset)
        {
            _writePending = false;
            if (_state < StateOpened)
            {
                if (_state < StateConnected)
                {
                    _delegate.FinishWrite(buffer, ref offset);
                }
                else
                {
                    _delegate.FinishWrite(_writeBuffer, ref _writeBufferOffset);
                }
                return;
            }

            int size = buffer.GetBytesCount();
            if (_writeBufferOffset < _writeBufferSize)
            {
                _delegate.FinishWrite(_writeBuffer, ref _writeBufferOffset);
            }
            else if (size > 0 && offset < size)
            {
                Debug.Assert(_incoming);
                _delegate.FinishWrite(buffer, ref offset);
            }

            if (_state == StateClosed)
            {
                _writeBuffer.Clear();
                _writeBufferOffset = 0;
                _writeBufferSize = 0;
                return;
            }

            PostWrite(size, ref offset, SocketOperation.None);
        }

        public string Protocol() => _instance.Protocol;

        public Ice.ConnectionInfo GetInfo()
        {
            var info = new Ice.WSConnectionInfo();
            info.Headers = _parser.GetHeaders();
            info.Underlying = _delegate.GetInfo();
            return info;
        }

        public void CheckSendSize(int size) => _delegate.CheckSendSize(size);

        public void SetBufferSize(int rcvSize, int sndSize) => _delegate.SetBufferSize(rcvSize, sndSize);

        public override string ToString() => _delegate.ToString();

        public string ToDetailedString() => _delegate.ToDetailedString();

        internal
        WSTransceiver(ProtocolInstance instance, ITransceiver del, string host, string resource) : this(instance, del)
        {
            _host = host;
            _resource = resource;
            _incoming = false;

            //
            // Write and read buffer size must be large enough to hold the frame header!
            //
            Debug.Assert(_readBufferSize > 256);
        }

        internal WSTransceiver(ProtocolInstance instance, ITransceiver del)
        {
            _instance = instance;
            _delegate = del;
            _state = StateInitializeDelegate;
            _parser = new HttpParser();
            _readState = ReadStateOpcode;
            _readBuffer = new Buffer(ByteBuffer.ByteOrder.BIG_ENDIAN); // Network byte order
            _readBufferSize = 1024;
            _readLastFrame = true;
            _readOpCode = 0;
            _readHeaderLength = 0;
            _readPayloadLength = 0;
            _writeState = WriteStateHeader;
            _writeBuffer = new List<ArraySegment<byte>>();
            _readPending = false;
            _finishRead = false;
            _writePending = false;
            _readMask = new byte[4];
            _writeMask = new byte[4];
            _key = "";
            _pingPayload = Array.Empty<byte>();
            _rand = new Random();
            _host = "";
            _resource = "";
            _incoming = true;

            //
            // Write and read buffer size must be large enough to hold the frame header!
            //
            Debug.Assert(_readBufferSize > 256);
        }

        private void HandleRequest()
        {
            //
            // HTTP/1.1
            //
            if (_parser.VersionMajor() != 1 || _parser.VersionMinor() != 1)
            {
                throw new WebSocketException("unsupported HTTP version");
            }

            //
            // "An |Upgrade| header field containing the value 'websocket',
            //  treated as an ASCII case-insensitive value."
            //
            string? val = _parser.GetHeader("Upgrade", true);
            if (val == null)
            {
                throw new WebSocketException("missing value for Upgrade field");
            }
            else if (!val.Equals("websocket"))
            {
                throw new WebSocketException("invalid value `" + val + "' for Upgrade field");
            }

            //
            // "A |Connection| header field that includes the token 'Upgrade',
            //  treated as an ASCII case-insensitive value.
            //
            val = _parser.GetHeader("Connection", true);
            if (val == null)
            {
                throw new WebSocketException("missing value for Connection field");
            }
            else if (val.IndexOf("upgrade") == -1)
            {
                throw new WebSocketException("invalid value `" + val + "' for Connection field");
            }

            //
            // "A |Sec-WebSocket-Version| header field, with a value of 13."
            //
            val = _parser.GetHeader("Sec-WebSocket-Version", false);
            if (val == null)
            {
                throw new WebSocketException("missing value for WebSocket version");
            }
            else if (!val.Equals("13"))
            {
                throw new WebSocketException("unsupported WebSocket version `" + val + "'");
            }

            //
            // "Optionally, a |Sec-WebSocket-Protocol| header field, with a list
            //  of values indicating which protocols the client would like to
            //  speak, ordered by preference."
            //
            bool addProtocol = false;
            val = _parser.GetHeader("Sec-WebSocket-Protocol", true);
            if (val != null)
            {
                string[]? protocols = IceUtilInternal.StringUtil.splitString(val, ",");
                if (protocols == null)
                {
                    throw new WebSocketException($"invalid value `{val}' for WebSocket protocol");
                }

                foreach (string p in protocols)
                {
                    if (!p.Trim().Equals(IceProtocol))
                    {
                        throw new WebSocketException($"unknown value `{p}' for WebSocket protocol");
                    }
                    addProtocol = true;
                }
            }

            //
            // "A |Sec-WebSocket-Key| header field with a base64-encoded
            //  value that, when decoded, is 16 bytes in length."
            //
            string? key = _parser.GetHeader("Sec-WebSocket-Key", false);
            if (key == null)
            {
                throw new WebSocketException("missing value for WebSocket key");
            }

            byte[] decodedKey = Convert.FromBase64String(key);
            if (decodedKey.Length != 16)
            {
                throw new WebSocketException($"invalid value `{key}' for WebSocket key");
            }

            //
            // Retain the target resource.
            //
            _resource = _parser.Uri();

            //
            // Compose the response.
            //
            var sb = new StringBuilder();
            sb.Append("HTTP/1.1 101 Switching Protocols\r\n");
            sb.Append("Upgrade: websocket\r\n");
            sb.Append("Connection: Upgrade\r\n");
            if (addProtocol)
            {
                sb.Append($"Sec-WebSocket-Protocol: {IceProtocol}\r\n");
            }

            //
            // The response includes:
            //
            // "A |Sec-WebSocket-Accept| header field.  The value of this
            //  header field is constructed by concatenating /key/, defined
            //  above in step 4 in Section 4.2.2, with the string "258EAFA5-
            //  E914-47DA-95CA-C5AB0DC85B11", taking the SHA-1 hash of this
            //  concatenated value to obtain a 20-byte value and base64-
            //  encoding (see Section 4 of [RFC4648]) this 20-byte hash.
            //
            sb.Append("Sec-WebSocket-Accept: ");
            string input = key + WsUUID;
#pragma warning disable CA5350 // Do Not Use Weak Cryptographic Algorithms
            byte[] hash = SHA1.Create().ComputeHash(_utf8.GetBytes(input));
#pragma warning restore CA5350 // Do Not Use Weak Cryptographic Algorithms
            sb.Append(Convert.ToBase64String(hash) + "\r\n" + "\r\n"); // EOM
            _writeBuffer.Clear();
            byte[] data = _utf8.GetBytes(sb.ToString());
            _writeBuffer.Add(data);
            _writeBufferSize = data.Length;
            _writeBufferOffset = 0;
        }

        private void HandleResponse()
        {
            //
            // HTTP/1.1
            //
            if (_parser.VersionMajor() != 1 || _parser.VersionMinor() != 1)
            {
                throw new WebSocketException("unsupported HTTP version");
            }

            //
            // "If the status code received from the server is not 101, the
            //  client handles the response per HTTP [RFC2616] procedures.  In
            //  particular, the client might perform authentication if it
            //  receives a 401 status code; the server might redirect the client
            //  using a 3xx status code (but clients are not required to follow
            //  them), etc."
            //
            if (_parser.Status() != 101)
            {
                var @out = new StringBuilder("unexpected status value " + _parser.Status());
                if (_parser.Reason().Length > 0)
                {
                    @out.Append(":\n" + _parser.Reason());
                }
                throw new WebSocketException(@out.ToString());
            }

            //
            // "If the response lacks an |Upgrade| header field or the |Upgrade|
            //  header field contains a value that is not an ASCII case-
            //  insensitive match for the value "websocket", the client MUST
            //  _Fail the WebSocket Connection_."
            //
            string? val = _parser.GetHeader("Upgrade", true);
            if (val == null)
            {
                throw new WebSocketException("missing value for Upgrade field");
            }
            else if (!val.Equals("websocket"))
            {
                throw new WebSocketException("invalid value `" + val + "' for Upgrade field");
            }

            //
            // "If the response lacks a |Connection| header field or the
            //  |Connection| header field doesn't contain a token that is an
            //  ASCII case-insensitive match for the value "Upgrade", the client
            //  MUST _Fail the WebSocket Connection_."
            //
            val = _parser.GetHeader("Connection", true);
            if (val == null)
            {
                throw new WebSocketException("missing value for Connection field");
            }
            else if (val.IndexOf("upgrade") == -1)
            {
                throw new WebSocketException("invalid value `" + val + "' for Connection field");
            }

            //
            // "If the response includes a |Sec-WebSocket-Protocol| header field
            //  and this header field indicates the use of a subprotocol that was
            //  not present in the client's handshake (the server has indicated a
            //  subprotocol not requested by the client), the client MUST _Fail
            //  the WebSocket Connection_."
            //
            val = _parser.GetHeader("Sec-WebSocket-Protocol", true);
            if (val != null && !val.Equals(IceProtocol))
            {
                throw new WebSocketException("invalid value `" + val + "' for WebSocket protocol");
            }

            //
            // "If the response lacks a |Sec-WebSocket-Accept| header field or
            //  the |Sec-WebSocket-Accept| contains a value other than the
            //  base64-encoded SHA-1 of the concatenation of the |Sec-WebSocket-
            //  Key| (as a string, not base64-decoded) with the string "258EAFA5-
            //  E914-47DA-95CA-C5AB0DC85B11" but ignoring any leading and
            //  trailing whitespace, the client MUST _Fail the WebSocket
            //  Connection_."
            //
            val = _parser.GetHeader("Sec-WebSocket-Accept", false);
            if (val == null)
            {
                throw new WebSocketException("missing value for Sec-WebSocket-Accept");
            }

            string input = _key + WsUUID;
#pragma warning disable CA5350 // Do Not Use Weak Cryptographic Algorithms
            byte[] hash = SHA1.Create().ComputeHash(_utf8.GetBytes(input));
#pragma warning restore CA5350 // Do Not Use Weak Cryptographic Algorithms
            if (!val.Equals(Convert.ToBase64String(hash)))
            {
                throw new WebSocketException("invalid value `" + val + "' for Sec-WebSocket-Accept");
            }
        }

        private bool PreRead(Buffer buf)
        {
            while (true)
            {
                if (_readState == ReadStateOpcode)
                {
                    //
                    // Is there enough data available to read the opcode?
                    //
                    if (!ReadBuffered(2))
                    {
                        return true;
                    }

                    //
                    // Most-significant bit indicates whether this is the
                    // last frame. Least-significant four bits hold the
                    // opcode.
                    //
                    int ch = _readBuffer.B.Get(_readBufferPos++);
                    _readOpCode = ch & 0xf;

                    //
                    // Remember if last frame if we're going to read a data or
                    // continuation frame, this is only for protocol
                    // correctness checking purpose.
                    //
                    if (_readOpCode == OP_DATA)
                    {
                        if (!_readLastFrame)
                        {
                            throw new Ice.ProtocolException("invalid data frame, no FIN on previous frame");
                        }
                        _readLastFrame = (ch & FLAG_FINAL) == FLAG_FINAL;
                    }
                    else if (_readOpCode == OP_CONT)
                    {
                        if (_readLastFrame)
                        {
                            throw new Ice.ProtocolException("invalid continuation frame, previous frame FIN set");
                        }
                        _readLastFrame = (ch & FLAG_FINAL) == FLAG_FINAL;
                    }

                    ch = _readBuffer.B.Get(_readBufferPos++);

                    //
                    // Check the MASK bit. Messages sent by a client must be masked;
                    // messages sent by a server must not be masked.
                    //
                    bool masked = (ch & FLAG_MASKED) == FLAG_MASKED;
                    if (masked != _incoming)
                    {
                        throw new Ice.ProtocolException("invalid masking");
                    }

                    //
                    // Extract the payload length, which can have the following values:
                    //
                    // 0-125: The payload length
                    // 126:   The subsequent two bytes contain the payload length
                    // 127:   The subsequent eight bytes contain the payload length
                    //
                    _readPayloadLength = ch & 0x7f;
                    if (_readPayloadLength < 126)
                    {
                        _readHeaderLength = 0;
                    }
                    else if (_readPayloadLength == 126)
                    {
                        _readHeaderLength = 2; // Need to read a 16-bit payload length.
                    }
                    else
                    {
                        _readHeaderLength = 8; // Need to read a 64-bit payload length.
                    }
                    if (masked)
                    {
                        _readHeaderLength += 4; // Need to read a 32-bit mask.
                    }

                    _readState = ReadStateHeader;
                }

                if (_readState == ReadStateHeader)
                {
                    //
                    // Is there enough data available to read the header?
                    //
                    if (_readHeaderLength > 0 && !ReadBuffered(_readHeaderLength))
                    {
                        return true;
                    }

                    if (_readPayloadLength == 126)
                    {
                        _readPayloadLength = _readBuffer.B.GetShort(_readBufferPos); // Uses network byte order.
                        if (_readPayloadLength < 0)
                        {
                            _readPayloadLength += 65536;
                        }
                        _readBufferPos += 2;
                    }
                    else if (_readPayloadLength == 127)
                    {
                        long l = _readBuffer.B.GetLong(_readBufferPos); // Uses network byte order.
                        _readBufferPos += 8;
                        if (l < 0 || l > int.MaxValue)
                        {
                            throw new Ice.ProtocolException("invalid WebSocket payload length: " + l);
                        }
                        _readPayloadLength = (int)l;
                    }

                    //
                    // Read the mask if this is an incoming connection.
                    //
                    if (_incoming)
                    {
                        //
                        // We must have needed to read the mask.
                        //
                        Debug.Assert(_readBuffer.B.Position() - _readBufferPos >= 4);
                        for (int i = 0; i < 4; ++i)
                        {
                            _readMask[i] = _readBuffer.B.Get(_readBufferPos++); // Copy the mask.
                        }
                    }

                    switch (_readOpCode)
                    {
                        case OP_TEXT: // Text frame
                            {
                                throw new Ice.ProtocolException("text frames not supported");
                            }
                        case OP_DATA: // Data frame
                        case OP_CONT: // Continuation frame
                            {
                                if (_instance.TraceLevel >= 2)
                                {
                                    _instance.Logger.Trace(_instance.TraceCategory, "received " + Protocol() +
                                                             (_readOpCode == OP_DATA ? " data" : " continuation") +
                                                             " frame with payload length of " + _readPayloadLength +
                                                             " bytes\n" + ToString());
                                }

                                if (_readPayloadLength <= 0)
                                {
                                    throw new Ice.ProtocolException("payload length is 0");
                                }
                                _readState = ReadStatePayload;
                                Debug.Assert(buf.B.HasRemaining());
                                _readFrameStart = buf.B.Position();
                                break;
                            }
                        case OP_CLOSE: // Connection close
                            {
                                if (_instance.TraceLevel >= 2)
                                {
                                    _instance.Logger.Trace(_instance.TraceCategory,
                                        $"received {Protocol()} connection close frame\n{this}");
                                }

                                _readState = ReadStateControlFrame;
                                int s = _nextState == StateOpened ? _state : _nextState;
                                if (s == StateClosingRequestPending)
                                {
                                    //
                                    // If we receive a close frame while we were actually
                                    // waiting to send one, change the role and send a
                                    // close frame response.
                                    //
                                    if (!_closingInitiator)
                                    {
                                        _closingInitiator = true;
                                    }
                                    if (_state == StateClosingRequestPending)
                                    {
                                        _state = StateClosingResponsePending;
                                    }
                                    else
                                    {
                                        _nextState = StateClosingResponsePending;
                                    }
                                    return false; // No longer interested in reading
                                }
                                else
                                {
                                    throw new Ice.ConnectionLostException();
                                }
                            }
                        case OP_PING:
                            {
                                if (_instance.TraceLevel >= 2)
                                {
                                    _instance.Logger.Trace(_instance.TraceCategory,
                                        $"received {Protocol()} connection ping frame\n{this}");
                                }
                                _readState = ReadStateControlFrame;
                                break;
                            }
                        case OP_PONG: // Pong
                            {
                                if (_instance.TraceLevel >= 2)
                                {
                                    _instance.Logger.Trace(_instance.TraceCategory,
                                        $"received {Protocol()} connection pong frame\n{this}");
                                }
                                _readState = ReadStateControlFrame;
                                break;
                            }
                        default:
                            {
                                throw new Ice.ProtocolException("unsupported opcode: " + _readOpCode);
                            }
                    }
                }

                if (_readState == ReadStateControlFrame)
                {
                    if (_readPayloadLength > 0 && !ReadBuffered(_readPayloadLength))
                    {
                        return true;
                    }

                    if (_readPayloadLength > 0 && _readOpCode == OP_PING)
                    {
                        _pingPayload = new byte[_readPayloadLength];
                        System.Buffer.BlockCopy(_readBuffer.B.RawBytes(), _readBufferPos, _pingPayload, 0,
                                                _readPayloadLength);
                    }

                    _readBufferPos += _readPayloadLength;
                    _readPayloadLength = 0;

                    if (_readOpCode == OP_PING)
                    {
                        if (_state == StateOpened)
                        {
                            _state = StatePongPending; // Send pong frame now
                        }
                        else if (_nextState < StatePongPending)
                        {
                            _nextState = StatePongPending; // Send pong frame next
                        }
                    }

                    //
                    // We've read the payload of the PING/PONG frame, we're ready
                    // to read a new frame.
                    //
                    _readState = ReadStateOpcode;
                }

                if (_readState == ReadStatePayload)
                {
                    //
                    // This must be assigned before the check for the buffer. If the buffer is empty
                    // or already read, postRead will return false.
                    //
                    _readStart = buf.B.Position();

                    if (buf.Empty() || !buf.B.HasRemaining())
                    {
                        return false;
                    }

                    int n = Math.Min(_readBuffer.B.Position() - _readBufferPos, buf.B.Remaining());
                    if (n > _readPayloadLength)
                    {
                        n = _readPayloadLength;
                    }
                    if (n > 0)
                    {
                        System.Buffer.BlockCopy(_readBuffer.B.RawBytes(), _readBufferPos, buf.B.RawBytes(),
                                                buf.B.Position(), n);
                        buf.B.Position(buf.B.Position() + n);
                        _readBufferPos += n;
                    }

                    //
                    // Continue reading if we didn't read the full message, otherwise give back
                    // the control to the connection
                    //
                    return buf.B.HasRemaining() && n < _readPayloadLength;
                }
            }
        }

        private bool PostRead(Buffer buf)
        {
            if (_readState != ReadStatePayload)
            {
                return _readStart < _readBuffer.B.Position(); // Returns true if data was read.
            }

            if (_readStart == buf.B.Position())
            {
                return false; // Nothing was read or nothing to read.
            }
            Debug.Assert(_readStart < buf.B.Position());

            if (_incoming)
            {
                //
                // Unmask the data we just read.
                //
                int pos = buf.B.Position();
                byte[] arr = buf.B.RawBytes();
                for (int n = _readStart; n < pos; ++n)
                {
                    arr[n] = (byte)(arr[n] ^ _readMask[(n - _readFrameStart) % 4]);
                }
            }

            _readPayloadLength -= buf.B.Position() - _readStart;
            _readStart = buf.B.Position();
            if (_readPayloadLength == 0)
            {
                //
                // We've read the complete payload, we're ready to read a new frame.
                //
                _readState = ReadStateOpcode;
            }
            return buf.B.HasRemaining();
        }

        private bool PreWrite(IList<ArraySegment<byte>> buf, int size, int offset)
        {
            if (_writeState == WriteStateHeader)
            {
                if (_state == StateOpened)
                {
                    if (size == 0 || size == offset)
                    {
                        return false;
                    }
                    Debug.Assert(offset == 0);
                    PrepareWriteHeader(OP_DATA, size);

                    _writeState = WriteStatePayload;
                }
                else if (_state == StatePingPending)
                {
                    PrepareWriteHeader(OP_PING, 0); // Don't send any payload

                    _writeState = WriteStateControlFrame;
                }
                else if (_state == StatePongPending)
                {
                    PrepareWriteHeader(OP_PONG, _pingPayload.Length);
                    if (_pingPayload.Length > 0)
                    {
                        _writeBuffer.Add(_pingPayload);
                    }

                    _pingPayload = Array.Empty<byte>();
                    _writeState = WriteStateControlFrame;
                }
                else if ((_state == StateClosingRequestPending && !_closingInitiator) ||
                         (_state == StateClosingResponsePending && _closingInitiator))
                {
                    PrepareWriteHeader(OP_CLOSE, 2);
                    byte[] buffer = new byte[2];

                    Debug.Assert(BitConverter.IsLittleEndian);
                    short reason = BinaryPrimitives.ReverseEndianness((short)_closingReason);
                    // Write closing reason
                    MemoryMarshal.Write(buffer.AsSpan(0, 2), ref reason);

                    if (!_incoming)
                    {
                        buffer[0] = (byte)(buffer[0] ^ _writeMask[0]);
                        buffer[1] = (byte)(buffer[1] ^ _writeMask[1]);
                    }
                    _writeBuffer.Add(buffer);
                    _writeState = WriteStateControlFrame;
                    _writeBufferSize += buffer.Length;
                }
                else
                {
                    Debug.Assert(_state != StateClosed);
                    return false; // Nothing to write in this state
                }

                _writePayloadLength = 0;
            }

            if (_writeState == WriteStatePayload)
            {
                //
                // For an outgoing connection, each message must be masked with a random
                // 32-bit value, so we copy the entire message into the internal buffer
                // for writing. For incoming connections, we borrow the segments and add the after
                // the header.
                //
                if (_writePayloadLength == 0)
                {
                    if (_incoming)
                    {
                        Debug.Assert(_incoming);
                        int n = 0;
                        foreach (ArraySegment<byte> segment in buf)
                        {
                            _writeBuffer.Add(segment); // Borrow data from the buffer
                            n += segment.Count;
                        }
                        _writeBufferSize += n;
                        _writePayloadLength = n;
                    }
                    else
                    {
                        int n = 0;
                        foreach (ArraySegment<byte> segment in buf)
                        {
                            byte[] data = new byte[segment.Count];
                            for (int i = 0; i < segment.Count; ++i, ++n)
                            {
                                data[i] = (byte)(segment[i] ^ _writeMask[n % 4]);
                            }
                            _writeBuffer.Add(data);
                        }
                        _writeBufferSize += n;
                        _writePayloadLength = n;
                    }
                }
                return true;
            }
            else if (_writeState == WriteStateControlFrame)
            {
                return _writeBufferOffset < _writeBufferSize;
            }
            else
            {
                Debug.Assert(_writeState == WriteStateFlush);
                return true;
            }
        }

        private bool PostWrite(int size, ref int offset, int status)
        {
            if (_state > StateOpened && _writeState == WriteStateControlFrame)
            {
                if (_writeBufferOffset == _writeBufferSize)
                {
                    if (_state == StatePingPending)
                    {
                        if (_instance.TraceLevel >= 2)
                        {
                            _instance.Logger.Trace(_instance.TraceCategory,
                                $"sent {Protocol()} connection ping frame\n{this}");
                        }
                    }
                    else if (_state == StatePongPending)
                    {
                        if (_instance.TraceLevel >= 2)
                        {
                            _instance.Logger.Trace(_instance.TraceCategory,
                                $"sent {Protocol()} connection pong frame\n{this}");
                        }
                    }
                    else if ((_state == StateClosingRequestPending && !_closingInitiator) ||
                            (_state == StateClosingResponsePending && _closingInitiator))
                    {
                        if (_instance.TraceLevel >= 2)
                        {
                            _instance.Logger.Trace(_instance.TraceCategory,
                                $"sent {Protocol()} connection close frame\n{this}");
                        }

                        if (_state == StateClosingRequestPending && !_closingInitiator)
                        {
                            _writeState = WriteStateHeader;
                            _state = StateClosingResponsePending;
                            return false;
                        }
                        else
                        {
                            throw new ConnectionLostException();
                        }
                    }
                    else if (_state == StateClosed)
                    {
                        return false;
                    }

                    _state = _nextState;
                    _nextState = StateOpened;
                    _writeState = WriteStateHeader;
                }
                else
                {
                    return status == SocketOperation.None;
                }
            }

            if (_writePayloadLength > 0 && _writeBufferOffset == _writeBufferSize)
            {
                offset = _writePayloadLength;
            }

            if (status == SocketOperation.Write && size == offset && _writeBufferSize == _writeBufferOffset)
            {
                //
                // Our buffers are empty but the delegate needs another call to write().
                //
                _writeState = WriteStateFlush;
                return false;
            }
            else if (size == offset)
            {
                _writeState = WriteStateHeader;
                if (_state == StatePingPending ||
                   _state == StatePongPending ||
                   (_state == StateClosingRequestPending && !_closingInitiator) ||
                   (_state == StateClosingResponsePending && _closingInitiator))
                {
                    return true;
                }
            }
            else if (_state == StateOpened)
            {
                return status == SocketOperation.None;
            }
            return false;
        }

        private bool ReadBuffered(int sz)
        {
            if (_readBufferPos == _readBuffer.B.Position())
            {
                _readBuffer.Resize(_readBufferSize, true);
                _readBufferPos = 0;
                _readBuffer.B.Position(0);
            }
            else
            {
                int available = _readBuffer.B.Position() - _readBufferPos;
                if (available < sz)
                {
                    if (_readBufferPos > 0)
                    {
                        _readBuffer.B.Limit(_readBuffer.B.Position());
                        _readBuffer.B.Position(_readBufferPos);
                        _readBuffer.B.Compact();
                        Debug.Assert(_readBuffer.B.Position() == available);
                    }
                    _readBuffer.Resize(Math.Max(_readBufferSize, sz), true);
                    _readBufferPos = 0;
                    _readBuffer.B.Position(available);
                }
            }

            _readStart = _readBuffer.B.Position();
            if (_readBufferPos + sz > _readBuffer.B.Position())
            {
                return false; // Not enough read.
            }
            Debug.Assert(_readBuffer.B.Position() > _readBufferPos);
            return true;
        }

        private void PrepareWriteHeader(byte opCode, int payloadLength)
        {
            //
            // We need to prepare the frame header.
            //
            byte[] buffer = new byte[16];
            int i = 0;
            //
            // Set the opcode - this is the one and only data frame.
            //
            buffer[i++] = (byte)(opCode | FLAG_FINAL);
            //
            // Set the payload length.
            //
            if (payloadLength <= 125)
            {
                buffer[i++] = (byte)payloadLength;
            }
            else if (payloadLength > 125 && payloadLength <= 65535)
            {
                //
                // Use an extra 16 bits to encode the payload length.
                //
                buffer[i++] = 126;
                Debug.Assert(BitConverter.IsLittleEndian);
                short length = BinaryPrimitives.ReverseEndianness((short)payloadLength);
                MemoryMarshal.Write(buffer.AsSpan(i, 2), ref length);
                i += 2;
            }
            else if (payloadLength > 65535)
            {
                //
                // Use an extra 64 bits to encode the payload length.
                //
                buffer[i++] = 127;
                Debug.Assert(BitConverter.IsLittleEndian);
                long length = BinaryPrimitives.ReverseEndianness((long)payloadLength);
                MemoryMarshal.Write(buffer.AsSpan(i, 8), ref length);
                i += 8;
            }

            if (!_incoming)
            {
                //
                // Add a random 32-bit mask to every outgoing frame, copy the payload data,
                // and apply the mask.
                //
                buffer[1] = (byte)(buffer[1] | FLAG_MASKED);
                _rand.NextBytes(_writeMask);
                System.Buffer.BlockCopy(_writeMask, 0, buffer, i, _writeMask.Length);
                i += _writeMask.Length;
            }
            _writeBuffer.Clear();
            _writeBuffer.Add(new ArraySegment<byte>(buffer, 0, i));
            _writeBufferSize = i;
            _writeBufferOffset = 0;
        }

        private readonly ProtocolInstance _instance;
        private readonly ITransceiver _delegate;
        private readonly string _host;
        private string _resource;
        private readonly bool _incoming;

        private const int StateInitializeDelegate = 0;
        private const int StateConnected = 1;
        private const int StateUpgradeRequestPending = 2;
        private const int StateUpgradeResponsePending = 3;
        private const int StateOpened = 4;
        private const int StatePingPending = 5;
        private const int StatePongPending = 6;
        private const int StateClosingRequestPending = 7;
        private const int StateClosingResponsePending = 8;
        private const int StateClosed = 9;

        private int _state;
        private int _nextState;

        private readonly HttpParser _parser;
        private string _key;

        private const int ReadStateOpcode = 0;
        private const int ReadStateHeader = 1;
        private const int ReadStateControlFrame = 2;
        private const int ReadStatePayload = 3;

        private int _readState;
        private readonly Buffer _readBuffer;
        private int _readBufferPos;
        private readonly int _readBufferSize;

        private bool _readLastFrame;
        private int _readOpCode;
        private int _readHeaderLength;
        private int _readPayloadLength;
        private int _readStart;
        private int _readFrameStart;
        private readonly byte[] _readMask;

        private const int WriteStateHeader = 0;
        private const int WriteStatePayload = 1;
        private const int WriteStateControlFrame = 2;
        private const int WriteStateFlush = 3;

        private int _writeState;
        private readonly IList<ArraySegment<byte>> _writeBuffer;
        private int _writeBufferSize;
        private int _writeBufferOffset;
        private readonly byte[] _writeMask;
        private int _writePayloadLength;

        private bool _closingInitiator;
        private int _closingReason;

        private bool _readPending;
        private bool _finishRead;
        private bool _writePending;

        private byte[] _pingPayload;

        private readonly Random _rand;

        //
        // WebSocket opcodes
        //
        private const int OP_CONT = 0x0;    // Continuation frame
        private const int OP_TEXT = 0x1;    // Text frame
        private const int OP_DATA = 0x2;    // Data frame
        // private const int OP_RES_0x3 = 0x3;    // Reserved
        // private const int OP_RES_0x4 = 0x4;    // Reserved
        // private const int OP_RES_0x5 = 0x5;    // Reserved
        // private const int OP_RES_0x6 = 0x6;    // Reserved
        // private const int OP_RES_0x7 = 0x7;    // Reserved
        private const int OP_CLOSE = 0x8;    // Connection close
        private const int OP_PING = 0x9;    // Ping
        private const int OP_PONG = 0xA;    // Pong
        // private const int OP_RES_0xB = 0xB;    // Reserved
        // private const int OP_RES_0xC = 0xC;    // Reserved
        // private const int OP_RES_0xD = 0xD;    // Reserved
        // private const int OP_RES_0xE = 0xE;    // Reserved
        // private const int OP_RES_0xF = 0xF;    // Reserved
        private const int FLAG_FINAL = 0x80;   // Last frame
        private const int FLAG_MASKED = 0x80;   // Payload is masked

        private const int CLOSURE_NORMAL = 1000;
        private const int CLOSURE_SHUTDOWN = 1001;
        private const int CLOSURE_PROTOCOL_ERROR = 1002;
        private const int CLOSURE_TOO_BIG = 1009;

        private const string IceProtocol = "ice.zeroc.com";
        private const string WsUUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        private static readonly UTF8Encoding _utf8 = new UTF8Encoding(false, true);
    }
}
