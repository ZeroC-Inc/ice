// Copyright (c) ZeroC, Inc.

#nullable enable

using Ice.Internal;
using Ice.UtilInternal;
using System.ComponentModel;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Ice;

/// <summary>
/// Base interface of all object proxies.
/// </summary>
public interface ObjectPrx : IEquatable<ObjectPrx>
{
    /// <summary>
    /// Returns the communicator that created this proxy.
    /// </summary>
    /// <returns>The communicator that created this proxy.</returns>
    Communicator ice_getCommunicator();

    /// <summary>
    /// Tests whether this object supports a specific Slice interface.
    /// </summary>
    /// <param name="id">The type ID of the Slice interface to test against.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>True if the target object has the interface specified by id or derives
    /// from the interface specified by id.</returns>
    bool ice_isA(string id, Dictionary<string, string>? context = null);

    /// <summary>
    /// Tests whether this object supports a specific Slice interface.
    /// </summary>
    /// <param name="id">The type ID of the Slice interface to test against.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task<bool> ice_isAAsync(string id,
                            Dictionary<string, string>? context = null,
                            IProgress<bool>? progress = null,
                            CancellationToken cancel = default);

    /// <summary>
    /// Tests whether the target object of this proxy can be reached.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    void ice_ping(Dictionary<string, string>? context = null);

    /// <summary>
    /// Tests whether the target object of this proxy can be reached.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task ice_pingAsync(Dictionary<string, string>? context = null,
                       IProgress<bool>? progress = null,
                       CancellationToken cancel = default);

    /// <summary>
    /// Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>The Slice type IDs of the interfaces supported by the target object, in alphabetical order.
    /// </returns>
    string[] ice_ids(Dictionary<string, string>? context = null);

    /// <summary>
    /// Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task<string[]> ice_idsAsync(Dictionary<string, string>? context = null,
                                IProgress<bool>? progress = null,
                                CancellationToken cancel = default);

    /// <summary>
    /// Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>The Slice type ID of the most-derived interface.</returns>
    string ice_id(Dictionary<string, string>? context = null);

    /// <summary>
    /// Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task<string> ice_idAsync(Dictionary<string, string>? context = null,
                             IProgress<bool>? progress = null,
                             CancellationToken cancel = default);

    /// <summary>
    /// Invokes an operation dynamically.
    /// </summary>
    /// <param name="operation">The name of the operation to invoke.</param>
    /// <param name="mode">The operation mode (normal or idempotent).</param>
    /// <param name="inEncaps">The encoded in-parameters for the operation.</param>
    /// <param name="outEncaps">The encoded out-parameters and return value
    /// for the operation. The return value follows any out-parameters.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>If the operation completed successfully, the return value
    /// is true. If the operation raises a user exception,
    /// the return value is false; in this case, outEncaps
    /// contains the encoded user exception. If the operation raises a run-time exception,
    /// it throws it directly.</returns>
    bool ice_invoke(string operation, OperationMode mode, byte[] inEncaps, out byte[] outEncaps,
                    Dictionary<string, string>? context = null);

    /// <summary>
    /// Invokes an operation dynamically.
    /// </summary>
    /// <param name="operation">The name of the operation to invoke.</param>
    /// <param name="mode">The operation mode (normal or idempotent).</param>
    /// <param name="inEncaps">The encoded in-parameters for the operation.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task<Object_Ice_invokeResult>
    ice_invokeAsync(string operation,
                    OperationMode mode,
                    byte[] inEncaps,
                    Dictionary<string, string>? context = null,
                    IProgress<bool>? progress = null,
                    CancellationToken cancel = default);

    /// <summary>
    /// Returns the identity embedded in this proxy.
    /// <returns>The identity of the target object.</returns>
    /// </summary>
    Identity ice_getIdentity();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the per-proxy context.
    /// <param name="newIdentity">The identity for the new proxy.</param>
    /// <returns>The proxy with the new identity.</returns>
    /// </summary>
    ObjectPrx ice_identity(Identity newIdentity);

    /// <summary>
    /// Returns the per-proxy context for this proxy.
    /// </summary>
    /// <returns>The per-proxy context. If the proxy does not have a per-proxy (implicit) context, the return value
    /// is null.</returns>
    Dictionary<string, string> ice_getContext();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the per-proxy context.
    /// </summary>
    /// <param name="newContext">The context for the new proxy.</param>
    /// <returns>The proxy with the new per-proxy context.</returns>
    ObjectPrx ice_context(Dictionary<string, string> newContext);

    /// <summary>
    /// Returns the facet for this proxy.
    /// </summary>
    /// <returns>The facet for this proxy. If the proxy uses the default facet, the return value is the
    /// empty string.</returns>
    string ice_getFacet();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the facet.
    /// </summary>
    /// <param name="newFacet">The facet for the new proxy.</param>
    /// <returns>The proxy with the new facet.</returns>
    ObjectPrx ice_facet(string newFacet);

    /// <summary>
    /// Returns the adapter ID for this proxy.
    /// </summary>
    /// <returns>The adapter ID. If the proxy does not have an adapter ID, the return value is the
    /// empty string.</returns>
    string ice_getAdapterId();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the adapter ID.
    /// </summary>
    /// <param name="newAdapterId">The adapter ID for the new proxy.</param>
    /// <returns>The proxy with the new adapter ID.</returns>
    ObjectPrx ice_adapterId(string newAdapterId);

    /// <summary>
    /// Returns the endpoints used by this proxy.
    /// </summary>
    /// <returns>The endpoints used by this proxy.</returns>
    Endpoint[] ice_getEndpoints();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the endpoints.
    /// </summary>
    /// <param name="newEndpoints">The endpoints for the new proxy.</param>
    /// <returns>The proxy with the new endpoints.</returns>
    ObjectPrx ice_endpoints(Endpoint[] newEndpoints);

    /// <summary>
    /// Returns the locator cache timeout of this proxy.
    /// </summary>
    /// <returns>The locator cache timeout value (in seconds).</returns>
    int ice_getLocatorCacheTimeout();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the locator cache timeout.
    /// </summary>
    /// <param name="timeout">The new locator cache timeout (in seconds).</param>
    ObjectPrx ice_locatorCacheTimeout(int timeout);

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the invocation timeout.
    /// </summary>
    /// <param name="timeout">The new invocation timeout (in seconds).</param>
    ObjectPrx ice_invocationTimeout(int timeout);

    /// <summary>
    /// Returns the invocation timeout of this proxy.
    /// </summary>
    /// <returns>The invocation timeout value (in seconds).</returns>
    int ice_getInvocationTimeout();

    /// <summary>
    /// Returns whether this proxy caches connections.
    /// </summary>
    /// <returns>True if this proxy caches connections; false, otherwise.</returns>
    bool ice_isConnectionCached();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for connection caching.
    /// </summary>
    /// <param name="newCache">True if the new proxy should cache connections; false, otherwise.</param>
    /// <returns>The new proxy with the specified caching policy.</returns>
    ObjectPrx ice_connectionCached(bool newCache);

    /// <summary>
    /// Returns how this proxy selects endpoints (randomly or ordered).
    /// </summary>
    /// <returns>The endpoint selection policy.</returns>
    EndpointSelectionType ice_getEndpointSelection();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the endpoint selection policy.
    /// </summary>
    /// <param name="newType">The new endpoint selection policy.</param>
    /// <returns>The new proxy with the specified endpoint selection policy.</returns>
    ObjectPrx ice_endpointSelection(EndpointSelectionType newType);

    /// <summary>
    /// Returns whether this proxy communicates only via secure endpoints.
    /// </summary>
    /// <returns>True if this proxy communicates only via secure endpoints; false, otherwise.</returns>
    bool ice_isSecure();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for how it selects endpoints.
    /// </summary>
    /// <param name="b"> If b is true, only endpoints that use a secure transport are
    /// used by the new proxy. If b is false, the returned proxy uses both secure and insecure
    /// endpoints.</param>
    /// <returns>The new proxy with the specified selection policy.</returns>
    ObjectPrx ice_secure(bool b);

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the encoding used to marshal
    /// parameters.
    /// </summary>
    /// <param name="e">The encoding version to use to marshal requests parameters.</param>
    /// <returns>The new proxy with the specified encoding version.</returns>
    ObjectPrx ice_encodingVersion(EncodingVersion e);

    /// <summary>Returns the encoding version used to marshal requests parameters.</summary>
    /// <returns>The encoding version.</returns>
    EncodingVersion ice_getEncodingVersion();

    /// <summary>
    /// Returns whether this proxy prefers secure endpoints.
    /// </summary>
    /// <returns>True if the proxy always attempts to invoke via secure endpoints before it
    /// attempts to use insecure endpoints; false, otherwise.</returns>
    bool ice_isPreferSecure();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its endpoint selection policy.
    /// </summary>
    /// <param name="b">If b is true, the new proxy will use secure endpoints for invocations
    /// and only use insecure endpoints if an invocation cannot be made via secure endpoints. If b is
    /// false, the proxy prefers insecure endpoints to secure ones.</param>
    /// <returns>The new proxy with the new endpoint selection policy.</returns>
    ObjectPrx ice_preferSecure(bool b);

    /// <summary>
    /// Returns the router for this proxy.
    /// </summary>
    /// <returns>The router for the proxy. If no router is configured for the proxy, the return value
    /// is null.</returns>
    RouterPrx? ice_getRouter();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the router.
    /// </summary>
    /// <param name="router">The router for the new proxy.</param>
    /// <returns>The new proxy with the specified router.</returns>
    ObjectPrx ice_router(RouterPrx? router);

    /// <summary>
    /// Returns the locator for this proxy.
    /// </summary>
    /// <returns>The locator for this proxy. If no locator is configured, the return value is null.</returns>
    LocatorPrx? ice_getLocator();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the locator.
    /// </summary>
    /// <param name="locator">The locator for the new proxy.</param>
    /// <returns>The new proxy with the specified locator.</returns>
    ObjectPrx ice_locator(LocatorPrx? locator);

    /// <summary>
    /// Returns whether this proxy uses collocation optimization.
    /// </summary>
    /// <returns>True if the proxy uses collocation optimization; false, otherwise.</returns>
    bool ice_isCollocationOptimized();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for collocation optimization.
    /// </summary>
    /// <param name="b">True if the new proxy enables collocation optimization; false, otherwise.</param>
    /// <returns>The new proxy the specified collocation optimization.</returns>
    ObjectPrx ice_collocationOptimized(bool b);

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses twoway invocations.
    /// </summary>
    /// <returns>A new proxy that uses twoway invocations.</returns>
    ObjectPrx ice_twoway();

    /// <summary>
    /// Returns whether this proxy uses twoway invocations.
    /// </summary>
    /// <returns>True if this proxy uses twoway invocations; false, otherwise.</returns>
    bool ice_isTwoway();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses oneway invocations.
    /// </summary>
    /// <returns>A new proxy that uses oneway invocations.</returns>
    ObjectPrx ice_oneway();

    /// <summary>
    /// Returns whether this proxy uses oneway invocations.
    /// </summary>
    /// <returns>True if this proxy uses oneway invocations; false, otherwise.</returns>
    bool ice_isOneway();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses batch oneway invocations.
    /// </summary>
    /// <returns>A new proxy that uses batch oneway invocations.</returns>
    ObjectPrx ice_batchOneway();

    /// <summary>
    /// Returns whether this proxy uses batch oneway invocations.
    /// </summary>
    /// <returns>True if this proxy uses batch oneway invocations; false, otherwise.</returns>
    bool ice_isBatchOneway();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses datagram invocations.
    /// </summary>
    /// <returns>A new proxy that uses datagram invocations.</returns>
    ObjectPrx ice_datagram();

    /// <summary>
    /// Returns whether this proxy uses datagram invocations.
    /// </summary>
    /// <returns>True if this proxy uses datagram invocations; false, otherwise.</returns>
    bool ice_isDatagram();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses batch datagram invocations.
    /// </summary>
    /// <returns>A new proxy that uses batch datagram invocations.</returns>
    ObjectPrx ice_batchDatagram();

    /// <summary>
    /// Returns whether this proxy uses batch datagram invocations.
    /// </summary>
    /// <returns>True if this proxy uses batch datagram invocations; false, otherwise.</returns>
    bool ice_isBatchDatagram();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for compression.
    /// </summary>
    /// <param name="co">True enables compression for the new proxy; false disables compression.</param>
    /// <returns>A new proxy with the specified compression setting.</returns>
    ObjectPrx ice_compress(bool co);

    /// <summary>
    /// Obtains the compression override setting of this proxy.
    /// </summary>
    /// <returns>The compression override setting. If no optional value is present, no override is
    /// set. Otherwise, true if compression is enabled, false otherwise.</returns>
    bool? ice_getCompress();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its timeout setting.
    /// </summary>
    /// <param name="t">The timeout for the new proxy in milliseconds.</param>
    /// <returns>A new proxy with the specified timeout.</returns>
    ObjectPrx ice_timeout(int t);

    /// <summary>
    /// Obtains the timeout override of this proxy.
    /// </summary>
    /// <returns>The timeout override. If no optional value is present, no override is set. Otherwise,
    /// returns the timeout override value.</returns>
    int? ice_getTimeout();

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its connection ID.
    /// </summary>
    /// <param name="connectionId">The connection ID for the new proxy. An empty string removes the
    /// connection ID.</param>
    /// <returns>A new proxy with the specified connection ID.</returns>
    ObjectPrx ice_connectionId(string connectionId);

    /// <summary>
    /// Returns the connection id of this proxy.
    /// </summary>
    /// <returns>The connection id.</returns>
    string ice_getConnectionId();

    /// <summary>
    /// Returns a proxy that is identical to this proxy, except it's a fixed proxy bound
    /// the given connection.
    /// </summary>
    /// <param name="connection">The fixed proxy connection.</param>
    /// <returns>A fixed proxy bound to the given connection.</returns>
    ObjectPrx ice_fixed(Ice.Connection connection);

    /// <summary>
    /// Returns whether this proxy is a fixed proxy.
    /// </summary>
    /// <returns>True if this is a fixed proxy, false otherwise.
    /// </returns>
    bool ice_isFixed();

    /// <summary>
    /// Returns the Connection for this proxy. If the proxy does not yet have an established connection,
    /// it first attempts to create a connection.
    /// </summary>
    /// <returns>The Connection for this proxy.</returns>
    Connection ice_getConnection();

    /// <summary>
    /// Asynchronously gets the connection for this proxy.
    /// </summary>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task<Connection> ice_getConnectionAsync(IProgress<bool>? progress = null,
                                            CancellationToken cancel = default);

    /// <summary>
    /// Returns the cached Connection for this proxy. If the proxy does not yet have an established
    /// connection, it does not attempt to create a connection.
    /// </summary>
    /// <returns>The cached Connection for this proxy (null if the proxy does not have
    /// an established connection).</returns>
    Connection? ice_getCachedConnection();

    /// <summary>
    /// Flushes any pending batched requests for this proxy. The call blocks until the flush is complete.
    /// </summary>
    void ice_flushBatchRequests();

    /// <summary>
    /// Asynchronously flushes any pending batched requests for this proxy.
    /// </summary>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    Task ice_flushBatchRequestsAsync(IProgress<bool>? progress = null,
                                     CancellationToken cancel = default);

    /// <summary>
    /// Write a proxy to the output stream.
    /// </summary>
    /// <param name="os">Output stream object to write the proxy.</param>
    [EditorBrowsable(EditorBrowsableState.Never)]
    void iceWrite(OutputStream os);

    /// <summary>
    /// Returns a scheduler object that uses the Ice thread pool.
    /// </summary>
    /// <returns>The task scheduler object.</returns>
    TaskScheduler ice_scheduler();
}

/// <summary>
/// Represent the result of the ice_invokeAsync operation
/// </summary>
public record struct Object_Ice_invokeResult(bool returnValue, byte[] outEncaps);

/// <summary>
/// Base class of all object proxies.
/// </summary>
public class ObjectPrxHelperBase : ObjectPrx
{
    public static bool operator ==(ObjectPrxHelperBase? lhs, ObjectPrxHelperBase? rhs) =>
        lhs is not null ? lhs.Equals(rhs) : rhs is null;

    public static bool operator !=(ObjectPrxHelperBase? lhs, ObjectPrxHelperBase? rhs) => !(lhs == rhs);

    // TODO: _reference is initialized by setup and iceCopyFrom. We should refactor this code.
    public ObjectPrxHelperBase() => _reference = null!;

    /// <summary>
    /// Returns whether this proxy equals the passed object. Two proxies are equal if they are equal in all
    /// respects, that is, if their object identity, endpoints timeout settings, and so on are all equal.
    /// </summary>
    /// <param name="r">The proxy to compare this proxy with.</param>
    /// <returns>True if this proxy is equal to r; false, otherwise.</returns>
    public bool Equals(ObjectPrx? other) =>
        other is not null && _reference == ((ObjectPrxHelperBase)other)._reference;

    public override bool Equals(object? other) => Equals(other as ObjectPrx);

    /// <summary>
    /// Returns a hash code for this proxy.
    /// </summary>
    /// <returns>The hash code.</returns>
    public override int GetHashCode()
    {
        return _reference.GetHashCode();
    }

    /// <summary>
    /// Returns the communicator that created this proxy.
    /// </summary>
    /// <returns>The communicator that created this proxy.</returns>
    public Communicator ice_getCommunicator()
    {
        return _reference.getCommunicator();
    }

    /// <summary>
    /// Returns the stringified form of this proxy.
    /// </summary>
    /// <returns>The stringified proxy.</returns>
    public override string ToString()
    {
        return _reference.ToString();
    }

    /// <summary>
    /// Tests whether this object supports a specific Slice interface.
    /// </summary>
    /// <param name="id">The type ID of the Slice interface to test against.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>True if the target object has the interface specified by id or derives
    /// from the interface specified by id.</returns>
    public bool ice_isA(string id, Dictionary<string, string>? context = null)
    {
        try
        {
            return iceI_ice_isAAsync(id, context, null, CancellationToken.None, true).Result;
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    /// <summary>
    /// Tests whether this object supports a specific Slice interface.
    /// </summary>
    /// <param name="id">The type ID of the Slice interface to test against.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    public Task<bool> ice_isAAsync(string id,
                                   Dictionary<string, string>? context = null,
                                   IProgress<bool>? progress = null,
                                   CancellationToken cancel = default)
    {
        return iceI_ice_isAAsync(id, context, progress, cancel, false);
    }

    private Task<bool>
    iceI_ice_isAAsync(string id, Dictionary<string, string>? context, IProgress<bool>? progress, CancellationToken cancel,
                      bool synchronous)
    {
        iceCheckTwowayOnly(_ice_isA_name);
        var completed = new OperationTaskCompletionCallback<bool>(progress, cancel);
        iceI_ice_isA(id, context, completed, synchronous);
        return completed.Task;
    }

    private const string _ice_isA_name = "ice_isA";

    private void iceI_ice_isA(string id,
                              Dictionary<string, string>? context,
                              OutgoingAsyncCompletionCallback completed,
                              bool synchronous)
    {
        iceCheckAsyncTwowayOnly(_ice_isA_name);
        getOutgoingAsync<bool>(completed).invoke(_ice_isA_name,
                                                 OperationMode.Idempotent,
                                                 FormatType.DefaultFormat,
                                                 context,
                                                 synchronous,
                                                 (OutputStream os) => { os.writeString(id); },
                                                 null,
                                                 (InputStream iss) => { return iss.readBool(); });
    }

    /// <summary>
    /// Tests whether the target object of this proxy can be reached.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    public void ice_ping(Dictionary<string, string>? context = null)
    {
        try
        {
            iceI_ice_pingAsync(context, null, CancellationToken.None, true).Wait();
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    /// <summary>
    /// Tests whether the target object of this proxy can be reached.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    public Task ice_pingAsync(Dictionary<string, string>? context = null,
                              IProgress<bool>? progress = null,
                              CancellationToken cancel = default)
    {
        return iceI_ice_pingAsync(context, progress, cancel, false);
    }

    private Task
    iceI_ice_pingAsync(Dictionary<string, string>? context, IProgress<bool>? progress, CancellationToken cancel, bool synchronous)
    {
        var completed = new OperationTaskCompletionCallback<object>(progress, cancel);
        iceI_ice_ping(context, completed, synchronous);
        return completed.Task;
    }

    private const string _ice_ping_name = "ice_ping";

    private void iceI_ice_ping(Dictionary<string, string>? context, OutgoingAsyncCompletionCallback completed,
                                   bool synchronous)
    {
        getOutgoingAsync<object>(completed).invoke(_ice_ping_name,
                                                   OperationMode.Idempotent,
                                                   FormatType.DefaultFormat,
                                                   context,
                                                   synchronous);
    }

    /// <summary>
    /// Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>The Slice type IDs of the interfaces supported by the target object, in alphabetical order.
    /// </returns>
    public string[] ice_ids(Dictionary<string, string>? context = null)
    {
        try
        {
            return iceI_ice_idsAsync(context, null, CancellationToken.None, true).Result;
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    /// <summary>
    /// Returns the Slice type IDs of the interfaces supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    public Task<string[]>
    ice_idsAsync(Dictionary<string, string>? context = null,
                 IProgress<bool>? progress = null,
                 CancellationToken cancel = default)
    {
        return iceI_ice_idsAsync(context, progress, cancel, false);
    }

    private Task<string[]> iceI_ice_idsAsync(Dictionary<string, string>? context, IProgress<bool>? progress, CancellationToken cancel,
                                             bool synchronous)
    {
        iceCheckTwowayOnly(_ice_ids_name);
        var completed = new OperationTaskCompletionCallback<string[]>(progress, cancel);
        iceI_ice_ids(context, completed, synchronous);
        return completed.Task;
    }

    private const string _ice_ids_name = "ice_ids";

    private void iceI_ice_ids(Dictionary<string, string>? context, OutgoingAsyncCompletionCallback completed,
                              bool synchronous)
    {
        iceCheckAsyncTwowayOnly(_ice_ids_name);
        getOutgoingAsync<string[]>(completed).invoke(_ice_ids_name,
                                                     OperationMode.Idempotent,
                                                     FormatType.DefaultFormat,
                                                     context,
                                                     synchronous,
                                                     read: (InputStream iss) => { return iss.readStringSeq(); });
    }

    /// <summary>
    /// Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
    /// </summary>
    /// <returns>The Slice type ID of the most-derived interface.</returns>
    public string ice_id(Dictionary<string, string>? context = null)
    {
        try
        {
            return iceI_ice_idAsync(context, null, CancellationToken.None, true).Result;
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    /// <summary>
    /// Returns the Slice type ID of the most-derived interface supported by the target object of this proxy.
    /// </summary>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    public Task<string> ice_idAsync(Dictionary<string, string>? context = null,
                                    IProgress<bool>? progress = null,
                                    CancellationToken cancel = default)
    {
        return iceI_ice_idAsync(context, progress, cancel, false);
    }

    private Task<string>
    iceI_ice_idAsync(Dictionary<string, string>? context, IProgress<bool>? progress, CancellationToken cancel, bool synchronous)
    {
        iceCheckTwowayOnly(_ice_id_name);
        var completed = new OperationTaskCompletionCallback<string>(progress, cancel);
        iceI_ice_id(context, completed, synchronous);
        return completed.Task;
    }

    private const string _ice_id_name = "ice_id";

    private void iceI_ice_id(Dictionary<string, string>? context,
                             OutgoingAsyncCompletionCallback completed,
                             bool synchronous)
    {
        getOutgoingAsync<string>(completed).invoke(_ice_id_name,
                                                   OperationMode.Idempotent,
                                                   FormatType.DefaultFormat,
                                                   context,
                                                   synchronous,
                                                   read: (InputStream iss) => { return iss.readString(); });
    }

    /// <summary>
    /// Invokes an operation dynamically.
    /// </summary>
    /// <param name="operation">The name of the operation to invoke.</param>
    /// <param name="mode">The operation mode (normal or idempotent).</param>
    /// <param name="inEncaps">The encoded in-parameters for the operation.</param>
    /// <param name="outEncaps">The encoded out-parameters and return value
    /// for the operation. The return value follows any out-parameters.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <returns>If the operation completed successfully, the return value
    /// is true. If the operation raises a user exception,
    /// the return value is false; in this case, outEncaps
    /// contains the encoded user exception. If the operation raises a run-time exception,
    /// it throws it directly.</returns>
    public bool ice_invoke(string operation,
                           OperationMode mode,
                           byte[] inEncaps,
                           out byte[] outEncaps,
                           Dictionary<string, string>? context = null)
    {
        try
        {
            var result = iceI_ice_invokeAsync(operation, mode, inEncaps, context, null, CancellationToken.None, true).Result;
            outEncaps = result.outEncaps;
            return result.returnValue;
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    /// <summary>
    /// Invokes an operation dynamically.
    /// </summary>
    /// <param name="operation">The name of the operation to invoke.</param>
    /// <param name="mode">The operation mode (normal or idempotent).</param>
    /// <param name="inEncaps">The encoded in-parameters for the operation.</param>
    /// <param name="context">The context dictionary for the invocation.</param>
    /// <param name="progress">Sent progress provider.</param>
    /// <param name="cancel">A cancellation token that receives the cancellation requests.</param>
    /// <returns>The task object representing the asynchronous operation.</returns>
    public Task<Object_Ice_invokeResult>
    ice_invokeAsync(string operation,
                    OperationMode mode,
                    byte[] inEncaps,
                    Dictionary<string, string>? context = null,
                    IProgress<bool>? progress = null,
                    CancellationToken cancel = default)
    {
        return iceI_ice_invokeAsync(operation, mode, inEncaps, context, progress, cancel, false);
    }

    private Task<Object_Ice_invokeResult>
    iceI_ice_invokeAsync(string operation,
                         OperationMode mode,
                         byte[] inEncaps,
                         Dictionary<string, string>? context,
                         IProgress<bool>? progress,
                         CancellationToken cancel,
                         bool synchronous)
    {
        var completed = new InvokeTaskCompletionCallback(progress, cancel);
        iceI_ice_invoke(operation, mode, inEncaps, context, completed, synchronous);
        return completed.Task;
    }

    private void iceI_ice_invoke(string operation,
                                 OperationMode mode,
                                 byte[] inEncaps,
                                 Dictionary<string, string>? context,
                                 OutgoingAsyncCompletionCallback completed,
                                 bool synchronous)
    {
        getInvokeOutgoingAsync(completed).invoke(operation, mode, inEncaps, context, synchronous);
    }

    /// <summary>
    /// Returns the identity embedded in this proxy.
    /// <returns>The identity of the target object.</returns>
    /// </summary>
    public Identity ice_getIdentity()
    {
        return (Identity)_reference.getIdentity().Clone();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the per-proxy context.
    /// <param name="newIdentity">The identity for the new proxy.</param>
    /// <returns>The proxy with the new identity.</returns>
    /// </summary>
    public ObjectPrx ice_identity(Identity newIdentity)
    {
        if (newIdentity.name.Length == 0)
        {
            throw new IllegalIdentityException();
        }
        if (newIdentity.Equals(_reference.getIdentity()))
        {
            return this;
        }
        else
        {
            var proxy = new ObjectPrxHelperBase();
            proxy.setup(_reference.changeIdentity(newIdentity));
            return proxy;
        }
    }

    /// <summary>
    /// Returns the per-proxy context for this proxy.
    /// </summary>
    /// <returns>The per-proxy context. If the proxy does not have a per-proxy (implicit) context, the return value
    /// is null.</returns>
    public Dictionary<string, string> ice_getContext()
    {
        return new Dictionary<string, string>(_reference.getContext());
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the per-proxy context.
    /// </summary>
    /// <param name="newContext">The context for the new proxy.</param>
    /// <returns>The proxy with the new per-proxy context.</returns>
    public ObjectPrx ice_context(Dictionary<string, string> newContext)
    {
        return newInstance(_reference.changeContext(newContext));
    }

    /// <summary>
    /// Returns the facet for this proxy.
    /// </summary>
    /// <returns>The facet for this proxy. If the proxy uses the default facet, the return value is the
    /// empty string.</returns>
    public string ice_getFacet()
    {
        return _reference.getFacet();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the facet.
    /// </summary>
    /// <param name="newFacet">The facet for the new proxy.</param>
    /// <returns>The proxy with the new facet.</returns>
    public ObjectPrx ice_facet(string newFacet)
    {
        newFacet ??= "";

        if (newFacet == _reference.getFacet())
        {
            return this;
        }
        else
        {
            var proxy = new ObjectPrxHelperBase();
            proxy.setup(_reference.changeFacet(newFacet));
            return proxy;
        }
    }

    /// <summary>
    /// Returns the adapter ID for this proxy.
    /// </summary>
    /// <returns>The adapter ID. If the proxy does not have an adapter ID, the return value is the
    /// empty string.</returns>
    public string ice_getAdapterId()
    {
        return _reference.getAdapterId();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the adapter ID.
    /// </summary>
    /// <param name="newAdapterId">The adapter ID for the new proxy.</param>
    /// <returns>The proxy with the new adapter ID.</returns>
    public ObjectPrx ice_adapterId(string newAdapterId)
    {
        newAdapterId ??= "";

        if (newAdapterId == _reference.getAdapterId())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeAdapterId(newAdapterId));
        }
    }

    /// <summary>
    /// Returns the endpoints used by this proxy.
    /// </summary>
    /// <returns>The endpoints used by this proxy.</returns>
    public Endpoint[] ice_getEndpoints()
    {
        return (Endpoint[])_reference.getEndpoints().Clone();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the endpoints.
    /// </summary>
    /// <param name="newEndpoints">The endpoints for the new proxy.</param>
    /// <returns>The proxy with the new endpoints.</returns>
    public ObjectPrx ice_endpoints(Endpoint[] newEndpoints)
    {
        if (Arrays.Equals(newEndpoints, _reference.getEndpoints()))
        {
            return this;
        }
        else
        {
            var endpts = new EndpointI[newEndpoints.Length];
            for (int i = 0; i < newEndpoints.Length; ++i)
            {
                endpts[i] = (EndpointI)newEndpoints[i];
            }
            return newInstance(_reference.changeEndpoints(endpts));
        }
    }

    /// <summary>
    /// Returns the locator cache timeout of this proxy.
    /// </summary>
    /// <returns>The locator cache timeout value (in seconds).</returns>
    public int ice_getLocatorCacheTimeout()
    {
        return _reference.getLocatorCacheTimeout();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the locator cache timeout.
    /// </summary>
    /// <param name="newTimeout">The new locator cache timeout (in seconds).</param>
    public ObjectPrx ice_locatorCacheTimeout(int newTimeout)
    {
        if (newTimeout < -1)
        {
            throw new ArgumentException("invalid value passed to ice_locatorCacheTimeout: " + newTimeout);
        }
        if (newTimeout == _reference.getLocatorCacheTimeout())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeLocatorCacheTimeout(newTimeout));
        }
    }

    /// <summary>
    /// Returns the invocation timeout of this proxy.
    /// </summary>
    /// <returns>The invocation timeout value (in seconds).</returns>
    public int ice_getInvocationTimeout()
    {
        return _reference.getInvocationTimeout();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the invocation timeout.
    /// </summary>
    /// <param name="newTimeout">The new invocation timeout (in seconds).</param>
    public ObjectPrx ice_invocationTimeout(int newTimeout)
    {
        if (newTimeout < 1 && newTimeout != -1 && newTimeout != -2)
        {
            throw new ArgumentException("invalid value passed to ice_invocationTimeout: " + newTimeout);
        }
        if (newTimeout == _reference.getInvocationTimeout())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeInvocationTimeout(newTimeout));
        }
    }

    /// <summary>
    /// Returns whether this proxy caches connections.
    /// </summary>
    /// <returns>True if this proxy caches connections; false, otherwise.</returns>
    public bool ice_isConnectionCached()
    {
        return _reference.getCacheConnection();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for connection caching.
    /// </summary>
    /// <param name="newCache">True if the new proxy should cache connections; false, otherwise.</param>
    /// <returns>The new proxy with the specified caching policy.</returns>
    public ObjectPrx ice_connectionCached(bool newCache)
    {
        if (newCache == _reference.getCacheConnection())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeCacheConnection(newCache));
        }
    }

    /// <summary>
    /// Returns how this proxy selects endpoints (randomly or ordered).
    /// </summary>
    /// <returns>The endpoint selection policy.</returns>
    public EndpointSelectionType ice_getEndpointSelection()
    {
        return _reference.getEndpointSelection();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the endpoint selection policy.
    /// </summary>
    /// <param name="newType">The new endpoint selection policy.</param>
    /// <returns>The new proxy with the specified endpoint selection policy.</returns>
    public ObjectPrx ice_endpointSelection(EndpointSelectionType newType)
    {
        if (newType == _reference.getEndpointSelection())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeEndpointSelection(newType));
        }
    }

    /// <summary>
    /// Returns whether this proxy communicates only via secure endpoints.
    /// </summary>
    /// <returns>True if this proxy communicates only vi secure endpoints; false, otherwise.</returns>
    public bool ice_isSecure()
    {
        return _reference.getSecure();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for how it selects endpoints.
    /// </summary>
    /// <param name="b"> If b is true, only endpoints that use a secure transport are
    /// used by the new proxy. If b is false, the returned proxy uses both secure and insecure
    /// endpoints.</param>
    /// <returns>The new proxy with the specified selection policy.</returns>
    public ObjectPrx ice_secure(bool b)
    {
        if (b == _reference.getSecure())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeSecure(b));
        }
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the encoding used to marshal
    /// parameters.
    /// </summary>
    /// <param name="e">The encoding version to use to marshal requests parameters.</param>
    /// <returns>The new proxy with the specified encoding version.</returns>
    public ObjectPrx ice_encodingVersion(EncodingVersion e)
    {
        if (e.Equals(_reference.getEncoding()))
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeEncoding(e));
        }
    }

    /// <summary>Returns the encoding version used to marshal requests parameters.</summary>
    /// <returns>The encoding version.</returns>
    public EncodingVersion ice_getEncodingVersion()
    {
        return _reference.getEncoding();
    }

    /// <summary>
    /// Returns whether this proxy prefers secure endpoints.
    /// </summary>
    /// <returns>True if the proxy always attempts to invoke via secure endpoints before it
    /// attempts to use insecure endpoints; false, otherwise.</returns>
    public bool ice_isPreferSecure()
    {
        return _reference.getPreferSecure();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its endpoint selection policy.
    /// </summary>
    /// <param name="b">If b is true, the new proxy will use secure endpoints for invocations
    /// and only use insecure endpoints if an invocation cannot be made via secure endpoints. If b is
    /// false, the proxy prefers insecure endpoints to secure ones.</param>
    /// <returns>The new proxy with the new endpoint selection policy.</returns>
    public ObjectPrx ice_preferSecure(bool b)
    {
        if (b == _reference.getPreferSecure())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changePreferSecure(b));
        }
    }

    /// <summary>
    /// Returns the router for this proxy.
    /// </summary>
    /// <returns>The router for the proxy. If no router is configured for the proxy, the return value
    /// is null.</returns>
    public RouterPrx? ice_getRouter()
    {
        RouterInfo ri = _reference.getRouterInfo();
        return ri != null ? ri.getRouter() : null;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the router.
    /// </summary>
    /// <param name="router">The router for the new proxy.</param>
    /// <returns>The new proxy with the specified router.</returns>
    public ObjectPrx ice_router(RouterPrx? router)
    {
        Reference @ref = _reference.changeRouter(router);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Returns the locator for this proxy.
    /// </summary>
    /// <returns>The locator for this proxy. If no locator is configured, the return value is null.</returns>
    public LocatorPrx? ice_getLocator()
    {
        var li = _reference.getLocatorInfo();
        return li != null ? li.getLocator() : null;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for the locator.
    /// </summary>
    /// <param name="locator">The locator for the new proxy.</param>
    /// <returns>The new proxy with the specified locator.</returns>
    public ObjectPrx ice_locator(LocatorPrx? locator)
    {
        var @ref = _reference.changeLocator(locator);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Returns whether this proxy uses collocation optimization.
    /// </summary>
    /// <returns>True if the proxy uses collocation optimization; false, otherwise.</returns>
    public bool ice_isCollocationOptimized()
    {
        return _reference.getCollocationOptimized();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for collocation optimization.
    /// </summary>
    /// <param name="b">True if the new proxy enables collocation optimization; false, otherwise.</param>
    /// <returns>The new proxy the specified collocation optimization.</returns>
    public ObjectPrx ice_collocationOptimized(bool b)
    {
        if (b == _reference.getCollocationOptimized())
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeCollocationOptimized(b));
        }
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses twoway invocations.
    /// </summary>
    /// <returns>A new proxy that uses twoway invocations.</returns>
    public ObjectPrx ice_twoway()
    {
        if (_reference.getMode() == Reference.Mode.ModeTwoway)
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeMode(Reference.Mode.ModeTwoway));
        }
    }

    /// <summary>
    /// Returns whether this proxy uses twoway invocations.
    /// </summary>
    /// <returns>True if this proxy uses twoway invocations; false, otherwise.</returns>
    public bool ice_isTwoway()
    {
        return _reference.getMode() == Reference.Mode.ModeTwoway;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses oneway invocations.
    /// </summary>
    /// <returns>A new proxy that uses oneway invocations.</returns>
    public ObjectPrx ice_oneway()
    {
        if (_reference.getMode() == Reference.Mode.ModeOneway)
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeMode(Reference.Mode.ModeOneway));
        }
    }

    /// <summary>
    /// Returns whether this proxy uses oneway invocations.
    /// </summary>
    /// <returns>True if this proxy uses oneway invocations; false, otherwise.</returns>
    public bool ice_isOneway()
    {
        return _reference.getMode() == Reference.Mode.ModeOneway;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses batch oneway invocations.
    /// </summary>
    /// <returns>A new proxy that uses batch oneway invocations.</returns>
    public ObjectPrx ice_batchOneway()
    {
        if (_reference.getMode() == Reference.Mode.ModeBatchOneway)
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeMode(Reference.Mode.ModeBatchOneway));
        }
    }

    /// <summary>
    /// Returns whether this proxy uses batch oneway invocations.
    /// </summary>
    /// <returns>True if this proxy uses batch oneway invocations; false, otherwise.</returns>
    public bool ice_isBatchOneway()
    {
        return _reference.getMode() == Reference.Mode.ModeBatchOneway;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses datagram invocations.
    /// </summary>
    /// <returns>A new proxy that uses datagram invocations.</returns>
    public ObjectPrx ice_datagram()
    {
        if (_reference.getMode() == Reference.Mode.ModeDatagram)
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeMode(Reference.Mode.ModeDatagram));
        }
    }

    /// <summary>
    /// Returns whether this proxy uses datagram invocations.
    /// </summary>
    /// <returns>True if this proxy uses datagram invocations; false, otherwise.</returns>
    public bool ice_isDatagram()
    {
        return _reference.getMode() == Reference.Mode.ModeDatagram;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, but uses batch datagram invocations.
    /// </summary>
    /// <returns>A new proxy that uses batch datagram invocations.</returns>
    public ObjectPrx ice_batchDatagram()
    {
        if (_reference.getMode() == Reference.Mode.ModeBatchDatagram)
        {
            return this;
        }
        else
        {
            return newInstance(_reference.changeMode(Reference.Mode.ModeBatchDatagram));
        }
    }

    /// <summary>
    /// Returns whether this proxy uses batch datagram invocations.
    /// </summary>
    /// <returns>True if this proxy uses batch datagram invocations; false, otherwise.</returns>
    public bool ice_isBatchDatagram()
    {
        return _reference.getMode() == Reference.Mode.ModeBatchDatagram;
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for compression.
    /// </summary>
    /// <param name="co">True enables compression for the new proxy; false disables compression.</param>
    /// <returns>A new proxy with the specified compression setting.</returns>
    public ObjectPrx ice_compress(bool co)
    {
        var @ref = _reference.changeCompress(co);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Obtains the compression override setting of this proxy.
    /// </summary>
    /// <returns>The compression override setting. If no optional value is present, no override is
    /// set. Otherwise, true if compression is enabled, false otherwise.</returns>
    public bool? ice_getCompress()
    {
        return _reference.getCompress();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its timeout setting.
    /// </summary>
    /// <param name="t">The timeout for the new proxy in milliseconds.</param>
    /// <returns>A new proxy with the specified timeout.</returns>
    public ObjectPrx ice_timeout(int t)
    {
        if (t < 1 && t != -1)
        {
            throw new ArgumentException("invalid value passed to ice_timeout: " + t);
        }
        var @ref = _reference.changeTimeout(t);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Obtains the timeout override of this proxy.
    /// </summary>
    /// <returns>The timeout override. If no optional value is present, no override is set. Otherwise,
    /// returns the timeout override value.</returns>
    public int? ice_getTimeout()
    {
        return _reference.getTimeout();
    }

    /// <summary>
    /// Creates a new proxy that is identical to this proxy, except for its connection ID.
    /// </summary>
    /// <param name="connectionId">The connection ID for the new proxy. An empty string removes the
    /// connection ID.</param>
    /// <returns>A new proxy with the specified connection ID.</returns>
    public ObjectPrx ice_connectionId(string connectionId)
    {
        var @ref = _reference.changeConnectionId(connectionId);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Returns the connection id of this proxy.
    /// </summary>
    /// <returns>The connection id.</returns>
    public string ice_getConnectionId()
    {
        return _reference.getConnectionId();
    }

    /// <summary>
    /// Returns a proxy that is identical to this proxy, except it's a fixed proxy bound
    /// the given connection.
    /// </summary>
    /// <param name="connection">The fixed proxy connection.</param>
    /// <returns>A fixed proxy bound to the given connection.</returns>
    public ObjectPrx ice_fixed(Ice.Connection connection)
    {
        if (connection is null)
        {
            throw new ArgumentException("invalid null connection passed to ice_fixed");
        }
        if (!(connection is Ice.ConnectionI))
        {
            throw new ArgumentException("invalid connection passed to ice_fixed");
        }
        var @ref = _reference.changeConnection((Ice.ConnectionI)connection);
        if (@ref.Equals(_reference))
        {
            return this;
        }
        else
        {
            return newInstance(@ref);
        }
    }

    /// <summary>
    /// Returns whether this proxy is a fixed proxy.
    /// </summary>
    /// <returns>True if this is a fixed proxy, false otherwise.
    /// </returns>
    public bool ice_isFixed()
    {
        return _reference is Ice.Internal.FixedReference;
    }

    public class GetConnectionTaskCompletionCallback : TaskCompletionCallback<Connection>
    {
        public GetConnectionTaskCompletionCallback(ObjectPrx proxy,
                                                   IProgress<bool>? progress = null,
                                                   CancellationToken cancellationToken = default) :
            base(progress, cancellationToken)
        {
            _proxy = proxy;
        }

        public override void handleInvokeResponse(bool ok, OutgoingAsyncBase og)
        {
            SetResult(((ProxyGetConnection)og).getConnection());
        }

        private ObjectPrx _proxy;
    }

    /// <summary>
    /// Returns the Connection for this proxy. If the proxy does not yet have an established connection,
    /// it first attempts to create a connection.
    /// </summary>
    /// <returns>The Connection for this proxy.</returns>
    public Connection ice_getConnection()
    {
        try
        {
            var completed = new GetConnectionTaskCompletionCallback(this);
            iceI_ice_getConnection(completed, true);
            return completed.Task.Result;
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    public Task<Connection> ice_getConnectionAsync(IProgress<bool>? progress = null,
                                                   CancellationToken cancel = default)
    {
        var completed = new GetConnectionTaskCompletionCallback(this, progress, cancel);
        iceI_ice_getConnection(completed, false);
        return completed.Task;
    }

    private const string _ice_getConnection_name = "ice_getConnection";

    private void iceI_ice_getConnection(OutgoingAsyncCompletionCallback completed, bool synchronous)
    {
        var outgoing = new ProxyGetConnection(this, completed);
        try
        {
            outgoing.invoke(_ice_getConnection_name, synchronous);
        }
        catch (Exception ex)
        {
            outgoing.abort(ex);
        }
    }

    /// <summary>
    /// Returns the cached Connection for this proxy. If the proxy does not yet have an established
    /// connection, it does not attempt to create a connection.
    /// </summary>
    /// <returns>The cached Connection for this proxy (null if the proxy does not have
    /// an established connection).</returns>
    public Connection? ice_getCachedConnection()
    {
        RequestHandler? handler;
        lock (this)
        {
            handler = _requestHandler;
        }

        if (handler != null)
        {
            try
            {
                return handler.getConnection();
            }
            catch (LocalException)
            {
            }
        }
        return null;
    }

    /// <summary>
    /// Flushes any pending batched requests for this communicator. The call blocks until the flush is complete.
    /// </summary>
    public void ice_flushBatchRequests()
    {
        try
        {
            var completed = new FlushBatchTaskCompletionCallback();
            iceI_ice_flushBatchRequests(completed, true);
            completed.Task.Wait();
        }
        catch (AggregateException ex)
        {
            throw ex.InnerException!;
        }
    }

    internal const string _ice_flushBatchRequests_name = "ice_flushBatchRequests";

    public Task ice_flushBatchRequestsAsync(IProgress<bool>? progress = null,
                                            CancellationToken cancel = default)
    {
        var completed = new FlushBatchTaskCompletionCallback(progress, cancel);
        iceI_ice_flushBatchRequests(completed, false);
        return completed.Task;
    }

    private void iceI_ice_flushBatchRequests(OutgoingAsyncCompletionCallback completed, bool synchronous)
    {
        var outgoing = new ProxyFlushBatchAsync(this, completed);
        try
        {
            outgoing.invoke(_ice_flushBatchRequests_name, synchronous);
        }
        catch (Exception ex)
        {
            outgoing.abort(ex);
        }
    }

    public System.Threading.Tasks.TaskScheduler ice_scheduler()
    {
        return _reference.getThreadPool();
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public void iceWrite(OutputStream os)
    {
        _reference.getIdentity().ice_writeMembers(os);
        _reference.streamWrite(os);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public Reference iceReference()
    {
        return _reference;
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public void iceCopyFrom(ObjectPrx from)
    {
        lock (from)
        {
            var h = (ObjectPrxHelperBase)from;
            _reference = h._reference;
            _requestHandler = h._requestHandler;
        }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public int iceHandleException(Exception ex, RequestHandler handler, OperationMode mode, bool sent,
                                 ref int cnt)
    {
        iceUpdateRequestHandler(handler, null); // Clear the request handler

        //
        // We only retry local exception, system exceptions aren't retried.
        //
        // A CloseConnectionException indicates graceful server shutdown, and is therefore
        // always repeatable without violating "at-most-once". That's because by sending a
        // close connection message, the server guarantees that all outstanding requests
        // can safely be repeated.
        //
        // An ObjectNotExistException can always be retried as well without violating
        // "at-most-once" (see the implementation of the checkRetryAfterException method
        //  of the ProxyFactory class for the reasons why it can be useful).
        //
        // If the request didn't get sent or if it's non-mutating or idempotent it can
        // also always be retried if the retry count isn't reached.
        //
        if (ex is LocalException && (!sent ||
                                    mode == OperationMode.Nonmutating || mode == OperationMode.Idempotent ||
                                    ex is CloseConnectionException ||
                                    ex is ObjectNotExistException))
        {
            try
            {
                return _reference.getInstance().proxyFactory().checkRetryAfterException((LocalException)ex,
                                                                                        _reference,
                                                                                        ref cnt);
            }
            catch (CommunicatorDestroyedException)
            {
                //
                // The communicator is already destroyed, so we cannot retry.
                //
                throw ex;
            }
        }
        else
        {
            throw ex; // Retry could break at-most-once semantics, don't retry.
        }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public void iceCheckTwowayOnly(string name)
    {
        //
        // No mutex lock necessary, there is nothing mutable in this
        // operation.
        //

        if (!ice_isTwoway())
        {
            throw new TwowayOnlyException(name);
        }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public void iceCheckAsyncTwowayOnly(string name)
    {
        //
        // No mutex lock necessary, there is nothing mutable in this
        // operation.
        //

        if (!ice_isTwoway())
        {
            throw new ArgumentException("`" + name + "' can only be called with a twoway proxy");
        }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public RequestHandler iceGetRequestHandler()
    {
        if (_reference.getCacheConnection())
        {
            lock (this)
            {
                if (_requestHandler != null)
                {
                    return _requestHandler;
                }
            }
        }
        return _reference.getRequestHandler(this);
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public BatchRequestQueue
    iceGetBatchRequestQueue()
    {
        lock (this)
        {
            _batchRequestQueue ??= _reference.getBatchRequestQueue();
            return _batchRequestQueue;
        }
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public RequestHandler
    iceSetRequestHandler(RequestHandler handler)
    {
        if (_reference.getCacheConnection())
        {
            lock (this)
            {
                _requestHandler ??= handler;
                return _requestHandler;
            }
        }
        return handler;
    }

    [EditorBrowsable(EditorBrowsableState.Never)]
    public void iceUpdateRequestHandler(RequestHandler? previous, RequestHandler? handler)
    {
        if (_reference.getCacheConnection() && previous != null)
        {
            lock (this)
            {
                if (_requestHandler != null && _requestHandler != handler)
                {
                    //
                    // Update the request handler only if "previous" is the same
                    // as the current request handler. This is called after
                    // connection binding by the connect request handler. We only
                    // replace the request handler if the current handler is the
                    // connect request handler.
                    //
                    _requestHandler = _requestHandler.update(previous, handler);
                }
            }
        }
    }

    protected OutgoingAsyncT<T>
    getOutgoingAsync<T>(OutgoingAsyncCompletionCallback completed)
    {
        bool haveEntry = false;
        InputStream? iss = null;
        OutputStream? os = null;

        if (_reference.getInstance().cacheMessageBuffers() > 0)
        {
            lock (this)
            {
                if (_streamCache != null && _streamCache.Count > 0)
                {
                    haveEntry = true;
                    iss = _streamCache.First!.Value.iss;
                    os = _streamCache.First.Value.os;

                    _streamCache.RemoveFirst();
                }
            }
        }
        if (!haveEntry)
        {
            return new OutgoingAsyncT<T>(this, completed);
        }
        else
        {
            return new OutgoingAsyncT<T>(this, completed, os, iss);
        }
    }

    private class InvokeOutgoingAsyncT : OutgoingAsync
    {
        public InvokeOutgoingAsyncT(ObjectPrxHelperBase prx,
                                    OutgoingAsyncCompletionCallback completionCallback,
                                    OutputStream? os = null,
                                    InputStream? iss = null) : base(prx, completionCallback, os, iss)
        {
        }

        public void invoke(string operation, OperationMode mode, byte[] inParams,
                           Dictionary<string, string>? context, bool synchronous)
        {
            try
            {
                prepare(operation, mode, context);
                if (inParams is null || inParams.Length == 0)
                {
                    os_.writeEmptyEncapsulation(encoding_);
                }
                else
                {
                    os_.writeEncapsulation(inParams);
                }
                invoke(operation, synchronous);
            }
            catch (Exception ex)
            {
                abort(ex);
            }
        }

        public Object_Ice_invokeResult
        getResult(bool ok)
        {
            try
            {
                var ret = new Object_Ice_invokeResult();
                EncodingVersion encoding;
                if (proxy_.iceReference().getMode() == Reference.Mode.ModeTwoway)
                {
                    ret.outEncaps = is_.readEncapsulation(out encoding);
                }
                else
                {
                    ret.outEncaps = [];
                }
                ret.returnValue = ok;
                return ret;
            }
            finally
            {
                cacheMessageBuffers();
            }
        }
    }

    private class InvokeTaskCompletionCallback : TaskCompletionCallback<Object_Ice_invokeResult>
    {
        public InvokeTaskCompletionCallback(IProgress<bool>? progress, CancellationToken cancellationToken) :
            base(progress, cancellationToken)
        {
        }

        public override void handleInvokeSent(bool sentSynchronously, bool done, bool alreadySent,
                                              OutgoingAsyncBase og)
        {
            if (progress_ != null && !alreadySent)
            {
                progress_.Report(sentSynchronously);
            }
            if (done)
            {
                SetResult(new Object_Ice_invokeResult(true, []));
            }
        }

        public override void handleInvokeResponse(bool ok, OutgoingAsyncBase og)
        {
            SetResult(((InvokeOutgoingAsyncT)og).getResult(ok));
        }
    }

    private InvokeOutgoingAsyncT
    getInvokeOutgoingAsync(OutgoingAsyncCompletionCallback completed)
    {
        bool haveEntry = false;
        InputStream? iss = null;
        OutputStream? os = null;

        if (_reference.getInstance().cacheMessageBuffers() > 0)
        {
            lock (this)
            {
                if (_streamCache != null && _streamCache.Count > 0)
                {
                    haveEntry = true;
                    iss = _streamCache.First!.Value.iss;
                    os = _streamCache.First.Value.os;

                    _streamCache.RemoveFirst();
                }
            }
        }
        if (!haveEntry)
        {
            return new InvokeOutgoingAsyncT(this, completed);
        }
        else
        {
            return new InvokeOutgoingAsyncT(this, completed, os, iss);
        }
    }

    /// <summary>
    /// Only for internal use by OutgoingAsync
    /// </summary>
    /// <param name="iss"></param>
    /// <param name="os"></param>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void
    cacheMessageBuffers(InputStream iss, OutputStream os)
    {
        lock (this)
        {
            _streamCache ??= new LinkedList<StreamCacheEntry>();
            StreamCacheEntry cacheEntry;
            cacheEntry.iss = iss;
            cacheEntry.os = os;
            _streamCache.AddLast(cacheEntry);
        }
    }

    /// <summary>
    /// Only for internal use by ProxyFactory
    /// </summary>
    /// <param name="ref"></param>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void setup(Reference @ref)
    {
        //
        // No need to synchronize, as this operation is only called
        // upon initial initialization.
        //
        Debug.Assert(_reference is null);
        Debug.Assert(_requestHandler is null);

        _reference = @ref;
    }

    private ObjectPrxHelperBase newInstance(Reference @ref)
    {
        var proxy = (ObjectPrxHelperBase)System.Activator.CreateInstance(GetType())!;
        proxy.setup(@ref);
        return proxy;
    }

    private Reference _reference;
    private RequestHandler? _requestHandler;
    private BatchRequestQueue? _batchRequestQueue;
    private struct StreamCacheEntry
    {
        public InputStream iss;
        public OutputStream os;
    }

    private LinkedList<StreamCacheEntry>? _streamCache;
}

/// <summary>
/// Base class for all proxy helpers.
/// </summary>
public class ObjectPrxHelper : ObjectPrxHelperBase
{
    /// <summary>
    /// Creates a new proxy that implements <see cref="ObjectPrx" />
    /// <summary>
    /// <param name="communicator">The communicator of the new proxy.</param>
    /// <param name="proxyString">The string representation of the proxy.</param>
    /// <returns>The new proxy.</returns>
    /// <exception name="ProxyParseException">Thrown when <paramref name="proxyString" /> is not a valid proxy string.
    /// </exception>
    public static ObjectPrx createProxy(Communicator communicator, string proxyString)
    {
        // TODO: rework this implementation
        if (proxyString.Length == 0)
        {
            throw new ProxyParseException("Invalid empty proxy string.");
        }
        return communicator.stringToProxy(proxyString);
    }

    /// Casts a proxy to {@link ObjectPrx}. This call contacts
    /// the server and throws an Ice run-time exception if the target
    /// object does not exist or the server cannot be reached.
    /// </summary>
    /// <param name="b">The proxy to cast to ObjectPrx.</param>
    /// <param name="ctx">The Context map for the invocation.</param>
    /// <returns>b.</returns>
    public static ObjectPrx? checkedCast(ObjectPrx? b, Dictionary<string, string>? context = null)
    {
        if (b is not null && b.ice_isA("::Ice::Object", context))
        {
            return b;
        }
        return null;
    }

    /// <summary>
    /// Creates a new proxy that is identical to the passed proxy, except
    /// for its facet. This call contacts
    /// the server and throws an Ice run-time exception if the target
    /// object does not exist, the specified facet does not exist, or the server cannot be reached.
    /// </summary>
    /// <param name="b">The proxy to cast to ObjectPrx.</param>
    /// <param name="f">The facet for the new proxy.</param>
    /// <param name="context">The Context map for the invocation.</param>
    /// <returns>The new proxy with the specified facet.</returns>
    public static ObjectPrx? checkedCast(ObjectPrx? b, string f, Dictionary<string, string>? context = null)
    {
        ObjectPrx? bb = b?.ice_facet(f);
        try
        {
            if (bb is not null && bb.ice_isA("::Ice::Object", context))
            {
                return bb;
            }
        }
        catch (FacetNotExistException)
        {
        }
        return null;
    }

    /// <summary>
    /// Casts a proxy to {@link ObjectPrx}. This call does
    /// not contact the server and always succeeds.
    /// </summary>
    /// <param name="b">The proxy to cast to ObjectPrx.</param>
    /// <returns>b.</returns>
    [return: NotNullIfNotNull("b")]
    public static ObjectPrx? uncheckedCast(ObjectPrx? b) => b;

    /// <summary>
    /// Creates a new proxy that is identical to the passed proxy, except
    /// for its facet. This call does not contact the server and always succeeds.
    /// </summary>
    /// <param name="b">The proxy to cast to ObjectPrx.</param>
    /// <param name="f">The facet for the new proxy.</param>
    /// <returns>The new proxy with the specified facet.</returns>
    [return: NotNullIfNotNull("b")]
    public static ObjectPrx? uncheckedCast(ObjectPrx? b, string f) => b?.ice_facet(f);

    /// <summary>
    /// Returns the Slice type id of the interface or class associated
    /// with this proxy class.
    /// </summary>
    /// <returns>The type id, "::Ice::Object".</returns>
    public static string ice_staticId()
    {
        return ObjectImpl.ice_staticId();
    }
}
