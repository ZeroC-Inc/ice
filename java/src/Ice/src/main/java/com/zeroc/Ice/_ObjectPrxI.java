//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

import com.zeroc.IceInternal.OutgoingAsync;
import com.zeroc.IceInternal.Reference;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/** Concrete proxy implementation. */
public class _ObjectPrxI implements ObjectPrx, java.io.Serializable {
  protected _ObjectPrxI(Reference ref) {
    _reference = ref;
  }

  protected _ObjectPrxI(ObjectPrx obj) {
    _ObjectPrxI source = (_ObjectPrxI) obj;
    _reference = source._reference;
    _requestHandler = source._requestHandler;
  }

  public Communicator ice_getCommunicator() {
    return _reference.getCommunicator();
  }

  public boolean ice_isA(String id) {
    return ice_isA(id, ObjectPrx.noExplicitContext);
  }

  public boolean ice_isA(String id, Map<String, String> context) {
    return _iceI_ice_isAAsync(id, context, true).waitForResponse();
  }

  public CompletableFuture<Boolean> ice_isAAsync(String id) {
    return _iceI_ice_isAAsync(id, ObjectPrx.noExplicitContext, false);
  }

  public CompletableFuture<Boolean> ice_isAAsync(String id, Map<String, String> context) {
    return _iceI_ice_isAAsync(id, context, false);
  }

  private OutgoingAsync<Boolean> _iceI_ice_isAAsync(
      String id, Map<String, String> context, boolean sync) {
    OutgoingAsync<Boolean> f =
        new OutgoingAsync<>(this, "ice_isA", OperationMode.Idempotent, sync, null);
    f.invoke(true, context, null, ostr -> ostr.writeString(id), istr -> istr.readBool());
    return f;
  }

  public void ice_ping() {
    ice_ping(ObjectPrx.noExplicitContext);
  }

  public void ice_ping(Map<String, String> context) {
    _iceI_ice_pingAsync(context, true).waitForResponse();
  }

  public CompletableFuture<Void> ice_pingAsync() {
    return _iceI_ice_pingAsync(ObjectPrx.noExplicitContext, false);
  }

  public CompletableFuture<Void> ice_pingAsync(Map<String, String> context) {
    return _iceI_ice_pingAsync(context, false);
  }

  private OutgoingAsync<Void> _iceI_ice_pingAsync(Map<String, String> context, boolean sync) {
    OutgoingAsync<Void> f =
        new OutgoingAsync<>(this, "ice_ping", OperationMode.Idempotent, sync, null);
    f.invoke(false, context, null, null, null);
    return f;
  }

  public String[] ice_ids() {
    return ice_ids(ObjectPrx.noExplicitContext);
  }

  public String[] ice_ids(Map<String, String> context) {
    return _iceI_ice_idsAsync(context, true).waitForResponse();
  }

  public CompletableFuture<String[]> ice_idsAsync() {
    return _iceI_ice_idsAsync(ObjectPrx.noExplicitContext, false);
  }

  public CompletableFuture<String[]> ice_idsAsync(Map<String, String> context) {
    return _iceI_ice_idsAsync(context, false);
  }

  private OutgoingAsync<String[]> _iceI_ice_idsAsync(Map<String, String> context, boolean sync) {
    OutgoingAsync<String[]> f =
        new OutgoingAsync<>(this, "ice_ids", OperationMode.Idempotent, sync, null);
    f.invoke(true, context, null, null, istr -> istr.readStringSeq());
    return f;
  }

  public String ice_id() {
    return ice_id(ObjectPrx.noExplicitContext);
  }

  public String ice_id(Map<String, String> context) {
    return _iceI_ice_idAsync(context, true).waitForResponse();
  }

  public CompletableFuture<String> ice_idAsync() {
    return _iceI_ice_idAsync(ObjectPrx.noExplicitContext, false);
  }

  public CompletableFuture<String> ice_idAsync(Map<String, String> context) {
    return _iceI_ice_idAsync(context, false);
  }

  private OutgoingAsync<String> _iceI_ice_idAsync(Map<String, String> context, boolean sync) {
    OutgoingAsync<String> f =
        new OutgoingAsync<>(this, "ice_id", OperationMode.Idempotent, sync, null);
    f.invoke(true, context, null, null, istr -> istr.readString());
    return f;
  }

  public Object.Ice_invokeResult ice_invoke(String operation, OperationMode mode, byte[] inParams) {
    return ice_invoke(operation, mode, inParams, ObjectPrx.noExplicitContext);
  }

  public Object.Ice_invokeResult ice_invoke(
      String operation, OperationMode mode, byte[] inParams, Map<String, String> context) {
    return _iceI_ice_invokeAsync(operation, mode, inParams, context, true).waitForResponse();
  }

  public CompletableFuture<Object.Ice_invokeResult> ice_invokeAsync(
      String operation, OperationMode mode, byte[] inParams) {
    return ice_invokeAsync(operation, mode, inParams, ObjectPrx.noExplicitContext);
  }

  public CompletableFuture<Object.Ice_invokeResult> ice_invokeAsync(
      String operation, OperationMode mode, byte[] inParams, Map<String, String> context) {
    return _iceI_ice_invokeAsync(operation, mode, inParams, context, false);
  }

  private com.zeroc.IceInternal.ProxyIceInvoke _iceI_ice_invokeAsync(
      String operation,
      OperationMode mode,
      byte[] inParams,
      Map<String, String> context,
      boolean sync) {
    com.zeroc.IceInternal.ProxyIceInvoke f =
        new com.zeroc.IceInternal.ProxyIceInvoke(this, operation, mode, sync);
    f.invoke(inParams, context);
    return f;
  }

  public Identity ice_getIdentity() {
    return _reference.getIdentity().clone();
  }

  public ObjectPrx ice_identity(Identity newIdentity) {
    if (newIdentity.name == null || newIdentity.name.isEmpty()) {
      throw new IllegalIdentityException();
    }
    if (newIdentity.equals(_reference.getIdentity())) {
      return this;
    } else {
      return new _ObjectPrxI(_reference.changeIdentity(newIdentity));
    }
  }

  public Map<String, String> ice_getContext() {
    return new java.util.HashMap<>(_reference.getContext());
  }

  public String ice_getFacet() {
    return _reference.getFacet();
  }

  public ObjectPrx ice_facet(String newFacet) {
    if (newFacet == null) {
      newFacet = "";
    }

    if (newFacet.equals(_reference.getFacet())) {
      return this;
    } else {
      return new _ObjectPrxI(_reference.changeFacet(newFacet));
    }
  }

  public String ice_getAdapterId() {
    return _reference.getAdapterId();
  }

  public Endpoint[] ice_getEndpoints() {
    return _reference.getEndpoints().clone();
  }

  public int ice_getLocatorCacheTimeout() {
    return _reference.getLocatorCacheTimeout();
  }

  public int ice_getInvocationTimeout() {
    return _reference.getInvocationTimeout();
  }

  public String ice_getConnectionId() {
    return _reference.getConnectionId();
  }

  public boolean ice_isConnectionCached() {
    return _reference.getCacheConnection();
  }

  public EndpointSelectionType ice_getEndpointSelection() {
    return _reference.getEndpointSelection();
  }

  public boolean ice_isFixed() {
    return _reference instanceof com.zeroc.IceInternal.FixedReference;
  }

  public boolean ice_isSecure() {
    return _reference.getSecure();
  }

  public EncodingVersion ice_getEncodingVersion() {
    return _reference.getEncoding().clone();
  }

  public boolean ice_isPreferSecure() {
    return _reference.getPreferSecure();
  }

  public RouterPrx ice_getRouter() {
    com.zeroc.IceInternal.RouterInfo ri = _reference.getRouterInfo();
    return ri != null ? ri.getRouter() : null;
  }

  public LocatorPrx ice_getLocator() {
    com.zeroc.IceInternal.LocatorInfo ri = _reference.getLocatorInfo();
    return ri != null ? ri.getLocator() : null;
  }

  public boolean ice_isCollocationOptimized() {
    return _reference.getCollocationOptimized();
  }

  public boolean ice_isTwoway() {
    return _reference.getMode() == Reference.ModeTwoway;
  }

  public boolean ice_isOneway() {
    return _reference.getMode() == Reference.ModeOneway;
  }

  public boolean ice_isBatchOneway() {
    return _reference.getMode() == Reference.ModeBatchOneway;
  }

  public boolean ice_isDatagram() {
    return _reference.getMode() == Reference.ModeDatagram;
  }

  public boolean ice_isBatchDatagram() {
    return _reference.getMode() == Reference.ModeBatchDatagram;
  }

  public java.util.Optional<Boolean> ice_getCompress() {
    return _reference.getCompress();
  }

  public Connection ice_getConnection() {
    return _iceI_ice_getConnectionAsync().waitForResponse();
  }

  public CompletableFuture<Connection> ice_getConnectionAsync() {
    return _iceI_ice_getConnectionAsync();
  }

  private com.zeroc.IceInternal.ProxyGetConnection _iceI_ice_getConnectionAsync() {
    com.zeroc.IceInternal.ProxyGetConnection r = new com.zeroc.IceInternal.ProxyGetConnection(this);
    r.invoke();
    return r;
  }

  public Connection ice_getCachedConnection() {
    com.zeroc.IceInternal.RequestHandler handler = null;
    synchronized (this) {
      handler = _requestHandler;
    }

    if (handler != null) {
      try {
        return handler.getConnection();
      } catch (LocalException ex) {
      }
    }
    return null;
  }

  public void ice_flushBatchRequests() {
    _iceI_ice_flushBatchRequestsAsync().waitForResponse();
  }

  public CompletableFuture<Void> ice_flushBatchRequestsAsync() {
    return _iceI_ice_flushBatchRequestsAsync();
  }

  private com.zeroc.IceInternal.ProxyFlushBatch _iceI_ice_flushBatchRequestsAsync() {
    com.zeroc.IceInternal.ProxyFlushBatch f = new com.zeroc.IceInternal.ProxyFlushBatch(this);
    try {
      f.invoke();
    } catch (Exception ex) {
      f.abort(ex);
    }
    return f;
  }

  @Override
  public boolean equals(java.lang.Object r) {
    if (this == r) {
      return true;
    }

    if (r instanceof _ObjectPrxI) {
      return _reference.equals(((_ObjectPrxI) r)._reference);
    }

    return false;
  }

  @Override
  public final int hashCode() {
    return _reference.hashCode();
  }

  @Override
  public final String toString() {
    return _reference.toString();
  }

  @Override
  public void _write(OutputStream os) {
    _reference.getIdentity().ice_writeMembers(os);
    _reference.streamWrite(os);
  }

  @Override
  public Reference _getReference() {
    return _reference;
  }

  @Override
  public ObjectPrx ice_context(java.util.Map<String, String> newContext) {
    return _newInstance(_reference.changeContext(newContext));
  }

  @Override
  public ObjectPrx ice_adapterId(String newAdapterId) {
    if (newAdapterId == null) {
      newAdapterId = "";
    }

    if (newAdapterId.equals(_reference.getAdapterId())) {
      return this;
    } else {
      return _newInstance(_reference.changeAdapterId(newAdapterId));
    }
  }

  @Override
  public ObjectPrx ice_endpoints(Endpoint[] newEndpoints) {
    if (java.util.Arrays.equals(newEndpoints, _reference.getEndpoints())) {
      return this;
    } else {
      com.zeroc.IceInternal.EndpointI[] edpts =
          new com.zeroc.IceInternal.EndpointI[newEndpoints.length];
      edpts = java.util.Arrays.asList(newEndpoints).toArray(edpts);
      return _newInstance(_reference.changeEndpoints(edpts));
    }
  }

  @Override
  public ObjectPrx ice_fixed(com.zeroc.Ice.Connection connection) {
    if (connection == null) {
      throw new IllegalArgumentException("invalid null connection passed to ice_fixed");
    }
    if (!(connection instanceof com.zeroc.Ice.ConnectionI)) {
      throw new IllegalArgumentException("invalid connection passed to ice_fixed");
    }
    if (connection == _reference.getConnection()) {
      return this;
    } else {
      return _newInstance(_reference.changeConnection((com.zeroc.Ice.ConnectionI) connection));
    }
  }

  @Override
  public ObjectPrx ice_locatorCacheTimeout(int newTimeout) {
    if (newTimeout < -1) {
      throw new IllegalArgumentException(
          "invalid value passed to ice_locatorCacheTimeout: " + newTimeout);
    }
    if (newTimeout == _reference.getLocatorCacheTimeout()) {
      return this;
    } else {
      return _newInstance(_reference.changeLocatorCacheTimeout(newTimeout));
    }
  }

  @Override
  public ObjectPrx ice_invocationTimeout(int newTimeout) {
    if (newTimeout < 1 && newTimeout != -1) {
      throw new IllegalArgumentException(
          "invalid value passed to ice_invocationTimeout: " + newTimeout);
    }
    if (newTimeout == _reference.getInvocationTimeout()) {
      return this;
    } else {
      return _newInstance(_reference.changeInvocationTimeout(newTimeout));
    }
  }

  @Override
  public ObjectPrx ice_connectionCached(boolean newCache) {
    if (newCache == _reference.getCacheConnection()) {
      return this;
    } else {
      return _newInstance(_reference.changeCacheConnection(newCache));
    }
  }

  @Override
  public ObjectPrx ice_endpointSelection(EndpointSelectionType newType) {
    if (newType == _reference.getEndpointSelection()) {
      return this;
    } else {
      return _newInstance(_reference.changeEndpointSelection(newType));
    }
  }

  @Override
  public ObjectPrx ice_secure(boolean b) {
    if (b == _reference.getSecure()) {
      return this;
    } else {
      return _newInstance(_reference.changeSecure(b));
    }
  }

  @Override
  public ObjectPrx ice_encodingVersion(EncodingVersion e) {
    if (e.equals(_reference.getEncoding())) {
      return this;
    } else {
      return _newInstance(_reference.changeEncoding(e));
    }
  }

  @Override
  public ObjectPrx ice_preferSecure(boolean b) {
    if (b == _reference.getPreferSecure()) {
      return this;
    } else {
      return _newInstance(_reference.changePreferSecure(b));
    }
  }

  @Override
  public ObjectPrx ice_router(RouterPrx newRouter) {
    com.zeroc.IceInternal.RouterInfo routerInfo = _reference.getRouterInfo();
    RouterPrx router = routerInfo != null ? routerInfo.getRouter() : null;
    if (router == newRouter || (router != null && router.equals(newRouter))) {
      return this;
    } else {
      return _newInstance(_reference.changeRouter(newRouter));
    }
  }

  @Override
  public ObjectPrx ice_locator(LocatorPrx newLocator) {
    com.zeroc.IceInternal.LocatorInfo locatorInfo = _reference.getLocatorInfo();
    LocatorPrx locator = locatorInfo != null ? locatorInfo.getLocator() : null;
    if (locator == newLocator || (locator != null && locator.equals(newLocator))) {
      return this;
    } else {
      return _newInstance(_reference.changeLocator(newLocator));
    }
  }

  @Override
  public ObjectPrx ice_collocationOptimized(boolean b) {
    if (b == _reference.getCollocationOptimized()) {
      return this;
    } else {
      return _newInstance(_reference.changeCollocationOptimized(b));
    }
  }

  @Override
  public ObjectPrx ice_twoway() {
    return ice_isTwoway() ? this : _newInstance(_reference.changeMode(Reference.ModeTwoway));
  }

  @Override
  public ObjectPrx ice_oneway() {
    return ice_isOneway() ? this : _newInstance(_reference.changeMode(Reference.ModeOneway));
  }

  @Override
  public ObjectPrx ice_batchOneway() {
    return ice_isBatchOneway()
        ? this
        : _newInstance(_reference.changeMode(Reference.ModeBatchOneway));
  }

  @Override
  public ObjectPrx ice_datagram() {
    return ice_isDatagram() ? this : _newInstance(_reference.changeMode(Reference.ModeDatagram));
  }

  @Override
  public ObjectPrx ice_batchDatagram() {
    return ice_isBatchDatagram()
        ? this
        : _newInstance(_reference.changeMode(Reference.ModeBatchDatagram));
  }

  @Override
  public ObjectPrx ice_compress(boolean compress) {
    var value = _reference.getCompress();
    if (value.isPresent() && value.get() == compress) {
      return this;
    } else {
      return _newInstance(_reference.changeCompress(compress));
    }
  }

  @Override
  public ObjectPrx ice_connectionId(String connectionId) {
    if (connectionId.equals(_reference.getConnectionId())) {
      return this;
    } else {
      return _newInstance(_reference.changeConnectionId(connectionId));
    }
  }

  public StreamPair _getCachedMessageBuffers() {
    synchronized (this) {
      if (_streamCache != null && !_streamCache.isEmpty()) {
        return _streamCache.remove(0);
      }
    }
    return null;
  }

  public void _cacheMessageBuffers(InputStream is, OutputStream os) {
    synchronized (this) {
      if (_streamCache == null) {
        _streamCache = new LinkedList<>();
      }
      _streamCache.add(new StreamPair(is, os));
    }
  }

  public int _handleException(
      Exception ex,
      com.zeroc.IceInternal.RequestHandler handler,
      OperationMode mode,
      boolean sent,
      com.zeroc.IceInternal.Holder<Integer> interval,
      int cnt) {
    _updateRequestHandler(handler, null); // Clear the request handler

    //
    // We only retry local exception.
    //
    // A CloseConnectionException indicates graceful server shutdown, and is therefore
    // always repeatable without violating "at-most-once". That's because by sending a
    // close connection message, the server guarantees that all outstanding requests
    // can safely be repeated.
    //
    // An ObjectNotExistException can always be retried as well without violating
    // "at-most-once" (see the implementation of the checkRetryAfterException method
    // below for the reasons why it can be useful).
    //
    // If the request didn't get sent or if it's non-mutating or idempotent it can
    // also always be retried if the retry count isn't reached.
    //
    if (ex instanceof LocalException
        && (!sent
            || mode == OperationMode.Nonmutating
            || mode == OperationMode.Idempotent
            || ex instanceof CloseConnectionException
            || ex instanceof ObjectNotExistException)) {
      try {
        return checkRetryAfterException((LocalException) ex, _reference, interval, cnt);
      } catch (CommunicatorDestroyedException exc) {
        //
        // The communicator is already destroyed, so we cannot retry.
        //
        throw ex;
      }
    } else {
      throw ex; // Retry could break at-most-once semantics, don't retry.
    }
  }

  public com.zeroc.IceInternal.RequestHandler _getRequestHandler() {
    if (_reference.getCacheConnection()) {
      synchronized (this) {
        if (_requestHandler != null) {
          return _requestHandler;
        }
      }
    }
    return _reference.getRequestHandler(this);
  }

  public synchronized com.zeroc.IceInternal.BatchRequestQueue _getBatchRequestQueue() {
    if (_batchRequestQueue == null) {
      _batchRequestQueue = _reference.getBatchRequestQueue();
    }
    return _batchRequestQueue;
  }

  public com.zeroc.IceInternal.RequestHandler _setRequestHandler(
      com.zeroc.IceInternal.RequestHandler handler) {
    if (_reference.getCacheConnection()) {
      synchronized (this) {
        if (_requestHandler == null) {
          _requestHandler = handler;
        }
        return _requestHandler;
      }
    }
    return handler;
  }

  public void _updateRequestHandler(
      com.zeroc.IceInternal.RequestHandler previous, com.zeroc.IceInternal.RequestHandler handler) {
    if (_reference.getCacheConnection() && previous != null) {
      synchronized (this) {
        if (_requestHandler != null && _requestHandler != handler) {
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

  private static int checkRetryAfterException(
      LocalException ex,
      Reference ref,
      com.zeroc.IceInternal.Holder<Integer> sleepInterval,
      int cnt) {

    com.zeroc.IceInternal.Instance instance = ref.getInstance();
    com.zeroc.IceInternal.TraceLevels traceLevels = instance.traceLevels();
    Logger logger = instance.initializationData().logger;
    int[] retryIntervals = instance.retryIntervals();

    //
    // We don't retry batch requests because the exception might have caused
    // all the requests batched with the connection to be aborted and we
    // want the application to be notified.
    //
    if (ref.getMode() == Reference.ModeBatchOneway
        || ref.getMode() == Reference.ModeBatchDatagram) {
      throw ex;
    }

    //
    // If it's a fixed proxy, retrying isn't useful as the proxy is tied to
    // the connection and the request will fail with the exception.
    //
    if (ref instanceof com.zeroc.IceInternal.FixedReference) {
      throw ex;
    }

    if (ex instanceof ObjectNotExistException) {
      ObjectNotExistException one = (ObjectNotExistException) ex;

      if (ref.getRouterInfo() != null && one.operation.equals("ice_add_proxy")) {
        //
        // If we have a router, an ObjectNotExistException with an
        // operation name "ice_add_proxy" indicates to the client
        // that the router isn't aware of the proxy (for example,
        // because it was evicted by the router). In this case, we
        // must *always* retry, so that the missing proxy is added
        // to the router.
        //

        ref.getRouterInfo().clearCache(ref);

        if (traceLevels.retry >= 1) {
          String s = "retrying operation call to add proxy to router\n" + ex.toString();
          logger.trace(traceLevels.retryCat, s);
        }

        if (sleepInterval != null) {
          sleepInterval.value = 0;
        }
        return cnt; // We must always retry, so we don't look at the retry count.
      } else if (ref.isIndirect()) {
        //
        // We retry ObjectNotExistException if the reference is
        // indirect.
        //

        if (ref.isWellKnown()) {
          com.zeroc.IceInternal.LocatorInfo li = ref.getLocatorInfo();
          if (li != null) {
            li.clearCache(ref);
          }
        }
      } else {
        //
        // For all other cases, we don't retry ObjectNotExistException.
        //
        throw ex;
      }
    } else if (ex instanceof RequestFailedException) {
      //
      // For all other cases, we don't retry ObjectNotExistException
      //
      throw ex;
    }

    //
    // There is no point in retrying an operation that resulted in a
    // MarshalException. This must have been raised locally (because
    // if it happened in a server it would result in an
    // UnknownLocalException instead), which means there was a problem
    // in this process that will not change if we try again.
    //
    // The most likely cause for a MarshalException is exceeding the
    // maximum message size, which is represented by the subclass
    // MemoryLimitException. For example, a client can attempt to send
    // a message that exceeds the maximum memory size, or accumulate
    // enough batch requests without flushing that the maximum size is
    // reached.
    //
    // This latter case is especially problematic, because if we were
    // to retry a batch request after a MarshalException, we would in
    // fact silently discard the accumulated requests and allow new
    // batch requests to accumulate. If the subsequent batched
    // requests do not exceed the maximum message size, it appears to
    // the client that all of the batched requests were accepted, when
    // in reality only the last few are actually sent.
    //
    if (ex instanceof MarshalException) {
      throw ex;
    }

    //
    // Don't retry if the communicator is destroyed, object adapter is deactivated,
    // or connection is manually closed.
    //
    if (ex instanceof CommunicatorDestroyedException
        || ex instanceof ObjectAdapterDeactivatedException
        || ex instanceof ConnectionManuallyClosedException) {
      throw ex;
    }

    //
    // Don't retry invocation timeouts.
    //
    if (ex instanceof InvocationTimeoutException || ex instanceof InvocationCanceledException) {
      throw ex;
    }

    //
    // Don't retry on OperationInterruptedException.
    //
    if (ex instanceof OperationInterruptedException) {
      throw ex;
    }

    ++cnt;
    assert (cnt > 0);

    int interval;
    if (cnt == (retryIntervals.length + 1) && ex instanceof CloseConnectionException) {
      //
      // A close connection exception is always retried at least once, even if the retry
      // limit is reached.
      //
      interval = 0;
    } else if (cnt > retryIntervals.length) {
      if (traceLevels.retry >= 1) {
        String s =
            "cannot retry operation call because retry limit has been exceeded\n" + ex.toString();
        logger.trace(traceLevels.retryCat, s);
      }
      throw ex;
    } else {
      interval = retryIntervals[cnt - 1];
    }

    if (traceLevels.retry >= 1) {
      String s = "retrying operation call";
      if (interval > 0) {
        s += " in " + interval + "ms";
      }
      s += " because of exception\n" + ex;
      logger.trace(traceLevels.retryCat, s);
    }

    sleepInterval.value = interval;
    return cnt;
  }

  private void writeObject(java.io.ObjectOutputStream out) throws java.io.IOException {
    out.writeUTF(toString());
  }

  private void readObject(java.io.ObjectInputStream in)
      throws java.io.IOException, ClassNotFoundException {
    String s = in.readUTF();
    try {
      Communicator communicator = ((ObjectInputStream) in).getCommunicator();
      if (communicator == null) {
        throw new java.io.IOException("Cannot deserialize proxy: no communicator provided");
      }
      _ObjectPrxI proxy = (_ObjectPrxI) communicator.stringToProxy(s);
      _reference = proxy._reference;
      assert (proxy._requestHandler == null);
    } catch (ClassCastException ex) {
      java.io.IOException e =
          new java.io.IOException("Cannot deserialize proxy: ObjectInputStream not found");
      e.initCause(ex);
      throw e;
    } catch (LocalException ex) {
      java.io.IOException e = new java.io.IOException("Failure occurred while deserializing proxy");
      e.initCause(ex);
      throw e;
    }
  }

  public static class StreamPair {
    StreamPair(InputStream is, OutputStream os) {
      this.is = is;
      this.os = os;
    }

    public InputStream is;
    public OutputStream os;
  }

  protected transient Reference _reference;
  private transient com.zeroc.IceInternal.RequestHandler _requestHandler;
  private transient com.zeroc.IceInternal.BatchRequestQueue _batchRequestQueue;
  private transient List<StreamPair> _streamCache;
  private static final long serialVersionUID = 0L;
}
