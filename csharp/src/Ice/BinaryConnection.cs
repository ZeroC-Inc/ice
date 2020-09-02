//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice
{
    public abstract class BinaryConnection : IAsyncDisposable
    {
        internal Endpoint Endpoint { get; }
        internal TimeSpan LastActivity;
        internal IConnectionObserver? Observer
        {
            get
            {
                lock (Mutex)
                {
                    return _observer;
                }
            }
            set
            {
                lock (Mutex)
                {
                    _observer = value;
                    _observer?.Attach();
                }
            }
        }

        protected Action? HeartbeatCallback;
        protected readonly int IncomingFrameSizeMax;
        protected readonly bool IsIncoming;
        protected readonly object Mutex = new object();

        private IConnectionObserver? _observer;

        public abstract ValueTask DisposeAsync();

        /// <summary>Gracefully closes the transport.</summary>
        internal abstract ValueTask CloseAsync(Exception ex, CancellationToken cancel);

        /// <summary>Sends a heartbeat.</summary>
        internal abstract ValueTask HeartbeatAsync(CancellationToken cancel);

        /// <summary>Initializes the transport.</summary>
        internal abstract ValueTask InitializeAsync(Action heartbeatCallback, CancellationToken cancel);

        /// <summary>Creates a new stream.</summary>
        internal abstract long NewStream(bool bidirectional);

        /// <summary>Receives a new frame.</summary>
        internal abstract ValueTask<(long StreamId, IncomingFrame? Frame, bool Fin)> ReceiveAsync(CancellationToken cancel);

        /// <summary>Resets the given stream.</summary>
        internal abstract ValueTask ResetAsync(long streamId);

        /// <summary>Sends the given frame on an existing stream.</summary>
        internal abstract ValueTask SendAsync(long streamId, OutgoingFrame frame, bool fin, CancellationToken cancel);

        protected BinaryConnection(Endpoint endpoint, ObjectAdapter? adapter)
        {
            Endpoint = endpoint;
            IsIncoming = adapter != null;
            IncomingFrameSizeMax = adapter?.IncomingFrameSizeMax ?? Endpoint.Communicator.IncomingFrameSizeMax;
            LastActivity = Time.Elapsed;
        }

        protected void Initialized()
        {
            lock (Mutex)
            {
                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 1)
            {
                var s = new StringBuilder();
                if (Endpoint.IsDatagram)
                {
                    s.Append("starting to ");
                    s.Append(IsIncoming ? "receive" : "send");
                    s.Append(' ');
                    s.Append(Endpoint.TransportName);
                    s.Append(" datagrams\n");
                }
                else
                {
                    s.Append(IsIncoming ? "accepted" : "established");
                    s.Append(' ');
                    s.Append(Endpoint.TransportName);
                    s.Append(" connection\n");
                }
                s.Append(ToString());
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory, s.ToString());
            }
        }

        protected void Received(int length)
        {
            lock (Mutex)
            {
                Debug.Assert(length > 0);
                _observer?.ReceivedBytes(length);

                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 3)
            {
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory,
                    $"received {length} bytes via {Endpoint.TransportName}\n{this}");
            }
        }

        protected void Sent(int length)
        {
            lock (Mutex)
            {
                Debug.Assert(length > 0);
                _observer?.SentBytes(length);

                LastActivity = Time.Elapsed;
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 3 && length > 0)
            {
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory,
                    $"sent {length} bytes via {Endpoint.TransportName}\n{this}");
            }
        }

        protected void Closed()
        {
            lock (Mutex)
            {
                _observer?.Detach();
            }

            if (Endpoint.Communicator.TraceLevels.Transport >= 1)
            {
                var s = new StringBuilder();
                s.Append("closed ");
                s.Append(Endpoint.TransportName);
                s.Append(" connection\n");
                s.Append(ToString());
                Endpoint.Communicator.Logger.Trace(Endpoint.Communicator.TraceLevels.TransportCategory, s.ToString());
            }
        }
    }
}
