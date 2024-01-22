# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.7.10
#
# <auto-generated>
#
# Generated from file `Endpoint.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice
import IcePy
import Ice.Version_ice
import Ice.BuiltinSequences_ice
import Ice.EndpointF_local

# Included module Ice
_M_Ice = Ice.openModule('Ice')

# Start of module Ice
__name__ = 'Ice'

if 'EndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.EndpointInfo = Ice.createTempClass()

    class EndpointInfo(object):
        """
         Base class providing access to the endpoint details.
        Members:
        underlying --  The information of the underyling endpoint or null if there's no underlying endpoint.
        timeout --  The timeout for the endpoint in milliseconds. 0 means non-blocking, -1 means no timeout.
        compress --  Specifies whether or not compression should be used if available when using this endpoint.
        """

        def __init__(self, underlying=None, timeout=0, compress=False):
            if Ice.getType(self) == _M_Ice.EndpointInfo:
                raise RuntimeError('Ice.EndpointInfo is an abstract class')
            self.underlying = underlying
            self.timeout = timeout
            self.compress = compress

        def type(self):
            """
             Returns the type of the endpoint.
            Returns: The endpoint type.
            """
            raise NotImplementedError("method 'type' not implemented")

        def datagram(self):
            """
             Returns true if this endpoint is a datagram endpoint.
            Returns: True for a datagram endpoint.
            """
            raise NotImplementedError("method 'datagram' not implemented")

        def secure(self):
            """
            Returns: True for a secure endpoint.
            """
            raise NotImplementedError("method 'secure' not implemented")

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_EndpointInfo)

        __repr__ = __str__

    _M_Ice._t_EndpointInfo = IcePy.declareValue('::Ice::EndpointInfo')

    _M_Ice._t_EndpointInfo = IcePy.defineValue('::Ice::EndpointInfo', EndpointInfo, -1, (), False, False, None, (
        ('underlying', (), _M_Ice._t_EndpointInfo, False, 0),
        ('timeout', (), IcePy._t_int, False, 0),
        ('compress', (), IcePy._t_bool, False, 0)
    ))
    EndpointInfo._ice_type = _M_Ice._t_EndpointInfo

    _M_Ice.EndpointInfo = EndpointInfo
    del EndpointInfo

if 'Endpoint' not in _M_Ice.__dict__:
    _M_Ice.Endpoint = Ice.createTempClass()

    class Endpoint(object):
        """
         The user-level interface to an endpoint.
        """

        def __init__(self):
            if Ice.getType(self) == _M_Ice.Endpoint:
                raise RuntimeError('Ice.Endpoint is an abstract class')

        def toString(self):
            """
             Return a string representation of the endpoint.
            Returns: The string representation of the endpoint.
            """
            raise NotImplementedError("method 'toString' not implemented")

        def getInfo(self):
            """
             Returns the endpoint information.
            Returns: The endpoint information class.
            """
            raise NotImplementedError("method 'getInfo' not implemented")

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_Endpoint)

        __repr__ = __str__

    _M_Ice._t_Endpoint = IcePy.defineValue('::Ice::Endpoint', Endpoint, -1, (), False, True, None, ())
    Endpoint._ice_type = _M_Ice._t_Endpoint

    _M_Ice.Endpoint = Endpoint
    del Endpoint

if 'IPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.IPEndpointInfo = Ice.createTempClass()

    class IPEndpointInfo(_M_Ice.EndpointInfo):
        """
         Provides access to the address details of a IP endpoint.
        Members:
        host --  The host or address configured with the endpoint.
        port --  The port number.
        sourceAddress --  The source IP address.
        """

        def __init__(self, underlying=None, timeout=0, compress=False, host='', port=0, sourceAddress=''):
            if Ice.getType(self) == _M_Ice.IPEndpointInfo:
                raise RuntimeError('Ice.IPEndpointInfo is an abstract class')
            _M_Ice.EndpointInfo.__init__(self, underlying, timeout, compress)
            self.host = host
            self.port = port
            self.sourceAddress = sourceAddress

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_IPEndpointInfo)

        __repr__ = __str__

    _M_Ice._t_IPEndpointInfo = IcePy.declareValue('::Ice::IPEndpointInfo')

    _M_Ice._t_IPEndpointInfo = IcePy.defineValue('::Ice::IPEndpointInfo', IPEndpointInfo, -1, (), False, False, _M_Ice._t_EndpointInfo, (
        ('host', (), IcePy._t_string, False, 0),
        ('port', (), IcePy._t_int, False, 0),
        ('sourceAddress', (), IcePy._t_string, False, 0)
    ))
    IPEndpointInfo._ice_type = _M_Ice._t_IPEndpointInfo

    _M_Ice.IPEndpointInfo = IPEndpointInfo
    del IPEndpointInfo

if 'TCPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.TCPEndpointInfo = Ice.createTempClass()

    class TCPEndpointInfo(_M_Ice.IPEndpointInfo):
        """
         Provides access to a TCP endpoint information.
        """

        def __init__(self, underlying=None, timeout=0, compress=False, host='', port=0, sourceAddress=''):
            if Ice.getType(self) == _M_Ice.TCPEndpointInfo:
                raise RuntimeError('Ice.TCPEndpointInfo is an abstract class')
            _M_Ice.IPEndpointInfo.__init__(self, underlying, timeout, compress, host, port, sourceAddress)

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_TCPEndpointInfo)

        __repr__ = __str__

    _M_Ice._t_TCPEndpointInfo = IcePy.declareValue('::Ice::TCPEndpointInfo')

    _M_Ice._t_TCPEndpointInfo = IcePy.defineValue(
        '::Ice::TCPEndpointInfo', TCPEndpointInfo, -1, (), False, False, _M_Ice._t_IPEndpointInfo, ())
    TCPEndpointInfo._ice_type = _M_Ice._t_TCPEndpointInfo

    _M_Ice.TCPEndpointInfo = TCPEndpointInfo
    del TCPEndpointInfo

if 'UDPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.UDPEndpointInfo = Ice.createTempClass()

    class UDPEndpointInfo(_M_Ice.IPEndpointInfo):
        """
         Provides access to an UDP endpoint information.
        Members:
        mcastInterface --  The multicast interface.
        mcastTtl --  The multicast time-to-live (or hops).
        """

        def __init__(self, underlying=None, timeout=0, compress=False, host='', port=0, sourceAddress='', mcastInterface='', mcastTtl=0):
            if Ice.getType(self) == _M_Ice.UDPEndpointInfo:
                raise RuntimeError('Ice.UDPEndpointInfo is an abstract class')
            _M_Ice.IPEndpointInfo.__init__(self, underlying, timeout, compress, host, port, sourceAddress)
            self.mcastInterface = mcastInterface
            self.mcastTtl = mcastTtl

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_UDPEndpointInfo)

        __repr__ = __str__

    _M_Ice._t_UDPEndpointInfo = IcePy.declareValue('::Ice::UDPEndpointInfo')

    _M_Ice._t_UDPEndpointInfo = IcePy.defineValue('::Ice::UDPEndpointInfo', UDPEndpointInfo, -1, (), False, False, _M_Ice._t_IPEndpointInfo, (
        ('mcastInterface', (), IcePy._t_string, False, 0),
        ('mcastTtl', (), IcePy._t_int, False, 0)
    ))
    UDPEndpointInfo._ice_type = _M_Ice._t_UDPEndpointInfo

    _M_Ice.UDPEndpointInfo = UDPEndpointInfo
    del UDPEndpointInfo

if 'WSEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.WSEndpointInfo = Ice.createTempClass()

    class WSEndpointInfo(_M_Ice.EndpointInfo):
        """
         Provides access to a WebSocket endpoint information.
        Members:
        resource --  The URI configured with the endpoint.
        """

        def __init__(self, underlying=None, timeout=0, compress=False, resource=''):
            if Ice.getType(self) == _M_Ice.WSEndpointInfo:
                raise RuntimeError('Ice.WSEndpointInfo is an abstract class')
            _M_Ice.EndpointInfo.__init__(self, underlying, timeout, compress)
            self.resource = resource

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_WSEndpointInfo)

        __repr__ = __str__

    _M_Ice._t_WSEndpointInfo = IcePy.declareValue('::Ice::WSEndpointInfo')

    _M_Ice._t_WSEndpointInfo = IcePy.defineValue('::Ice::WSEndpointInfo', WSEndpointInfo, -1,
                                                 (), False, False, _M_Ice._t_EndpointInfo, (('resource', (), IcePy._t_string, False, 0),))
    WSEndpointInfo._ice_type = _M_Ice._t_WSEndpointInfo

    _M_Ice.WSEndpointInfo = WSEndpointInfo
    del WSEndpointInfo

if 'OpaqueEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice.OpaqueEndpointInfo = Ice.createTempClass()

    class OpaqueEndpointInfo(_M_Ice.EndpointInfo):
        """
         Provides access to the details of an opaque endpoint.
        Members:
        rawEncoding --  The encoding version of the opaque endpoint (to decode or encode the rawBytes).
        rawBytes --   The raw encoding of the opaque endpoint.
        """

        def __init__(self, underlying=None, timeout=0, compress=False, rawEncoding=Ice._struct_marker, rawBytes=None):
            if Ice.getType(self) == _M_Ice.OpaqueEndpointInfo:
                raise RuntimeError('Ice.OpaqueEndpointInfo is an abstract class')
            _M_Ice.EndpointInfo.__init__(self, underlying, timeout, compress)
            if rawEncoding is Ice._struct_marker:
                self.rawEncoding = _M_Ice.EncodingVersion()
            else:
                self.rawEncoding = rawEncoding
            self.rawBytes = rawBytes

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_OpaqueEndpointInfo)

        __repr__ = __str__

    _M_Ice._t_OpaqueEndpointInfo = IcePy.declareValue('::Ice::OpaqueEndpointInfo')

    _M_Ice._t_OpaqueEndpointInfo = IcePy.defineValue('::Ice::OpaqueEndpointInfo', OpaqueEndpointInfo, -1, (), False, False, _M_Ice._t_EndpointInfo, (
        ('rawEncoding', (), _M_Ice._t_EncodingVersion, False, 0),
        ('rawBytes', (), _M_Ice._t_ByteSeq, False, 0)
    ))
    OpaqueEndpointInfo._ice_type = _M_Ice._t_OpaqueEndpointInfo

    _M_Ice.OpaqueEndpointInfo = OpaqueEndpointInfo
    del OpaqueEndpointInfo

# End of module Ice
