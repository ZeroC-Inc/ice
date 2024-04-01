//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_UDP_CONNECTOR_H
#define ICE_UDP_CONNECTOR_H

#include "TransceiverF.h"
#include "ProtocolInstanceF.h"
#include "Connector.h"
#include "Network.h"

namespace IceInternal
{
    class UdpConnector final : public Connector
    {
    public:
        UdpConnector(
            const ProtocolInstancePtr&,
            const Address&,
            const Address&,
            const std::string&,
            int,
            const std::string&);

        ~UdpConnector();
        TransceiverPtr connect() final;

        std::int16_t type() const final;
        std::string toString() const final;

        bool operator==(const Connector&) const final;
        bool operator<(const Connector&) const final;

    private:
        const ProtocolInstancePtr _instance;
        const Address _addr;
        const Address _sourceAddr;
        const std::string _mcastInterface;
        const int _mcastTtl;
        const std::string _connectionId;
    };
}
#endif
