//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[["cpp:dll-export:ICE_API"]]
[["cpp:doxygen:include:Ice/Ice.h"]]
[["cpp:header-ext:h"]]

[["ice-prefix"]]

[["js:module:ice"]]
[["js:cjs-module"]]

[["objc:dll-export:ICE_API"]]
[["objc:header-dir:objc"]]

[["python:pkgdir:Ice"]]

#include <Ice/Version.ice>
#include <Ice/BuiltinSequences.ice>
#include <Ice/EndpointF.ice>

[["java:package:com.zeroc"]]

["objc:prefix:ICE"]
module Ice
{

#if !defined(__SLICE2PHP__) && !defined(__SLICE2MATLAB__)

/// Base class providing access to the endpoint details.
local class EndpointInfo
{
    /// The information of the underyling endpoint or null if there's no underlying endpoint.
    EndpointInfo underlying;

    /// The timeout for the endpoint in milliseconds. 0 means non-blocking, -1 means no timeout.
    int timeout;

    /// Specifies whether or not compression should be used if available when using this endpoint.
    bool compress;

    /// Returns the type of the endpoint.
    /// @return The endpoint type.
    ["cpp:const", "cpp:noexcept", "swift:noexcept"] short type();

    /// Returns true if this endpoint is a datagram endpoint.
    /// @return True for a datagram endpoint.
    ["cpp:const", "cpp:noexcept", "swift:noexcept"] bool datagram();

    /// @return True for a secure endpoint.
    ["cpp:const", "cpp:noexcept", "swift:noexcept"] bool secure();
}

/// The user-level interface to an endpoint.
["cpp:comparable", "js:comparable", "swift:inherits:Swift.CustomStringConvertible"]
local interface Endpoint
{
    /// Return a string representation of the endpoint.
    /// @return The string representation of the endpoint.
    ["cpp:const", "cpp:noexcept", "swift:noexcept"] string toString();

    /// Returns the endpoint information.
    /// @return The endpoint information class.
    ["cpp:const", "cpp:noexcept", "swift:noexcept"] EndpointInfo getInfo();
}

/// Provides access to the address details of a IP endpoint.
/// @see Endpoint
local class IPEndpointInfo extends EndpointInfo
{
    /// The host or address configured with the endpoint.
    string host;

    /// The port number.
    int port;

    /// The source IP address.
    string sourceAddress;
}

/// Provides access to a TCP endpoint information.
/// @see Endpoint
local class TCPEndpointInfo extends IPEndpointInfo
{
}

/// Provides access to an UDP endpoint information.
/// @see Endpoint
local class UDPEndpointInfo extends IPEndpointInfo
{
    /// The multicast interface.
    string mcastInterface;

    /// The multicast time-to-live (or hops).
    int mcastTtl;
}

/// Provides access to a WebSocket endpoint information.
local class WSEndpointInfo extends EndpointInfo
{
    /// The URI configured with the endpoint.
    string resource;
}

/// Provides access to the details of an opaque endpoint.
/// @see Endpoint
local class OpaqueEndpointInfo extends EndpointInfo
{
    /// The encoding version of the opaque endpoint (to decode or encode the rawBytes).
    EncodingVersion rawEncoding;

    ///  The raw encoding of the opaque endpoint.
    ByteSeq rawBytes;
}

#endif

}
