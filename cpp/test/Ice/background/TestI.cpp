//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <TestI.h>
#include "Ice/Ice.h"

using namespace std;
using namespace Ice;

void
BackgroundI::op(const Current& current)
{
    _controller->checkCallPause(current);
}

void
BackgroundI::opWithPayload(ByteSeq, const Current& current)
{
    _controller->checkCallPause(current);
}

void
BackgroundI::shutdown(const Current& current)
{
    current.adapter->getCommunicator()->shutdown();
}

BackgroundI::BackgroundI(const BackgroundControllerIPtr& controller) : _controller(controller) {}

void
BackgroundControllerI::pauseCall(string opName, const Current&)
{
    lock_guard lock(_mutex);
    _pausedCalls.insert(opName);
}

void
BackgroundControllerI::resumeCall(string opName, const Current&)
{
    lock_guard lock(_mutex);
    _pausedCalls.erase(opName);
    _condition.notify_all();
}

void
BackgroundControllerI::checkCallPause(const Current& current)
{
    unique_lock lock(_mutex);
    _condition.wait(lock, [this, &current] { return _pausedCalls.find(current.operation) == _pausedCalls.end(); });
}

void
BackgroundControllerI::holdAdapter(const Current&)
{
    _adapter->hold();
}

void
BackgroundControllerI::resumeAdapter(const Current&)
{
    _adapter->activate();
}

void
BackgroundControllerI::initializeSocketOperation(int status, const Current&)
{
    _configuration->initializeSocketOperation(static_cast<IceInternal::SocketOperation>(status));
}

void
BackgroundControllerI::initializeException(bool enable, const Current&)
{
    _configuration->initializeException(enable ? make_exception_ptr(SocketException(__FILE__, __LINE__)) : nullptr);
}

void
BackgroundControllerI::readReady(bool enable, const Current&)
{
    _configuration->readReady(enable);
}

void
BackgroundControllerI::readException(bool enable, const Current&)
{
    _configuration->readException(enable ? make_exception_ptr(SocketException(__FILE__, __LINE__)) : nullptr);
}

void
BackgroundControllerI::writeReady(bool enable, const Current&)
{
    _configuration->writeReady(enable);
}

void
BackgroundControllerI::writeException(bool enable, const Current&)
{
    _configuration->writeException(enable ? make_exception_ptr(SocketException(__FILE__, __LINE__)) : nullptr);
}

void
BackgroundControllerI::buffered(bool enable, const Current&)
{
    _configuration->buffered(enable);
}

BackgroundControllerI::BackgroundControllerI(const ObjectAdapterPtr& adapter, const ConfigurationPtr& configuration)
    : _adapter(adapter),
      _configuration(configuration)
{
}
