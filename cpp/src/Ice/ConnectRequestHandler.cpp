//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/ConnectRequestHandler.h>
#include <Ice/ConnectionRequestHandler.h>
#include <Ice/RequestHandlerFactory.h>
#include <Ice/Instance.h>
#include <Ice/Proxy.h>
#include <Ice/ConnectionI.h>
#include <Ice/RouterInfo.h>
#include <Ice/OutgoingAsync.h>
#include <Ice/Protocol.h>
#include <Ice/Properties.h>
#include <Ice/ThreadPool.h>
#include <Ice/ProxyFactory.h>

using namespace std;
using namespace IceInternal;

ConnectRequestHandler::ConnectRequestHandler(const ReferencePtr& ref) :
    RequestHandler(ref),
    _initialized(false),
    _flushing(false)
{
}

RequestHandlerPtr
ConnectRequestHandler::connect()
{
    unique_lock lock(_mutex);
    return _requestHandler ? _requestHandler : shared_from_this();
}

AsyncStatus
ConnectRequestHandler::sendAsyncRequest(const ProxyOutgoingAsyncBasePtr& out)
{
    {
        unique_lock lock(_mutex);
        if(!_initialized)
        {
            out->cancelable(shared_from_this()); // This will throw if the request is canceled
        }

        if(!initialized(lock))
        {
            _requests.push_back(out);
            return AsyncStatusQueued;
        }
    }
    return out->invokeRemote(_connection, _compress, _response);
}

void
ConnectRequestHandler::asyncRequestCanceled(const OutgoingAsyncBasePtr& outAsync, const Ice::LocalException& ex)
{
    {
        unique_lock lock(_mutex);
        if(_exception)
        {
            return; // The request has been notified of a failure already.
        }

        if(!initialized(lock))
        {
            for(deque<ProxyOutgoingAsyncBasePtr>::iterator p = _requests.begin(); p != _requests.end(); ++p)
            {
                if(p->get() == outAsync.get())
                {
                    _requests.erase(p);
                    if(outAsync->exception(ex))
                    {
                        outAsync->invokeExceptionAsync();
                    }
                    return;
                }
            }
        }
    }
    _connection->asyncRequestCanceled(outAsync, ex);
}

Ice::ConnectionIPtr
ConnectRequestHandler::getConnection()
{
    lock_guard lock(_mutex);
    //
    // First check for the connection, it's important otherwise the user could first get a connection
    // and then the exception if he tries to obtain the proxy cached connection mutiple times (the
    // exception can be set after the connection is set if the flush of pending requests fails).
    //
    if(_connection)
    {
        return _connection;
    }
    else if(_exception)
    {
        _exception->ice_throw();
    }
    return nullptr;
}

Ice::ConnectionIPtr
ConnectRequestHandler::waitForConnection()
{
    unique_lock lock(_mutex);
    if(_exception)
    {
        throw RetryException(*_exception);
    }
    //
    // Wait for the connection establishment to complete or fail.
    //
    _conditionVariable.wait(lock, [this] { return _initialized || _exception; });

    if(_exception)
    {
        _exception->ice_throw();
        return 0; // Keep the compiler happy.
    }
    else
    {
        return _connection;
    }
}

void
ConnectRequestHandler::setConnection(const Ice::ConnectionIPtr& connection, bool compress)
{
    {
        lock_guard lock(_mutex);
        assert(!_flushing && !_exception && !_connection);
        _connection = connection;
        _compress = compress;
    }

    //
    // If this proxy is for a non-local object, and we are using a router, then
    // add this proxy to the router info object.
    //
    RouterInfoPtr ri = _reference->getRouterInfo();
    Ice::ObjectPrxPtr proxy = _reference->getInstance()->proxyFactory()->referenceToProxy(_reference);
    if(ri && !ri->addProxy(proxy, shared_from_this()))
    {
        return; // The request handler will be initialized once addProxy returns.
    }

    //
    // We can now send the queued requests.
    //
    flushRequests();
}

void
ConnectRequestHandler::setException(const Ice::LocalException& ex)
{
    {
        lock_guard lock(_mutex);
        assert(!_flushing && !_initialized && !_exception);
        _flushing = true; // Ensures request handler is removed before processing new requests.
        _exception = ex.ice_clone();
    }

    //
    // NOTE: remove the request handler *before* notifying the requests that the connection
    // failed. It's important to ensure that future invocations will obtain a new connect
    // request handler once invocations are notified.
    //
    try
    {
        _reference->getInstance()->requestHandlerFactory()->removeRequestHandler(_reference, shared_from_this());
    }
    catch(const Ice::CommunicatorDestroyedException&)
    {
        // Ignore
    }

    for(deque<ProxyOutgoingAsyncBasePtr>::const_iterator p = _requests.begin(); p != _requests.end(); ++p)
    {
        if((*p)->exception(ex))
        {
            (*p)->invokeExceptionAsync();
        }
    }
    _requests.clear();

    {
        lock_guard lock(_mutex);
        _flushing = false;
        _conditionVariable.notify_all();
    }
}

void
ConnectRequestHandler::addedProxy()
{
    //
    // The proxy was added to the router info, we're now ready to send the
    // queued requests.
    //
    flushRequests();
}

bool
ConnectRequestHandler::initialized(unique_lock<mutex>& lock)
{
    // Must be called with the mutex locked.

    if(_initialized)
    {
        assert(_connection);
        return true;
    }
    else
    {
        _conditionVariable.wait(lock, [this] { return !_flushing; });

        if(_exception)
        {
            if(_connection)
            {
                //
                // Only throw if the connection didn't get established. If
                // it died after being established, we allow the caller to
                // retry the connection establishment by not throwing here
                // (the connection will throw RetryException).
                //
                return true;
            }
            _exception->ice_throw();
            return false; // Keep the compiler happy.
        }
        else
        {
            return _initialized;
        }
    }
}

void
ConnectRequestHandler::flushRequests()
{
    {
        lock_guard lock(_mutex);
        assert(_connection && !_initialized);

        //
        // We set the _flushing flag to true to prevent any additional queuing. Callers
        // might block for a little while as the queued requests are being sent but this
        // shouldn't be an issue as the request sends are non-blocking.
        //
        _flushing = true;
    }

    std::unique_ptr<Ice::LocalException> exception;
    while(!_requests.empty()) // _requests is immutable when _flushing = true
    {
        ProxyOutgoingAsyncBasePtr& req = _requests.front();
        try
        {
            if(req->invokeRemote(_connection, _compress, _response) & AsyncStatusInvokeSentCallback)
            {
                req->invokeSentAsync();
            }
        }
        catch(const RetryException& ex)
        {
            exception = ex.get()->ice_clone();

            // Remove the request handler before retrying.
            _reference->getInstance()->requestHandlerFactory()->removeRequestHandler(_reference, shared_from_this());

            req->retryException(*exception);
        }
        catch(const Ice::LocalException& ex)
        {
            exception = ex.ice_clone();

            if(req->exception(ex))
            {
                req->invokeExceptionAsync();
            }
        }
        _requests.pop_front();
    }

    {
        lock_guard lock(_mutex);
        assert(!_initialized);
        swap(_exception, exception);
        _initialized = !_exception;
        _flushing = false;

        //
        // Only remove once all the requests are flushed to
        // guarantee serialization.
        //
        _reference->getInstance()->requestHandlerFactory()->removeRequestHandler(_reference, shared_from_this());

        _conditionVariable.notify_all();
    }
}
