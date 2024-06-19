//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `LocalException.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * This exception indicates that a connection has been shut down because it has been idle for some
 * time.
 */
public class ConnectionTimeoutException extends TimeoutException {
  public ConnectionTimeoutException() {
    super();
  }

  public ConnectionTimeoutException(Throwable cause) {
    super(cause);
  }

  public String ice_id() {
    return "::Ice::ConnectionTimeoutException";
  }

  private static final long serialVersionUID = 1630370601897802194L;
}
