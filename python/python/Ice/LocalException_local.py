# -*- coding: utf-8 -*-
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

import Ice
import IcePy
import Ice.Identity_ice
import Ice.Version_ice
import Ice.BuiltinSequences_ice

# Included module Ice
_M_Ice = Ice.openModule("Ice")

# Start of module Ice
__name__ = "Ice"

if "InitializationException" not in _M_Ice.__dict__:
    _M_Ice.InitializationException = Ice.createTempClass()

    class InitializationException(Ice.LocalException):
        """
         This exception is raised when a failure occurs during initialization.
        Members:
        reason --  The reason for the failure.
        """

        def __init__(self, reason=""):
            self.reason = reason

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::InitializationException"

    _M_Ice._t_InitializationException = IcePy.defineException(
        "::Ice::InitializationException",
        InitializationException,
        (),
        None,
        (("reason", (), IcePy._t_string, False, 0),),
    )
    InitializationException._ice_type = _M_Ice._t_InitializationException

    _M_Ice.InitializationException = InitializationException
    del InitializationException

if "PluginInitializationException" not in _M_Ice.__dict__:
    _M_Ice.PluginInitializationException = Ice.createTempClass()

    class PluginInitializationException(Ice.LocalException):
        """
         This exception indicates that a failure occurred while initializing a plug-in.
        Members:
        reason --  The reason for the failure.
        """

        def __init__(self, reason=""):
            self.reason = reason

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::PluginInitializationException"

    _M_Ice._t_PluginInitializationException = IcePy.defineException(
        "::Ice::PluginInitializationException",
        PluginInitializationException,
        (),
        None,
        (("reason", (), IcePy._t_string, False, 0),),
    )
    PluginInitializationException._ice_type = _M_Ice._t_PluginInitializationException

    _M_Ice.PluginInitializationException = PluginInitializationException
    del PluginInitializationException

if "AlreadyRegisteredException" not in _M_Ice.__dict__:
    _M_Ice.AlreadyRegisteredException = Ice.createTempClass()

    class AlreadyRegisteredException(Ice.LocalException):
        """
         An attempt was made to register something more than once with the Ice run time. This exception is raised if an
         attempt is made to register a servant, servant locator, facet, value factory, plug-in, object adapter, object, or
         user exception factory more than once for the same ID.
        Members:
        kindOfObject --  The kind of object that could not be removed: "servant", "facet", "object", "default servant",
         "servant locator", "value factory", "plugin", "object adapter", "object adapter with router", "replica group".
        id --  The ID (or name) of the object that is registered already.
        """

        def __init__(self, kindOfObject="", id=""):
            self.kindOfObject = kindOfObject
            self.id = id

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::AlreadyRegisteredException"

    _M_Ice._t_AlreadyRegisteredException = IcePy.defineException(
        "::Ice::AlreadyRegisteredException",
        AlreadyRegisteredException,
        (),
        None,
        (
            ("kindOfObject", (), IcePy._t_string, False, 0),
            ("id", (), IcePy._t_string, False, 0),
        ),
    )
    AlreadyRegisteredException._ice_type = _M_Ice._t_AlreadyRegisteredException

    _M_Ice.AlreadyRegisteredException = AlreadyRegisteredException
    del AlreadyRegisteredException

if "NotRegisteredException" not in _M_Ice.__dict__:
    _M_Ice.NotRegisteredException = Ice.createTempClass()

    class NotRegisteredException(Ice.LocalException):
        """
         An attempt was made to find or deregister something that is not registered with the Ice run time or Ice locator.
         This exception is raised if an attempt is made to remove a servant, servant locator, facet, value factory, plug-in,
         object adapter, object, or user exception factory that is not currently registered. It's also raised if the Ice
         locator can't find an object or object adapter when resolving an indirect proxy or when an object adapter is
         activated.
        Members:
        kindOfObject --  The kind of object that could not be removed: "servant", "facet", "object", "default servant",
         "servant locator", "value factory", "plugin", "object adapter", "object adapter with router", "replica group".
        id --  The ID (or name) of the object that could not be removed.
        """

        def __init__(self, kindOfObject="", id=""):
            self.kindOfObject = kindOfObject
            self.id = id

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::NotRegisteredException"

    _M_Ice._t_NotRegisteredException = IcePy.defineException(
        "::Ice::NotRegisteredException",
        NotRegisteredException,
        (),
        None,
        (
            ("kindOfObject", (), IcePy._t_string, False, 0),
            ("id", (), IcePy._t_string, False, 0),
        ),
    )
    NotRegisteredException._ice_type = _M_Ice._t_NotRegisteredException

    _M_Ice.NotRegisteredException = NotRegisteredException
    del NotRegisteredException

if "TwowayOnlyException" not in _M_Ice.__dict__:
    _M_Ice.TwowayOnlyException = Ice.createTempClass()

    class TwowayOnlyException(Ice.LocalException):
        """
         The operation can only be invoked with a twoway request. This exception is raised if an attempt is made to invoke
         an operation with ice_oneway, ice_batchOneway, ice_datagram, or
         ice_batchDatagram and the operation has a return value, out-parameters, or an exception specification.
        Members:
        operation --  The name of the operation that was invoked.
        """

        def __init__(self, operation=""):
            self.operation = operation

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::TwowayOnlyException"

    _M_Ice._t_TwowayOnlyException = IcePy.defineException(
        "::Ice::TwowayOnlyException",
        TwowayOnlyException,
        (),
        None,
        (("operation", (), IcePy._t_string, False, 0),),
    )
    TwowayOnlyException._ice_type = _M_Ice._t_TwowayOnlyException

    _M_Ice.TwowayOnlyException = TwowayOnlyException
    del TwowayOnlyException

if "UnknownException" not in _M_Ice.__dict__:
    _M_Ice.UnknownException = Ice.createTempClass()

    class UnknownException(Ice.LocalException):
        """
         This exception is raised if an operation call on a server raises an unknown exception. For example, for C++, this
         exception is raised if the server throws a C++ exception that is not directly or indirectly derived from
         Ice::LocalException or Ice::UserException.
        Members:
        unknown --  This field is set to the textual representation of the unknown exception if available.
        """

        def __init__(self, unknown=""):
            self.unknown = unknown

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::UnknownException"

    _M_Ice._t_UnknownException = IcePy.defineException(
        "::Ice::UnknownException",
        UnknownException,
        (),
        None,
        (("unknown", (), IcePy._t_string, False, 0),),
    )
    UnknownException._ice_type = _M_Ice._t_UnknownException

    _M_Ice.UnknownException = UnknownException
    del UnknownException

if "UnknownLocalException" not in _M_Ice.__dict__:
    _M_Ice.UnknownLocalException = Ice.createTempClass()

    class UnknownLocalException(_M_Ice.UnknownException):
        """
        This exception is raised if an operation call on a server raises a  local exception. Because local exceptions are
        not transmitted by the Ice protocol, the client receives all local exceptions raised by the server as
        UnknownLocalException. The only exception to this rule are all exceptions derived from
        RequestFailedException, which are transmitted by the Ice protocol even though they are declared
        local.
        """

        def __init__(self, unknown=""):
            _M_Ice.UnknownException.__init__(self, unknown)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::UnknownLocalException"

    _M_Ice._t_UnknownLocalException = IcePy.defineException(
        "::Ice::UnknownLocalException",
        UnknownLocalException,
        (),
        _M_Ice._t_UnknownException,
        (),
    )
    UnknownLocalException._ice_type = _M_Ice._t_UnknownLocalException

    _M_Ice.UnknownLocalException = UnknownLocalException
    del UnknownLocalException

if "UnknownUserException" not in _M_Ice.__dict__:
    _M_Ice.UnknownUserException = Ice.createTempClass()

    class UnknownUserException(_M_Ice.UnknownException):
        """
        An operation raised an incorrect user exception. This exception is raised if an operation raises a user exception
        that is not declared in the exception's throws clause. Such undeclared exceptions are not transmitted
        from the server to the client by the Ice protocol, but instead the client just gets an UnknownUserException.
        This is necessary in order to not violate the contract established by an operation's signature: Only local
        exceptions and user exceptions declared in the throws clause can be raised.
        """

        def __init__(self, unknown=""):
            _M_Ice.UnknownException.__init__(self, unknown)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::UnknownUserException"

    _M_Ice._t_UnknownUserException = IcePy.defineException(
        "::Ice::UnknownUserException",
        UnknownUserException,
        (),
        _M_Ice._t_UnknownException,
        (),
    )
    UnknownUserException._ice_type = _M_Ice._t_UnknownUserException

    _M_Ice.UnknownUserException = UnknownUserException
    del UnknownUserException

if "CommunicatorDestroyedException" not in _M_Ice.__dict__:
    _M_Ice.CommunicatorDestroyedException = Ice.createTempClass()

    class CommunicatorDestroyedException(Ice.LocalException):
        """
        This exception is raised if the Communicator has been destroyed.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::CommunicatorDestroyedException"

    _M_Ice._t_CommunicatorDestroyedException = IcePy.defineException(
        "::Ice::CommunicatorDestroyedException",
        CommunicatorDestroyedException,
        (),
        None,
        (),
    )
    CommunicatorDestroyedException._ice_type = _M_Ice._t_CommunicatorDestroyedException

    _M_Ice.CommunicatorDestroyedException = CommunicatorDestroyedException
    del CommunicatorDestroyedException

if "ObjectAdapterDeactivatedException" not in _M_Ice.__dict__:
    _M_Ice.ObjectAdapterDeactivatedException = Ice.createTempClass()

    class ObjectAdapterDeactivatedException(Ice.LocalException):
        """
         This exception is raised if an attempt is made to use a deactivated ObjectAdapter.
        Members:
        name --  Name of the adapter.
        """

        def __init__(self, name=""):
            self.name = name

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ObjectAdapterDeactivatedException"

    _M_Ice._t_ObjectAdapterDeactivatedException = IcePy.defineException(
        "::Ice::ObjectAdapterDeactivatedException",
        ObjectAdapterDeactivatedException,
        (),
        None,
        (("name", (), IcePy._t_string, False, 0),),
    )
    ObjectAdapterDeactivatedException._ice_type = (
        _M_Ice._t_ObjectAdapterDeactivatedException
    )

    _M_Ice.ObjectAdapterDeactivatedException = ObjectAdapterDeactivatedException
    del ObjectAdapterDeactivatedException

if "ObjectAdapterIdInUseException" not in _M_Ice.__dict__:
    _M_Ice.ObjectAdapterIdInUseException = Ice.createTempClass()

    class ObjectAdapterIdInUseException(Ice.LocalException):
        """
         This exception is raised if an ObjectAdapter cannot be activated. This happens if the Locator
         detects another active ObjectAdapter with the same adapter id.
        Members:
        id --  Adapter ID.
        """

        def __init__(self, id=""):
            self.id = id

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ObjectAdapterIdInUseException"

    _M_Ice._t_ObjectAdapterIdInUseException = IcePy.defineException(
        "::Ice::ObjectAdapterIdInUseException",
        ObjectAdapterIdInUseException,
        (),
        None,
        (("id", (), IcePy._t_string, False, 0),),
    )
    ObjectAdapterIdInUseException._ice_type = _M_Ice._t_ObjectAdapterIdInUseException

    _M_Ice.ObjectAdapterIdInUseException = ObjectAdapterIdInUseException
    del ObjectAdapterIdInUseException

if "NoEndpointException" not in _M_Ice.__dict__:
    _M_Ice.NoEndpointException = Ice.createTempClass()

    class NoEndpointException(Ice.LocalException):
        """
         This exception is raised if no suitable endpoint is available.
        Members:
        proxy --  The stringified proxy for which no suitable endpoint is available.
        """

        def __init__(self, proxy=""):
            self.proxy = proxy

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::NoEndpointException"

    _M_Ice._t_NoEndpointException = IcePy.defineException(
        "::Ice::NoEndpointException",
        NoEndpointException,
        (),
        None,
        (("proxy", (), IcePy._t_string, False, 0),),
    )
    NoEndpointException._ice_type = _M_Ice._t_NoEndpointException

    _M_Ice.NoEndpointException = NoEndpointException
    del NoEndpointException

if "ParseException" not in _M_Ice.__dict__:
    _M_Ice.ParseException = Ice.createTempClass()

    class ParseException(Ice.LocalException):
        """
         This exception is raised if there was an error while parsing a string.
        Members:
        str --  Describes the failure and includes the string that could not be parsed.
        """

        def __init__(self, str=""):
            self.str = str

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ParseException"

    _M_Ice._t_ParseException = IcePy.defineException(
        "::Ice::ParseException",
        ParseException,
        (),
        None,
        (("str", (), IcePy._t_string, False, 0),),
    )
    ParseException._ice_type = _M_Ice._t_ParseException

    _M_Ice.ParseException = ParseException
    del ParseException

if "IllegalIdentityException" not in _M_Ice.__dict__:
    _M_Ice.IllegalIdentityException = Ice.createTempClass()

    class IllegalIdentityException(Ice.LocalException):
        """
          This exception is raised if an identity with an empty name is encountered.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::IllegalIdentityException"

    _M_Ice._t_IllegalIdentityException = IcePy.defineException(
        "::Ice::IllegalIdentityException",
        IllegalIdentityException,
        (),
        None,
        (),
    )
    IllegalIdentityException._ice_type = _M_Ice._t_IllegalIdentityException

    _M_Ice.IllegalIdentityException = IllegalIdentityException
    del IllegalIdentityException

if "IllegalServantException" not in _M_Ice.__dict__:
    _M_Ice.IllegalServantException = Ice.createTempClass()

    class IllegalServantException(Ice.LocalException):
        """
         This exception is raised to reject an illegal servant (typically a null servant).
        Members:
        reason --  Describes why this servant is illegal.
        """

        def __init__(self, reason=""):
            self.reason = reason

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::IllegalServantException"

    _M_Ice._t_IllegalServantException = IcePy.defineException(
        "::Ice::IllegalServantException",
        IllegalServantException,
        (),
        None,
        (("reason", (), IcePy._t_string, False, 0),),
    )
    IllegalServantException._ice_type = _M_Ice._t_IllegalServantException

    _M_Ice.IllegalServantException = IllegalServantException
    del IllegalServantException

if "RequestFailedException" not in _M_Ice.__dict__:
    _M_Ice.RequestFailedException = Ice.createTempClass()

    class RequestFailedException(Ice.LocalException):
        """
         This exception is raised if a request failed. This exception, and all exceptions derived from
         RequestFailedException, are transmitted by the Ice protocol, even though they are declared
         local.
        Members:
        id --  The identity of the Ice Object to which the request was sent.
        facet --  The facet to which the request was sent.
        operation --  The operation name of the request.
        """

        def __init__(self, id=Ice._struct_marker, facet="", operation=""):
            if id is Ice._struct_marker:
                self.id = _M_Ice.Identity()
            else:
                self.id = id
            self.facet = facet
            self.operation = operation

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::RequestFailedException"

    _M_Ice._t_RequestFailedException = IcePy.defineException(
        "::Ice::RequestFailedException",
        RequestFailedException,
        (),
        None,
        (
            ("id", (), _M_Ice._t_Identity, False, 0),
            ("facet", (), IcePy._t_string, False, 0),
            ("operation", (), IcePy._t_string, False, 0),
        ),
    )
    RequestFailedException._ice_type = _M_Ice._t_RequestFailedException

    _M_Ice.RequestFailedException = RequestFailedException
    del RequestFailedException

if "ObjectNotExistException" not in _M_Ice.__dict__:
    _M_Ice.ObjectNotExistException = Ice.createTempClass()

    class ObjectNotExistException(_M_Ice.RequestFailedException):
        """
        This exception is raised if an object does not exist on the server, that is, if no facets with the given identity
        exist.
        """

        def __init__(self, id=Ice._struct_marker, facet="", operation=""):
            _M_Ice.RequestFailedException.__init__(self, id, facet, operation)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ObjectNotExistException"

    _M_Ice._t_ObjectNotExistException = IcePy.defineException(
        "::Ice::ObjectNotExistException",
        ObjectNotExistException,
        (),
        _M_Ice._t_RequestFailedException,
        (),
    )
    ObjectNotExistException._ice_type = _M_Ice._t_ObjectNotExistException

    _M_Ice.ObjectNotExistException = ObjectNotExistException
    del ObjectNotExistException

if "FacetNotExistException" not in _M_Ice.__dict__:
    _M_Ice.FacetNotExistException = Ice.createTempClass()

    class FacetNotExistException(_M_Ice.RequestFailedException):
        """
        This exception is raised if no facet with the given name exists, but at least one facet with the given identity
        exists.
        """

        def __init__(self, id=Ice._struct_marker, facet="", operation=""):
            _M_Ice.RequestFailedException.__init__(self, id, facet, operation)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::FacetNotExistException"

    _M_Ice._t_FacetNotExistException = IcePy.defineException(
        "::Ice::FacetNotExistException",
        FacetNotExistException,
        (),
        _M_Ice._t_RequestFailedException,
        (),
    )
    FacetNotExistException._ice_type = _M_Ice._t_FacetNotExistException

    _M_Ice.FacetNotExistException = FacetNotExistException
    del FacetNotExistException

if "OperationNotExistException" not in _M_Ice.__dict__:
    _M_Ice.OperationNotExistException = Ice.createTempClass()

    class OperationNotExistException(_M_Ice.RequestFailedException):
        """
        This exception is raised if an operation for a given object does not exist on the server. Typically this is caused
        by either the client or the server using an outdated Slice specification.
        """

        def __init__(self, id=Ice._struct_marker, facet="", operation=""):
            _M_Ice.RequestFailedException.__init__(self, id, facet, operation)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::OperationNotExistException"

    _M_Ice._t_OperationNotExistException = IcePy.defineException(
        "::Ice::OperationNotExistException",
        OperationNotExistException,
        (),
        _M_Ice._t_RequestFailedException,
        (),
    )
    OperationNotExistException._ice_type = _M_Ice._t_OperationNotExistException

    _M_Ice.OperationNotExistException = OperationNotExistException
    del OperationNotExistException

if "SyscallException" not in _M_Ice.__dict__:
    _M_Ice.SyscallException = Ice.createTempClass()

    class SyscallException(Ice.LocalException):
        """
         This exception is raised if a system error occurred in the server or client process. There are many possible causes
         for such a system exception. For details on the cause, SyscallException#error should be inspected.
        Members:
        error --  The error number describing the system exception. For C++ and Unix, this is equivalent to errno.
         For C++ and Windows, this is the value returned by GetLastError() or
         WSAGetLastError().
        """

        def __init__(self, error=0):
            self.error = error

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::SyscallException"

    _M_Ice._t_SyscallException = IcePy.defineException(
        "::Ice::SyscallException",
        SyscallException,
        (),
        None,
        (("error", (), IcePy._t_int, False, 0),),
    )
    SyscallException._ice_type = _M_Ice._t_SyscallException

    _M_Ice.SyscallException = SyscallException
    del SyscallException

if "SocketException" not in _M_Ice.__dict__:
    _M_Ice.SocketException = Ice.createTempClass()

    class SocketException(_M_Ice.SyscallException):
        """
        This exception indicates socket errors.
        """

        def __init__(self, error=0):
            _M_Ice.SyscallException.__init__(self, error)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::SocketException"

    _M_Ice._t_SocketException = IcePy.defineException(
        "::Ice::SocketException",
        SocketException,
        (),
        _M_Ice._t_SyscallException,
        (),
    )
    SocketException._ice_type = _M_Ice._t_SocketException

    _M_Ice.SocketException = SocketException
    del SocketException

if "CFNetworkException" not in _M_Ice.__dict__:
    _M_Ice.CFNetworkException = Ice.createTempClass()

    class CFNetworkException(_M_Ice.SocketException):
        """
         This exception indicates CFNetwork errors.
        Members:
        domain --  The domain of the error.
        """

        def __init__(self, error=0, domain=""):
            _M_Ice.SocketException.__init__(self, error)
            self.domain = domain

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::CFNetworkException"

    _M_Ice._t_CFNetworkException = IcePy.defineException(
        "::Ice::CFNetworkException",
        CFNetworkException,
        (),
        _M_Ice._t_SocketException,
        (("domain", (), IcePy._t_string, False, 0),),
    )
    CFNetworkException._ice_type = _M_Ice._t_CFNetworkException

    _M_Ice.CFNetworkException = CFNetworkException
    del CFNetworkException

if "FileException" not in _M_Ice.__dict__:
    _M_Ice.FileException = Ice.createTempClass()

    class FileException(_M_Ice.SyscallException):
        """
         This exception indicates file errors.
        Members:
        path --  The path of the file responsible for the error.
        """

        def __init__(self, error=0, path=""):
            _M_Ice.SyscallException.__init__(self, error)
            self.path = path

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::FileException"

    _M_Ice._t_FileException = IcePy.defineException(
        "::Ice::FileException",
        FileException,
        (),
        _M_Ice._t_SyscallException,
        (("path", (), IcePy._t_string, False, 0),),
    )
    FileException._ice_type = _M_Ice._t_FileException

    _M_Ice.FileException = FileException
    del FileException

if "ConnectFailedException" not in _M_Ice.__dict__:
    _M_Ice.ConnectFailedException = Ice.createTempClass()

    class ConnectFailedException(_M_Ice.SocketException):
        """
        This exception indicates connection failures.
        """

        def __init__(self, error=0):
            _M_Ice.SocketException.__init__(self, error)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectFailedException"

    _M_Ice._t_ConnectFailedException = IcePy.defineException(
        "::Ice::ConnectFailedException",
        ConnectFailedException,
        (),
        _M_Ice._t_SocketException,
        (),
    )
    ConnectFailedException._ice_type = _M_Ice._t_ConnectFailedException

    _M_Ice.ConnectFailedException = ConnectFailedException
    del ConnectFailedException

if "ConnectionRefusedException" not in _M_Ice.__dict__:
    _M_Ice.ConnectionRefusedException = Ice.createTempClass()

    class ConnectionRefusedException(_M_Ice.ConnectFailedException):
        """
        This exception indicates a connection failure for which the server host actively refuses a connection.
        """

        def __init__(self, error=0):
            _M_Ice.ConnectFailedException.__init__(self, error)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectionRefusedException"

    _M_Ice._t_ConnectionRefusedException = IcePy.defineException(
        "::Ice::ConnectionRefusedException",
        ConnectionRefusedException,
        (),
        _M_Ice._t_ConnectFailedException,
        (),
    )
    ConnectionRefusedException._ice_type = _M_Ice._t_ConnectionRefusedException

    _M_Ice.ConnectionRefusedException = ConnectionRefusedException
    del ConnectionRefusedException

if "ConnectionLostException" not in _M_Ice.__dict__:
    _M_Ice.ConnectionLostException = Ice.createTempClass()

    class ConnectionLostException(_M_Ice.SocketException):
        """
        This exception indicates a lost connection.
        """

        def __init__(self, error=0):
            _M_Ice.SocketException.__init__(self, error)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectionLostException"

    _M_Ice._t_ConnectionLostException = IcePy.defineException(
        "::Ice::ConnectionLostException",
        ConnectionLostException,
        (),
        _M_Ice._t_SocketException,
        (),
    )
    ConnectionLostException._ice_type = _M_Ice._t_ConnectionLostException

    _M_Ice.ConnectionLostException = ConnectionLostException
    del ConnectionLostException

if "DNSException" not in _M_Ice.__dict__:
    _M_Ice.DNSException = Ice.createTempClass()

    class DNSException(Ice.LocalException):
        """
         This exception indicates a DNS problem. For details on the cause, DNSException#error should be inspected.
        Members:
        error --  The error number describing the DNS problem. For C++ and Unix, this is equivalent to h_errno. For
         C++ and Windows, this is the value returned by WSAGetLastError().
        host --  The host name that could not be resolved.
        """

        def __init__(self, error=0, host=""):
            self.error = error
            self.host = host

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::DNSException"

    _M_Ice._t_DNSException = IcePy.defineException(
        "::Ice::DNSException",
        DNSException,
        (),
        None,
        (
            ("error", (), IcePy._t_int, False, 0),
            ("host", (), IcePy._t_string, False, 0),
        ),
    )
    DNSException._ice_type = _M_Ice._t_DNSException

    _M_Ice.DNSException = DNSException
    del DNSException

if "ConnectionIdleException" not in _M_Ice.__dict__:
    _M_Ice.ConnectionIdleException = Ice.createTempClass()

    class ConnectionIdleException(Ice.LocalException):
        """
        This exception indicates that a connection was aborted by the idle check.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectionIdleException"

    _M_Ice._t_ConnectionIdleException = IcePy.defineException(
        "::Ice::ConnectionIdleException",
        ConnectionIdleException,
        (),
        None,
        (),
    )
    ConnectionIdleException._ice_type = _M_Ice._t_ConnectionIdleException

    _M_Ice.ConnectionIdleException = ConnectionIdleException
    del ConnectionIdleException

if "TimeoutException" not in _M_Ice.__dict__:
    _M_Ice.TimeoutException = Ice.createTempClass()

    class TimeoutException(Ice.LocalException):
        """
        This exception indicates a timeout condition.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::TimeoutException"

    _M_Ice._t_TimeoutException = IcePy.defineException(
        "::Ice::TimeoutException", TimeoutException, (), None, ()
    )
    TimeoutException._ice_type = _M_Ice._t_TimeoutException

    _M_Ice.TimeoutException = TimeoutException
    del TimeoutException

if "ConnectTimeoutException" not in _M_Ice.__dict__:
    _M_Ice.ConnectTimeoutException = Ice.createTempClass()

    class ConnectTimeoutException(_M_Ice.TimeoutException):
        """
        This exception indicates a connection establishment timeout condition.
        """

        def __init__(self):
            _M_Ice.TimeoutException.__init__(self)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectTimeoutException"

    _M_Ice._t_ConnectTimeoutException = IcePy.defineException(
        "::Ice::ConnectTimeoutException",
        ConnectTimeoutException,
        (),
        _M_Ice._t_TimeoutException,
        (),
    )
    ConnectTimeoutException._ice_type = _M_Ice._t_ConnectTimeoutException

    _M_Ice.ConnectTimeoutException = ConnectTimeoutException
    del ConnectTimeoutException

if "CloseTimeoutException" not in _M_Ice.__dict__:
    _M_Ice.CloseTimeoutException = Ice.createTempClass()

    class CloseTimeoutException(_M_Ice.TimeoutException):
        """
        This exception indicates a connection closure timeout condition.
        """

        def __init__(self):
            _M_Ice.TimeoutException.__init__(self)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::CloseTimeoutException"

    _M_Ice._t_CloseTimeoutException = IcePy.defineException(
        "::Ice::CloseTimeoutException",
        CloseTimeoutException,
        (),
        _M_Ice._t_TimeoutException,
        (),
    )
    CloseTimeoutException._ice_type = _M_Ice._t_CloseTimeoutException

    _M_Ice.CloseTimeoutException = CloseTimeoutException
    del CloseTimeoutException

if "InvocationTimeoutException" not in _M_Ice.__dict__:
    _M_Ice.InvocationTimeoutException = Ice.createTempClass()

    class InvocationTimeoutException(_M_Ice.TimeoutException):
        """
        This exception indicates that an invocation failed because it timed out.
        """

        def __init__(self):
            _M_Ice.TimeoutException.__init__(self)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::InvocationTimeoutException"

    _M_Ice._t_InvocationTimeoutException = IcePy.defineException(
        "::Ice::InvocationTimeoutException",
        InvocationTimeoutException,
        (),
        _M_Ice._t_TimeoutException,
        (),
    )
    InvocationTimeoutException._ice_type = _M_Ice._t_InvocationTimeoutException

    _M_Ice.InvocationTimeoutException = InvocationTimeoutException
    del InvocationTimeoutException

if "InvocationCanceledException" not in _M_Ice.__dict__:
    _M_Ice.InvocationCanceledException = Ice.createTempClass()

    class InvocationCanceledException(Ice.LocalException):
        """
        This exception indicates that an asynchronous invocation failed because it was canceled explicitly by the user.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::InvocationCanceledException"

    _M_Ice._t_InvocationCanceledException = IcePy.defineException(
        "::Ice::InvocationCanceledException",
        InvocationCanceledException,
        (),
        None,
        (),
    )
    InvocationCanceledException._ice_type = _M_Ice._t_InvocationCanceledException

    _M_Ice.InvocationCanceledException = InvocationCanceledException
    del InvocationCanceledException

if "ProtocolException" not in _M_Ice.__dict__:
    _M_Ice.ProtocolException = Ice.createTempClass()

    class ProtocolException(Ice.LocalException):
        """
         A generic exception base for all kinds of protocol error conditions.
        Members:
        reason --  The reason for the failure.
        """

        def __init__(self, reason=""):
            self.reason = reason

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ProtocolException"

    _M_Ice._t_ProtocolException = IcePy.defineException(
        "::Ice::ProtocolException",
        ProtocolException,
        (),
        None,
        (("reason", (), IcePy._t_string, False, 0),),
    )
    ProtocolException._ice_type = _M_Ice._t_ProtocolException

    _M_Ice.ProtocolException = ProtocolException
    del ProtocolException

if "CloseConnectionException" not in _M_Ice.__dict__:
    _M_Ice.CloseConnectionException = Ice.createTempClass()

    class CloseConnectionException(_M_Ice.ProtocolException):
        """
        This exception indicates that the connection has been gracefully shut down by the server. The operation call that
        caused this exception has not been executed by the server. In most cases you will not get this exception, because
        the client will automatically retry the operation call in case the server shut down the connection. However, if
        upon retry the server shuts down the connection again, and the retry limit has been reached, then this exception is
        propagated to the application code.
        """

        def __init__(self, reason=""):
            _M_Ice.ProtocolException.__init__(self, reason)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::CloseConnectionException"

    _M_Ice._t_CloseConnectionException = IcePy.defineException(
        "::Ice::CloseConnectionException",
        CloseConnectionException,
        (),
        _M_Ice._t_ProtocolException,
        (),
    )
    CloseConnectionException._ice_type = _M_Ice._t_CloseConnectionException

    _M_Ice.CloseConnectionException = CloseConnectionException
    del CloseConnectionException

if "ConnectionManuallyClosedException" not in _M_Ice.__dict__:
    _M_Ice.ConnectionManuallyClosedException = Ice.createTempClass()

    class ConnectionManuallyClosedException(Ice.LocalException):
        """
         This exception is raised by an operation call if the application closes the connection locally using
         Connection#close.
        Members:
        graceful --  True if the connection was closed gracefully, false otherwise.
        """

        def __init__(self, graceful=False):
            self.graceful = graceful

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::ConnectionManuallyClosedException"

    _M_Ice._t_ConnectionManuallyClosedException = IcePy.defineException(
        "::Ice::ConnectionManuallyClosedException",
        ConnectionManuallyClosedException,
        (),
        None,
        (("graceful", (), IcePy._t_bool, False, 0),),
    )
    ConnectionManuallyClosedException._ice_type = (
        _M_Ice._t_ConnectionManuallyClosedException
    )

    _M_Ice.ConnectionManuallyClosedException = ConnectionManuallyClosedException
    del ConnectionManuallyClosedException

if "DatagramLimitException" not in _M_Ice.__dict__:
    _M_Ice.DatagramLimitException = Ice.createTempClass()

    class DatagramLimitException(_M_Ice.ProtocolException):
        """
        A datagram exceeds the configured size. This exception is raised if a datagram exceeds the configured send or
        receive buffer size, or exceeds the maximum payload size of a UDP packet (65507 bytes).
        """

        def __init__(self, reason=""):
            _M_Ice.ProtocolException.__init__(self, reason)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::DatagramLimitException"

    _M_Ice._t_DatagramLimitException = IcePy.defineException(
        "::Ice::DatagramLimitException",
        DatagramLimitException,
        (),
        _M_Ice._t_ProtocolException,
        (),
    )
    DatagramLimitException._ice_type = _M_Ice._t_DatagramLimitException

    _M_Ice.DatagramLimitException = DatagramLimitException
    del DatagramLimitException

if "MarshalException" not in _M_Ice.__dict__:
    _M_Ice.MarshalException = Ice.createTempClass()

    class MarshalException(_M_Ice.ProtocolException):
        """
        This exception is raised for errors during marshaling or unmarshaling data.
        """

        def __init__(self, reason=""):
            _M_Ice.ProtocolException.__init__(self, reason)

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::MarshalException"

    _M_Ice._t_MarshalException = IcePy.defineException(
        "::Ice::MarshalException",
        MarshalException,
        (),
        _M_Ice._t_ProtocolException,
        (),
    )
    MarshalException._ice_type = _M_Ice._t_MarshalException

    _M_Ice.MarshalException = MarshalException
    del MarshalException

if "FeatureNotSupportedException" not in _M_Ice.__dict__:
    _M_Ice.FeatureNotSupportedException = Ice.createTempClass()

    class FeatureNotSupportedException(Ice.LocalException):
        """
         This exception is raised if an unsupported feature is used. The unsupported feature string contains the name of the
         unsupported feature.
        Members:
        unsupportedFeature --  The name of the unsupported feature.
        """

        def __init__(self, unsupportedFeature=""):
            self.unsupportedFeature = unsupportedFeature

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::FeatureNotSupportedException"

    _M_Ice._t_FeatureNotSupportedException = IcePy.defineException(
        "::Ice::FeatureNotSupportedException",
        FeatureNotSupportedException,
        (),
        None,
        (("unsupportedFeature", (), IcePy._t_string, False, 0),),
    )
    FeatureNotSupportedException._ice_type = _M_Ice._t_FeatureNotSupportedException

    _M_Ice.FeatureNotSupportedException = FeatureNotSupportedException
    del FeatureNotSupportedException

if "SecurityException" not in _M_Ice.__dict__:
    _M_Ice.SecurityException = Ice.createTempClass()

    class SecurityException(Ice.LocalException):
        """
         This exception indicates a failure in a security subsystem, such as the IceSSL plug-in.
        Members:
        reason --  The reason for the failure.
        """

        def __init__(self, reason=""):
            self.reason = reason

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::SecurityException"

    _M_Ice._t_SecurityException = IcePy.defineException(
        "::Ice::SecurityException",
        SecurityException,
        (),
        None,
        (("reason", (), IcePy._t_string, False, 0),),
    )
    SecurityException._ice_type = _M_Ice._t_SecurityException

    _M_Ice.SecurityException = SecurityException
    del SecurityException

if "FixedProxyException" not in _M_Ice.__dict__:
    _M_Ice.FixedProxyException = Ice.createTempClass()

    class FixedProxyException(Ice.LocalException):
        """
        This exception indicates that an attempt has been made to change the connection properties of a fixed proxy.
        """

        def __init__(self):
            pass

        def __str__(self):
            return IcePy.stringifyException(self)

        __repr__ = __str__

        _ice_id = "::Ice::FixedProxyException"

    _M_Ice._t_FixedProxyException = IcePy.defineException(
        "::Ice::FixedProxyException", FixedProxyException, (), None, ()
    )
    FixedProxyException._ice_type = _M_Ice._t_FixedProxyException

    _M_Ice.FixedProxyException = FixedProxyException
    del FixedProxyException

# End of module Ice
