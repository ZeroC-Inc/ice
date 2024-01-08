# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.7.10
#
# <auto-generated>
#
# Generated from file `EndpointSelectionType.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

from sys import version_info as _version_info_
import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

if 'EndpointSelectionType' not in _M_Ice.__dict__:
    _M_Ice.EndpointSelectionType = Ice.createTempClass()
    class EndpointSelectionType(Ice.EnumBase):
        """
         Determines the order in which the Ice run time uses the endpoints in a proxy when establishing a connection.
        Enumerators:
        Random --  Random causes the endpoints to be arranged in a random order.
        Ordered --  Ordered forces the Ice run time to use the endpoints in the order they appeared in the proxy.
        """

        def __init__(self, _n, _v):
            Ice.EnumBase.__init__(self, _n, _v)

        def valueOf(self, _n):
            if _n in self._enumerators:
                return self._enumerators[_n]
            return None
        valueOf = classmethod(valueOf)

    EndpointSelectionType.Random = EndpointSelectionType("Random", 0)
    EndpointSelectionType.Ordered = EndpointSelectionType("Ordered", 1)
    EndpointSelectionType._enumerators = { 0:EndpointSelectionType.Random, 1:EndpointSelectionType.Ordered }

    _M_Ice._t_EndpointSelectionType = IcePy.defineEnum('::Ice::EndpointSelectionType', EndpointSelectionType, (), EndpointSelectionType._enumerators)

    _M_Ice.EndpointSelectionType = EndpointSelectionType
    del EndpointSelectionType

# End of module Ice
