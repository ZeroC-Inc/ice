//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceInternal;

public abstract class Reference implements Cloneable {
  public static final int ModeTwoway = 0;
  public static final int ModeOneway = 1;
  public static final int ModeBatchOneway = 2;
  public static final int ModeDatagram = 3;
  public static final int ModeBatchDatagram = 4;
  public static final int ModeLast = ModeBatchDatagram;

  public interface GetConnectionCallback {
    void setConnection(com.zeroc.Ice.ConnectionI connection, boolean compress);

    void setException(com.zeroc.Ice.LocalException ex);
  }

  public final int getMode() {
    return _mode;
  }

  public final boolean getSecure() {
    return _secure;
  }

  public final com.zeroc.Ice.ProtocolVersion getProtocol() {
    return _protocol;
  }

  public final com.zeroc.Ice.EncodingVersion getEncoding() {
    return _encoding;
  }

  public final com.zeroc.Ice.Identity getIdentity() {
    return _identity;
  }

  public final String getFacet() {
    return _facet;
  }

  public final Instance getInstance() {
    return _instance;
  }

  public final java.util.Map<String, String> getContext() {
    return _context;
  }

  public int getInvocationTimeout() {
    return _invocationTimeout;
  }

  public java.util.Optional<Boolean> getCompress() {
    return _compress;
  }

  public final com.zeroc.Ice.Communicator getCommunicator() {
    return _communicator;
  }

  public abstract EndpointI[] getEndpoints();

  public abstract String getAdapterId();

  public abstract RouterInfo getRouterInfo();

  public abstract LocatorInfo getLocatorInfo();

  public abstract boolean getCollocationOptimized();

  public abstract boolean getCacheConnection();

  public abstract boolean getPreferSecure();

  public abstract com.zeroc.Ice.EndpointSelectionType getEndpointSelection();

  public abstract int getLocatorCacheTimeout();

  public abstract String getConnectionId();

  public abstract com.zeroc.IceInternal.ThreadPool getThreadPool();

  public abstract com.zeroc.Ice.ConnectionI getConnection();

  //
  // The change* methods (here and in derived classes) create
  // a new reference based on the existing one, with the
  // corresponding value changed.
  //
  public final Reference changeContext(java.util.Map<String, String> newContext) {
    if (newContext == null) {
      newContext = _emptyContext;
    }
    Reference r = _instance.referenceFactory().copy(this);
    if (newContext.isEmpty()) {
      r._context = _emptyContext;
    } else {
      r._context = new java.util.HashMap<>(newContext);
    }
    return r;
  }

  public final Reference changeMode(int newMode) {
    Reference r = _instance.referenceFactory().copy(this);
    r._mode = newMode;
    return r;
  }

  public Reference changeSecure(boolean newSecure) {
    Reference r = _instance.referenceFactory().copy(this);
    r._secure = newSecure;
    return r;
  }

  public final Reference changeIdentity(com.zeroc.Ice.Identity newIdentity) {
    Reference r = _instance.referenceFactory().copy(this);
    r._identity = newIdentity.clone();
    return r;
  }

  public final Reference changeFacet(String newFacet) {
    Reference r = _instance.referenceFactory().copy(this);
    r._facet = newFacet;
    return r;
  }

  public final Reference changeInvocationTimeout(int newTimeout) {
    Reference r = _instance.referenceFactory().copy(this);
    r._invocationTimeout = newTimeout;
    return r;
  }

  public Reference changeEncoding(com.zeroc.Ice.EncodingVersion newEncoding) {
    Reference r = _instance.referenceFactory().copy(this);
    r._encoding = newEncoding;
    return r;
  }

  public Reference changeCompress(boolean newCompress) {
    Reference r = _instance.referenceFactory().copy(this);
    r._compress = java.util.Optional.of(newCompress);
    return r;
  }

  public abstract Reference changeAdapterId(String newAdapterId);

  public abstract Reference changeEndpoints(EndpointI[] newEndpoints);

  public abstract Reference changeLocator(com.zeroc.Ice.LocatorPrx newLocator);

  public abstract Reference changeRouter(com.zeroc.Ice.RouterPrx newRouter);

  public abstract Reference changeCollocationOptimized(boolean newCollocationOptimized);

  public abstract Reference changeCacheConnection(boolean newCache);

  public abstract Reference changePreferSecure(boolean newPreferSecure);

  public abstract Reference changeEndpointSelection(com.zeroc.Ice.EndpointSelectionType newType);

  public abstract Reference changeLocatorCacheTimeout(int newTimeout);

  public abstract Reference changeConnectionId(String connectionId);

  public abstract Reference changeConnection(com.zeroc.Ice.ConnectionI connection);

  @Override
  public  int hashCode() {
    int h = 5381;
    h = HashUtil.hashAdd(h, _mode);
    h = HashUtil.hashAdd(h, _secure);
    h = HashUtil.hashAdd(h, _identity);
    h = HashUtil.hashAdd(h, _context);
    h = HashUtil.hashAdd(h, _facet);
    if (_compress.isPresent()) {
      h = HashUtil.hashAdd(h, _compress.get());
    }
    // We don't include protocol and encoding in the hash; they are using 1.0 and 1.1, respectively.
    h = HashUtil.hashAdd(h, _invocationTimeout);

    return h;
  }

  // Gets the effective compression setting, taking into account the override.
  public java.util.Optional<Boolean> getCompressOverride() {
    DefaultsAndOverrides defaultsAndOverrides = getInstance().defaultsAndOverrides();

    return defaultsAndOverrides.overrideCompress.isPresent()
        ? defaultsAndOverrides.overrideCompress
        : _compress;
  }

  //
  // Utility methods
  //
  public abstract boolean isIndirect();

  public abstract boolean isWellKnown();

  //
  // Marshal the reference.
  //
  public void streamWrite(com.zeroc.Ice.OutputStream s) {
    //
    // Don't write the identity here. Operations calling streamWrite
    // write the identity.
    //

    //
    // For compatibility with the old FacetPath.
    //
    if (_facet.isEmpty()) {
      s.writeStringSeq(null);
    } else {
      String[] facetPath = {_facet};
      s.writeStringSeq(facetPath);
    }

    s.writeByte((byte) _mode);

    s.writeBool(_secure);

    if (!s.getEncoding().equals(com.zeroc.Ice.Util.Encoding_1_0)) {
      _protocol.ice_writeMembers(s);
      _encoding.ice_writeMembers(s);
    }

    // Derived class writes the remainder of the reference.
  }

  //
  // Convert the reference to its string form.
  //
  @Override
  public String toString() {
    //
    // WARNING: Certain features, such as proxy validation in Glacier2,
    // depend on the format of proxy strings. Changes to toString() and
    // methods called to generate parts of the reference string could break
    // these features. Please review for all features that depend on the
    // format of proxyToString() before changing this and related code.
    //
    StringBuilder s = new StringBuilder(128);

    com.zeroc.Ice.ToStringMode toStringMode = _instance.toStringMode();
    final String separators = " :@";

    String id = com.zeroc.Ice.Util.identityToString(_identity, toStringMode);

    //
    // If the encoded identity string contains characters which
    // the reference parser uses as separators, then we enclose
    // the identity string in quotes.
    //

    if (com.zeroc.IceUtilInternal.StringUtil.findFirstOf(id, separators) != -1) {
      s.append('"');
      s.append(id);
      s.append('"');
    } else {
      s.append(id);
    }

    if (!_facet.isEmpty()) {
      //
      // If the encoded facet string contains characters which
      // the reference parser uses as separators, then we enclose
      // the facet string in quotes.
      //
      s.append(" -f ");
      String fs = com.zeroc.IceUtilInternal.StringUtil.escapeString(_facet, "", toStringMode);
      if (com.zeroc.IceUtilInternal.StringUtil.findFirstOf(fs, separators) != -1) {
        s.append('"');
        s.append(fs);
        s.append('"');
      } else {
        s.append(fs);
      }
    }

    switch (_mode) {
      case ModeTwoway:
        {
          s.append(" -t");
          break;
        }

      case ModeOneway:
        {
          s.append(" -o");
          break;
        }

      case ModeBatchOneway:
        {
          s.append(" -O");
          break;
        }

      case ModeDatagram:
        {
          s.append(" -d");
          break;
        }

      case ModeBatchDatagram:
        {
          s.append(" -D");
          break;
        }
    }

    if (_secure) {
      s.append(" -s");
    }

    if (!_protocol.equals(com.zeroc.Ice.Util.Protocol_1_0)) {
      //
      // We only print the protocol if it's not 1.0. It's fine as
      // long as we don't add Ice.Default.ProtocolVersion, a
      // stringified proxy will convert back to the same proxy with
      // stringToProxy.
      //
      s.append(" -p ");
      s.append(com.zeroc.Ice.Util.protocolVersionToString(_protocol));
    }

    //
    // Always print the encoding version to ensure a stringified proxy
    // will convert back to a proxy with the same encoding with
    // stringToProxy (and won't use Ice.Default.EncodingVersion).
    //
    s.append(" -e ");
    s.append(com.zeroc.Ice.Util.encodingVersionToString(_encoding));

    return s.toString();

    // Derived class writes the remainder of the string.
  }

  //
  // Convert the reference to its property form.
  //
  public abstract java.util.Map<String, String> toProperty(String prefix);

  public abstract RequestHandler getRequestHandler(com.zeroc.Ice._ObjectPrxI proxy);

  public abstract BatchRequestQueue getBatchRequestQueue();

  @Override
  public boolean equals(java.lang.Object obj) {
    //
    // Note: if(this == obj) and type test are performed by each non-abstract derived class.
    //

    Reference r = (Reference) obj; // Guaranteed to succeed.

    if (_mode != r._mode) {
      return false;
    }

    if (_secure != r._secure) {
      return false;
    }

    if (!_identity.equals(r._identity)) {
      return false;
    }

    if (!_context.equals(r._context)) {
      return false;
    }

    if (!_facet.equals(r._facet)) {
      return false;
    }

    if (!_compress.equals(r._compress)) {
      return false;
    }

    if (!_protocol.equals(r._protocol)) {
      return false;
    }

    if (!_encoding.equals(r._encoding)) {
      return false;
    }

    if (_invocationTimeout != r._invocationTimeout) {
      return false;
    }

    return true;
  }

  @Override
  public Reference clone() {
    Reference c = null;
    try {
      c = (Reference) super.clone();
    } catch (CloneNotSupportedException ex) {
      assert false;
    }
    return c;
  }

  private static java.util.Map<String, String> _emptyContext = new java.util.HashMap<>();

  private final Instance _instance;
  private final com.zeroc.Ice.Communicator _communicator;

  private final int _mode;
  private final boolean _secure;
  private final java.util.Optional<Boolean> _compress;
  private final com.zeroc.Ice.Identity _identity;
  private final java.util.Map<String, String> _context;
  private final String _facet;
  private final com.zeroc.Ice.ProtocolVersion _protocol;
  private final com.zeroc.Ice.EncodingVersion _encoding;
  private final int _invocationTimeout;

  protected Reference(
      Instance instance,
      com.zeroc.Ice.Communicator communicator,
      com.zeroc.Ice.Identity identity,
      String facet,
      int mode,
      boolean secure,
      java.util.Optional<Boolean> compress,
      com.zeroc.Ice.ProtocolVersion protocol,
      com.zeroc.Ice.EncodingVersion encoding,
      int invocationTimeout,
      java.util.Map<String, String> context) {
    //
    // Validate string arguments.
    //
    assert (identity.name != null);
    assert (identity.category != null);
    assert (facet != null);

    _instance = instance;
    _communicator = communicator;
    _mode = mode;
    _secure = secure;
    _compress = compress;
    _identity = identity;
    _context = context != null ? new java.util.HashMap<>(context) : _emptyContext;
    _facet = facet;
    _protocol = protocol;
    _encoding = encoding;
    _invocationTimeout = invocationTimeout;
  }
}
