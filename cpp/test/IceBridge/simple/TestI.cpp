//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "TestI.h"
#include "Ice/Ice.h"
#include "TestHelper.h"

using namespace std;
using namespace Test;

namespace
{
    Ice::Identity callbackId = {"callback", ""};
}

void
MyClassI::callCallbackAsync(function<void()> response, function<void(exception_ptr)> error, const Ice::Current& current)
{
    checkConnection(current.con);
    auto prx = current.con->createProxy<CallbackPrx>(callbackId);

    prx->pingAsync(
        [response = std::move(response)]() { response(); },
        [error = std::move(error)](exception_ptr e) { error(e); });
}

void
MyClassI::getCallbackCountAsync(
    function<void(int)> response,
    function<void(exception_ptr)> error,
    const Ice::Current& current)
{
    checkConnection(current.con);
    auto prx = current.con->createProxy<CallbackPrx>(callbackId);

    prx->getCountAsync(
        [response = std::move(response)](int count) { response(count); },
        [error = std::move(error)](exception_ptr e) { error(e); });
}

int
MyClassI::getConnectionCount(const Ice::Current& current)
{
    checkConnection(current.con);
    return static_cast<int>(_connections.size());
}

string
MyClassI::getConnectionInfo(const Ice::Current& current)
{
    checkConnection(current.con);
    return current.con->toString();
}

void
MyClassI::closeConnection(bool forceful, const Ice::Current& current)
{
    checkConnection(current.con);
    if (forceful)
    {
        current.con->abort();
    }
    else
    {
        current.con->close(nullptr, nullptr);
    }
}

void
MyClassI::datagram(const Ice::Current& current)
{
    checkConnection(current.con);
    test(current.con->getEndpoint()->getInfo()->datagram());
    ++_datagramCount;
}

int
MyClassI::getDatagramCount(const Ice::Current& current)
{
    checkConnection(current.con);
    return _datagramCount;
}

void
MyClassI::callDatagramCallback(const Ice::Current& current)
{
    checkConnection(current.con);
    test(current.con->getEndpoint()->getInfo()->datagram());
    current.con->createProxy<CallbackPrx>(callbackId)->datagram();
}

void
MyClassI::getCallbackDatagramCountAsync(
    function<void(int)> response,
    function<void(exception_ptr)> error,
    const Ice::Current& current)
{
    checkConnection(current.con);
    auto prx = current.con->createProxy<CallbackPrx>(callbackId);

    prx->getDatagramCountAsync(
        [response = std::move(response)](int count) { response(count); },
        [error = std::move(error)](auto e) { error(e); });
}

void
MyClassI::shutdown(const Ice::Current& current)
{
    checkConnection(current.con);
    current.adapter->getCommunicator()->shutdown();
}

void
MyClassI::removeConnection(const shared_ptr<Ice::Connection>& con)
{
    lock_guard<mutex> lg(_lock);
    _connections.erase(con);
}

void
MyClassI::checkConnection(const shared_ptr<Ice::Connection>& con)
{
    lock_guard<mutex> lg(_lock);
    if (_connections.find(con) == _connections.end())
    {
        _connections.insert(make_pair(con, 0));
        con->setCloseCallback([self = shared_from_this()](const auto& c) { self->removeConnection(c); });
    }
}
