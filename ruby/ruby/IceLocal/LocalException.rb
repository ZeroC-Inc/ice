# encoding: utf-8
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

require 'Ice'
require 'Ice/Identity.rb'
require 'Ice/Version.rb'
require 'Ice/BuiltinSequences.rb'

module ::Ice

    if not defined?(::Ice::InitializationException)
        class InitializationException < Ice::LocalException
            def initialize(reason='')
                @reason = reason
            end

            def to_s
                '::Ice::InitializationException'
            end

            attr_accessor :reason
        end

        T_InitializationException = ::Ice::__defineException('::Ice::InitializationException', InitializationException, nil, [["reason", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::PluginInitializationException)
        class PluginInitializationException < Ice::LocalException
            def initialize(reason='')
                @reason = reason
            end

            def to_s
                '::Ice::PluginInitializationException'
            end

            attr_accessor :reason
        end

        T_PluginInitializationException = ::Ice::__defineException('::Ice::PluginInitializationException', PluginInitializationException, nil, [["reason", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::AlreadyRegisteredException)
        class AlreadyRegisteredException < Ice::LocalException
            def initialize(kindOfObject='', id='')
                @kindOfObject = kindOfObject
                @id = id
            end

            def to_s
                '::Ice::AlreadyRegisteredException'
            end

            attr_accessor :kindOfObject, :id
        end

        T_AlreadyRegisteredException = ::Ice::__defineException('::Ice::AlreadyRegisteredException', AlreadyRegisteredException, nil, [
            ["kindOfObject", ::Ice::T_string, false, 0],
            ["id", ::Ice::T_string, false, 0]
        ])
    end

    if not defined?(::Ice::NotRegisteredException)
        class NotRegisteredException < Ice::LocalException
            def initialize(kindOfObject='', id='')
                @kindOfObject = kindOfObject
                @id = id
            end

            def to_s
                '::Ice::NotRegisteredException'
            end

            attr_accessor :kindOfObject, :id
        end

        T_NotRegisteredException = ::Ice::__defineException('::Ice::NotRegisteredException', NotRegisteredException, nil, [
            ["kindOfObject", ::Ice::T_string, false, 0],
            ["id", ::Ice::T_string, false, 0]
        ])
    end

    if not defined?(::Ice::TwowayOnlyException)
        class TwowayOnlyException < Ice::LocalException
            def initialize(operation='')
                @operation = operation
            end

            def to_s
                '::Ice::TwowayOnlyException'
            end

            attr_accessor :operation
        end

        T_TwowayOnlyException = ::Ice::__defineException('::Ice::TwowayOnlyException', TwowayOnlyException, nil, [["operation", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::UnknownException)
        class UnknownException < Ice::LocalException
            def initialize(unknown='')
                @unknown = unknown
            end

            def to_s
                '::Ice::UnknownException'
            end

            attr_accessor :unknown
        end

        T_UnknownException = ::Ice::__defineException('::Ice::UnknownException', UnknownException, nil, [["unknown", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::UnknownLocalException)
        class UnknownLocalException < ::Ice::UnknownException
            def initialize(unknown='')
                super(unknown)
            end

            def to_s
                '::Ice::UnknownLocalException'
            end
        end

        T_UnknownLocalException = ::Ice::__defineException('::Ice::UnknownLocalException', UnknownLocalException, ::Ice::T_UnknownException, [])
    end

    if not defined?(::Ice::UnknownUserException)
        class UnknownUserException < ::Ice::UnknownException
            def initialize(unknown='')
                super(unknown)
            end

            def to_s
                '::Ice::UnknownUserException'
            end
        end

        T_UnknownUserException = ::Ice::__defineException('::Ice::UnknownUserException', UnknownUserException, ::Ice::T_UnknownException, [])
    end

    if not defined?(::Ice::CommunicatorDestroyedException)
        class CommunicatorDestroyedException < Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::CommunicatorDestroyedException'
            end
        end

        T_CommunicatorDestroyedException = ::Ice::__defineException('::Ice::CommunicatorDestroyedException', CommunicatorDestroyedException, nil, [])
    end

    if not defined?(::Ice::ObjectAdapterDeactivatedException)
        class ObjectAdapterDeactivatedException < Ice::LocalException
            def initialize(name='')
                @name = name
            end

            def to_s
                '::Ice::ObjectAdapterDeactivatedException'
            end

            attr_accessor :name
        end

        T_ObjectAdapterDeactivatedException = ::Ice::__defineException('::Ice::ObjectAdapterDeactivatedException', ObjectAdapterDeactivatedException, nil, [["name", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::ObjectAdapterIdInUseException)
        class ObjectAdapterIdInUseException < Ice::LocalException
            def initialize(id='')
                @id = id
            end

            def to_s
                '::Ice::ObjectAdapterIdInUseException'
            end

            attr_accessor :id
        end

        T_ObjectAdapterIdInUseException = ::Ice::__defineException('::Ice::ObjectAdapterIdInUseException', ObjectAdapterIdInUseException, nil, [["id", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::NoEndpointException)
        class NoEndpointException < Ice::LocalException
            def initialize(proxy='')
                @proxy = proxy
            end

            def to_s
                '::Ice::NoEndpointException'
            end

            attr_accessor :proxy
        end

        T_NoEndpointException = ::Ice::__defineException('::Ice::NoEndpointException', NoEndpointException, nil, [["proxy", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::ParseException)
        class ParseException < Ice::LocalException
            def initialize(str='')
                @str = str
            end

            def to_s
                '::Ice::ParseException'
            end

            attr_accessor :str
        end

        T_ParseException = ::Ice::__defineException('::Ice::ParseException', ParseException, nil, [["str", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::IllegalIdentityException)
        class IllegalIdentityException < Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::IllegalIdentityException'
            end
        end

        T_IllegalIdentityException = ::Ice::__defineException('::Ice::IllegalIdentityException', IllegalIdentityException, nil, [])
    end

    if not defined?(::Ice::IllegalServantException)
        class IllegalServantException < Ice::LocalException
            def initialize(reason='')
                @reason = reason
            end

            def to_s
                '::Ice::IllegalServantException'
            end

            attr_accessor :reason
        end

        T_IllegalServantException = ::Ice::__defineException('::Ice::IllegalServantException', IllegalServantException, nil, [["reason", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::RequestFailedException)
        class RequestFailedException < Ice::LocalException
            def initialize(id=::Ice::Identity.new, facet='', operation='')
                @id = id
                @facet = facet
                @operation = operation
            end

            def to_s
                '::Ice::RequestFailedException'
            end

            attr_accessor :id, :facet, :operation
        end

        T_RequestFailedException = ::Ice::__defineException('::Ice::RequestFailedException', RequestFailedException, nil, [
            ["id", ::Ice::T_Identity, false, 0],
            ["facet", ::Ice::T_string, false, 0],
            ["operation", ::Ice::T_string, false, 0]
        ])
    end

    if not defined?(::Ice::ObjectNotExistException)
        class ObjectNotExistException < ::Ice::RequestFailedException
            def initialize(id=::Ice::Identity.new, facet='', operation='')
                super(id, facet, operation)
            end

            def to_s
                '::Ice::ObjectNotExistException'
            end
        end

        T_ObjectNotExistException = ::Ice::__defineException('::Ice::ObjectNotExistException', ObjectNotExistException, ::Ice::T_RequestFailedException, [])
    end

    if not defined?(::Ice::FacetNotExistException)
        class FacetNotExistException < ::Ice::RequestFailedException
            def initialize(id=::Ice::Identity.new, facet='', operation='')
                super(id, facet, operation)
            end

            def to_s
                '::Ice::FacetNotExistException'
            end
        end

        T_FacetNotExistException = ::Ice::__defineException('::Ice::FacetNotExistException', FacetNotExistException, ::Ice::T_RequestFailedException, [])
    end

    if not defined?(::Ice::OperationNotExistException)
        class OperationNotExistException < ::Ice::RequestFailedException
            def initialize(id=::Ice::Identity.new, facet='', operation='')
                super(id, facet, operation)
            end

            def to_s
                '::Ice::OperationNotExistException'
            end
        end

        T_OperationNotExistException = ::Ice::__defineException('::Ice::OperationNotExistException', OperationNotExistException, ::Ice::T_RequestFailedException, [])
    end

    if not defined?(::Ice::SyscallException)
        class SyscallException < Ice::LocalException
            def initialize(error=0)
                @error = error
            end

            def to_s
                '::Ice::SyscallException'
            end

            attr_accessor :error
        end

        T_SyscallException = ::Ice::__defineException('::Ice::SyscallException', SyscallException, nil, [["error", ::Ice::T_int, false, 0]])
    end

    if not defined?(::Ice::SocketException)
        class SocketException < ::Ice::SyscallException
            def initialize(error=0)
                super(error)
            end

            def to_s
                '::Ice::SocketException'
            end
        end

        T_SocketException = ::Ice::__defineException('::Ice::SocketException', SocketException, ::Ice::T_SyscallException, [])
    end

    if not defined?(::Ice::CFNetworkException)
        class CFNetworkException < ::Ice::SocketException
            def initialize(error=0, domain='')
                super(error)
                @domain = domain
            end

            def to_s
                '::Ice::CFNetworkException'
            end

            attr_accessor :domain
        end

        T_CFNetworkException = ::Ice::__defineException('::Ice::CFNetworkException', CFNetworkException, ::Ice::T_SocketException, [["domain", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::FileException)
        class FileException < ::Ice::SyscallException
            def initialize(error=0, path='')
                super(error)
                @path = path
            end

            def to_s
                '::Ice::FileException'
            end

            attr_accessor :path
        end

        T_FileException = ::Ice::__defineException('::Ice::FileException', FileException, ::Ice::T_SyscallException, [["path", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::ConnectFailedException)
        class ConnectFailedException < ::Ice::SocketException
            def initialize(error=0)
                super(error)
            end

            def to_s
                '::Ice::ConnectFailedException'
            end
        end

        T_ConnectFailedException = ::Ice::__defineException('::Ice::ConnectFailedException', ConnectFailedException, ::Ice::T_SocketException, [])
    end

    if not defined?(::Ice::ConnectionRefusedException)
        class ConnectionRefusedException < ::Ice::ConnectFailedException
            def initialize(error=0)
                super(error)
            end

            def to_s
                '::Ice::ConnectionRefusedException'
            end
        end

        T_ConnectionRefusedException = ::Ice::__defineException('::Ice::ConnectionRefusedException', ConnectionRefusedException, ::Ice::T_ConnectFailedException, [])
    end

    if not defined?(::Ice::ConnectionLostException)
        class ConnectionLostException < ::Ice::SocketException
            def initialize(error=0)
                super(error)
            end

            def to_s
                '::Ice::ConnectionLostException'
            end
        end

        T_ConnectionLostException = ::Ice::__defineException('::Ice::ConnectionLostException', ConnectionLostException, ::Ice::T_SocketException, [])
    end

    if not defined?(::Ice::DNSException)
        class DNSException < Ice::LocalException
            def initialize(error=0, host='')
                @error = error
                @host = host
            end

            def to_s
                '::Ice::DNSException'
            end

            attr_accessor :error, :host
        end

        T_DNSException = ::Ice::__defineException('::Ice::DNSException', DNSException, nil, [
            ["error", ::Ice::T_int, false, 0],
            ["host", ::Ice::T_string, false, 0]
        ])
    end

    if not defined?(::Ice::ConnectionIdleException)
        class ConnectionIdleException < ::Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::ConnectionIdleException'
            end
        end

        T_ConnectionIdleException = ::Ice::__defineException('::Ice::ConnectionIdleException', ConnectionIdleException, nil, [])
    end

    if not defined?(::Ice::TimeoutException)
        class TimeoutException < Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::TimeoutException'
            end
        end

        T_TimeoutException = ::Ice::__defineException('::Ice::TimeoutException', TimeoutException, nil, [])
    end

    if not defined?(::Ice::ConnectTimeoutException)
        class ConnectTimeoutException < ::Ice::TimeoutException
            def initialize
            end

            def to_s
                '::Ice::ConnectTimeoutException'
            end
        end

        T_ConnectTimeoutException = ::Ice::__defineException('::Ice::ConnectTimeoutException', ConnectTimeoutException, ::Ice::T_TimeoutException, [])
    end

    if not defined?(::Ice::CloseTimeoutException)
        class CloseTimeoutException < ::Ice::TimeoutException
            def initialize
            end

            def to_s
                '::Ice::CloseTimeoutException'
            end
        end

        T_CloseTimeoutException = ::Ice::__defineException('::Ice::CloseTimeoutException', CloseTimeoutException, ::Ice::T_TimeoutException, [])
    end

    if not defined?(::Ice::InvocationTimeoutException)
        class InvocationTimeoutException < ::Ice::TimeoutException
            def initialize
            end

            def to_s
                '::Ice::InvocationTimeoutException'
            end
        end

        T_InvocationTimeoutException = ::Ice::__defineException('::Ice::InvocationTimeoutException', InvocationTimeoutException, ::Ice::T_TimeoutException, [])
    end

    if not defined?(::Ice::InvocationCanceledException)
        class InvocationCanceledException < Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::InvocationCanceledException'
            end
        end

        T_InvocationCanceledException = ::Ice::__defineException('::Ice::InvocationCanceledException', InvocationCanceledException, nil, [])
    end

    if not defined?(::Ice::ProtocolException)
        class ProtocolException < Ice::LocalException
            def initialize(reason='')
                @reason = reason
            end

            def to_s
                '::Ice::ProtocolException'
            end

            attr_accessor :reason
        end

        T_ProtocolException = ::Ice::__defineException('::Ice::ProtocolException', ProtocolException, nil, [["reason", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::CloseConnectionException)
        class CloseConnectionException < ::Ice::ProtocolException
            def initialize(reason='')
                super(reason)
            end

            def to_s
                '::Ice::CloseConnectionException'
            end
        end

        T_CloseConnectionException = ::Ice::__defineException('::Ice::CloseConnectionException', CloseConnectionException, ::Ice::T_ProtocolException, [])
    end

    if not defined?(::Ice::ConnectionManuallyClosedException)
        class ConnectionManuallyClosedException < Ice::LocalException
            def initialize(graceful=false)
                @graceful = graceful
            end

            def to_s
                '::Ice::ConnectionManuallyClosedException'
            end

            attr_accessor :graceful
        end

        T_ConnectionManuallyClosedException = ::Ice::__defineException('::Ice::ConnectionManuallyClosedException', ConnectionManuallyClosedException, nil, [["graceful", ::Ice::T_bool, false, 0]])
    end

    if not defined?(::Ice::DatagramLimitException)
        class DatagramLimitException < ::Ice::ProtocolException
            def initialize(reason='')
                super(reason)
            end

            def to_s
                '::Ice::DatagramLimitException'
            end
        end

        T_DatagramLimitException = ::Ice::__defineException('::Ice::DatagramLimitException', DatagramLimitException, ::Ice::T_ProtocolException, [])
    end

    if not defined?(::Ice::MarshalException)
        class MarshalException < ::Ice::ProtocolException
            def initialize(reason='')
                super(reason)
            end

            def to_s
                '::Ice::MarshalException'
            end
        end

        T_MarshalException = ::Ice::__defineException('::Ice::MarshalException', MarshalException, ::Ice::T_ProtocolException, [])
    end

    if not defined?(::Ice::FeatureNotSupportedException)
        class FeatureNotSupportedException < Ice::LocalException
            def initialize(unsupportedFeature='')
                @unsupportedFeature = unsupportedFeature
            end

            def to_s
                '::Ice::FeatureNotSupportedException'
            end

            attr_accessor :unsupportedFeature
        end

        T_FeatureNotSupportedException = ::Ice::__defineException('::Ice::FeatureNotSupportedException', FeatureNotSupportedException, nil, [["unsupportedFeature", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::SecurityException)
        class SecurityException < Ice::LocalException
            def initialize(reason='')
                @reason = reason
            end

            def to_s
                '::Ice::SecurityException'
            end

            attr_accessor :reason
        end

        T_SecurityException = ::Ice::__defineException('::Ice::SecurityException', SecurityException, nil, [["reason", ::Ice::T_string, false, 0]])
    end

    if not defined?(::Ice::FixedProxyException)
        class FixedProxyException < Ice::LocalException
            def initialize
            end

            def to_s
                '::Ice::FixedProxyException'
            end
        end

        T_FixedProxyException = ::Ice::__defineException('::Ice::FixedProxyException', FixedProxyException, nil, [])
    end
end
