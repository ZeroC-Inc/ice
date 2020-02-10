//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;

namespace IceInternal
{
    public sealed class StreamSocket
    {
        public StreamSocket(ProtocolInstance instance, INetworkProxy proxy, EndPoint addr, EndPoint sourceAddr)
        {
            _instance = instance;
            _proxy = proxy;
            _addr = addr;
            _sourceAddr = sourceAddr;
            _fd = Network.CreateSocket(false, (_proxy != null ? _proxy.GetAddress() : _addr).AddressFamily);
            _state = StateNeedConnect;

            Network.SetBlock(_fd, false);
            Network.SetTcpBufSize(_fd, _instance);

            _readEventArgs = new SocketAsyncEventArgs();
            _readEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            _writeEventArgs = new SocketAsyncEventArgs();
            _writeEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            //
            // For timeouts to work properly, we need to receive/send
            // the data in several chunks. Otherwise, we would only be
            // notified when all the data is received/written. The
            // connection timeout could easily be triggered when
            // receiging/sending large messages.
            //
            _maxSendPacketSize = Math.Max(512, Network.GetSendBufferSize(_fd));
            _maxRecvPacketSize = Math.Max(512, Network.GetRecvBufferSize(_fd));
        }

        public StreamSocket(ProtocolInstance instance, Socket fd)
        {
            _instance = instance;
            _fd = fd;
            _state = StateConnected;

            try
            {
                _desc = Network.FdToString(_fd);
            }
            catch (Exception)
            {
                Network.CloseSocketNoThrow(_fd);
                throw;
            }

            Network.SetBlock(_fd, false);
            Network.SetTcpBufSize(_fd, _instance);

            _readEventArgs = new SocketAsyncEventArgs();
            _readEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            _writeEventArgs = new SocketAsyncEventArgs();
            _writeEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            //
            // For timeouts to work properly, we need to receive/send
            // the data in several chunks. Otherwise, we would only be
            // notified when all the data is received/written. The
            // connection timeout could easily be triggered when
            // receiging/sending large messages.
            //
            _maxSendPacketSize = Math.Max(512, Network.GetSendBufferSize(_fd));
            _maxRecvPacketSize = Math.Max(512, Network.GetRecvBufferSize(_fd));
        }

        public void SetBlock(bool block)
        {
            Debug.Assert(_fd != null);
            Network.SetBlock(_fd, block);
        }

        public int Connect(Buffer readBuffer, Buffer writeBuffer, ref bool moreData)
        {
            if (_state == StateNeedConnect)
            {
                _state = StateConnectPending;
                return SocketOperation.Connect;
            }
            else if (_state <= StateConnectPending)
            {
                Debug.Assert(_writeEventArgs != null);
                if (_writeEventArgs.SocketError != SocketError.Success)
                {
                    var ex = new SocketException((int)_writeEventArgs.SocketError);
                    if (Network.ConnectionRefused(ex))
                    {
                        throw new Ice.ConnectionRefusedException(ex);
                    }
                    else
                    {
                        throw new Ice.ConnectFailedException(ex);
                    }
                }
                Debug.Assert(_fd != null);
                _desc = Network.FdToString(_fd, _proxy, _addr);
                _state = _proxy != null ? StateProxyWrite : StateConnected;
            }

            if (_state == StateProxyWrite)
            {
                Debug.Assert(_proxy != null);
                Debug.Assert(_addr != null);
                _proxy.BeginWrite(_addr, writeBuffer);
                return SocketOperation.Write;
            }
            else if (_state == StateProxyRead)
            {
                Debug.Assert(_proxy != null);
                _proxy.BeginRead(readBuffer);
                return SocketOperation.Read;
            }
            else if (_state == StateProxyConnected)
            {
                Debug.Assert(_proxy != null);
                _proxy.Finish(readBuffer, writeBuffer);

                readBuffer.Clear();
                writeBuffer.Clear();

                _state = StateConnected;
            }

            Debug.Assert(_state == StateConnected);
            return SocketOperation.None;
        }

        public bool IsConnected() => _state == StateConnected && _fd != null;

        public Socket? Fd() => _fd;

        public int GetSendPacketSize(int length) => _maxSendPacketSize > 0 ? Math.Min(length, _maxSendPacketSize) : length;

        public int GetRecvPacketSize(int length) => _maxRecvPacketSize > 0 ? Math.Min(length, _maxRecvPacketSize) : length;

        public void SetBufferSize(int rcvSize, int sndSize)
        {
            Debug.Assert(_fd != null);
            Network.SetTcpBufSize(_fd, rcvSize, sndSize, _instance);
        }

        public int Read(Buffer buf)
        {
            if (_state == StateProxyRead)
            {
                Debug.Assert(_proxy != null);
                while (true)
                {
                    int ret = Read(buf.B);
                    if (ret == 0)
                    {
                        return SocketOperation.Read;
                    }

                    _state = ToState(_proxy.EndRead(buf));
                    if (_state != StateProxyRead)
                    {
                        return SocketOperation.None;
                    }
                }
            }
            Read(buf.B);
            return buf.B.HasRemaining() ? SocketOperation.Read : SocketOperation.None;
        }

        public int Write(Buffer buf)
        {
            if (_state == StateProxyWrite)
            {
                Debug.Assert(_proxy != null);
                while (true)
                {
                    int ret = Write(buf.B);
                    if (ret == 0)
                    {
                        return SocketOperation.Write;
                    }
                    _state = ToState(_proxy.EndWrite(buf));
                    if (_state != StateProxyWrite)
                    {
                        return SocketOperation.None;
                    }
                }
            }
            Write(buf.B);
            return buf.B.HasRemaining() ? SocketOperation.Write : SocketOperation.None;
        }

        public bool StartRead(Buffer buf, AsyncCallback callback, object state)
        {
            Debug.Assert(_fd != null && _readEventArgs != null);

            int packetSize = GetRecvPacketSize(buf.B.Remaining());
            try
            {
                _readCallback = callback;
                _readEventArgs.UserToken = state;
                _readEventArgs.SetBuffer(buf.B.RawBytes(), buf.B.Position(), packetSize);
                return !_fd.ReceiveAsync(_readEventArgs);
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new Ice.ConnectionLostException(ex);
                }
                throw new Ice.SocketException(ex);
            }
        }

        public void FinishRead(Buffer buf)
        {
            if (_fd == null) // Transceiver was closed
            {
                return;
            }

            Debug.Assert(_fd != null && _readEventArgs != null);
            try
            {
                if (_readEventArgs.SocketError != SocketError.Success)
                {
                    throw new SocketException((int)_readEventArgs.SocketError);
                }
                int ret = _readEventArgs.BytesTransferred;
                _readEventArgs.SetBuffer(null, 0, 0);

                if (ret == 0)
                {
                    throw new Ice.ConnectionLostException();
                }

                Debug.Assert(ret > 0);
                buf.B.Position(buf.B.Position() + ret);

                if (_state == StateProxyRead)
                {
                    Debug.Assert(_proxy != null);
                    _state = ToState(_proxy.EndRead(buf));
                }
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new Ice.ConnectionLostException(ex);
                }
                throw new Ice.SocketException(ex);
            }
            catch (ObjectDisposedException ex)
            {
                throw new Ice.ConnectionLostException(ex);
            }
        }

        public bool StartWrite(Buffer buf, AsyncCallback callback, object state, out bool completed)
        {
            Debug.Assert(_fd != null && _writeEventArgs != null);
            if (_state == StateConnectPending)
            {
                completed = false;
                _writeCallback = callback;
                try
                {
                    EndPoint addr;
                    if (_proxy != null)
                    {
                        addr = _proxy.GetAddress();
                    }
                    else
                    {
                        Debug.Assert(_addr != null);
                        addr = _addr;
                    }
                    _writeEventArgs.RemoteEndPoint = addr;
                    _writeEventArgs.UserToken = state;
                    return !_fd.ConnectAsync(_writeEventArgs);
                }
                catch (Exception ex)
                {
                    throw new Ice.SocketException(ex);
                }
            }

            int packetSize = GetSendPacketSize(buf.B.Remaining());
            try
            {
                _writeCallback = callback;
                _writeEventArgs.UserToken = state;
                _writeEventArgs.SetBuffer(buf.B.RawBytes(), buf.B.Position(), packetSize);
                bool completedSynchronously = !_fd.SendAsync(_writeEventArgs);
                completed = packetSize == buf.B.Remaining();
                return completedSynchronously;
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new Ice.ConnectionLostException(ex);
                }
                throw new Ice.SocketException(ex);
            }
            catch (ObjectDisposedException ex)
            {
                throw new Ice.ConnectionLostException(ex);
            }
        }

        public void FinishWrite(Buffer buf)
        {
            if (_fd == null) // Transceiver was closed
            {
                if (buf.Size() - buf.B.Position() < _maxSendPacketSize)
                {
                    buf.B.Position(buf.B.Limit()); // Assume all the data was sent for at-most-once semantics.
                }
                return;
            }

            Debug.Assert(_fd != null && _writeEventArgs != null);

            if (_state < StateConnected && _state != StateProxyWrite)
            {
                return;
            }

            try
            {
                if (_writeEventArgs.SocketError != SocketError.Success)
                {
                    throw new SocketException((int)_writeEventArgs.SocketError);
                }
                int ret = _writeEventArgs.BytesTransferred;
                _writeEventArgs.SetBuffer(null, 0, 0);
                if (ret == 0)
                {
                    throw new Ice.ConnectionLostException();
                }

                Debug.Assert(ret > 0);
                buf.B.Position(buf.B.Position() + ret);

                if (_state == StateProxyWrite)
                {
                    Debug.Assert(_proxy != null);
                    _state = ToState(_proxy.EndWrite(buf));
                }
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new Ice.ConnectionLostException(ex);
                }

                throw new Ice.SocketException(ex);
            }
            catch (ObjectDisposedException ex)
            {
                throw new Ice.ConnectionLostException(ex);
            }
        }

        public void Close()
        {
            Debug.Assert(_fd != null);
            try
            {
                Network.CloseSocket(_fd);
            }
            finally
            {
                _fd = null;
            }
        }

        public void Destroy()
        {
            Debug.Assert(_readEventArgs != null && _writeEventArgs != null);
            _readEventArgs.Dispose();
            _writeEventArgs.Dispose();
        }

        public override string ToString() => _desc;

        private int Read(ByteBuffer buf)
        {
            Debug.Assert(_fd != null);
            if (AssemblyUtil.IsMono)
            {
                //
                // Mono on Android and iOS don't support the use of synchronous socket
                // operations on a non-blocking socket. Returning 0 here forces the caller to schedule
                // an asynchronous operation.
                //
                return 0;
            }
            int read = 0;
            while (buf.HasRemaining())
            {
                try
                {
                    int ret = _fd.Receive(buf.RawBytes(), buf.Position(), buf.Remaining(), SocketFlags.None);
                    if (ret == 0)
                    {
                        throw new Ice.ConnectionLostException();
                    }
                    read += ret;
                    buf.Position(buf.Position() + ret);
                }
                catch (SocketException ex)
                {
                    if (Network.WouldBlock(ex))
                    {
                        return read;
                    }
                    else if (Network.Interrupted(ex))
                    {
                        continue;
                    }
                    else if (Network.ConnectionLost(ex))
                    {
                        throw new Ice.ConnectionLostException(ex);
                    }

                    throw new Ice.SocketException(ex);
                }
            }
            return read;
        }

        private int Write(ByteBuffer buf)
        {
            Debug.Assert(_fd != null);
            if (AssemblyUtil.IsMono)
            {
                //
                // Mono on Android and iOS don't support the use of synchronous socket
                // operations on a non-blocking socket. Returning 0 here forces the caller to schedule
                // an asynchronous operation.
                //
                return 0;
            }
            int packetSize = buf.Remaining();
            if (AssemblyUtil.IsWindows)
            {
                //
                // On Windows, limiting the buffer size is important to prevent
                // poor throughput performances when transfering large amount of
                // data. See Microsoft KB article KB823764.
                //
                if (_maxSendPacketSize > 0 && packetSize > _maxSendPacketSize / 2)
                {
                    packetSize = _maxSendPacketSize / 2;
                }
            }

            int sent = 0;
            while (buf.HasRemaining())
            {
                try
                {
                    int ret = _fd.Send(buf.RawBytes(), buf.Position(), packetSize, SocketFlags.None);
                    Debug.Assert(ret > 0);

                    sent += ret;
                    buf.Position(buf.Position() + ret);
                    if (packetSize > buf.Remaining())
                    {
                        packetSize = buf.Remaining();
                    }
                }
                catch (SocketException ex)
                {
                    if (Network.WouldBlock(ex))
                    {
                        return sent;
                    }
                    else if (Network.ConnectionLost(ex))
                    {
                        throw new Ice.ConnectionLostException(ex);
                    }
                    throw new Ice.SocketException(ex);
                }
            }
            return sent;
        }
        private void IoCompleted(object sender, SocketAsyncEventArgs e)
        {
            switch (e.LastOperation)
            {
                case SocketAsyncOperation.Receive:
                    Debug.Assert(_readCallback != null);
                    _readCallback(e.UserToken);
                    break;
                case SocketAsyncOperation.Send:
                case SocketAsyncOperation.Connect:
                    Debug.Assert(_writeCallback != null);
                    _writeCallback(e.UserToken);
                    break;
                default:
                    throw new ArgumentException("The last operation completed on the socket was not a receive or send");
            }
        }

        private int ToState(int operation)
        {
            return operation switch
            {
                SocketOperation.Read => StateProxyRead,
                SocketOperation.Write => StateProxyWrite,
                _ => StateProxyConnected,
            };
        }

        private readonly ProtocolInstance _instance;
        private readonly INetworkProxy? _proxy;
        private readonly EndPoint? _addr;
        private readonly EndPoint? _sourceAddr;

        private Socket? _fd;
        private readonly int _maxSendPacketSize;
        private readonly int _maxRecvPacketSize;
        private int _state;
        private string _desc = "";

        private readonly SocketAsyncEventArgs? _writeEventArgs;
        private readonly SocketAsyncEventArgs? _readEventArgs;

        private AsyncCallback? _writeCallback;
        private AsyncCallback? _readCallback;

        private const int StateNeedConnect = 0;
        private const int StateConnectPending = 1;
        private const int StateProxyWrite = 2;
        private const int StateProxyRead = 3;
        private const int StateProxyConnected = 4;
        private const int StateConnected = 5;
    }

}
