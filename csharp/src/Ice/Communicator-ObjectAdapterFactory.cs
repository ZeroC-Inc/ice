//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    public sealed partial class Communicator
    {
        /// <summary>Shuts down this communicator's server functionality. This triggers the deactivation of all
        /// object adapters. After this method returns, no new requests are processed. However, requests that have
        /// been started before ShutdownAsync was called might still be active until the returned task completes. You
        /// can also await the task returned by <see cref="WaitForShutdownAsync"/> to wait for the completion of all
        /// requests.</summary>
        public async Task ShutdownAsync()
        {
            lock (_mutex)
            {
                _shutdownTask ??= PerformShutdownAsync(new List<ObjectAdapter>(_adapters));
            }
            await _shutdownTask.ConfigureAwait(false);

            async Task PerformShutdownAsync(List<ObjectAdapter> adapters)
            {
                try
                {
                    // Deactivate outside the lock to avoid deadlocks, _adapters are immutable at this point.
                    await Task.WhenAll(
                        adapters.Select(adapter => adapter.DisposeAsync().AsTask())).ConfigureAwait(false);
                }
                finally
                {
                    // Don't call SetResult directly to avoid continuations running synchronously
                    _ = Task.Run(() => _waitForShutdownCompletionSource?.SetResult(null));
                }
            }
        }

        /// <summary>Block the calling thread until the communicator has been shutdown. On the server side, the
        /// operation completes once all executing operations have completed. On the client side, it completes once
        /// <see cref="ShutdownAsync"/> has been called. A typical use of this method is to call it from the main
        /// thread of a server, which will be completed once the shutdown process completes, and then the caller can
        /// do some cleanup work before calling <see cref="Dispose"/> to dispose the runtime and finally exists the
        /// application.</summary>
        public void WaitForShutdown()
        {
            try
            {
                WaitForShutdownAsync().Wait();
            }
            catch (AggregateException ex)
            {
                Debug.Assert(ex.InnerException != null);
                throw ExceptionUtil.Throw(ex.InnerException);
            }
        }

        /// <summary>Returns a task that completes after the communicator has been shutdown. On the server side, the
        /// task returned by this operation completes once all executing operations have completed. On the client side,
        /// the task simply completes once <see cref="ShutdownAsync"/> has been called. A typical use of this method is
        /// to await the returned task from the main thread of a server, which will be completed once the shutdown
        /// process completes, and then the caller can do some cleanup work before calling <see cref="Dispose"/> to
        /// dispose the runtime and finally exists the application.</summary>
        public async Task WaitForShutdownAsync()
        {
            Task shutdownTask;
            lock (_mutex)
            {
                if (_shutdownTask == null)
                {
                    _waitForShutdownCompletionSource ??= new TaskCompletionSource<object?>();
                    shutdownTask = _waitForShutdownCompletionSource.Task;
                }
                else
                {
                    shutdownTask = _shutdownTask;
                }
            }
            await shutdownTask.ConfigureAwait(false);
        }

        /// <summary>Creates a new object adapter. The communicator uses the object adapter's name to lookup its
        /// properties, such as name.Endpoints.</summary>
        /// <param name="name">The object adapter name. Cannot be empty.</param>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapter(
            string name,
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null) =>
            AddObjectAdapter(name, serializeDispatch, taskScheduler);

        /// <summary>Creates a new nameless object adapter. Such an object adapter has no configuration and can be
        /// associated with a bi-directional connection.</summary>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapter(
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null) =>
            AddObjectAdapter(serializeDispatch: serializeDispatch, taskScheduler: taskScheduler);

        /// <summary>Creates a new object adapter with the specified endpoint string. Calling this method is equivalent
        /// to setting the name.Endpoints property and then calling
        /// <see cref="CreateObjectAdapter(string, bool, TaskScheduler?)"/>.</summary>
        /// <param name="name">The object adapter name. Cannot be empty.</param>
        /// <param name="endpoints">The endpoint string for the object adapter.</param>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapterWithEndpoints(string name, string endpoints,
            bool serializeDispatch = false, TaskScheduler? taskScheduler = null)
        {
            if (name.Length == 0)
            {
                throw new ArgumentException("the empty string is not a valid object adapter name", nameof(name));
            }

            SetProperty($"{name}.Endpoints", endpoints);
            return AddObjectAdapter(name, serializeDispatch, taskScheduler);
        }

        /// <summary>Creates a new object adapter with the specified endpoint string. This method generates a UUID for
        /// the object adapter name and then calls
        /// <see cref="CreateObjectAdapterWithEndpoints(string, string, bool, TaskScheduler?)"/>.</summary>
        /// <param name="endpoints">The endpoint string for the object adapter.</param>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapterWithEndpoints(
            string endpoints,
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null) =>
            CreateObjectAdapterWithEndpoints(Guid.NewGuid().ToString(), endpoints, serializeDispatch, taskScheduler);

        /// <summary>Creates a new object adapter with the specified router proxy. Calling this method is equivalent
        /// to setting the name.Router property and then calling
        /// <see cref="CreateObjectAdapter(string, bool, TaskScheduler?)"/>.</summary>
        /// <param name="name">The object adapter name. Cannot be empty.</param>
        /// <param name="router">The proxy to the router.</param>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapterWithRouter(
            string name,
            IRouterPrx router,
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null)
        {
            if (name.Length == 0)
            {
                throw new ArgumentException("the empty string is not a valid object adapter name", nameof(name));
            }

            // We set the proxy properties here, although we still use the proxy supplied.
            Dictionary<string, string> properties = router.ToProperty($"{name}.Router");
            foreach (KeyValuePair<string, string> entry in properties)
            {
                SetProperty(entry.Key, entry.Value);
            }

            return AddObjectAdapter(name, serializeDispatch, taskScheduler, router);
        }

        /// <summary>Creates a new object adapter with the specified router proxy. This method generates a UUID for
        /// the object adapter name and then calls
        /// <see cref="CreateObjectAdapterWithRouter(string, IRouterPrx, bool, TaskScheduler?)"/>.</summary>
        /// <param name="router">The proxy to the router.</param>
        /// <param name="serializeDispatch">Indicates whether or not this object adapter serializes the dispatching of
        /// of requests received over the same connection.</param>
        /// <param name="taskScheduler">The optional task scheduler to use for dispatching requests.</param>
        /// <returns>The new object adapter.</returns>
        public ObjectAdapter CreateObjectAdapterWithRouter(
            IRouterPrx router,
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null) =>
            CreateObjectAdapterWithRouter(Guid.NewGuid().ToString(), router, serializeDispatch, taskScheduler);

        internal ObjectAdapter? FindObjectAdapter(Reference reference)
        {
            List<ObjectAdapter> adapters;
            lock (_mutex)
            {
                if (IsDisposed)
                {
                    throw new CommunicatorDisposedException();
                }
                adapters = new List<ObjectAdapter>(_adapters);
            }

            foreach (ObjectAdapter adapter in adapters)
            {
                try
                {
                    if (adapter.IsLocal(reference))
                    {
                        return adapter;
                    }
                }
                catch (ObjectDisposedException)
                {
                    // Ignore.
                }
            }

            return null;
        }

        internal void RemoveObjectAdapter(ObjectAdapter adapter)
        {
            // Called by the object adapter to remove itself once destroyed.
            lock (_mutex)
            {
                _adapters.Remove(adapter);
                if (adapter.Name.Length > 0)
                {
                    _adapterNamesInUse.Remove(adapter.Name);
                }
            }
        }

        private ObjectAdapter AddObjectAdapter(
            string? name = null,
            bool serializeDispatch = false,
            TaskScheduler? taskScheduler = null,
            IRouterPrx? router = null)
        {
            if (name != null && name.Length == 0)
            {
                throw new ArgumentException("the empty string is not a valid object adapter name", nameof(name));
            }

            lock (_mutex)
            {
                if (IsDisposed)
                {
                    throw new CommunicatorDisposedException();
                }

                if (name != null)
                {
                    if (_adapterNamesInUse.Contains(name))
                    {
                        throw new ArgumentException($"an object adapter with name `{name}' is already registered",
                            nameof(name));
                    }
                    _adapterNamesInUse.Add(name);
                }
            }

            // Must be called outside the synchronization since the constructor can make client invocations
            // on the router if it's set.
            ObjectAdapter adapter;
            try
            {
                adapter = new ObjectAdapter(this, name ?? "", serializeDispatch, taskScheduler, router);
            }
            catch
            {
                if (name != null)
                {
                    lock (_mutex)
                    {
                        _adapterNamesInUse.Remove(name);
                    }
                }
                throw;
            }

            lock (_mutex)
            {
                if (IsDisposed)
                {
                    adapter.Dispose();
                    throw new CommunicatorDisposedException();
                }
                _adapters.Add(adapter);
                return adapter;
            }
        }
    }
}
