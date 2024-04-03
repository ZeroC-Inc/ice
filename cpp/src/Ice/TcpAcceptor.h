//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_TCP_ACCEPTOR_H
#define ICE_TCP_ACCEPTOR_H

#include "Acceptor.h"
#include "Network.h"
#include "ProtocolInstanceF.h"
#include "TransceiverF.h"

namespace IceInternal
{
    class TcpEndpoint;

    class TcpAcceptor final : public Acceptor, public NativeInfo, public std::enable_shared_from_this<TcpAcceptor>
    {
    public:
        TcpAcceptor(const TcpEndpointIPtr&, const ProtocolInstancePtr&, const std::string&, int);
        ~TcpAcceptor();
        NativeInfoPtr getNativeInfo() final;
#if defined(ICE_USE_IOCP)
        AsyncInfo* getAsyncInfo(SocketOperation) final;
#endif

        void close() final;
        EndpointIPtr listen() final;
#if defined(ICE_USE_IOCP)
        void startAccept() final;
        void finishAccept() final;
#endif

        TransceiverPtr accept() final;
        std::string protocol() const final;
        std::string toString() const final;
        std::string toDetailedString() const final;

        int effectivePort() const;

    private:
        friend class TcpEndpointI;

        TcpEndpointIPtr _endpoint;
        const ProtocolInstancePtr _instance;
        const Address _addr;

        int _backlog;
#if defined(ICE_USE_IOCP)
        SOCKET _acceptFd;
        int _acceptError;
        std::vector<char> _acceptBuf;
        AsyncInfo _info;
#endif
    };
}
#endif
