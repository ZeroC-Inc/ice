# encoding: utf-8
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#
#
# Ice version 3.7.10
#
# <auto-generated>
#
# Generated from file `Connection.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

require 'Ice'
require 'IceLocal/ObjectAdapterF.rb'
require 'Ice/Identity.rb'
require 'IceLocal/Endpoint.rb'

module ::Ice

    if not defined?(::Ice::CompressBatch)
        class CompressBatch
            include Comparable

            def initialize(name, value)
                @name = name
                @value = value
            end

            def CompressBatch.from_int(val)
                @@_enumerators[val]
            end

            def to_s
                @name
            end

            def to_i
                @value
            end

            def <=>(other)
                other.is_a?(CompressBatch) or raise ArgumentError, "value must be a CompressBatch"
                @value <=> other.to_i
            end

            def hash
                @value.hash
            end

            def CompressBatch.each(&block)
                @@_enumerators.each_value(&block)
            end

            Yes = CompressBatch.new("Yes", 0)
            No = CompressBatch.new("No", 1)
            BasedOnProxy = CompressBatch.new("BasedOnProxy", 2)

            @@_enumerators = {0=>Yes, 1=>No, 2=>BasedOnProxy}

            def CompressBatch._enumerators
                @@_enumerators
            end

            private_class_method :new
        end

        T_CompressBatch = ::Ice::__defineEnum('::Ice::CompressBatch', CompressBatch, CompressBatch::_enumerators)
    end

    if not defined?(::Ice::ConnectionInfo_Mixin)

        module ::Ice::ConnectionInfo_Mixin
        end
        class ConnectionInfo

            def initialize(underlying=nil, incoming=false, adapterName='', connectionId='')
                @underlying = underlying
                @incoming = incoming
                @adapterName = adapterName
                @connectionId = connectionId
            end

            attr_accessor :underlying, :incoming, :adapterName, :connectionId
        end

        if not defined?(::Ice::T_ConnectionInfo)
            T_ConnectionInfo = ::Ice::__declareLocalClass('::Ice::ConnectionInfo')
        end

        T_ConnectionInfo.defineClass(ConnectionInfo, -1, false, nil, [
            ['underlying', ::Ice::T_ConnectionInfo, false, 0],
            ['incoming', ::Ice::T_bool, false, 0],
            ['adapterName', ::Ice::T_string, false, 0],
            ['connectionId', ::Ice::T_string, false, 0]
        ])
    end

    if not defined?(::Ice::T_Connection)
        T_Connection = ::Ice::__declareLocalClass('::Ice::Connection')
    end

    if not defined?(::Ice::T_CloseCallback)
        T_CloseCallback = ::Ice::__declareLocalClass('::Ice::CloseCallback')
    end

    if not defined?(::Ice::T_HeartbeatCallback)
        T_HeartbeatCallback = ::Ice::__declareLocalClass('::Ice::HeartbeatCallback')
    end

    if not defined?(::Ice::ConnectionClose)
        class ConnectionClose
            include Comparable

            def initialize(name, value)
                @name = name
                @value = value
            end

            def ConnectionClose.from_int(val)
                @@_enumerators[val]
            end

            def to_s
                @name
            end

            def to_i
                @value
            end

            def <=>(other)
                other.is_a?(ConnectionClose) or raise ArgumentError, "value must be a ConnectionClose"
                @value <=> other.to_i
            end

            def hash
                @value.hash
            end

            def ConnectionClose.each(&block)
                @@_enumerators.each_value(&block)
            end

            Forcefully = ConnectionClose.new("Forcefully", 0)
            Gracefully = ConnectionClose.new("Gracefully", 1)
            GracefullyWithWait = ConnectionClose.new("GracefullyWithWait", 2)

            @@_enumerators = {0=>Forcefully, 1=>Gracefully, 2=>GracefullyWithWait}

            def ConnectionClose._enumerators
                @@_enumerators
            end

            private_class_method :new
        end

        T_ConnectionClose = ::Ice::__defineEnum('::Ice::ConnectionClose', ConnectionClose, ConnectionClose::_enumerators)
    end

    if not defined?(::Ice::IPConnectionInfo_Mixin)

        module ::Ice::IPConnectionInfo_Mixin
        end
        class IPConnectionInfo < ::Ice::ConnectionInfo

            def initialize(underlying=nil, incoming=false, adapterName='', connectionId='', localAddress="", localPort=-1, remoteAddress="", remotePort=-1)
                super(underlying, incoming, adapterName, connectionId)
                @localAddress = localAddress
                @localPort = localPort
                @remoteAddress = remoteAddress
                @remotePort = remotePort
            end

            attr_accessor :localAddress, :localPort, :remoteAddress, :remotePort
        end

        if not defined?(::Ice::T_IPConnectionInfo)
            T_IPConnectionInfo = ::Ice::__declareLocalClass('::Ice::IPConnectionInfo')
        end

        T_IPConnectionInfo.defineClass(IPConnectionInfo, -1, false, ::Ice::T_ConnectionInfo, [
            ['localAddress', ::Ice::T_string, false, 0],
            ['localPort', ::Ice::T_int, false, 0],
            ['remoteAddress', ::Ice::T_string, false, 0],
            ['remotePort', ::Ice::T_int, false, 0]
        ])
    end

    if not defined?(::Ice::TCPConnectionInfo_Mixin)

        module ::Ice::TCPConnectionInfo_Mixin
        end
        class TCPConnectionInfo < ::Ice::IPConnectionInfo

            def initialize(underlying=nil, incoming=false, adapterName='', connectionId='', localAddress="", localPort=-1, remoteAddress="", remotePort=-1, rcvSize=0, sndSize=0)
                super(underlying, incoming, adapterName, connectionId, localAddress, localPort, remoteAddress, remotePort)
                @rcvSize = rcvSize
                @sndSize = sndSize
            end

            attr_accessor :rcvSize, :sndSize
        end

        if not defined?(::Ice::T_TCPConnectionInfo)
            T_TCPConnectionInfo = ::Ice::__declareLocalClass('::Ice::TCPConnectionInfo')
        end

        T_TCPConnectionInfo.defineClass(TCPConnectionInfo, -1, false, ::Ice::T_IPConnectionInfo, [
            ['rcvSize', ::Ice::T_int, false, 0],
            ['sndSize', ::Ice::T_int, false, 0]
        ])
    end

    if not defined?(::Ice::UDPConnectionInfo_Mixin)

        module ::Ice::UDPConnectionInfo_Mixin
        end
        class UDPConnectionInfo < ::Ice::IPConnectionInfo

            def initialize(underlying=nil, incoming=false, adapterName='', connectionId='', localAddress="", localPort=-1, remoteAddress="", remotePort=-1, mcastAddress='', mcastPort=-1, rcvSize=0, sndSize=0)
                super(underlying, incoming, adapterName, connectionId, localAddress, localPort, remoteAddress, remotePort)
                @mcastAddress = mcastAddress
                @mcastPort = mcastPort
                @rcvSize = rcvSize
                @sndSize = sndSize
            end

            attr_accessor :mcastAddress, :mcastPort, :rcvSize, :sndSize
        end

        if not defined?(::Ice::T_UDPConnectionInfo)
            T_UDPConnectionInfo = ::Ice::__declareLocalClass('::Ice::UDPConnectionInfo')
        end

        T_UDPConnectionInfo.defineClass(UDPConnectionInfo, -1, false, ::Ice::T_IPConnectionInfo, [
            ['mcastAddress', ::Ice::T_string, false, 0],
            ['mcastPort', ::Ice::T_int, false, 0],
            ['rcvSize', ::Ice::T_int, false, 0],
            ['sndSize', ::Ice::T_int, false, 0]
        ])
    end

    if not defined?(::Ice::T_HeaderDict)
        T_HeaderDict = ::Ice::__defineDictionary('::Ice::HeaderDict', ::Ice::T_string, ::Ice::T_string)
    end

    if not defined?(::Ice::WSConnectionInfo_Mixin)

        module ::Ice::WSConnectionInfo_Mixin
        end
        class WSConnectionInfo < ::Ice::ConnectionInfo

            def initialize(underlying=nil, incoming=false, adapterName='', connectionId='', headers=nil)
                super(underlying, incoming, adapterName, connectionId)
                @headers = headers
            end

            attr_accessor :headers
        end

        if not defined?(::Ice::T_WSConnectionInfo)
            T_WSConnectionInfo = ::Ice::__declareLocalClass('::Ice::WSConnectionInfo')
        end

        T_WSConnectionInfo.defineClass(WSConnectionInfo, -1, false, ::Ice::T_ConnectionInfo, [['headers', ::Ice::T_HeaderDict, false, 0]])
    end
end
