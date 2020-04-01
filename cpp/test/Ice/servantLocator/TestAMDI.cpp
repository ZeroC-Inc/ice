//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <TestAMDI.h>

using namespace std;
using namespace Ice;

void
TestAMDI::requestFailedExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::unknownUserExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::unknownLocalExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::unknownExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::userExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::localExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::stdExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::cppExceptionAsync(function<void()> response, function<void(exception_ptr)>, const Current&)
{
    response();
}

void
TestAMDI::unknownExceptionWithServantExceptionAsync(function<void()>,
                                                    function<void(exception_ptr)> error,
                                                    const Current&)
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
TestAMDI::impossibleExceptionAsync(bool _cpp_throw,
                                   function<void(const string&)> response,
                                   function<void(exception_ptr)> error,
                                   const Current&)
{
    if(_cpp_throw)
    {
        try
        {
            throw Test::TestImpossibleException();
        }
        catch(...)
        {
            error(current_exception());
        }
    }
    else
    {
        //
        // Return a value so we can be sure that the stream position
        // is reset correctly if finished() throws.
        //
        response("Hello");
    }
}

void
TestAMDI::intfUserExceptionAsync(bool _cpp_throw,
                                 function<void(const string&)> response,
                                 function<void(exception_ptr)> error,
                                 const Current&)
{
    if(_cpp_throw)
    {
        try
        {
            throw Test::TestIntfUserException();
        }
        catch(...)
        {
            error(current_exception());
        }
    }
    else
    {
        //
        // Return a value so we can be sure that the stream position
        // is reset correctly if finished() throws.
        //
        response("Hello");
    }
}

void
TestAMDI::asyncResponseAsync(function<void()> response,
                             function<void(exception_ptr)>,
                             const Current&)
{
    response();
    throw Ice::ObjectNotExistException(__FILE__, __LINE__);
}

void
TestAMDI::asyncExceptionAsync(function<void()>,
                              function<void(exception_ptr)> error,
                              const Current&)
{
    try
    {
        throw Test::TestIntfUserException();
    }
    catch(...)
    {
        error(current_exception());
    }
    throw Ice::ObjectNotExistException(__FILE__, __LINE__);
}

void
TestAMDI::shutdownAsync(function<void()> response,
                        function<void(exception_ptr)>,
                        const Current& current)
{
    current.adapter->deactivate();
    response();
}

Cookie::~Cookie()
{
}

string
Cookie::message() const
{
    return "blahblah";
}
