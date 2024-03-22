//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_WSCONNECTOR_I_H
#define ICE_WSCONNECTOR_I_H

#include <Ice/Logger.h>
#include <Ice/TransceiverF.h>
#include <Ice/Connector.h>
#include <Ice/ProtocolInstance.h>

namespace IceInternal
{
    class WSEndpoint;

    class WSConnector final : public Connector
    {
    public:
        WSConnector(const ProtocolInstancePtr&, const ConnectorPtr&, const std::string&, const std::string&);
        ~WSConnector();
        TransceiverPtr connect() final;

        std::int16_t type() const final;
        std::string toString() const final;

        bool operator==(const Connector&) const final;
        bool operator<(const Connector&) const final;

    private:
        const ProtocolInstancePtr _instance;
        const ConnectorPtr _delegate;
        const std::string _host;
        const std::string _resource;
    };
}

#endif
