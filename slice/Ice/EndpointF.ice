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

[["java:package:com.zeroc"]]

["objc:prefix:ICE"]
module Ice
{

local class EndpointInfo;
local class IPEndpointInfo;
local class TCPEndpointInfo;
local class UDPEndpointInfo;
local class WSEndpointInfo;
local interface Endpoint;

/// A sequence of endpoints.
["swift:nonnull"] local sequence<Endpoint> EndpointSeq;

}
