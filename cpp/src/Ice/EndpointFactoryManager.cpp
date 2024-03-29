//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "EndpointFactoryManager.h"
#include "Ice/Endpoint.h"
#include "OpaqueEndpointI.h"
#include "Ice/OutputStream.h"
#include "Ice/InputStream.h"
#include "Ice/LocalException.h"
#include "Instance.h"
#include "Ice/Properties.h"
#include "DefaultsAndOverrides.h"
#include "IceUtil/StringUtil.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

IceInternal::EndpointFactoryManager::EndpointFactoryManager(const InstancePtr& instance) : _instance(instance) {}

void
IceInternal::EndpointFactoryManager::initialize() const
{
    for (vector<EndpointFactoryPtr>::size_type i = 0; i < _factories.size(); i++)
    {
        _factories[i]->initialize();
    }
}

void
IceInternal::EndpointFactoryManager::add(const EndpointFactoryPtr& factory)
{
    lock_guard lock(_mutex); // TODO: Necessary?

    //
    // TODO: Optimize with a map?
    //
    for (vector<EndpointFactoryPtr>::size_type i = 0; i < _factories.size(); i++)
    {
        if (_factories[i]->type() == factory->type())
        {
            assert(false); // TODO: Exception?
        }
    }
    _factories.push_back(factory);
}

EndpointFactoryPtr
IceInternal::EndpointFactoryManager::get(int16_t type) const
{
    lock_guard lock(_mutex); // TODO: Necessary?

    //
    // TODO: Optimize with a map?
    //
    for (vector<EndpointFactoryPtr>::size_type i = 0; i < _factories.size(); i++)
    {
        if (_factories[i]->type() == type)
        {
            return _factories[i];
        }
    }
    return nullptr;
}

EndpointIPtr
IceInternal::EndpointFactoryManager::create(const string& str, bool oaEndpoint) const
{
    vector<string> v;
    bool b = IceUtilInternal::splitString(str, " \t\n\r", v);
    if (!b)
    {
        throw EndpointParseException(__FILE__, __LINE__, "mismatched quote");
    }

    if (v.empty())
    {
        throw EndpointParseException(__FILE__, __LINE__, "value has no non-whitespace characters");
    }

    string protocol = v.front();
    v.erase(v.begin());

    if (protocol == "default")
    {
        protocol = _instance->defaultsAndOverrides()->defaultProtocol;
    }

    EndpointFactoryPtr factory;
    {
        lock_guard lock(_mutex); // TODO: Necessary?

        //
        // TODO: Optimize with a map?
        //
        for (vector<EndpointFactoryPtr>::size_type i = 0; i < _factories.size(); i++)
        {
            if (_factories[i]->protocol() == protocol)
            {
                factory = _factories[i];
            }
        }
    }

    if (factory)
    {
        EndpointIPtr e = factory->create(v, oaEndpoint);
        if (!v.empty())
        {
            throw EndpointParseException(
                __FILE__,
                __LINE__,
                "unrecognized argument `" + v.front() + "' in endpoint `" + str + "'");
        }
        return e;
    }

    //
    // If the stringified endpoint is opaque, create an unknown endpoint,
    // then see whether the type matches one of the known endpoints.
    //
    if (protocol == "opaque")
    {
        EndpointIPtr ue = make_shared<OpaqueEndpointI>(v);
        if (!v.empty())
        {
            throw EndpointParseException(
                __FILE__,
                __LINE__,
                "unrecognized argument `" + v.front() + "' in endpoint `" + str + "'");
        }
        factory = get(ue->type());
        if (factory)
        {
            //
            // Make a temporary stream, write the opaque endpoint data into the stream,
            // and ask the factory to read the endpoint data from that stream to create
            // the actual endpoint.
            //
            OutputStream bs(_instance.get(), Ice::currentProtocolEncoding);
            bs.write(ue->type());
            ue->streamWrite(&bs);
            InputStream is(bs.instance(), bs.getEncoding(), bs);
            short type;
            is.read(type);
            is.startEncapsulation();
            EndpointIPtr e = factory->read(&is);
            is.endEncapsulation();
            return e;
        }
        return ue; // Endpoint is opaque, but we don't have a factory for its type.
    }

    return nullptr;
}

EndpointIPtr
IceInternal::EndpointFactoryManager::read(InputStream* s) const
{
    int16_t type;
    s->read(type);

    EndpointFactoryPtr factory = get(type);
    EndpointIPtr e;

    s->startEncapsulation();

    if (factory)
    {
        e = factory->read(s);
    }
    //
    // If the factory failed to read the endpoint, return an opaque endpoint. This can
    // occur if for example the factory delegates to another factory and this factory
    // isn't available. In this case, the factory needs to make sure the stream position
    // is preserved for reading the opaque endpoint.
    //
    if (!e)
    {
        e = make_shared<OpaqueEndpointI>(type, s);
    }

    s->endEncapsulation();

    return e;
}

void
IceInternal::EndpointFactoryManager::destroy()
{
    for (vector<EndpointFactoryPtr>::size_type i = 0; i < _factories.size(); i++)
    {
        _factories[i]->destroy();
    }
    _factories.clear();
}
