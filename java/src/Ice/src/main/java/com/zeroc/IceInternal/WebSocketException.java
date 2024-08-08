// Copyright (c) ZeroC, Inc.

package com.zeroc.IceInternal;

final class WebSocketException extends java.lang.RuntimeException {
  public WebSocketException(String reason) {
    super(reason);
  }

  public WebSocketException(Throwable cause) {
    super(cause);
  }

  private static final long serialVersionUID = 133989672864895760L;
}
