//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "FixedRequestHandler.h"
#include "ConnectionI.h"
#include "Ice/OutgoingAsync.h"
#include "Ice/Proxy.h"
#include "Reference.h"
#include "RouterInfo.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

FixedRequestHandler::FixedRequestHandler(
    const ReferencePtr& reference,
    const Ice::ConnectionIPtr& connection,
    bool compress)
    : RequestHandler(reference),
      _connection(connection),
      _compress(compress)
{
}

AsyncStatus
FixedRequestHandler::sendAsyncRequest(const ProxyOutgoingAsyncBasePtr& out)
{
    return out->invokeRemote(_connection, _compress, _response);
}

void
FixedRequestHandler::asyncRequestCanceled(const OutgoingAsyncBasePtr& outAsync, std::exception_ptr ex)
{
    _connection->asyncRequestCanceled(outAsync, ex);
}

Ice::ConnectionIPtr
FixedRequestHandler::getConnection()
{
    return _connection;
}

Ice::ConnectionIPtr
FixedRequestHandler::waitForConnection()
{
    return _connection;
}
