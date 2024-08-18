//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_OBJECT_ADAPTER_FACTORY_H
#define ICE_OBJECT_ADAPTER_FACTORY_H

#include "ObjectAdapterI.h"

#include <condition_variable>
#include <mutex>
#include <set>

namespace IceInternal
{
    class ObjectAdapterFactory : public std::enable_shared_from_this<ObjectAdapterFactory>
    {
    public:
        void shutdown();
        void waitForShutdown();
        bool isShutdown() const;
        void destroy();

        void updateObservers(void (Ice::ObjectAdapterI::*)());

        Ice::ObjectAdapterPtr createObjectAdapter(
            const std::string& name,
            const std::optional<Ice::RouterPrx>&,
            const std::optional<Ice::SSL::ServerAuthenticationOptions>&);

        Ice::ObjectAdapterPtr findObjectAdapter(const IceInternal::ReferencePtr&);
        void removeObjectAdapter(const Ice::ObjectAdapterPtr&);
        void flushAsyncBatchRequests(const CommunicatorFlushBatchAsyncPtr&, Ice::CompressBatch) const;

        ObjectAdapterFactory(const InstancePtr&, const Ice::CommunicatorPtr&);
        virtual ~ObjectAdapterFactory();

    private:
        const std::weak_ptr<Instance> _instance;
        const std::weak_ptr<Ice::Communicator> _communicator;
        bool _isShutdown = false;
        std::set<std::string> _adapterNamesInUse;
        std::list<std::shared_ptr<Ice::ObjectAdapterI>> _adapters;
        mutable std::recursive_mutex _mutex;
        std::condition_variable_any _conditionVariable;
    };
}

#endif
