//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Instrumentation.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//


using _System = global::System;

#pragma warning disable 1591

namespace Ice
{
    namespace Instrumentation
    {
        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface Observer
        {
            #region Slice operations


            /// <summary>
            /// This method is called when the instrumented object is created or when the observer is attached to an existing
            ///  object.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void attach();


            /// <summary>
            /// This method is called when the instrumented object is destroyed and as a result the observer detached from the
            ///  object.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void detach();


            /// <summary>
            /// Notification of a failure.
            /// </summary>
            /// <param name="exceptionName">The name of the exception.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void failed(string exceptionName);

            #endregion
        }

        /// <summary>
        /// The thread state enumeration keeps track of the different possible states of Ice threads.
        /// </summary>

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
        public enum ThreadState
        {
            /// <summary>
            /// The thread is idle.
            /// </summary>

            ThreadStateIdle,
            /// <summary>
            /// The thread is in use performing reads or writes for Ice connections.
            /// This state is only for threads from an Ice
            ///  thread pool.
            /// </summary>

            ThreadStateInUseForIO,
            /// <summary>
            /// The thread is calling user code (servant implementation, AMI callbacks).
            /// This state is only for threads from an
            ///  Ice thread pool.
            /// </summary>

            ThreadStateInUseForUser,
            /// <summary>
            /// The thread is performing other internal activities (DNS lookups, timer callbacks, etc).
            /// </summary>

            ThreadStateInUseForOther
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface ThreadObserver : Observer
        {
            #region Slice operations


            /// <summary>
            /// Notification of thread state change.
            /// </summary>
            /// <param name="oldState">The previous thread state.
            ///  </param>
            /// <param name="newState">The new thread state.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void stateChanged(ThreadState oldState, ThreadState newState);

            #endregion
        }

        /// <summary>
        /// The state of an Ice connection.
        /// </summary>

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
        public enum ConnectionState
        {
            /// <summary>
            /// The connection is being validated.
            /// </summary>

            ConnectionStateValidating,
            /// <summary>
            /// The connection is holding the reception of new messages.
            /// </summary>

            ConnectionStateHolding,
            /// <summary>
            /// The connection is active and can send and receive messages.
            /// </summary>

            ConnectionStateActive,
            /// <summary>
            /// The connection is being gracefully shutdown and waits for the peer to close its end of the connection.
            /// </summary>

            ConnectionStateClosing,
            /// <summary>
            /// The connection is closed and waits for potential dispatch to be finished before being destroyed and detached
            ///  from the observer.
            /// </summary>

            ConnectionStateClosed
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface ConnectionObserver : Observer
        {
            #region Slice operations


            /// <summary>
            /// Notification of sent bytes over the connection.
            /// </summary>
            /// <param name="num">The number of bytes sent.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void sentBytes(int num);


            /// <summary>
            /// Notification of received bytes over the connection.
            /// </summary>
            /// <param name="num">The number of bytes received.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void receivedBytes(int num);

            #endregion
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface DispatchObserver : Observer
        {
            #region Slice operations


            /// <summary>
            /// Notification of a user exception.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void userException();


            /// <summary>
            /// Reply notification.
            /// </summary>
            /// <param name="size">The size of the reply.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void reply(int size);

            #endregion
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface ChildInvocationObserver : Observer
        {
            #region Slice operations


            /// <summary>
            /// Reply notification.
            /// </summary>
            /// <param name="size">The size of the reply.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void reply(int size);

            #endregion
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface RemoteObserver : ChildInvocationObserver
        {
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface CollocatedObserver : ChildInvocationObserver
        {
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface InvocationObserver : Observer
        {
            #region Slice operations


            /// <summary>
            /// Notification of the invocation being retried.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void retried();


            /// <summary>
            /// Notification of a user exception.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void userException();


            /// <summary>
            /// Get a remote observer for this invocation.
            /// </summary>
            /// <param name="con">The connection information.
            ///  </param>
            /// <param name="endpt">The connection endpoint.
            ///  </param>
            /// <param name="requestId">The ID of the invocation.
            ///  </param>
            /// <param name="size">The size of the invocation.
            ///  </param>
            /// <returns>The observer to instrument the remote invocation.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            RemoteObserver getRemoteObserver(global::Ice.ConnectionInfo con, global::Ice.Endpoint endpt, int requestId, int size);


            /// <summary>
            /// Get a collocated observer for this invocation.
            /// </summary>
            /// <param name="adapter">The object adapter hosting the collocated Ice object.
            ///  </param>
            /// <param name="requestId">The ID of the invocation.
            ///  </param>
            /// <param name="size">The size of the invocation.
            ///  </param>
            /// <returns>The observer to instrument the collocated invocation.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            CollocatedObserver getCollocatedObserver(global::Ice.ObjectAdapter adapter, int requestId, int size);

            #endregion
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface ObserverUpdater
        {
            #region Slice operations


            /// <summary>
            /// Update connection observers associated with each of the Ice connection from the communicator and its object
            ///  adapters.
            /// When called, this method goes through all the connections and for each connection
            ///  CommunicatorObserver.getConnectionObserver is called. The implementation of getConnectionObserver has
            ///  the possibility to return an updated observer if necessary.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void updateConnectionObservers();


            /// <summary>
            /// Update thread observers associated with each of the Ice thread from the communicator and its object adapters.
            /// When called, this method goes through all the threads and for each thread
            ///  CommunicatorObserver.getThreadObserver is called. The implementation of getThreadObserver has the
            ///  possibility to return an updated observer if necessary.
            /// </summary>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void updateThreadObservers();

            #endregion
        }

        [global::System.Runtime.InteropServices.ComVisible(false)]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
        [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
        public partial interface CommunicatorObserver
        {
            #region Slice operations


            /// <summary>
            /// This method should return an observer for the given endpoint information and connector.
            /// The Ice run-time calls
            ///  this method for each connection establishment attempt.
            /// </summary>
            ///  <param name="endpt">The endpoint.
            ///  </param>
            /// <param name="connector">The description of the connector. For IP transports, this is typically the IP address to
            ///  connect to.
            ///  </param>
            /// <returns>The observer to instrument the connection establishment.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            Observer getConnectionEstablishmentObserver(global::Ice.Endpoint endpt, string connector);


            /// <summary>
            /// This method should return an observer for the given endpoint information.
            /// The Ice run-time calls this method to
            ///  resolve an endpoint and obtain the list of connectors. For IP endpoints, this typically involves doing a DNS
            ///  lookup to obtain the IP addresses associated with the DNS name.
            /// </summary>
            ///  <param name="endpt">The endpoint.
            ///  </param>
            /// <returns>The observer to instrument the endpoint lookup.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            Observer getEndpointLookupObserver(global::Ice.Endpoint endpt);


            /// <summary>
            /// This method should return a connection observer for the given connection.
            /// The Ice run-time calls this method
            ///  for each new connection and for all the Ice communicator connections when
            ///  ObserverUpdater.updateConnectionObservers is called.
            /// </summary>
            ///  <param name="c">The connection information.
            ///  </param>
            /// <param name="e">The connection endpoint.
            ///  </param>
            /// <param name="s">The state of the connection.
            ///  </param>
            /// <param name="o">The old connection observer if one is already set or a null reference otherwise.
            ///  </param>
            /// <returns>The connection observer to instrument the connection.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            ConnectionObserver getConnectionObserver(global::Ice.ConnectionInfo c, global::Ice.Endpoint e, ConnectionState s, ConnectionObserver o);


            /// <summary>
            /// This method should return a thread observer for the given thread.
            /// The Ice run-time calls this method for each
            ///  new thread and for all the Ice communicator threads when ObserverUpdater.updateThreadObservers is
            ///  called.
            /// </summary>
            ///  <param name="parent">The parent of the thread.
            ///  </param>
            /// <param name="id">The ID of the thread to observe.
            ///  </param>
            /// <param name="s">The state of the thread.
            ///  </param>
            /// <param name="o">The old thread observer if one is already set or a null reference otherwise.
            ///  </param>
            /// <returns>The thread observer to instrument the thread.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            ThreadObserver getThreadObserver(string parent, string id, ThreadState s, ThreadObserver o);


            /// <summary>
            /// This method should return an invocation observer for the given invocation.
            /// The Ice run-time calls this method
            ///  for each new invocation on a proxy.
            /// </summary>
            ///  <param name="prx">The proxy used for the invocation.
            ///  </param>
            /// <param name="operation">The name of the operation.
            ///  </param>
            /// <param name="ctx">The context specified by the user.
            ///  </param>
            /// <returns>The invocation observer to instrument the invocation.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            InvocationObserver getInvocationObserver(global::Ice.ObjectPrx prx, string operation, global::System.Collections.Generic.Dictionary<string, string> ctx);


            /// <summary>
            /// This method should return a dispatch observer for the given dispatch.
            /// The Ice run-time calls this method each
            ///  time it receives an incoming invocation to be dispatched for an Ice object.
            /// </summary>
            ///  <param name="c">The current object as provided to the Ice servant dispatching the invocation.
            ///  </param>
            /// <param name="size">The size of the dispatch.
            ///  </param>
            /// <returns>The dispatch observer to instrument the dispatch.</returns>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            DispatchObserver getDispatchObserver(global::Ice.Current c, int size);


            /// <summary>
            /// The Ice run-time calls this method when the communicator is initialized.
            /// The add-in implementing this
            ///  interface can use this object to get the Ice run-time to re-obtain observers for observed objects.
            /// </summary>
            ///  <param name="updater">The observer updater object.</param>

            [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
            void setObserverUpdater(ObserverUpdater updater);

            #endregion
        }
    }
}

namespace Ice
{
    namespace Instrumentation
    {
    }
}
