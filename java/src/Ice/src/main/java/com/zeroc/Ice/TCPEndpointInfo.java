//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Endpoint.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * Provides access to a TCP endpoint information.
 *
 * @see Endpoint
 */
public abstract class TCPEndpointInfo extends IPEndpointInfo {
  public TCPEndpointInfo() {
    super();
  }

  public TCPEndpointInfo(
      EndpointInfo underlying,
      int timeout,
      boolean compress,
      String host,
      int port,
      String sourceAddress) {
    super(underlying, timeout, compress, host, port, sourceAddress);
  }

  public TCPEndpointInfo clone() {
    return (TCPEndpointInfo) super.clone();
  }
}
