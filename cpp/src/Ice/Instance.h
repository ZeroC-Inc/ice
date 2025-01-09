//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_INSTANCE_H
#define ICE_INSTANCE_H

#include "ConnectionFactoryF.h"
#include "ConnectionOptions.h"
#include "DefaultsAndOverridesF.h"
#include "EndpointFactoryManagerF.h"
#include "IPEndpointIF.h"
#include "Ice/CommunicatorF.h"
#include "Ice/Config.h"
#include "Ice/FacetMap.h"
#include "Ice/ImplicitContext.h"
#include "Ice/Initialize.h"
#include "Ice/InstanceF.h"
#include "Ice/Instrumentation.h"
#include "Ice/Plugin.h"
#include "Ice/Process.h"
#include "Ice/StringConverter.h"
#include "Ice/Timer.h"
#include "LocatorInfoF.h"
#include "NetworkF.h"
#include "NetworkProxyF.h"
#include "ObjectAdapterFactoryF.h"
#include "Protocol.h"
#include "ReferenceFactoryF.h"
#include "RetryQueueF.h"
#include "RouterInfoF.h"
#include "SSL/SSLEngineF.h"
#include "ThreadPoolF.h"
#include "TraceLevelsF.h"

#include <list>

namespace IceInternal
{
    class ThreadObserverTimer;
    using ThreadObserverTimerPtr = std::shared_ptr<ThreadObserverTimer>;

    class MetricsAdminI;
    using MetricsAdminIPtr = std::shared_ptr<MetricsAdminI>;

    //
    // Structure to track warnings for attempts to set socket buffer sizes
    //
    struct BufSizeWarnInfo
    {
        // Whether send size warning has been emitted
        bool sndWarn;

        // The send size for which the warning was emitted
        int sndSize;

        // Whether receive size warning has been emitted
        bool rcvWarn;

        // The receive size for which the warning was emitted
        int rcvSize;
    };

    class Instance : public std::enable_shared_from_this<Instance>
    {
    public:
        static InstancePtr create(const Ice::CommunicatorPtr&, const Ice::InitializationData&);
        virtual ~Instance();
        [[nodiscard]] [[nodiscard]] bool destroyed() const;
        [[nodiscard]] [[nodiscard]] const Ice::InitializationData& initializationData() const { return _initData; }
        [[nodiscard]] [[nodiscard]] TraceLevelsPtr traceLevels() const;
        [[nodiscard]] [[nodiscard]] DefaultsAndOverridesPtr defaultsAndOverrides() const;
        [[nodiscard]] [[nodiscard]] RouterManagerPtr routerManager() const;
        [[nodiscard]] [[nodiscard]] LocatorManagerPtr locatorManager() const;
        [[nodiscard]] [[nodiscard]] ReferenceFactoryPtr referenceFactory() const;
        [[nodiscard]] [[nodiscard]] OutgoingConnectionFactoryPtr outgoingConnectionFactory() const;
        [[nodiscard]] [[nodiscard]] ObjectAdapterFactoryPtr objectAdapterFactory() const;
        [[nodiscard]] [[nodiscard]] ProtocolSupport protocolSupport() const;
        [[nodiscard]] [[nodiscard]] bool preferIPv6() const;
        [[nodiscard]] [[nodiscard]] NetworkProxyPtr networkProxy() const;
        ThreadPoolPtr clientThreadPool();
        ThreadPoolPtr serverThreadPool();
        EndpointHostResolverPtr endpointHostResolver();
        RetryQueuePtr retryQueue();
        [[nodiscard]] [[nodiscard]] const std::vector<int>& retryIntervals() const { return _retryIntervals; }
        IceInternal::TimerPtr timer();
        [[nodiscard]] [[nodiscard]] EndpointFactoryManagerPtr endpointFactoryManager() const;
        [[nodiscard]] [[nodiscard]] Ice::PluginManagerPtr pluginManager() const;
        [[nodiscard]] [[nodiscard]] size_t messageSizeMax() const { return _messageSizeMax; }
        [[nodiscard]] [[nodiscard]] size_t batchAutoFlushSize() const { return _batchAutoFlushSize; }
        [[nodiscard]] [[nodiscard]] size_t classGraphDepthMax() const { return _classGraphDepthMax; }
        [[nodiscard]] [[nodiscard]] Ice::ToStringMode toStringMode() const { return _toStringMode; }
        [[nodiscard]] [[nodiscard]] bool acceptClassCycles() const { return _acceptClassCycles; }

        [[nodiscard]] [[nodiscard]] const Ice::ConnectionOptions& clientConnectionOptions() const noexcept { return _clientConnectionOptions; }
        [[nodiscard]] [[nodiscard]] Ice::ConnectionOptions serverConnectionOptions(const std::string& adapterName) const;

        Ice::ObjectPrx createAdmin(const Ice::ObjectAdapterPtr&, const Ice::Identity&);
        std::optional<Ice::ObjectPrx> getAdmin();
        void addAdminFacet(Ice::ObjectPtr, std::string);
        Ice::ObjectPtr removeAdminFacet(std::string_view);
        Ice::ObjectPtr findAdminFacet(std::string_view);
        Ice::FacetMap findAllAdminFacets();

        [[nodiscard]] [[nodiscard]] const Ice::ImplicitContextPtr& getImplicitContext() const;

        void setDefaultLocator(const std::optional<Ice::LocatorPrx>&);
        void setDefaultRouter(const std::optional<Ice::RouterPrx>&);

        void setLogger(const Ice::LoggerPtr&);
        void setThreadHook(std::function<void()>, std::function<void()>);

        [[nodiscard]] [[nodiscard]] const Ice::StringConverterPtr& getStringConverter() const { return _stringConverter; }
        [[nodiscard]] [[nodiscard]] const Ice::WstringConverterPtr& getWstringConverter() const { return _wstringConverter; }

        BufSizeWarnInfo getBufSizeWarn(std::int16_t type);
        void setSndBufSizeWarn(std::int16_t type, int size);
        void setRcvBufSizeWarn(std::int16_t type, int size);

        [[nodiscard]] [[nodiscard]] Ice::SSL::SSLEnginePtr sslEngine() const { return _sslEngine; }

    private:
        Instance(const Ice::InitializationData&);
        void initialize(const Ice::CommunicatorPtr&);
        void finishSetup(int&, const char*[], const Ice::CommunicatorPtr&);
        void destroy();

        friend class Ice::Communicator;

        void updateConnectionObservers();
        void updateThreadObservers();
        friend class ObserverUpdaterI;

        void addAllAdminFacets();
        void setServerProcessProxy(const Ice::ObjectAdapterPtr&, const Ice::Identity&);

        BufSizeWarnInfo getBufSizeWarnInternal(std::int16_t type);

        [[nodiscard]] [[nodiscard]] Ice::ConnectionOptions readConnectionOptions(const std::string& propertyPrefix) const;

        enum State
        {
            StateActive,
            StateDestroyInProgress,
            StateDestroyed
        };
        State _state;
        Ice::InitializationData _initData;
        const TraceLevelsPtr _traceLevels;                   // Immutable, not reset by destroy().
        const DefaultsAndOverridesPtr _defaultsAndOverrides; // Immutable, not reset by destroy().
        const size_t _messageSizeMax;                        // Immutable, not reset by destroy().
        const size_t _batchAutoFlushSize;                    // Immutable, not reset by destroy().
        const size_t _classGraphDepthMax;                    // Immutable, not reset by destroy().
        const Ice::ToStringMode _toStringMode;               // Immutable, not reset by destroy()
        const bool _acceptClassCycles;                       // Immutable, not reset by destroy()
        Ice::ConnectionOptions _clientConnectionOptions;
        Ice::ConnectionOptions _serverConnectionOptions;
        RouterManagerPtr _routerManager;
        LocatorManagerPtr _locatorManager;
        ReferenceFactoryPtr _referenceFactory;
        OutgoingConnectionFactoryPtr _outgoingConnectionFactory;
        ObjectAdapterFactoryPtr _objectAdapterFactory;
        ProtocolSupport _protocolSupport;
        bool _preferIPv6;
        NetworkProxyPtr _networkProxy;
        ThreadPoolPtr _clientThreadPool;
        ThreadPoolPtr _serverThreadPool;
        EndpointHostResolverPtr _endpointHostResolver;
        std::thread _endpointHostResolverThread;
        RetryQueuePtr _retryQueue;
        std::vector<int> _retryIntervals;
        ThreadObserverTimerPtr _timer;
        EndpointFactoryManagerPtr _endpointFactoryManager;
        Ice::PluginManagerPtr _pluginManager;
        const Ice::ImplicitContextPtr _implicitContext;
        Ice::StringConverterPtr _stringConverter;
        Ice::WstringConverterPtr _wstringConverter;
        bool _adminEnabled;
        Ice::ObjectAdapterPtr _adminAdapter;
        Ice::FacetMap _adminFacets;
        Ice::Identity _adminIdentity;
        std::set<std::string, std::less<>> _adminFacetFilter;
        IceInternal::MetricsAdminIPtr _metricsAdmin;
        std::map<std::int16_t, BufSizeWarnInfo> _setBufSizeWarn;
        std::mutex _setBufSizeWarnMutex;
        mutable std::recursive_mutex _mutex;
        std::condition_variable_any _conditionVariable;

        enum ImplicitContextKind
        {
            None,
            PerThread,
            Shared
        };
        ImplicitContextKind _implicitContextKind;
        // Only set when _implicitContextKind == Shared.
        Ice::ImplicitContextPtr _sharedImplicitContext;
        Ice::SSL::SSLEnginePtr _sslEngine;
    };

    class ProcessI final : public Ice::Process
    {
    public:
        ProcessI(const Ice::CommunicatorPtr&);

        void shutdown(const Ice::Current&) final;
        void writeMessage(std::string message, std::int32_t fd, const Ice::Current&) final;

    private:
        const Ice::CommunicatorPtr _communicator;
    };
}

#endif
