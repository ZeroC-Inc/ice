//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <TestAMDI.h>

using namespace std;

void
MetricsI::opAsync(function<void()> response, function<void(exception_ptr)>, const Ice::Current&)
{
    response();
}

void
MetricsI::failAsync(function<void()> response, function<void(exception_ptr)>, const Ice::Current& current)
{
    current.con->close(Ice::ConnectionClose::Forcefully);
    response();
}

void
MetricsI::opWithUserExceptionAsync(function<void()>, function<void(exception_ptr)> error, const Ice::Current&)
{
    try
    {
        throw Test::UserEx();
    }
    catch(...)
    {
        error(current_exception());
    }
}

void
MetricsI::opWithRequestFailedExceptionAsync(function<void()>, function<void(exception_ptr)> error,
                                             const Ice::Current&)
{
    try
    {
        throw Ice::ObjectNotExistException(__FILE__, __LINE__);
    }
    catch(...)
    {
        error(current_exception());
    }
}

void
MetricsI::opWithLocalExceptionAsync(function<void()>, function<void(exception_ptr)> error, const Ice::Current&)
{
    try
    {
        throw Ice::SyscallException(__FILE__, __LINE__);
    }
    catch(...)
    {
        error(current_exception());
    }
}

void
MetricsI::opWithUnknownExceptionAsync(function<void()>, function<void(exception_ptr)>, const Ice::Current&)
{
    throw "Test";
}

void
MetricsI::opByteSAsync(Test::ByteSeq, function<void()> response, function<void(exception_ptr)>, const Ice::Current&)
{
    response();
}

Ice::ObjectPrxPtr
MetricsI::getAdmin(const Ice::Current& current)
{
    return current.adapter->getCommunicator()->getAdmin();
}

void
MetricsI::shutdown(const Ice::Current& current)
{
    current.adapter->getCommunicator()->shutdown();
}

ControllerI::ControllerI(const Ice::ObjectAdapterPtr& adapter) : _adapter(adapter)
{
}

void
ControllerI::hold(const Ice::Current&)
{
    _adapter->hold();
    _adapter->waitForHold();
}

void
ControllerI::resume(const Ice::Current&)
{
    _adapter->activate();
}
