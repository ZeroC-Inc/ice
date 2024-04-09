//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "IdleTimeoutTransceiverDecorator.h"
#include "Ice/Buffer.h"

#include <chrono>

using namespace std;
using namespace Ice;
using namespace IceInternal;

namespace
{
    class HeartbeatTimerTask final : public IceUtil::TimerTask
    {
    public:
        HeartbeatTimerTask(const ConnectionIPtr& connection) : _connection(connection) {}

        void runTimerTask() final
        {
            if (auto connection = _connection.lock())
            {
                connection->sendHeartbeat();
            }
            // else nothing to do, the connection is already gone.
        }

    private:
        const std::weak_ptr<ConnectionI> _connection;
    };

    class IdleCheckTimerTask final : public IceUtil::TimerTask, public std::enable_shared_from_this<IdleCheckTimerTask>
    {
    public:
        IdleCheckTimerTask(const ConnectionIPtr& connection, const chrono::milliseconds& idleTimeout)
            : _connection(connection),
              _idleTimeout(idleTimeout)
        {
        }

        void runTimerTask() final
        {
            if (auto connection = _connection.lock())
            {
                connection->idleCheck(shared_from_this(), _idleTimeout);
            }
            // else nothing to do, the connection is already gone.
        }

    private:
        const std::weak_ptr<ConnectionI> _connection;
        const chrono::milliseconds _idleTimeout;
    };
}

void
IdleTimeoutTransceiverDecorator::decoratorInit(const ConnectionIPtr& connection)
{
    _heartbeatTimerTask = make_shared<HeartbeatTimerTask>(connection);
    if (_enableIdleCheck)
    {
        _idleCheckTimerTask = make_shared<IdleCheckTimerTask>(connection, _idleTimeout);
    }
}

SocketOperation
IdleTimeoutTransceiverDecorator::initialize(Buffer& readBuffer, Buffer& writeBuffer)
{
    SocketOperation op = _decoratee->initialize(readBuffer, writeBuffer);

    if (op == SocketOperationNone) // connected
    {
        _timer->schedule(_heartbeatTimerTask, _idleTimeout / 2);
        if (_enableIdleCheck)
        {
            // Reschedule because with SSL, the connection is connected after a read.
            _timer->schedule(_idleCheckTimerTask, _idleTimeout, true);
        }
    }

    return op;
}

IdleTimeoutTransceiverDecorator::~IdleTimeoutTransceiverDecorator()
{
    // If we destroy this object before calling init(), _heartbeatTimerTask and _idleCheckTimerTask will be null.
    if (_heartbeatTimerTask)
    {
        _timer->cancel(_heartbeatTimerTask);
    }
    if (_idleCheckTimerTask)
    {
        _timer->cancel(_idleCheckTimerTask);
    }
}

void
IdleTimeoutTransceiverDecorator::close()
{
    _timer->cancel(_heartbeatTimerTask);
    if (_enableIdleCheck)
    {
        _timer->cancel(_idleCheckTimerTask);
    }
    _decoratee->close();
}

SocketOperation
IdleTimeoutTransceiverDecorator::write(Buffer& buf)
{
    // We're about to write something - we don't need to send a concurrent heartbeat.
    _timer->cancel(_heartbeatTimerTask);

    Buffer::Container::iterator start = buf.i;
    SocketOperation op = _decoratee->write(buf);
    if (buf.i != start)
    {
        // Schedule heartbeat after writing some data.
        _timer->schedule(_heartbeatTimerTask, _idleTimeout / 2);
    }

    return op;
}

#if defined(ICE_USE_IOCP)
bool
IdleTimeoutTransceiverDecorator::startWrite(Buffer& buf)
{
    // We're about to write something - we don't need to send a concurrent heartbeat.
    _timer->cancel(_heartbeatTimerTask);

    Buffer::Container::iterator start = buf.i;
    bool allWritten = _decoratee->startWrite(buf);
    if (buf.i != start)
    {
        // Schedule heartbeat after writing some data.
        assert(false); // TODO: temporary to check startWrite ever moves buf.i.
        _timer->schedule(_heartbeatTimerTask, _idleTimeout / 2);
    }
    return allWritten;
}

void
IdleTimeoutTransceiverDecorator::finishWrite(Buffer& buf)
{
    Buffer::Container::iterator start = buf.i;
    _decoratee->finishWrite(buf);
    if (buf.i != start)
    {
        // Schedule heartbeat after writing some data.
        _timer->schedule(_heartbeatTimerTask, _idleTimeout / 2);
    }
}

void
IdleTimeoutTransceiverDecorator::startRead(Buffer& buf)
{
    // We always call finishRead or read to actually read the data.
    _decoratee->startRead(buf);
}

void
IdleTimeoutTransceiverDecorator::finishRead(Buffer& buf)
{
    if (_enableIdleCheck)
    {
        // We reschedule the idle check as soon as possible to reduce the chances it kicks in while we're reading.
        _timer->schedule(_idleCheckTimerTask, _idleTimeout, true);
    }

    _decoratee->finishRead(buf);
}

#endif

SocketOperation
IdleTimeoutTransceiverDecorator::read(Buffer& buf)
{
    if (_enableIdleCheck)
    {
        // We reschedule the idle check as soon as possible to reduce the chances it kicks in while we're reading.
        _timer->schedule(_idleCheckTimerTask, _idleTimeout, true);
    }
    return _decoratee->read(buf);
}
