# encoding: utf-8
#
# Copyright (c) ZeroC, Inc.

# This file provides all the exceptions derived from Ice::LocalException.

module Ice
    #
    # The 6 (7 with the RequestFailedException base class) special local exceptions that can be marshaled in an Ice
    # reply message. Other local exceptions can't be marshaled.
    #

    class RequestFailedException < LocalException
        def initialize(id, facet, operation, msg)
            super(msg)
            @id = id
            @facet = facet
            @operation = operation
        end

        attr_reader :id, :facet, :operation
    end

    class ObjectNotExistException < RequestFailedException
    end

    class FacetNotExistException < RequestFailedException
    end

    class OperationNotExistException < RequestFailedException
    end

    class UnknownException < LocalException
    end

    class UnknownLocalException < UnknownException
    end

    class UnknownUserException < UnknownException
    end

    #
    # Protocol exceptions
    #

    class ProtocolException < LocalException
    end

    class CloseConnectionException < ProtocolException
    end

    class DatagramLimitException < ProtocolException
    end

    class MarshalException < ProtocolException
    end

    #
    # Timeout exceptions
    #

    class TimeoutException < LocalException
    end

    class ConnectTimeoutException < TimeoutException
    end

    class CloseTimeoutException < TimeoutException
    end

    class InvocationTimeoutException < TimeoutException
    end

    #
    # Syscall exceptions
    #

    class SyscallException < LocalException
    end

    class DNSException < SyscallException
    end

    #
    # Socket exceptions
    #

    class SocketException < SyscallException
    end

    class ConnectFailedException < SocketException
    end

    class ConnectionLostException < SocketException
    end

    class ConnectionRefusedException < ConnectFailedException
    end

    #
    # Other leaf local exceptions in alphabetical order.
    #

    # The only exception that can be thrown by application code in Ruby.
    class AlreadyRegisteredException < LocalException
        def initialize(kindOfObject, id)
            super("another #{kindOfObject} is already registered with ID '#{id}'")
            @kindOfObject = kindOfObject
            @id = id
        end

        attr_reader :kindOfObject, :id
    end

    class CommunicatorDestroyedException < LocalException
    end

    class ConnectionAbortedException < LocalException
    end

    class ConnectionClosedException < LocalException
    end

    class FeatureNotSupportedException < LocalException
    end

    class FixedProxyException < LocalException
    end

    class InitializationException < LocalException
    end

    class InvocationCanceledException < LocalException
    end

    class NoEndpointException < LocalException
    end

    # API is like AlreadyRegisteredException even though it should not be thrown by application code.
    class NotRegisteredException < LocalException
        def initialize(kindOfObject, id)
            super("no #{kindOfObject} is registered with ID '#{id}'")
            @kindOfObject = kindOfObject
            @id = id
        end

        attr_reader :kindOfObject, :id
    end

    class ParseException < LocalException
    end

    class PluginInitializationException < LocalException
    end

    class SecurityException < LocalException
    end

    class TwowayOnlyException < LocalException
    end
end
