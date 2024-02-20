//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/ObjectAdapterFactory.h>
#include <Ice/ObjectAdapterI.h>
#include <Ice/Object.h>
#include <Ice/LocalException.h>
#include <Ice/UUID.h>
#include "Ice/Router.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

void
IceInternal::ObjectAdapterFactory::shutdown()
{
    list<shared_ptr<ObjectAdapterI>> adapters;

    {
        lock_guard lock(_mutex);

        //
        // Ignore shutdown requests if the object adapter factory has
        // already been shut down.
        //
        if(!_instance)
        {
            return;
        }

        adapters = _adapters;

        _instance = nullptr;
        _communicator = nullptr;

        _conditionVariable.notify_all();
    }

    // Deactivate outside the thread synchronization, to avoid deadlocks.
    for_each(adapters.begin(), adapters.end(), [](const shared_ptr<ObjectAdapterI>& adapter) { adapter->deactivate(); });
}

void
IceInternal::ObjectAdapterFactory::waitForShutdown()
{
    list<shared_ptr<ObjectAdapterI>> adapters;

    {
        unique_lock lock(_mutex);

        //
        // First we wait for the shutdown of the factory itself.
        //
        _conditionVariable.wait(lock, [this] { return !_instance; });
        adapters = _adapters;
    }

    // Now we wait for deactivation of each object adapter.
    for_each(adapters.begin(), adapters.end(), [](const shared_ptr<ObjectAdapterI>& adapter) { adapter->waitForDeactivate(); });
}

bool
IceInternal::ObjectAdapterFactory::isShutdown() const
{
    lock_guard lock(_mutex);

    return _instance == 0;
}

void
IceInternal::ObjectAdapterFactory::destroy()
{
    //
    // First wait for shutdown to finish.
    //
    waitForShutdown();

    list<shared_ptr<ObjectAdapterI>> adapters;

    {
        lock_guard lock(_mutex);
        adapters = _adapters;
    }

    // Now we destroy each object adapter.
    for_each(adapters.begin(), adapters.end(), [](const shared_ptr<ObjectAdapterI>& adapter) { adapter->destroy(); });
    {
        lock_guard lock(_mutex);
        _adapters.clear();
    }
}

void
IceInternal::ObjectAdapterFactory::updateObservers(void (ObjectAdapterI::*fn)())
{
    list<shared_ptr<ObjectAdapterI>> adapters;

    {
        lock_guard lock(_mutex);
        adapters = _adapters;
    }

    for_each(adapters.begin(), adapters.end(),
             [fn](const shared_ptr<ObjectAdapterI>& adapter)
             {
                 (adapter.get() ->* fn)();
             });
}

ObjectAdapterPtr
IceInternal::ObjectAdapterFactory::createObjectAdapter(const string& name, const optional<RouterPrx>& router)
{
    shared_ptr<ObjectAdapterI> adapter;
    {
        lock_guard lock(_mutex);

        if(!_instance)
        {
            throw CommunicatorDestroyedException(__FILE__, __LINE__);
        }

        if(name.empty())
        {
            string uuid = Ice::generateUUID();
            adapter = make_shared<ObjectAdapterI>(_instance, _communicator, shared_from_this(), uuid, true);
        }
        else
        {
            if(_adapterNamesInUse.find(name) != _adapterNamesInUse.end())
            {
                throw AlreadyRegisteredException(__FILE__, __LINE__, "object adapter", name);
            }
            adapter = make_shared<ObjectAdapterI>(_instance, _communicator, shared_from_this(), name, false);
            _adapterNamesInUse.insert(name);
        }
    }

    //
    // Must be called outside the synchronization since initialize can make client invocations
    // on the router if it's set.
    //
    bool initialized = false;
    try
    {
        adapter->initialize(router);
        initialized = true;

        lock_guard lock(_mutex);
        if(!_instance)
        {
            throw CommunicatorDestroyedException(__FILE__, __LINE__);
        }
        _adapters.push_back(adapter);
    }
    catch(const Ice::CommunicatorDestroyedException&)
    {
        if(initialized)
        {
            adapter->destroy();
        }
        throw;
    }
    catch(const std::exception&)
    {
        if(!name.empty())
        {
            lock_guard lock(_mutex);
            _adapterNamesInUse.erase(name);
        }
        throw;
    }

    return adapter;
}

ObjectAdapterPtr
IceInternal::ObjectAdapterFactory::findObjectAdapter(const ReferencePtr& reference)
{
    list<shared_ptr<ObjectAdapterI>> adapters;
    {
        lock_guard lock(_mutex);

        if(!_instance)
        {
            return nullptr;
        }

        adapters = _adapters;
    }

    for(list<shared_ptr<ObjectAdapterI>>::iterator p = adapters.begin(); p != adapters.end(); ++p)
    {
        try
        {
            if((*p)->isLocal(reference))
            {
                return *p;
            }
        }
        catch(const ObjectAdapterDeactivatedException&)
        {
            // Ignore.
        }
    }

    return nullptr;
}

void
IceInternal::ObjectAdapterFactory::removeObjectAdapter(const ObjectAdapterPtr& adapter)
{
    lock_guard lock(_mutex);

    if(!_instance)
    {
        return;
    }

    for(list<shared_ptr<ObjectAdapterI>>::iterator p = _adapters.begin(); p != _adapters.end(); ++p)
    {
        if(*p == adapter)
        {
            _adapters.erase(p);
            break;
        }
    }
    _adapterNamesInUse.erase(adapter->getName());
}

void
IceInternal::ObjectAdapterFactory::flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr& outAsync,
                                                           CompressBatch compressBatch) const
{
    list<shared_ptr<ObjectAdapterI>> adapters;
    {
        lock_guard lock(_mutex);

        adapters = _adapters;
    }

    for(list<shared_ptr<ObjectAdapterI>>::const_iterator p = adapters.begin(); p != adapters.end(); ++p)
    {
        (*p)->flushAsyncBatchRequests(outAsync, compressBatch);
    }
}

IceInternal::ObjectAdapterFactory::ObjectAdapterFactory(const InstancePtr& instance,
                                                        const CommunicatorPtr& communicator) :
    _instance(instance),
    _communicator(communicator)
{
}

IceInternal::ObjectAdapterFactory::~ObjectAdapterFactory()
{
    assert(!_instance);
    assert(!_communicator);
    assert(_adapters.empty());
}
