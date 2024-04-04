//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_IDLE_TIMEOUT_TRANSCEIVER_DECORATOR_H
#define ICE_IDLE_TIMEOUT_TRANSCEIVER_DECORATOR_H

#include "Transceiver.h"
#include "IceUtil/Timer.h"

namespace IceInternal
{
    // Decorates Transceiver to send heartbeats and detect when no byte is received/read for a while.
    // This decorator must not be applied on UDP "connections".
    class IdleTimeoutTransceiverDecorator final : public Transceiver
    {
    public:
        IdleTimeoutTransceiverDecorator(
            const TransceiverPtr& decoratee,
            std::chrono::milliseconds readIdleTimeout,
            std::chrono::milliseconds writeIdleTimeout,
            const IceUtil::TimerPtr& timer) :
            _decoratee(decoratee),
            _readIdleTimeout(readIdleTimeout),
            _writeIdleTimeout(writeIdleTimeout),
            _timer(timer)
        {
        }

        ~IdleTimeoutTransceiverDecorator();

        // Sets the keep-alive action. This is an "init", not thread-safe function.
        void keepAliveAction(const IceUtil::TimerTaskPtr& keepAliveAction)
        {
            assert(!_keepAliveAction); // Must be called only once.
            _keepAliveAction = keepAliveAction;
        }

        NativeInfoPtr getNativeInfo() final { return _decoratee->getNativeInfo(); }

        SocketOperation initialize(Buffer& readBuffer, Buffer& writeBuffer) final;
        SocketOperation closing(bool initiator, std::exception_ptr ex) final { return _decoratee->closing(initiator, ex); }
        void close() final;

        EndpointIPtr bind() final { return _decoratee->bind(); }

        SocketOperation write(Buffer&) final;
        SocketOperation read(Buffer&) final;

#if defined(ICE_USE_IOCP)
        bool startWrite(Buffer&) final;
        void finishWrite(Buffer&) final;
        void startRead(Buffer&) final;
        void finishRead(Buffer&) final;
#endif

        std::string protocol() const final { return _decoratee->protocol(); }
        std::string toString() const final { return _decoratee->toString(); }
        std::string toDetailedString() const final { return _decoratee->toDetailedString();}
        Ice::ConnectionInfoPtr getInfo() const final { return _decoratee->getInfo(); }
        void checkSendSize(const Buffer& buf) final { _decoratee->checkSendSize(buf); };
        void setBufferSize(int rcvSize, int sndSize) final { _decoratee->setBufferSize(rcvSize, sndSize);}

    private:
        void rescheduleKeepAlive();

        const TransceiverPtr _decoratee;
        IceUtil::TimerTaskPtr _keepAliveAction;
        [[maybe_unused]] const std::chrono::milliseconds _readIdleTimeout;
        const std::chrono::milliseconds _writeIdleTimeout;
        const IceUtil::TimerPtr _timer;
    };
}

#endif
