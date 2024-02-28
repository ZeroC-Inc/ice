//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <BlobjectI.h>
#include <Test.h>
#include <TestHelper.h>

using namespace std;

bool
invokeInternal(Ice::InputStream& in, vector<uint8_t>& outEncaps, const Ice::Current& current)
{
    Ice::CommunicatorPtr communicator = current.adapter->getCommunicator();
    Ice::OutputStream out(communicator);
    out.startEncapsulation();
    if(current.operation == "opOneway")
    {
        return true;
    }
    else if(current.operation == "opString")
    {
        string s;
        in.startEncapsulation();
        in.read(s);
        in.endEncapsulation();
        out.write(s);
        out.write(s);
        out.endEncapsulation();
        out.finished(outEncaps);
        return true;
    }
    else if(current.operation == "opException")
    {
        if(current.ctx.find("raise") != current.ctx.end())
        {
            throw Test::MyException();
        }
        Test::MyException ex;
        out.writeException(ex);
        out.endEncapsulation();
        out.finished(outEncaps);
        return false;
    }
    else if(current.operation == "shutdown")
    {
        out.endEncapsulation();
        out.finished(outEncaps);
        communicator->shutdown();
        return true;
    }
    else if(current.operation == "ice_isA")
    {
        string s;
        in.startEncapsulation();
        in.read(s);
        in.endEncapsulation();
        if(s == "::Test::MyClass")
        {
            out.write(true);
        }
        else
        {
            out.write(false);
        }
        out.endEncapsulation();
        out.finished(outEncaps);
        return true;
    }
    else
    {
        Ice::OperationNotExistException ex(__FILE__, __LINE__);
        ex.id = current.id;
        ex.facet = current.facet;
        ex.operation = current.operation;
        throw ex;
    }
}

bool
BlobjectI::ice_invoke(vector<uint8_t> inEncaps, vector<uint8_t>& outEncaps, const Ice::Current& current)
{
    Ice::InputStream in(current.adapter->getCommunicator(), current.encoding, inEncaps);
    return invokeInternal(in, outEncaps, current);
}

bool
BlobjectArrayI::ice_invoke(pair<const uint8_t*, const uint8_t*> inEncaps, vector<uint8_t>& outEncaps,
                           const Ice::Current& current)
{
    Ice::InputStream in(current.adapter->getCommunicator(), current.encoding, inEncaps);
    return invokeInternal(in, outEncaps, current);
}

void
BlobjectAsyncI::ice_invokeAsync(vector<uint8_t> inEncaps,
                                function<void(bool, const vector<uint8_t>&)> response,
                                function<void(exception_ptr)>,
                                const Ice::Current& current)
{
    Ice::InputStream in(current.adapter->getCommunicator(), inEncaps);
    vector<uint8_t> outEncaps;
    bool ok = invokeInternal(in, outEncaps, current);
    response(ok, outEncaps);
}

void
BlobjectArrayAsyncI::ice_invokeAsync(pair<const uint8_t*, const uint8_t*> inEncaps,
                                     function<void(bool, const pair<const uint8_t*, const uint8_t*>&)> response,
                                     function<void(exception_ptr)>,
                                     const Ice::Current& current)
{
    Ice::InputStream in(current.adapter->getCommunicator(), inEncaps);
    vector<uint8_t> outEncaps;
    bool ok = invokeInternal(in, outEncaps, current);
    pair<const uint8_t*, const uint8_t*> outPair(static_cast<const uint8_t*>(nullptr), static_cast<const uint8_t*>(nullptr));
    if(outEncaps.size() != 0)
    {
        outPair.first = &outEncaps[0];
        outPair.second = &outEncaps[0] + outEncaps.size();
    }
    response(ok, outPair);
}
