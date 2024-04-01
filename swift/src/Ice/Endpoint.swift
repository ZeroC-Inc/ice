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

import Foundation

/// Base class providing access to the endpoint details.
public protocol EndpointInfo: Swift.AnyObject {
  /// The information of the underyling endpoint or null if there's no underlying endpoint.
  var underlying: EndpointInfo? { get set }
  /// The timeout for the endpoint in milliseconds. 0 means non-blocking, -1 means no timeout.
  var timeout: Swift.Int32 { get set }
  /// Specifies whether or not compression should be used if available when using this endpoint.
  var compress: Swift.Bool { get set }

  /// Returns the type of the endpoint.
  ///
  /// - returns: `Swift.Int16` - The endpoint type.
  func type() -> Swift.Int16

  /// Returns true if this endpoint is a datagram endpoint.
  ///
  /// - returns: `Swift.Bool` - True for a datagram endpoint.
  func datagram() -> Swift.Bool

  ///
  /// - returns: `Swift.Bool` - True for a secure endpoint.
  func secure() -> Swift.Bool
}

/// The user-level interface to an endpoint.
public protocol Endpoint: Swift.AnyObject, Swift.CustomStringConvertible {
  /// Return a string representation of the endpoint.
  ///
  /// - returns: `Swift.String` - The string representation of the endpoint.
  func toString() -> Swift.String

  /// Returns the endpoint information.
  ///
  /// - returns: `EndpointInfo?` - The endpoint information class.
  func getInfo() -> EndpointInfo?
}

/// Provides access to the address details of a IP endpoint.
public protocol IPEndpointInfo: EndpointInfo {
  /// The host or address configured with the endpoint.
  var host: Swift.String { get set }
  /// The port number.
  var port: Swift.Int32 { get set }
  /// The source IP address.
  var sourceAddress: Swift.String { get set }
}

/// Provides access to a TCP endpoint information.
public protocol TCPEndpointInfo: IPEndpointInfo {}

/// Provides access to an UDP endpoint information.
public protocol UDPEndpointInfo: IPEndpointInfo {
  /// The multicast interface.
  var mcastInterface: Swift.String { get set }
  /// The multicast time-to-live (or hops).
  var mcastTtl: Swift.Int32 { get set }
}

/// Provides access to a WebSocket endpoint information.
public protocol WSEndpointInfo: EndpointInfo {
  /// The URI configured with the endpoint.
  var resource: Swift.String { get set }
}

/// Provides access to the details of an opaque endpoint.
public protocol OpaqueEndpointInfo: EndpointInfo {
  /// The encoding version of the opaque endpoint (to decode or encode the rawBytes).
  var rawEncoding: EncodingVersion { get set }
  /// The raw encoding of the opaque endpoint.
  var rawBytes: ByteSeq { get set }
}

public typealias EndpointSeq = [Endpoint]
