// Copyright (c) ZeroC, Inc.

import Foundation

/// The user-level interface to an endpoint.
public protocol Endpoint: AnyObject, CustomStringConvertible {
    /// Return a string representation of the endpoint.
    ///
    /// - returns: `String` - The string representation of the endpoint.
    func toString() -> String

    /// Returns the endpoint information.
    ///
    /// - returns: `EndpointInfo?` - The endpoint information class.
    func getInfo() -> EndpointInfo?
}

public typealias EndpointSeq = [Endpoint]

/// Base class providing access to the endpoint details.
open class EndpointInfo {
    /// The information of the underlying endpoint or null if there's no underlying endpoint.
    public let underlying: EndpointInfo?
    /// The timeout for the endpoint in milliseconds. 0 means non-blocking, -1 means no timeout.
    public let timeout: Int32
    /// Specifies whether or not compression should be used if available when using this endpoint.
    public let compress: Bool

    /// Returns the type of the endpoint.
    ///
    /// - returns: `Int16` - The endpoint type.
    public func type() -> Int16 {
        underlying?.type() ?? -1
    }

    /// Returns true if this endpoint is a datagram endpoint.
    ///
    /// - returns: `Bool` - True for a datagram endpoint.
    public func datagram() -> Bool {
        underlying?.datagram() ?? false
    }

    /// Returns true if this endpoint is a secure endpoint.
    ///
    /// - returns: `Bool` - True for a secure endpoint.
    public func secure() -> Bool {
        underlying?.secure() ?? false
    }

    public init(underlying: EndpointInfo) {
        self.underlying = underlying
        self.timeout = underlying.timeout
        self.compress = underlying.compress
    }

    public init(timeout: Int32, compress: Bool) {
        self.underlying = nil
        self.timeout = timeout
        self.compress = compress
    }
}

/// Provides access to the address details of a IP endpoint.
open class IPEndpointInfo: EndpointInfo {
    /// The host or address configured with the endpoint.
    public let host: String
    /// The port number.
    public let port: Int32
    /// The source IP address.
    public let sourceAddress: String

    public init(timeout: Int32, compress: Bool, host: String, port: Int32, sourceAddress: String) {
        self.host = host
        self.port = port
        self.sourceAddress = sourceAddress
        super.init(timeout: timeout, compress: compress)
    }
}

/// Provides access to a TCP endpoint information.
public final class TCPEndpointInfo: IPEndpointInfo {
    private let _type: Int16
    private let _secure: Bool

    public override func type() -> Int16 {
        _type
    }

    public override func secure() -> Bool {
        _secure
    }

    internal init(
        timeout: Int32, compress: Bool, host: String, port: Int32, sourceAddress: String, type: Int16, secure: Bool
    ) {
        self._type = type
        self._secure = secure
        super.init(timeout: timeout, compress: compress, host: host, port: port, sourceAddress: sourceAddress)
    }
}

/// Provides access to an UDP endpoint information.
public final class UDPEndpointInfo: IPEndpointInfo {
    /// The multicast interface.
    public let mcastInterface: String
    /// The multicast time-to-live (or hops).
    public let mcastTtl: Int32

    public override func type() -> Int16 {
        UDPEndpointType
    }

    public override func datagram() -> Bool {
        true
    }

    internal init(
        compress: Bool, host: String, port: Int32, sourceAddress: String, mcastInterface: String, mcastTtl: Int32
    ) {
        self.mcastInterface = mcastInterface
        self.mcastTtl = mcastTtl
        super.init(timeout: -1, compress: compress, host: host, port: port, sourceAddress: sourceAddress)
    }
}

/// Provides access to a WebSocket endpoint information.
public final class WSEndpointInfo: EndpointInfo {
    /// The URI configured with the endpoint.
    public let resource: String

    internal init(underlying: EndpointInfo, resource: String) {
        self.resource = resource
        super.init(underlying: underlying)
    }
}

/// Provides access to the details of an opaque endpoint.
public final class OpaqueEndpointInfo: EndpointInfo {
    /// The encoding version of the opaque endpoint (to decode or encode the rawBytes).
    public let rawEncoding: EncodingVersion
    /// The raw encoding of the opaque endpoint.
    public let rawBytes: ByteSeq

    private let _type: Int16

    public override func type() -> Int16 {
        _type
    }

    internal init(type: Int16, rawEncoding: EncodingVersion, rawBytes: ByteSeq) {
        self.rawEncoding = rawEncoding
        self.rawBytes = rawBytes
        self._type = type
        super.init(timeout: -1, compress: false)
    }
}
