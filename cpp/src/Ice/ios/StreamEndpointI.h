//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_STREAM_ENDPOINT_I_H
#define ICE_STREAM_ENDPOINT_I_H

#include "Ice/Config.h"

#if TARGET_OS_IPHONE != 0

#    include "../EndpointFactory.h"
#    include "../IPEndpointI.h"
#    include "../ProtocolInstance.h"
#    include "../UniqueRef.h"
#    include "../WSEndpoint.h"
#    include "Ice/InstanceF.h"
#    include "Ice/SSL/ServerAuthenticationOptions.h"

#    include <CFNetwork/CFNetwork.h>
#    include <CoreFoundation/CFDictionary.h>

#    include <optional>

namespace Ice
{
    class OutputStream;
    class InputStream;
}

namespace IceObjC
{
    class Instance;
    using InstancePtr = std::shared_ptr<Instance>;

    class Instance : public IceInternal::ProtocolInstance
    {
    public:
        Instance(const Ice::CommunicatorPtr&, std::int16_t, const std::string&, bool);
        ~Instance() = default;

        const std::string& proxyHost() const { return _proxyHost; }

        int proxyPort() const { return _proxyPort; }

        void setupStreams(CFReadStreamRef, CFWriteStreamRef, bool, const std::string&) const;

        Ice::CommunicatorPtr communicator();

    private:
        // Use a weak pointer to avoid circular references. The communicator owns the endpoint factory, which in
        // turn own this protocol instance.
        const std::weak_ptr<Ice::Communicator> _communicator;
        IceInternal::UniqueRef<CFMutableDictionaryRef> _proxySettings;
        std::string _proxyHost;
        int _proxyPort;
    };

    class StreamAcceptor;
    using StreamAcceptorPtr = std::shared_ptr<StreamAcceptor>;

    class StreamEndpointI;
    using StreamEndpointIPtr = std::shared_ptr<StreamEndpointI>;

    class StreamEndpointI final : public IceInternal::IPEndpointI
    {
    public:
        StreamEndpointI(
            const InstancePtr&,
            const std::string&,
            std::int32_t,
            const IceInternal::Address&,
            std::int32_t,
            const std::string&,
            bool);
        StreamEndpointI(const InstancePtr&);
        StreamEndpointI(const InstancePtr&, Ice::InputStream*);

        Ice::EndpointInfoPtr getInfo() const noexcept final;

        std::int32_t timeout() const final;
        IceInternal::EndpointIPtr timeout(std::int32_t) const final;
        bool compress() const final;
        IceInternal::EndpointIPtr compress(bool) const final;
        bool datagram() const final;
        bool secure() const final;

        void connectorsAsync(
            Ice::EndpointSelectionType,
            std::function<void(std::vector<IceInternal::ConnectorPtr>)> response,
            std::function<void(std::exception_ptr)> exception) const;
        IceInternal::TransceiverPtr transceiver() const final;
        IceInternal::AcceptorPtr
        acceptor(const std::string&, const std::optional<Ice::SSL::ServerAuthenticationOptions>&) const final;
        std::string options() const final;

        std::shared_ptr<StreamEndpointI> shared_from_this()
        {
            return std::static_pointer_cast<StreamEndpointI>(IceInternal::IPEndpointI::shared_from_this());
        }

        bool operator==(const Ice::Endpoint&) const final;
        bool operator<(const Ice::Endpoint&) const final;

        std::size_t hash() const noexcept final;

        StreamEndpointIPtr endpoint(const StreamAcceptorPtr&) const;

        using IPEndpointI::connectionId;

    protected:
        void streamWriteImpl(Ice::OutputStream*) const final;
        bool checkOption(const std::string&, const std::string&, const std::string&) final;

        IceInternal::ConnectorPtr
        createConnector(const IceInternal::Address&, const IceInternal::NetworkProxyPtr&) const final;
        IceInternal::IPEndpointIPtr createEndpoint(const std::string&, int, const std::string&) const final;

    private:
        const InstancePtr _streamInstance;

        //
        // All members are const, because endpoints are immutable.
        //
        const std::int32_t _timeout;
        const bool _compress;
    };

    class StreamEndpointFactory final : public IceInternal::EndpointFactory
    {
    public:
        StreamEndpointFactory(const InstancePtr&);
        ~StreamEndpointFactory() = default;

        std::int16_t type() const final;
        std::string protocol() const final;
        IceInternal::EndpointIPtr create(std::vector<std::string>&, bool) const final;
        IceInternal::EndpointIPtr read(Ice::InputStream*) const final;

        IceInternal::EndpointFactoryPtr clone(const IceInternal::ProtocolInstancePtr&) const final;

    private:
        const InstancePtr _instance;
    };
}

#endif

#endif
