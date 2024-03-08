//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <BlobjectI.h>

using namespace std;

BlobjectI::BlobjectI() : _startBatch(false) {}

void
BlobjectI::setConnection(const Ice::ConnectionPtr& connection)
{
    lock_guard lock(_mutex);
    _connection = connection;
    _condition.notify_all();
}

void
BlobjectI::startBatch()
{
    assert(!_batchProxy);
    _startBatch = true;
}

void
BlobjectI::flushBatch()
{
    assert(_batchProxy);
    _batchProxy->ice_flushBatchRequests();
    _batchProxy = nullopt;
}

void
BlobjectI::ice_invokeAsync(
    std::vector<uint8_t> inEncaps,
    std::function<void(bool, const std::vector<uint8_t>&)> response,
    std::function<void(std::exception_ptr)> ex,
    const Ice::Current& current)
{
    auto connection = getConnection(current);
    const bool twoway = current.requestId > 0;
    auto obj = connection->createProxy(current.id);
    if (!twoway)
    {
        if (_startBatch)
        {
            _startBatch = false;
            _batchProxy = obj->ice_batchOneway();
        }
        if (_batchProxy)
        {
            obj = _batchProxy.value();
        }

        if (!current.facet.empty())
        {
            obj = obj->ice_facet(current.facet);
        }

        if (_batchProxy)
        {
            vector<uint8_t> out;
            obj->ice_invoke(current.operation, current.mode, inEncaps, out, current.ctx);
            response(true, vector<uint8_t>());
        }
        else
        {
            obj->ice_oneway()->ice_invokeAsync(
                current.operation,
                current.mode,
                inEncaps,
                [](bool, const std::vector<uint8_t>&) { assert(0); },
                ex,
                [&](bool) { response(true, vector<uint8_t>()); },
                current.ctx);
        }
    }
    else
    {
        if (!current.facet.empty())
        {
            obj = obj->ice_facet(current.facet);
        }

        obj->ice_invokeAsync(current.operation, current.mode, inEncaps, response, ex, nullptr, current.ctx);
    }
}

Ice::ConnectionPtr
BlobjectI::getConnection(const Ice::Current& current)
{
    unique_lock lock(_mutex);
    if (!_connection)
    {
        return current.con;
    }

    try
    {
        _connection->throwException();
    }
    catch (const Ice::ConnectionLostException&)
    {
        // If we lost the connection, wait 5 seconds for the server to re-establish it. Some tests,
        // involve connection closure (e.g.: exceptions MemoryLimitException test) and the server
        // automatically re-establishes the connection with the echo server.
        _condition.wait_for(lock, chrono::seconds(5));
        if (!_connection)
        {
            throw;
        }
    }
    return _connection;
}
