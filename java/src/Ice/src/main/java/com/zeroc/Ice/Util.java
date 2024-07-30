//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

import com.zeroc.IceUtilInternal.StringUtil;

/** Utility methods for the Ice run time. */
public final class Util {
  /**
   * Creates a new empty property set.
   *
   * @return A new empty property set.
   * @deprecated Use {@link Properties#Properties()} instead.
   */
  @Deprecated
  public static Properties createProperties() {
    return new Properties();
  }

  /**
   * Creates a property set initialized from an argument vector.
   *
   * @param args A command-line argument vector, possibly containing options to set properties. If
   *     the command-line options include a <code>--Ice.Config</code> option, the corresponding
   *     configuration files are parsed. If the same property is set in a configuration file and in
   *     the argument vector, the argument vector takes precedence.
   * @return A new property set initialized with the property settings that were removed from the
   *     argument vector.
   * @deprecated Use {@link Properties#Properties(String[])} instead.
   */
  @Deprecated
  public static Properties createProperties(String[] args) {
    return new Properties(args, null, null);
  }

  /**
   * Creates a property set initialized from an argument vector and return the remaining arguments.
   *
   * @param args A command-line argument vector, possibly containing options to set properties. If
   *     the command-line options include a <code>--Ice.Config</code> option, the corresponding
   *     configuration files are parsed. If the same property is set in a configuration file and in
   *     the argument vector, the argument vector takes precedence.
   * @param remainingArgs If non null, the given list will contain on return the command-line
   *     arguments that were not used to set properties.
   * @return A new property set initialized with the property settings that were removed from the
   *     argument vector.
   * @deprecated Use {@link Properties#Properties(String[], java.util.List)} instead.
   */
  @Deprecated
  public static Properties createProperties(String[] args, java.util.List<String> remainingArgs) {
    return new Properties(args, null, remainingArgs);
  }

  /**
   * Creates a property set initialized from an argument vector.
   *
   * @param args A command-line argument vector, possibly containing options to set properties. If
   *     the command-line options include a <code>--Ice.Config</code> option, the corresponding
   *     configuration files are parsed. If the same property is set in a configuration file and in
   *     the argument vector, the argument vector takes precedence.
   * @param defaults Default values for the property set. Settings in configuration files and <code>
   *     args</code> override these defaults.
   * @return A new property set initialized with the property settings that were removed from the
   *     argument vector.
   * @deprecated Use {@link Properties#Properties(String[], Properties)} instead.
   */
  @Deprecated
  public static Properties createProperties(String[] args, Properties defaults) {
    return new Properties(args, defaults, null);
  }

  /**
   * Creates a property set initialized from an argument vector and return the remaining arguments.
   *
   * @param args A command-line argument vector, possibly containing options to set properties. If
   *     the command-line options include a <code>--Ice.Config</code> option, the corresponding
   *     configuration files are parsed. If the same property is set in a configuration file and in
   *     the argument vector, the argument vector takes precedence.
   * @param defaults Default values for the property set. Settings in configuration files and <code>
   *     args</code> override these defaults.
   * @param remainingArgs If non null, the given list will contain on return the command-line
   *     arguments that were not used to set properties.
   * @return A new property set initialized with the property settings that were removed from the
   *     argument vector.
   * @deprecated Use {@link Properties#Properties(String[], Properties, java.util.List)} instead.
   */
  @Deprecated
  public static Properties createProperties(
      String[] args, Properties defaults, java.util.List<String> remainingArgs) {
    return new Properties(args, defaults, remainingArgs);
  }

  /**
   * Creates a communicator using a default configuration.
   *
   * @return A new communicator instance.
   */
  public static Communicator initialize() {
    return initialize(new InitializationData());
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @return The new communicator.
   */
  public static Communicator initialize(String[] args) {
    return initialize(args, (InitializationData) null, null);
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @param remainingArgs If non null, the given list will contain on return the command-line
   *     arguments that were not used to set properties.
   * @return The new communicator.
   */
  public static Communicator initialize(String[] args, java.util.List<String> remainingArgs) {
    return initialize(args, (InitializationData) null, remainingArgs);
  }

  /**
   * Creates a communicator.
   *
   * @param initData Additional initialization data.
   * @return The new communicator.
   */
  public static Communicator initialize(InitializationData initData) {
    return initialize(null, initData, null);
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @param initData Additional initialization data.
   * @return The new communicator.
   */
  public static Communicator initialize(String[] args, InitializationData initData) {
    return initialize(args, initData, null);
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @param configFile Path to a config file that sets the new communicator's default properties.
   * @return The new communicator.
   */
  public static Communicator initialize(String[] args, String configFile) {
    return initialize(args, configFile, null);
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @param initData Additional initialization data. Property settings in <code>args</code> override
   *     property settings in <code>initData</code>.
   * @param remainingArgs If non null, the given list will contain on return the command-line
   *     arguments that were not used to set properties.
   * @return The new communicator.
   * @see InitializationData
   */
  public static Communicator initialize(
      String[] args, InitializationData initData, java.util.List<String> remainingArgs) {
    if (initData == null) {
      initData = new InitializationData();
    } else {
      initData = initData.clone();
    }

    if (args != null) {
      java.util.List<String> rArgs = new java.util.ArrayList<>();
      initData.properties = createProperties(args, initData.properties, rArgs);
      args = rArgs.toArray(new String[rArgs.size()]);
    }

    var communicator = new Communicator(initData);
    communicator.finishSetup(args != null ? args : new String[0], remainingArgs);
    return communicator;
  }

  /**
   * Creates a communicator.
   *
   * @param args A command-line argument vector. Any Ice-related options in this vector are used to
   *     initialize the communicator.
   * @param configFile Path to a config file that sets the new communicator's default properties.
   * @param remainingArgs If non null, the given list will contain on return the command-line
   *     arguments that were not used to set properties.
   * @return The new communicator.
   */
  public static Communicator initialize(
      String[] args, String configFile, java.util.List<String> remainingArgs) {
    InitializationData initData = null;
    if (configFile != null) {
      initData = new InitializationData();
      initData.properties = new Properties();
      initData.properties.load(configFile);
    }

    return initialize(args, initData, remainingArgs);
  }

  /**
   * Converts a string to an object identity.
   *
   * @param s The string to convert.
   * @return The converted object identity.
   */
  public static Identity stringToIdentity(String s) {
    Identity ident = new Identity();

    //
    // Find unescaped separator; note that the string may contain an escaped
    // backslash before the separator.
    //
    int slash = -1, pos = 0;
    while ((pos = s.indexOf('/', pos)) != -1) {
      int escapes = 0;
      while (pos - escapes > 0 && s.charAt(pos - escapes - 1) == '\\') {
        escapes++;
      }

      //
      // We ignore escaped escapes
      //
      if (escapes % 2 == 0) {
        if (slash == -1) {
          slash = pos;
        } else {
          //
          // Extra unescaped slash found.
          //
          throw new ParseException("unescaped backslash in identity string '" + s + "'");
        }
      }
      pos++;
    }

    if (slash == -1) {
      ident.category = "";
      try {
        ident.name = StringUtil.unescapeString(s, 0, s.length(), "/");
      } catch (IllegalArgumentException ex) {
        throw new ParseException("invalid name in identity string '" + s + "'", ex);
      }
    } else {
      try {
        ident.category = StringUtil.unescapeString(s, 0, slash, "/");
      } catch (IllegalArgumentException e) {
        throw new ParseException("invalid category in identity string '" + s + "'", ex);
      }
      if (slash + 1 < s.length()) {
        try {
          ident.name = StringUtil.unescapeString(s, slash + 1, s.length(), "/");
        } catch (IllegalArgumentException e) {
          throw new ParseException("invalid name in identity string '" + s + "'", ex);
        }
      } else {
        ident.name = "";
      }
    }

    return ident;
  }

  /**
   * Converts an object identity to a string.
   *
   * @param ident The object identity to convert.
   * @param toStringMode Specifies if and how non-printable ASCII characters are escaped in the
   *     result.
   * @return The string representation of the object identity.
   */
  public static String identityToString(Identity ident, ToStringMode toStringMode) {
    if (ident.category == null || ident.category.isEmpty()) {
      return StringUtil.escapeString(ident.name, "/", toStringMode);
    } else {
      return StringUtil.escapeString(ident.category, "/", toStringMode)
          + '/'
          + StringUtil.escapeString(ident.name, "/", toStringMode);
    }
  }

  /**
   * Converts an object identity to a string.
   *
   * @param ident The object identity to convert.
   * @return The string representation of the object identity using the default mode (Unicode)
   */
  public static String identityToString(Identity ident) {
    return identityToString(ident, ToStringMode.Unicode);
  }

  /**
   * Compares the object identities of two proxies.
   *
   * @param lhs A proxy.
   * @param rhs A proxy.
   * @return -1 if the identity in <code>lhs</code> compares less than the identity in <code>rhs
   *     </code>; 0 if the identities compare equal; 1, otherwise.
   * @see ProxyIdentityKey
   * @see ProxyIdentityFacetKey
   * @see #proxyIdentityAndFacetCompare
   */
  public static int proxyIdentityCompare(ObjectPrx lhs, ObjectPrx rhs) {
    if (lhs == null && rhs == null) {
      return 0;
    } else if (lhs == null && rhs != null) {
      return -1;
    } else if (lhs != null && rhs == null) {
      return 1;
    } else {
      Identity lhsIdentity = lhs.ice_getIdentity();
      Identity rhsIdentity = rhs.ice_getIdentity();
      int n;
      if ((n = lhsIdentity.name.compareTo(rhsIdentity.name)) != 0) {
        return n;
      }
      return lhsIdentity.category.compareTo(rhsIdentity.category);
    }
  }

  /**
   * Compares the object identities and facets of two proxies.
   *
   * @param lhs A proxy.
   * @param rhs A proxy.
   * @return -1 if the identity and facet in <code>lhs</code> compare less than the identity and
   *     facet in <code>rhs</code>; 0 if the identities and facets compare equal; 1, otherwise.
   * @see ProxyIdentityFacetKey
   * @see ProxyIdentityKey
   * @see #proxyIdentityCompare
   */
  public static int proxyIdentityAndFacetCompare(ObjectPrx lhs, ObjectPrx rhs) {
    if (lhs == null && rhs == null) {
      return 0;
    } else if (lhs == null && rhs != null) {
      return -1;
    } else if (lhs != null && rhs == null) {
      return 1;
    } else {
      Identity lhsIdentity = lhs.ice_getIdentity();
      Identity rhsIdentity = rhs.ice_getIdentity();
      int n;
      if ((n = lhsIdentity.name.compareTo(rhsIdentity.name)) != 0) {
        return n;
      }
      if ((n = lhsIdentity.category.compareTo(rhsIdentity.category)) != 0) {
        return n;
      }

      String lhsFacet = lhs.ice_getFacet();
      String rhsFacet = rhs.ice_getFacet();
      if (lhsFacet == null && rhsFacet == null) {
        return 0;
      } else if (lhsFacet == null) {
        return -1;
      } else if (rhsFacet == null) {
        return 1;
      }
      return lhsFacet.compareTo(rhsFacet);
    }
  }

  /**
   * Returns the process-wide logger.
   *
   * @return The process-wide logger.
   */
  public static Logger getProcessLogger() {
    synchronized (_processLoggerMutex) {
      if (_processLogger == null) {
        //
        // TODO: Would be nice to be able to use process name as prefix by default.
        //
        _processLogger = new LoggerI("", "");
      }

      return _processLogger;
    }
  }

  /**
   * Changes the process-wide logger.
   *
   * @param logger The new process-wide logger.
   */
  public static void setProcessLogger(Logger logger) {
    synchronized (_processLoggerMutex) {
      _processLogger = logger;
    }
  }

  /**
   * Returns the Ice version in the form <code>A.B.C</code>, where <code>A</code> indicates the
   * major version, <code>B</code> indicates the minor version, and <code>C</code> indicates the
   * patch level.
   *
   * @return The Ice version.
   */
  public static String stringVersion() {
    return "3.8.0-alpha.0"; // "A.B.C", with A=major, B=minor, C=patch
  }

  /**
   * Returns the Ice version as an integer in the form <code>A.BB.CC</code>, where <code>A</code>
   * indicates the major version, <code>BB</code> indicates the minor version, and <code>CC</code>
   * indicates the patch level. For example, for Ice 3.3.1, the returned value is 30301.
   *
   * @return The Ice version.
   */
  public static int intVersion() {
    return 30850; // AABBCC, with AA=major, BB=minor, CC=patch
  }

  /**
   * Converts a string to a protocol version.
   *
   * @param version The string to convert.
   * @return The converted protocol version.
   */
  public static ProtocolVersion stringToProtocolVersion(String version) {
    return new ProtocolVersion(stringToMajor(version), stringToMinor(version));
  }

  /**
   * Converts a string to an encoding version.
   *
   * @param version The string to convert.
   * @return The converted encoding version.
   */
  public static EncodingVersion stringToEncodingVersion(String version) {
    return new EncodingVersion(stringToMajor(version), stringToMinor(version));
  }

  /**
   * Converts a protocol version to a string.
   *
   * @param v The protocol version to convert.
   * @return The converted string.
   */
  public static String protocolVersionToString(ProtocolVersion v) {
    return majorMinorToString(v.major, v.minor);
  }

  /**
   * Converts an encoding version to a string.
   *
   * @param v The encoding version to convert.
   * @return The converted string.
   */
  public static String encodingVersionToString(EncodingVersion v) {
    return majorMinorToString(v.major, v.minor);
  }

  /**
   * Returns the supported Ice protocol version.
   *
   * @return The Ice protocol version.
   */
  public static ProtocolVersion currentProtocol() {
    return com.zeroc.IceInternal.Protocol.currentProtocol.clone();
  }

  /**
   * Returns the supported Ice encoding version.
   *
   * @return The Ice encoding version.
   */
  public static EncodingVersion currentEncoding() {
    return com.zeroc.IceInternal.Protocol.currentEncoding.clone();
  }

  /**
   * Returns the InvocationFuture equivalent of the given CompletableFuture.
   *
   * @param f The CompletableFuture returned by an asynchronous Ice proxy invocation.
   * @param <T> The result type.
   * @return The InvocationFuture object.
   */
  public static <T> InvocationFuture<T> getInvocationFuture(
      java.util.concurrent.CompletableFuture<T> f) {
    if (!(f instanceof InvocationFuture)) {
      throw new IllegalArgumentException(
          "future did not originate from an asynchronous proxy invocation");
    }
    return (InvocationFuture<T>) f;
  }

  /**
   * Translates a Slice type id to a Java class name.
   *
   * @param id The Slice type id, such as <code>::Module::Type</code>.
   * @return The equivalent Java class name, or null if the type id is malformed.
   */
  public static String typeIdToClass(String id) {
    if (!id.startsWith("::")) {
      return null;
    }

    StringBuilder buf = new StringBuilder(id.length());
    int start = 2;
    boolean done = false;
    while (!done) {
      int end = id.indexOf(':', start);
      String s;
      if (end != -1) {
        s = id.substring(start, end);
        start = end + 2;
      } else {
        s = id.substring(start);
        done = true;
      }
      if (buf.length() > 0) {
        buf.append('.');
      }
      buf.append(fixKwd(s));
    }

    return buf.toString();
  }

  private static String fixKwd(String name) {
    //
    // Keyword list. *Must* be kept in alphabetical order. Note that checkedCast and uncheckedCast
    // are not Java keywords, but are in this list to prevent illegal code being generated if
    // someone defines Slice operations with that name.
    //
    final String[] keywordList = {
      "abstract",
      "assert",
      "boolean",
      "break",
      "byte",
      "case",
      "catch",
      "char",
      "checkedCast",
      "class",
      "clone",
      "const",
      "continue",
      "default",
      "do",
      "double",
      "else",
      "enum",
      "equals",
      "extends",
      "false",
      "final",
      "finalize",
      "finally",
      "float",
      "for",
      "getClass",
      "goto",
      "hashCode",
      "if",
      "implements",
      "import",
      "instanceof",
      "int",
      "interface",
      "long",
      "native",
      "new",
      "notify",
      "notifyAll",
      "null",
      "package",
      "private",
      "protected",
      "public",
      "return",
      "short",
      "static",
      "strictfp",
      "super",
      "switch",
      "synchronized",
      "this",
      "throw",
      "throws",
      "toString",
      "transient",
      "true",
      "try",
      "uncheckedCast",
      "void",
      "volatile",
      "wait",
      "while"
    };
    boolean found = java.util.Arrays.binarySearch(keywordList, name) >= 0;
    return found ? "_" + name : name;
  }

  private static byte stringToMajor(String str) {
    int pos = str.indexOf('.');
    if (pos == -1) {
      throw new ParseException("malformed version value '" + str + "'");
    }

    String majStr = str.substring(0, pos);
    int majVersion;
    try {
      majVersion = Integer.parseInt(majStr);
    } catch (NumberFormatException ex) {
      throw new ParseException("invalid version value '" + str + "'", ex);
    }

    if (majVersion < 1 || majVersion > 255) {
      throw new ParseException("range error in version '" + str + "'");
    }

    return (byte) majVersion;
  }

  private static byte stringToMinor(String str) {
    int pos = str.indexOf('.');
    if (pos == -1) {
      throw new ParseException("malformed version value '" + str + "'");
    }

    String minStr = str.substring(pos + 1, str.length());
    int minVersion;
    try {
      minVersion = Integer.parseInt(minStr);
    } catch (NumberFormatException ex) {
      throw new ParseException("invalid version value '" + str + "'", ex);
    }

    if (minVersion < 0 || minVersion > 255) {
      throw new ParseException("range error in version '" + str + "'");
    }

    return (byte) minVersion;
  }

  private static String majorMinorToString(byte major, byte minor) {
    StringBuilder str = new StringBuilder();
    str.append(major < 0 ? major + 255 : (int) major);
    str.append(".");
    str.append(minor < 0 ? minor + 255 : (int) minor);
    return str.toString();
  }

  public static final ProtocolVersion Protocol_1_0 = new ProtocolVersion((byte) 1, (byte) 0);

  public static final EncodingVersion Encoding_1_0 = new EncodingVersion((byte) 1, (byte) 0);
  public static final EncodingVersion Encoding_1_1 = new EncodingVersion((byte) 1, (byte) 1);

  private static java.lang.Object _processLoggerMutex = new java.lang.Object();
  private static Logger _processLogger = null;
}
