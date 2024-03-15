//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_TRANSCEIVER_H
#define TEST_TRANSCEIVER_H

#include <Ice/Buffer.h>
#include <Ice/Transceiver.h>
#include <Configuration.h>

class Transceiver final : public IceInternal::Transceiver
{
public:
    Transceiver(const IceInternal::TransceiverPtr&);
    IceInternal::NativeInfoPtr getNativeInfo() final;

    IceInternal::SocketOperation closing(bool, std::exception_ptr) final;
    void close();
    IceInternal::SocketOperation write(IceInternal::Buffer&) final;
    IceInternal::SocketOperation read(IceInternal::Buffer&) final;
#ifdef ICE_USE_IOCP
    bool startWrite(IceInternal::Buffer&) final;
    void finishWrite(IceInternal::Buffer&) final;
    void startRead(IceInternal::Buffer&) final;
    void finishRead(IceInternal::Buffer&) final;
#endif
    std::string protocol() const final;
    std::string toString() const final;
    std::string toDetailedString() const final;
    Ice::ConnectionInfoPtr getInfo() const final;
    IceInternal::SocketOperation initialize(IceInternal::Buffer&, IceInternal::Buffer&) final;
    void checkSendSize(const IceInternal::Buffer&) final;
    void setBufferSize(int rcvSize, int sndSize) final;

    IceInternal::TransceiverPtr delegate() const { return _transceiver; }

private:
    friend class Connector;
    friend class Acceptor;
    friend class EndpointI;

    const IceInternal::TransceiverPtr _transceiver;
    const ConfigurationPtr _configuration;
    bool _initialized;

    IceInternal::Buffer _readBuffer;
    IceInternal::Buffer::Container::const_iterator _readBufferPos;
    bool _buffered;
};

#endif
