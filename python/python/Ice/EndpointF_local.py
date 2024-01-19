# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.8.50
#
# <auto-generated>
#
# Generated from file `EndpointF.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice
import IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

if 'EndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_EndpointInfo = IcePy.declareValue('::Ice::EndpointInfo')

if 'IPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_IPEndpointInfo = IcePy.declareValue('::Ice::IPEndpointInfo')

if 'TCPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_TCPEndpointInfo = IcePy.declareValue('::Ice::TCPEndpointInfo')

if 'UDPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_UDPEndpointInfo = IcePy.declareValue('::Ice::UDPEndpointInfo')

if 'WSEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_WSEndpointInfo = IcePy.declareValue('::Ice::WSEndpointInfo')

if 'Endpoint' not in _M_Ice.__dict__:
    _M_Ice._t_Endpoint = IcePy.declareValue('::Ice::Endpoint')

if '_t_EndpointSeq' not in _M_Ice.__dict__:
    _M_Ice._t_EndpointSeq = IcePy.defineSequence('::Ice::EndpointSeq', (), _M_Ice._t_Endpoint)

# End of module Ice
