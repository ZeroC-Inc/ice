// Copyright (c) ZeroC, Inc.

package com.zeroc.Ice;

/** Base class for Ice run-time exceptions. */
public class LocalException extends Exception {
  public LocalException(String message) {
    super(message);
  }

  public LocalException(String message, Throwable cause) {
    super(message, cause);
  }

  @Override
  public String ice_id() {
    return "::Ice::LocalException";
  }

  private static final long serialVersionUID = 0L;
}
