//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Instance.h"
#include "CheckIdentity.h"
#include "ConnectionFactory.h"
#include "ConsoleUtil.h"
#include "DefaultsAndOverrides.h"
#include "DisableWarnings.h"
#include "EndpointFactoryManager.h"
#include "FileUtil.h"
#include "IPEndpointI.h" // For EndpointHostResolver
#include "Ice/Communicator.h"
#include "Ice/Initialize.h"
#include "Ice/LocalExceptions.h"
#include "Ice/Locator.h"
#include "Ice/LoggerUtil.h"
#include "Ice/ObserverHelper.h"
#include "Ice/Properties.h"
#include "Ice/ProxyFunctions.h"
#include "Ice/Router.h"
#include "Ice/StringUtil.h"
#include "Ice/UUID.h"
#include "InstrumentationI.h"
#include "LocatorInfo.h"
#include "LoggerAdminI.h"
#include "LoggerI.h"
#include "NetworkProxy.h"
#include "ObjectAdapterFactory.h"
#include "PluginManagerI.h"
#include "PropertiesAdminI.h"
#include "ProtocolInstance.h"
#include "ReferenceFactory.h"
#include "RegisterPluginsInit.h"
#include "RetryQueue.h"
#include "RouterInfo.h"
#include "SSL/SSLEngine.h"
#include "ThreadPool.h"
#include "TimeUtil.h"
#include "TraceLevels.h"
#include "ValueFactoryManagerI.h"
#include "WSEndpoint.h"

#include <list>
#include <mutex>
#include <stdio.h>

#if defined(_WIN32)
#    include "SSL/SchannelEngine.h"
#elif defined(__APPLE__)
#    include "SSL/SecureTransportEngine.h"
#else
#    include "SSL/OpenSSLEngine.h"
#endif

#ifdef __APPLE__
#    include "OSLogLoggerI.h"
#endif

#ifndef _WIN32
#    include "SysLoggerI.h"
#    include "SystemdJournalI.h"

#    include <pwd.h>
#    include <signal.h>
#    include <sys/types.h>
#    include <syslog.h>
#endif

#if defined(__linux__) || defined(__GLIBC__)
#    include <grp.h> // for initgroups
#endif

using namespace std;
using namespace Ice;
using namespace IceInternal;

namespace IceInternal
{
    extern bool printStackTraces;
};

namespace
{
    mutex staticMutex;
    bool oneOfDone = false;
    std::list<IceInternal::Instance*>* instanceList = 0;

#ifndef _WIN32
    struct sigaction oldAction;
#endif
    bool printProcessIdDone = false;
    string identForOpenlog;

    //
    // Should be called with staticMutex locked
    //
    size_t instanceCount()
    {
        if (instanceList == 0)
        {
            return 0;
        }
        else
        {
            return instanceList->size();
        }
    }

    class Init
    {
    public:
        Init()
        {
            // Although probably not necessary here, we consistently lock
            // staticMutex before accessing instanceList.
            lock_guard lock(staticMutex);
            instanceList = new std::list<IceInternal::Instance*>;
        }

        ~Init()
        {
            {
                lock_guard lock(staticMutex);
                int notDestroyedCount = 0;

                for (std::list<IceInternal::Instance*>::const_iterator p = instanceList->begin();
                     p != instanceList->end();
                     ++p)
                {
                    if (!(*p)->destroyed())
                    {
                        notDestroyedCount++;
                    }
                }

                if (notDestroyedCount > 0)
                {
                    consoleErr << "!! " << timePointToDateTimeString(chrono::system_clock::now()) << " error: ";
                    if (notDestroyedCount == 1)
                    {
                        consoleErr << "communicator ";
                    }
                    else
                    {
                        consoleErr << notDestroyedCount << " communicators ";
                    }
                    consoleErr << "not destroyed during global destruction.";
                }

                delete instanceList;
                instanceList = 0;
            }
        }
    };

    Init init;

    //
    // Static initializer to register plugins.
    //
    IceInternal::RegisterPluginsInit initPlugins;
}

namespace IceInternal // Required because ObserverUpdaterI is a friend of Instance
{
    class ObserverUpdaterI : public Ice::Instrumentation::ObserverUpdater
    {
    public:
        ObserverUpdaterI(const InstancePtr&);

        virtual void updateConnectionObservers();
        virtual void updateThreadObservers();

    private:
        const InstancePtr _instance;
    };

    //
    // Timer specialization which supports the thread observer
    //
    class ThreadObserverTimer final : public Ice::Timer
    {
    public:
        ThreadObserverTimer() : _hasObserver(false) {}

        void updateObserver(const Ice::Instrumentation::CommunicatorObserverPtr&);

    private:
        void runTimerTask(const Ice::TimerTaskPtr&) final;

        std::mutex _mutex;
        std::atomic<bool> _hasObserver;
        ObserverHelperT<Ice::Instrumentation::ThreadObserver> _observer;
    };
}

void
ThreadObserverTimer::updateObserver(const Ice::Instrumentation::CommunicatorObserverPtr& obsv)
{
    lock_guard lock(_mutex);
    assert(obsv);
    _observer.attach(obsv->getThreadObserver(
        "Communicator",
        "Ice.Timer",
        Instrumentation::ThreadState::ThreadStateIdle,
        _observer.get()));
    _hasObserver.exchange(_observer.get() ? 1 : 0);
}

void
ThreadObserverTimer::runTimerTask(const Ice::TimerTaskPtr& task)
{
    if (_hasObserver)
    {
        Ice::Instrumentation::ThreadObserverPtr threadObserver;
        {
            lock_guard lock(_mutex);
            threadObserver = _observer.get();
        }
        if (threadObserver)
        {
            threadObserver->stateChanged(
                Instrumentation::ThreadState::ThreadStateIdle,
                Instrumentation::ThreadState::ThreadStateInUseForOther);
        }
        try
        {
            task->runTimerTask();
        }
        catch (...)
        {
            if (threadObserver)
            {
                threadObserver->stateChanged(
                    Instrumentation::ThreadState::ThreadStateInUseForOther,
                    Instrumentation::ThreadState::ThreadStateIdle);
            }
            throw;
        }

        if (threadObserver)
        {
            threadObserver->stateChanged(
                Instrumentation::ThreadState::ThreadStateInUseForOther,
                Instrumentation::ThreadState::ThreadStateIdle);
        }
    }
    else
    {
        task->runTimerTask();
    }
}

IceInternal::ObserverUpdaterI::ObserverUpdaterI(const InstancePtr& instance) : _instance(instance) {}

void
IceInternal::ObserverUpdaterI::updateConnectionObservers()
{
    _instance->updateConnectionObservers();
}

void
IceInternal::ObserverUpdaterI::updateThreadObservers()
{
    _instance->updateThreadObservers();
}

bool
IceInternal::Instance::destroyed() const
{
    lock_guard lock(_mutex);
    return _state == StateDestroyed;
}

TraceLevelsPtr
IceInternal::Instance::traceLevels() const
{
    // No mutex lock, immutable.
    assert(_traceLevels);
    return _traceLevels;
}

DefaultsAndOverridesPtr
IceInternal::Instance::defaultsAndOverrides() const
{
    // No mutex lock, immutable.
    assert(_defaultsAndOverrides);
    return _defaultsAndOverrides;
}

RouterManagerPtr
IceInternal::Instance::routerManager() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_routerManager);
    return _routerManager;
}

LocatorManagerPtr
IceInternal::Instance::locatorManager() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_locatorManager);
    return _locatorManager;
}

ReferenceFactoryPtr
IceInternal::Instance::referenceFactory() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_referenceFactory);
    return _referenceFactory;
}

OutgoingConnectionFactoryPtr
IceInternal::Instance::outgoingConnectionFactory() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_outgoingConnectionFactory);
    return _outgoingConnectionFactory;
}

ObjectAdapterFactoryPtr
IceInternal::Instance::objectAdapterFactory() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_objectAdapterFactory);
    return _objectAdapterFactory;
}

ProtocolSupport
IceInternal::Instance::protocolSupport() const
{
    return _protocolSupport;
}

bool
IceInternal::Instance::preferIPv6() const
{
    return _preferIPv6;
}

NetworkProxyPtr
IceInternal::Instance::networkProxy() const
{
    return _networkProxy;
}

ThreadPoolPtr
IceInternal::Instance::clientThreadPool()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_clientThreadPool);
    return _clientThreadPool;
}

ThreadPoolPtr
IceInternal::Instance::serverThreadPool()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    if (!_serverThreadPool) // Lazy initialization.
    {
        if (_state == StateDestroyInProgress)
        {
            throw CommunicatorDestroyedException(__FILE__, __LINE__);
        }
        int timeout = _initData.properties->getIcePropertyAsInt("Ice.ServerIdleTime");
        _serverThreadPool = ThreadPool::create(shared_from_this(), "Ice.ThreadPool.Server", timeout);
    }

    return _serverThreadPool;
}

EndpointHostResolverPtr
IceInternal::Instance::endpointHostResolver()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_endpointHostResolver);
    return _endpointHostResolver;
}

RetryQueuePtr
IceInternal::Instance::retryQueue()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_retryQueue);
    return _retryQueue;
}

Ice::TimerPtr
IceInternal::Instance::timer()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }
    assert(_timer);
    return _timer;
}

EndpointFactoryManagerPtr
IceInternal::Instance::endpointFactoryManager() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_endpointFactoryManager);
    return _endpointFactoryManager;
}

PluginManagerPtr
IceInternal::Instance::pluginManager() const
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    assert(_pluginManager);
    return _pluginManager;
}

ConnectionOptions
IceInternal::Instance::serverConnectionOptions(const string& adapterName) const
{
    assert(!adapterName.empty());
    string propertyPrefix = adapterName + ".Connection";

    const PropertiesPtr& properties = _initData.properties;

    ConnectionOptions connectionOptions;

    connectionOptions.connectTimeout = chrono::seconds(properties->getPropertyAsIntWithDefault(
        propertyPrefix + ".ConnectTimeout",
        static_cast<int>(_serverConnectionOptions.connectTimeout.count())));

    connectionOptions.closeTimeout = chrono::seconds(properties->getPropertyAsIntWithDefault(
        propertyPrefix + ".CloseTimeout",
        static_cast<int>(_serverConnectionOptions.closeTimeout.count())));

    connectionOptions.idleTimeout = chrono::seconds(properties->getPropertyAsIntWithDefault(
        propertyPrefix + ".IdleTimeout",
        static_cast<int>(_serverConnectionOptions.idleTimeout.count())));

    connectionOptions.enableIdleCheck = properties->getPropertyAsIntWithDefault(
                                            propertyPrefix + ".EnableIdleCheck",
                                            _serverConnectionOptions.enableIdleCheck ? 1 : 0) > 0;

    connectionOptions.inactivityTimeout = chrono::seconds(properties->getPropertyAsIntWithDefault(
        propertyPrefix + ".InactivityTimeout",
        static_cast<int>(_serverConnectionOptions.inactivityTimeout.count())));

    connectionOptions.maxDispatches = properties->getPropertyAsIntWithDefault(
        propertyPrefix + ".MaxDispatches",
        _serverConnectionOptions.maxDispatches);

    return connectionOptions;
}

Ice::ObjectPrx
IceInternal::Instance::createAdmin(const ObjectAdapterPtr& adminAdapter, const Identity& adminIdentity)
{
    ObjectAdapterPtr adapter = adminAdapter;
    bool createAdapter = !adminAdapter;

    unique_lock lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    checkIdentity(adminIdentity, __FILE__, __LINE__);

    if (_adminAdapter)
    {
        throw InitializationException(__FILE__, __LINE__, "Admin already created");
    }

    if (!_adminEnabled)
    {
        throw InitializationException(__FILE__, __LINE__, "Admin is disabled");
    }

    if (createAdapter)
    {
        if (_initData.properties->getIceProperty("Ice.Admin.Endpoints") != "")
        {
            adapter = _objectAdapterFactory->createObjectAdapter("Ice.Admin", nullopt, nullopt);
        }
        else
        {
            throw InitializationException(__FILE__, __LINE__, "Ice.Admin.Endpoints is not set");
        }
    }

    _adminIdentity = adminIdentity;
    _adminAdapter = adapter;
    addAllAdminFacets();
    lock.unlock();

    if (createAdapter)
    {
        try
        {
            adapter->activate();
        }
        catch (...)
        {
            //
            // We clean it up, even through this error is not recoverable
            // (can't call again createAdmin after fixing the problem since all the facets
            // in the adapter are lost)
            //
            adapter->destroy();
            lock.lock();
            _adminAdapter = nullptr;
            throw;
        }
    }
    setServerProcessProxy(adapter, adminIdentity);
    return adapter->createProxy(adminIdentity);
}

std::optional<ObjectPrx>
IceInternal::Instance::getAdmin()
{
    unique_lock lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    if (_adminAdapter)
    {
        return _adminAdapter->createProxy(_adminIdentity);
    }
    else if (_adminEnabled)
    {
        ObjectAdapterPtr adapter;
        if (_initData.properties->getIceProperty("Ice.Admin.Endpoints") != "")
        {
            adapter = _objectAdapterFactory->createObjectAdapter("Ice.Admin", nullopt, nullopt);
        }
        else
        {
            return nullopt;
        }

        Identity adminIdentity;
        adminIdentity.name = "admin";
        adminIdentity.category = _initData.properties->getIceProperty("Ice.Admin.InstanceName");
        if (adminIdentity.category.empty())
        {
            adminIdentity.category = Ice::generateUUID();
        }

        _adminIdentity = adminIdentity;
        _adminAdapter = adapter;
        addAllAdminFacets();
        lock.unlock();
        try
        {
            adapter->activate();
        }
        catch (...)
        {
            //
            // We clean it up, even through this error is not recoverable
            // (can't call again createAdmin after fixing the problem since all the facets
            // in the adapter are lost)
            //
            adapter->destroy();
            lock.lock();
            _adminAdapter = nullptr;
            throw;
        }

        setServerProcessProxy(adapter, adminIdentity);
        return adapter->createProxy(adminIdentity);
    }
    else
    {
        return nullopt;
    }
}

void
IceInternal::Instance::addAllAdminFacets()
{
    // must be called with this locked

    //
    // Add all facets to OA
    //
    FacetMap filteredFacets;

    for (FacetMap::iterator p = _adminFacets.begin(); p != _adminFacets.end(); ++p)
    {
        if (_adminFacetFilter.empty() || _adminFacetFilter.find(p->first) != _adminFacetFilter.end())
        {
            _adminAdapter->addFacet(p->second, _adminIdentity, p->first);
        }
        else
        {
            filteredFacets[p->first] = p->second;
        }
    }
    _adminFacets.swap(filteredFacets);
}

void
IceInternal::Instance::setServerProcessProxy(const ObjectAdapterPtr& adminAdapter, const Identity& adminIdentity)
{
    ObjectPrx admin = adminAdapter->createProxy(adminIdentity);
    optional<LocatorPrx> locator = adminAdapter->getLocator();
    const string serverId = _initData.properties->getIceProperty("Ice.Admin.ServerId");
    if (locator && serverId != "")
    {
        auto process = admin->ice_facet<ProcessPrx>("Process");
        try
        {
            //
            // Note that as soon as the process proxy is registered, the communicator might be
            // shutdown by a remote client and admin facets might start receiving calls.
            //
            locator->getRegistry()->setServerProcessProxy(serverId, process);
        }
        catch (const ServerNotFoundException&)
        {
            if (_traceLevels->location >= 1)
            {
                Trace out(_initData.logger, _traceLevels->locationCat);
                out << "couldn't register server `" + serverId + "' with the locator registry:\n";
                out << "the server is not known to the locator registry";
            }

            throw InitializationException(
                __FILE__,
                __LINE__,
                "Locator `" + locator->ice_toString() + "' knows nothing about server `" + serverId + "'");
        }
        catch (const LocalException& ex)
        {
            if (_traceLevels->location >= 1)
            {
                Trace out(_initData.logger, _traceLevels->locationCat);
                out << "couldn't register server `" + serverId + "' with the locator registry:\n" << ex;
            }
            throw;
        }

        if (_traceLevels->location >= 1)
        {
            Trace out(_initData.logger, _traceLevels->locationCat);
            out << "registered server `" + serverId + "' with the locator registry";
        }
    }
}

void
IceInternal::Instance::addAdminFacet(const ObjectPtr& servant, const string& facet)
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    if (_adminAdapter == 0 || (!_adminFacetFilter.empty() && _adminFacetFilter.find(facet) == _adminFacetFilter.end()))
    {
        if (_adminFacets.insert(FacetMap::value_type(facet, servant)).second == false)
        {
            throw AlreadyRegisteredException(__FILE__, __LINE__, "facet", facet);
        }
    }
    else
    {
        _adminAdapter->addFacet(servant, _adminIdentity, facet);
    }
}

ObjectPtr
IceInternal::Instance::removeAdminFacet(const string& facet)
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    ObjectPtr result;

    if (_adminAdapter == 0 || (!_adminFacetFilter.empty() && _adminFacetFilter.find(facet) == _adminFacetFilter.end()))
    {
        FacetMap::iterator p = _adminFacets.find(facet);
        if (p == _adminFacets.end())
        {
            throw NotRegisteredException(__FILE__, __LINE__, "facet", facet);
        }
        else
        {
            result = p->second;
            _adminFacets.erase(p);
        }
    }
    else
    {
        result = _adminAdapter->removeFacet(_adminIdentity, facet);
    }

    return result;
}

ObjectPtr
IceInternal::Instance::findAdminFacet(const string& facet)
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    ObjectPtr result;

    //
    // If the _adminAdapter was not yet created, or this facet is filtered out, we check _adminFacets
    //
    if (!_adminAdapter || (!_adminFacetFilter.empty() && _adminFacetFilter.find(facet) == _adminFacetFilter.end()))
    {
        FacetMap::iterator p = _adminFacets.find(facet);
        if (p != _adminFacets.end())
        {
            result = p->second;
        }
    }
    else
    {
        // Otherwise, just check the _adminAdapter
        result = _adminAdapter->findFacet(_adminIdentity, facet);
    }

    return result;
}

FacetMap
IceInternal::Instance::findAllAdminFacets()
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    if (!_adminAdapter)
    {
        return _adminFacets;
    }
    else
    {
        FacetMap result = _adminAdapter->findAllFacets(_adminIdentity);
        if (!_adminFacets.empty())
        {
            // Also returns filtered facets
            result.insert(_adminFacets.begin(), _adminFacets.end());
        }
        return result;
    }
}

void
IceInternal::Instance::setDefaultLocator(const optional<LocatorPrx>& defaultLocator)
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    _referenceFactory = _referenceFactory->setDefaultLocator(defaultLocator);
}

void
IceInternal::Instance::setDefaultRouter(const optional<RouterPrx>& defaultRouter)
{
    lock_guard lock(_mutex);

    if (_state == StateDestroyed)
    {
        throw CommunicatorDestroyedException(__FILE__, __LINE__);
    }

    _referenceFactory = _referenceFactory->setDefaultRouter(defaultRouter);
}

void
IceInternal::Instance::setLogger(const Ice::LoggerPtr& logger)
{
    //
    // No locking, as it can only be called during plug-in loading
    //
    _initData.logger = logger;
}

void
IceInternal::Instance::setThreadHook(function<void()> threadStart, function<void()> threadStop)
{
    _initData.threadStart = std::move(threadStart);
    _initData.threadStop = std::move(threadStop);
}
namespace
{
    bool logStdErrConvert = true;
}

InstancePtr
IceInternal::Instance::create(const Ice::CommunicatorPtr& communicator, const Ice::InitializationData& initData)
{
    auto instance = shared_ptr<Instance>(new Instance(initData));
    instance->initialize(communicator);
    return instance;
}

IceInternal::Instance::Instance(const InitializationData& initData)
    : _state(StateActive),
      _initData(initData),
      _messageSizeMax(0),
      _batchAutoFlushSize(0),
      _classGraphDepthMax(0),
      _toStringMode(ToStringMode::Unicode),
      _acceptClassCycles(false),
      _stringConverter(Ice::getProcessStringConverter()),
      _wstringConverter(Ice::getProcessWstringConverter()),
      _adminEnabled(false)
{
#if defined(ICE_USE_SCHANNEL)
    if (_initData.clientAuthenticationOptions && _initData.clientAuthenticationOptions->trustedRootCertificates)
    {
        CertDuplicateStore(_initData.clientAuthenticationOptions->trustedRootCertificates);
    }
#elif defined(ICE_USE_SECURE_TRANSPORT)
    if (_initData.clientAuthenticationOptions && _initData.clientAuthenticationOptions->trustedRootCertificates)
    {
        CFRetain(_initData.clientAuthenticationOptions->trustedRootCertificates);
    }
#endif
}

void
IceInternal::Instance::initialize(const Ice::CommunicatorPtr& communicator)
{
    try
    {
        {
            lock_guard lock(staticMutex);
            instanceList->push_back(this);

            if (!_initData.properties)
            {
                _initData.properties = createProperties();
            }

            if (!oneOfDone)
            {
                //
                // StdOut and StdErr redirection
                //
                string stdOutFilename = _initData.properties->getIceProperty("Ice.StdOut");
                string stdErrFilename = _initData.properties->getIceProperty("Ice.StdErr");

                if (stdOutFilename != "")
                {
                    FILE* file = IceInternal::freopen(stdOutFilename, "a", stdout);
                    if (file == 0)
                    {
                        throw FileException(__FILE__, __LINE__, stdOutFilename);
                    }
                }

                if (stdErrFilename != "")
                {
                    FILE* file = IceInternal::freopen(stdErrFilename, "a", stderr);
                    if (file == 0)
                    {
                        throw FileException(__FILE__, __LINE__, stdErrFilename);
                    }
                }

#ifdef NDEBUG
                if (_initData.properties->getIcePropertyAsInt("Ice.PrintStackTraces") > 0)
                {
                    IceInternal::printStackTraces = true;
                }
#else
                if (_initData.properties->getPropertyAsIntWithDefault("Ice.PrintStackTraces", 1) == 0)
                {
                    IceInternal::printStackTraces = false;
                }
#endif
                oneOfDone = true;
            }

            if (instanceCount() == 1)
            {
#if defined(_WIN32)
                WORD version = MAKEWORD(1, 1);
                WSADATA data;
                if (WSAStartup(version, &data) != 0)
                {
                    throw SocketException(__FILE__, __LINE__, getSocketErrno());
                }
#endif

#ifndef _WIN32
                struct sigaction action;
                action.sa_handler = SIG_IGN;
                sigemptyset(&action.sa_mask);
                action.sa_flags = 0;
                sigaction(SIGPIPE, &action, &oldAction);
                if (_initData.properties->getIcePropertyAsInt("Ice.UseSyslog") > 0)
                {
                    identForOpenlog = _initData.properties->getIceProperty("Ice.ProgramName");
                    if (identForOpenlog.empty())
                    {
                        identForOpenlog = "<Unknown Ice Program>";
                    }
                    openlog(identForOpenlog.c_str(), LOG_PID, LOG_USER);
                }
#else
                logStdErrConvert = _initData.properties->getIcePropertyAsInt("Ice.LogStdErr.Convert") > 0 &&
                                   _initData.properties->getIceProperty("Ice.StdErr").empty();
#endif
            }
        }

        if (!_initData.logger)
        {
            string logfile = _initData.properties->getIceProperty("Ice.LogFile");
#ifndef _WIN32
            if (_initData.properties->getIcePropertyAsInt("Ice.UseSyslog") > 0)
            {
                if (!logfile.empty())
                {
                    throw InitializationException(__FILE__, __LINE__, "Both syslog and file logger cannot be enabled.");
                }

                _initData.logger = make_shared<SysLoggerI>(
                    _initData.properties->getIceProperty("Ice.ProgramName"),
                    _initData.properties->getIceProperty("Ice.SyslogFacility"));
            }
            else
#endif

#ifdef __APPLE__
                if (!_initData.logger && _initData.properties->getIcePropertyAsInt("Ice.UseOSLog") > 0)
            {
                _initData.logger = make_shared<OSLogLoggerI>(_initData.properties->getIceProperty("Ice.ProgramName"));
            }
            else
#endif

#ifdef ICE_USE_SYSTEMD
                if (_initData.properties->getIcePropertyAsInt("Ice.UseSystemdJournal") > 0)
            {
                _initData.logger =
                    make_shared<SystemdJournalI>(_initData.properties->getIceProperty("Ice.ProgramName"));
            }
            else
#endif
                if (!logfile.empty())
            {
                int32_t sz = _initData.properties->getIcePropertyAsInt("Ice.LogFile.SizeMax");
                if (sz < 0)
                {
                    sz = 0;
                }
                _initData.logger = make_shared<LoggerI>(
                    _initData.properties->getIceProperty("Ice.ProgramName"),
                    logfile,
                    true,
                    static_cast<size_t>(sz));
            }
            else
            {
                _initData.logger = getProcessLogger();
                if (dynamic_pointer_cast<LoggerI>(_initData.logger))
                {
                    _initData.logger = make_shared<LoggerI>(
                        _initData.properties->getIceProperty("Ice.ProgramName"),
                        "",
                        logStdErrConvert);
                }
            }
        }

        const_cast<TraceLevelsPtr&>(_traceLevels) = make_shared<TraceLevels>(_initData.properties);

        const_cast<DefaultsAndOverridesPtr&>(_defaultsAndOverrides) =
            make_shared<DefaultsAndOverrides>(_initData.properties, _initData.logger);

        const_cast<ConnectionOptions&>(_clientConnectionOptions) = readConnectionOptions("Ice.Connection.Client");
        const_cast<ConnectionOptions&>(_serverConnectionOptions) = readConnectionOptions("Ice.Connection.Server");

        {
            int32_t num = _initData.properties->getIcePropertyAsInt("Ice.MessageSizeMax");
            if (num < 1 || static_cast<size_t>(num) > static_cast<size_t>(0x7fffffff / 1024))
            {
                const_cast<size_t&>(_messageSizeMax) = static_cast<size_t>(0x7fffffff);
            }
            else
            {
                // Property is in kilobytes, _messageSizeMax in bytes.
                const_cast<size_t&>(_messageSizeMax) = static_cast<size_t>(num) * 1024;
            }
        }

        if (_initData.properties->getIceProperty("Ice.BatchAutoFlushSize").empty() &&
            !_initData.properties->getIceProperty("Ice.BatchAutoFlush").empty())
        {
            if (_initData.properties->getIcePropertyAsInt("Ice.BatchAutoFlush") > 0)
            {
                const_cast<size_t&>(_batchAutoFlushSize) = _messageSizeMax;
            }
        }
        else
        {
            int32_t num = _initData.properties->getIcePropertyAsInt("Ice.BatchAutoFlushSize"); // 1MB default
            if (num < 1)
            {
                const_cast<size_t&>(_batchAutoFlushSize) = static_cast<size_t>(num);
            }
            else if (static_cast<size_t>(num) > static_cast<size_t>(0x7fffffff / 1024))
            {
                const_cast<size_t&>(_batchAutoFlushSize) = static_cast<size_t>(0x7fffffff);
            }
            else
            {
                // Property is in kilobytes, convert in bytes.
                const_cast<size_t&>(_batchAutoFlushSize) = static_cast<size_t>(num) * 1024;
            }
        }

        {
            int32_t num = _initData.properties->getIcePropertyAsInt("Ice.ClassGraphDepthMax");
            if (num < 1 || static_cast<size_t>(num) > static_cast<size_t>(0x7fffffff))
            {
                const_cast<size_t&>(_classGraphDepthMax) = static_cast<size_t>(0x7fffffff);
            }
            else
            {
                const_cast<size_t&>(_classGraphDepthMax) = static_cast<size_t>(num);
            }
        }

        string toStringModeStr = _initData.properties->getIceProperty("Ice.ToStringMode");
        if (toStringModeStr == "ASCII")
        {
            const_cast<ToStringMode&>(_toStringMode) = ToStringMode::ASCII;
        }
        else if (toStringModeStr == "Compat")
        {
            const_cast<ToStringMode&>(_toStringMode) = ToStringMode::Compat;
        }
        else if (toStringModeStr != "Unicode")
        {
            throw InitializationException(
                __FILE__,
                __LINE__,
                "The value for Ice.ToStringMode must be Unicode, ASCII or Compat");
        }

        const_cast<bool&>(_acceptClassCycles) = _initData.properties->getIcePropertyAsInt("Ice.AcceptClassCycles") > 0;

        string implicitContextKind = _initData.properties->getIceProperty("Ice.ImplicitContext");
        if (implicitContextKind == "Shared")
        {
            _implicitContextKind = ImplicitContextKind::Shared;
            _sharedImplicitContext = std::make_shared<ImplicitContext>();
        }
        else if (implicitContextKind == "PerThread")
        {
            _implicitContextKind = ImplicitContextKind::PerThread;
        }
        else if (implicitContextKind == "None")
        {
            _implicitContextKind = ImplicitContextKind::None;
        }
        else
        {
            throw Ice::InitializationException(
                __FILE__,
                __LINE__,
                "'" + implicitContextKind + "' is not a valid value for Ice.ImplicitContext");
        }

        _routerManager = make_shared<RouterManager>();

        _locatorManager = make_shared<LocatorManager>(_initData.properties);

        _referenceFactory = make_shared<ReferenceFactory>(shared_from_this(), communicator);

        const bool isIPv6Supported = IceInternal::isIPv6Supported();
        const bool ipv4 = _initData.properties->getIcePropertyAsInt("Ice.IPv4") > 0;
        const bool ipv6 = isIPv6Supported ? (_initData.properties->getIcePropertyAsInt("Ice.IPv6") > 0) : false;
        if (!ipv4 && !ipv6)
        {
            throw InitializationException(__FILE__, __LINE__, "Both IPV4 and IPv6 support cannot be disabled.");
        }
        else if (ipv4 && ipv6)
        {
            _protocolSupport = EnableBoth;
        }
        else if (ipv4)
        {
            _protocolSupport = EnableIPv4;
        }
        else
        {
            _protocolSupport = EnableIPv6;
        }
        _preferIPv6 = _initData.properties->getIcePropertyAsInt("Ice.PreferIPv6Address") > 0;

        _networkProxy = IceInternal::createNetworkProxy(_initData.properties, _protocolSupport);

        _endpointFactoryManager = make_shared<EndpointFactoryManager>(shared_from_this());

        _pluginManager = make_shared<PluginManagerI>(communicator);

        if (!_initData.valueFactoryManager)
        {
            _initData.valueFactoryManager = make_shared<ValueFactoryManagerI>();
        }

        _outgoingConnectionFactory = make_shared<OutgoingConnectionFactory>(communicator, shared_from_this());

        _objectAdapterFactory = make_shared<ObjectAdapterFactory>(shared_from_this(), communicator);

        _retryQueue = make_shared<RetryQueue>(shared_from_this());

        StringSeq retryValues = _initData.properties->getIcePropertyAsList("Ice.RetryIntervals");
        if (retryValues.size() == 0)
        {
            _retryIntervals.push_back(0);
        }
        else
        {
            for (StringSeq::const_iterator p = retryValues.begin(); p != retryValues.end(); ++p)
            {
                istringstream value(*p);

                int v;
                if (!(value >> v) || !value.eof())
                {
                    v = 0;
                }

                //
                // If -1 is the first value, no retry and wait intervals.
                //
                if (v == -1 && _retryIntervals.empty())
                {
                    break;
                }

                _retryIntervals.push_back(v > 0 ? v : 0);
            }
        }

#if defined(_WIN32)
        _sslEngine = make_shared<Ice::SSL::Schannel::SSLEngine>(shared_from_this());
#elif defined(__APPLE__)
        _sslEngine = make_shared<Ice::SSL::SecureTransport::SSLEngine>(shared_from_this());
#else
        _sslEngine = make_shared<Ice::SSL::OpenSSL::SSLEngine>(shared_from_this());
#endif
        _sslEngine->initialize();
    }
    catch (...)
    {
        {
            lock_guard lock(staticMutex);
            instanceList->remove(this);
        }
        destroy();
        throw;
    }
}

const Ice::ImplicitContextPtr&
IceInternal::Instance::getImplicitContext() const
{
    switch (_implicitContextKind)
    {
        case ImplicitContextKind::PerThread:
        {
            static thread_local std::map<const IceInternal::Instance*, ImplicitContextPtr> perThreadImplicitContextMap;
            auto it = perThreadImplicitContextMap.find(this);
            if (it == perThreadImplicitContextMap.end())
            {
                auto r = perThreadImplicitContextMap.emplace(make_pair(this, std::make_shared<ImplicitContext>()));
                return r.first->second;
            }
            else
            {
                return it->second;
            }
        }
        case ImplicitContextKind::Shared:
        {
            assert(_sharedImplicitContext);
            return _sharedImplicitContext;
        }
        default:
        {
            assert(_sharedImplicitContext == nullptr);
            assert(_implicitContextKind == ImplicitContextKind::None);
            return _sharedImplicitContext;
        }
    }
}

IceInternal::Instance::~Instance()
{
    assert(_state == StateDestroyed);
    assert(!_referenceFactory);
    assert(!_outgoingConnectionFactory);

    assert(!_objectAdapterFactory);
    assert(!_clientThreadPool);
    assert(!_serverThreadPool);
    assert(!_endpointHostResolver);
    assert(!_endpointHostResolverThread.joinable());
    assert(!_retryQueue);
    assert(!_timer);
    assert(!_routerManager);
    assert(!_locatorManager);
    assert(!_endpointFactoryManager);
    assert(!_pluginManager);

    lock_guard lock(staticMutex);
    if (instanceList != 0)
    {
        instanceList->remove(this);
    }
    if (instanceCount() == 0)
    {
#if defined(_WIN32)
        WSACleanup();
#endif

#ifndef _WIN32
        sigaction(SIGPIPE, &oldAction, 0);

        if (!identForOpenlog.empty())
        {
            closelog();
            identForOpenlog.clear();
        }
#endif
    }
}

void
IceInternal::Instance::finishSetup(int& argc, const char* argv[], const Ice::CommunicatorPtr& communicator)
{
    //
    // Load plug-ins.
    //
    assert(!_serverThreadPool);
    PluginManagerI* pluginManagerImpl = dynamic_cast<PluginManagerI*>(_pluginManager.get());
    assert(pluginManagerImpl);
    pluginManagerImpl->loadPlugins(argc, argv);

    //
    // Initialize the endpoint factories once all the plugins are loaded. This gives
    // the opportunity for the endpoint factories to find underlying factories.
    //
    _endpointFactoryManager->initialize();

    //
    // Reset _stringConverter and _wstringConverter, in case a plugin changed them
    //
    _stringConverter = Ice::getProcessStringConverter();
    _wstringConverter = Ice::getProcessWstringConverter();

    //
    // Create Admin facets, if enabled.
    //
    // Note that any logger-dependent admin facet must be created after we load all plugins,
    // since one of these plugins can be a Logger plugin that sets a new logger during loading
    //

    if (_initData.properties->getIceProperty("Ice.Admin.Enabled") == "")
    {
        _adminEnabled = _initData.properties->getIceProperty("Ice.Admin.Endpoints") != "";
    }
    else
    {
        _adminEnabled = _initData.properties->getIcePropertyAsInt("Ice.Admin.Enabled") > 0;
    }

    StringSeq facetSeq = _initData.properties->getIcePropertyAsList("Ice.Admin.Facets");
    if (!facetSeq.empty())
    {
        _adminFacetFilter.insert(facetSeq.begin(), facetSeq.end());
    }

    if (_adminEnabled)
    {
        //
        // Process facet
        //
        const string processFacetName = "Process";
        if (_adminFacetFilter.empty() || _adminFacetFilter.find(processFacetName) != _adminFacetFilter.end())
        {
            _adminFacets.insert(make_pair(processFacetName, make_shared<ProcessI>(communicator)));
        }

        //
        // Logger facet
        //
        const string loggerFacetName = "Logger";
        if (_adminFacetFilter.empty() || _adminFacetFilter.find(loggerFacetName) != _adminFacetFilter.end())
        {
            LoggerAdminLoggerPtr logger = createLoggerAdminLogger(_initData.properties, _initData.logger);
            setLogger(logger);
            _adminFacets.insert(make_pair(loggerFacetName, logger->getFacet()));
        }

        //
        // Properties facet
        //
        const string propertiesFacetName = "Properties";
        PropertiesAdminIPtr propsAdmin;
        if (_adminFacetFilter.empty() || _adminFacetFilter.find(propertiesFacetName) != _adminFacetFilter.end())
        {
            propsAdmin = make_shared<PropertiesAdminI>(shared_from_this());
            _adminFacets.insert(make_pair(propertiesFacetName, propsAdmin));
        }

        //
        // Metrics facet
        //
        const string metricsFacetName = "Metrics";
        if (_adminFacetFilter.empty() || _adminFacetFilter.find(metricsFacetName) != _adminFacetFilter.end())
        {
            CommunicatorObserverIPtr observer = make_shared<CommunicatorObserverI>(_initData);
            _initData.observer = observer;
            _adminFacets.insert(make_pair(metricsFacetName, observer->getFacet()));

            //
            // Make sure the metrics admin facet receives property updates.
            //
            if (propsAdmin)
            {
                auto metricsAdmin = observer->getFacet();
                propsAdmin->addUpdateCallback([metricsAdmin](const PropertyDict& changes)
                                              { metricsAdmin->updated(changes); });
            }
        }
    }

    //
    // Set observer updater
    //
    if (_initData.observer)
    {
        _initData.observer->setObserverUpdater(make_shared<ObserverUpdaterI>(shared_from_this()));
    }

    //
    // Create threads.
    //
    try
    {
        _timer = make_shared<ThreadObserverTimer>();
    }
    catch (const Ice::Exception& ex)
    {
        Error out(_initData.logger);
        out << "cannot create thread for timer:\n" << ex;
        throw;
    }

    try
    {
        _endpointHostResolver = make_shared<EndpointHostResolver>(shared_from_this());
        _endpointHostResolverThread = std::thread([this] { _endpointHostResolver->run(); });
    }
    catch (const Ice::Exception& ex)
    {
        Error out(_initData.logger);
        out << "cannot create thread for endpoint host resolver:\n" << ex;
        throw;
    }

    _clientThreadPool = ThreadPool::create(shared_from_this(), "Ice.ThreadPool.Client", 0);

    //
    // The default router/locator may have been set during the loading of plugins.
    // Therefore we make sure it is not already set before checking the property.
    //
    if (!_referenceFactory->getDefaultRouter())
    {
        auto router = communicator->propertyToProxy<RouterPrx>("Ice.Default.Router");
        if (router)
        {
            _referenceFactory = _referenceFactory->setDefaultRouter(router);
        }
    }

    if (!_referenceFactory->getDefaultLocator())
    {
        auto locator = communicator->propertyToProxy<LocatorPrx>("Ice.Default.Locator");
        if (locator)
        {
            _referenceFactory = _referenceFactory->setDefaultLocator(locator);
        }
    }

    //
    // Show process id if requested (but only once).
    //
    bool printProcessId = false;
    if (!printProcessIdDone && _initData.properties->getIcePropertyAsInt("Ice.PrintProcessId") > 0)
    {
        //
        // Safe double-check locking (no dependent variable!)
        //
        lock_guard lock(staticMutex);
        printProcessId = !printProcessIdDone;

        //
        // We anticipate: we want to print it once, and we don't care when.
        //
        printProcessIdDone = true;
    }

    if (printProcessId)
    {
#ifdef _MSC_VER
        consoleOut << GetCurrentProcessId() << endl;
#else
        consoleOut << getpid() << endl;
#endif
    }

    //
    // Server thread pool initialization is lazy in serverThreadPool().
    //

    //
    // An application can set Ice.InitPlugins=0 if it wants to postpone
    // initialization until after it has interacted directly with the
    // plug-ins.
    //
    if (_initData.properties->getIcePropertyAsInt("Ice.InitPlugins") > 0)
    {
        pluginManagerImpl->initializePlugins();
    }

    //
    // This must be done last as this call creates the Ice.Admin object adapter
    // and eventually register a process proxy with the Ice locator (allowing
    // remote clients to invoke Admin facets as soon as it's registered).
    //
    // Note: getAdmin here can return 0 and do nothing in the event the
    // application set Ice.Admin.Enabled but did not set Ice.Admin.Endpoints
    // and one or more of the properties required to create the Admin object.
    //
    if (_adminEnabled && _initData.properties->getIcePropertyAsInt("Ice.Admin.DelayCreation") <= 0)
    {
        getAdmin();
    }
}

void
IceInternal::Instance::destroy()
{
    {
        unique_lock lock(_mutex);

        //
        // If destroy is in progress, wait for it to be done. This is
        // necessary in case destroy() is called concurrently by
        // multiple threads.
        //
        _conditionVariable.wait(lock, [this] { return _state != StateDestroyInProgress; });

        if (_state == StateDestroyed)
        {
            return;
        }
        _state = StateDestroyInProgress;
    }

    //
    // Shutdown and destroy all the incoming and outgoing Ice
    // connections and wait for the connections to be finished.
    //
    if (_objectAdapterFactory)
    {
        _objectAdapterFactory->shutdown();
    }

    if (_outgoingConnectionFactory)
    {
        _outgoingConnectionFactory->destroy();
    }

    if (_objectAdapterFactory)
    {
        _objectAdapterFactory->destroy();
    }

    if (_outgoingConnectionFactory)
    {
        _outgoingConnectionFactory->waitUntilFinished();
    }

    if (_retryQueue)
    {
        _retryQueue->destroy(); // Must be called before destroying thread pools.
    }

    if (_initData.observer)
    {
        CommunicatorObserverIPtr observer = dynamic_pointer_cast<CommunicatorObserverI>(_initData.observer);
        if (observer)
        {
            observer->destroy(); // Break cyclic reference counts. Don't clear _observer, it's immutable.
        }
        _initData.observer->setObserverUpdater(0); // Break cyclic reference count.
    }

#if defined(ICE_USE_SCHANNEL)
    if (_initData.clientAuthenticationOptions && _initData.clientAuthenticationOptions->trustedRootCertificates)
    {
        CertCloseStore(_initData.clientAuthenticationOptions->trustedRootCertificates, 0);
        const_cast<Ice::InitializationData&>(_initData).clientAuthenticationOptions = nullopt;
    }
#elif defined(ICE_USE_SECURE_TRANSPORT)
    if (_initData.clientAuthenticationOptions && _initData.clientAuthenticationOptions->trustedRootCertificates)
    {
        CFRelease(_initData.clientAuthenticationOptions->trustedRootCertificates);
        const_cast<Ice::InitializationData&>(_initData).clientAuthenticationOptions = nullopt;
    }
#endif

    LoggerAdminLoggerPtr logger = dynamic_pointer_cast<LoggerAdminLogger>(_initData.logger);
    if (logger)
    {
        //
        // This only disables the remote logging; we don't set or reset _initData.logger
        //
        logger->destroy();
    }

    //
    // Now, destroy the thread pools. This must be done *only* after
    // all the connections are finished (the connections destruction
    // can require invoking callbacks with the thread pools).
    //
    if (_serverThreadPool)
    {
        _serverThreadPool->destroy();
    }
    if (_clientThreadPool)
    {
        _clientThreadPool->destroy();
    }
    if (_endpointHostResolver)
    {
        _endpointHostResolver->destroy();
    }
    if (_timer)
    {
        _timer->destroy();
    }

    //
    // Wait for all the threads to be finished.
    //
    if (_clientThreadPool)
    {
        _clientThreadPool->joinWithAllThreads();
    }
    if (_serverThreadPool)
    {
        _serverThreadPool->joinWithAllThreads();
    }
    if (_endpointHostResolverThread.joinable())
    {
        _endpointHostResolverThread.join();
    }

    if (_routerManager)
    {
        _routerManager->destroy();
    }

    if (_locatorManager)
    {
        _locatorManager->destroy();
    }

    if (_endpointFactoryManager)
    {
        _endpointFactoryManager->destroy();
    }

    if (_initData.properties->getIcePropertyAsInt("Ice.Warn.UnusedProperties") > 0)
    {
        set<string> unusedProperties = _initData.properties.get()->getUnusedProperties();
        if (unusedProperties.size() != 0)
        {
            Warning out(_initData.logger);
            out << "The following properties were set but never read:";
            for (set<string>::const_iterator p = unusedProperties.begin(); p != unusedProperties.end(); ++p)
            {
                out << "\n    " << *p;
            }
        }
    }

    //
    // Destroy last so that a Logger plugin can receive all log/traces before its destruction.
    //
    if (_pluginManager)
    {
        _pluginManager->destroy();
    }

    {
        lock_guard lock(_mutex);

        _objectAdapterFactory = nullptr;
        _outgoingConnectionFactory = nullptr;
        _retryQueue = nullptr;

        _serverThreadPool = nullptr;
        _clientThreadPool = nullptr;
        _endpointHostResolver = nullptr;
        _timer = nullptr;

        _referenceFactory = nullptr;
        _routerManager = nullptr;
        _locatorManager = nullptr;
        _endpointFactoryManager = nullptr;
        _pluginManager = nullptr;

        _adminAdapter = nullptr;
        _adminFacets.clear();

        _sslEngine = nullptr;

        _state = StateDestroyed;
        _conditionVariable.notify_all();
    }
}

void
IceInternal::Instance::updateConnectionObservers()
{
    try
    {
        assert(_outgoingConnectionFactory);
        _outgoingConnectionFactory->updateConnectionObservers();
        assert(_objectAdapterFactory);
        _objectAdapterFactory->updateObservers(&ObjectAdapterI::updateConnectionObservers);
    }
    catch (const Ice::CommunicatorDestroyedException&)
    {
    }
}

void
IceInternal::Instance::updateThreadObservers()
{
    try
    {
        if (_clientThreadPool)
        {
            _clientThreadPool->updateObservers();
        }
        if (_serverThreadPool)
        {
            _serverThreadPool->updateObservers();
        }
        assert(_objectAdapterFactory);
        _objectAdapterFactory->updateObservers(&ObjectAdapterI::updateThreadObservers);
        if (_endpointHostResolver)
        {
            _endpointHostResolver->updateObserver();
        }
        if (_timer)
        {
            _timer->updateObserver(_initData.observer);
        }
    }
    catch (const Ice::CommunicatorDestroyedException&)
    {
    }
}

BufSizeWarnInfo
IceInternal::Instance::getBufSizeWarn(int16_t type)
{
    lock_guard lock(_setBufSizeWarnMutex);

    return getBufSizeWarnInternal(type);
}

BufSizeWarnInfo
IceInternal::Instance::getBufSizeWarnInternal(int16_t type)
{
    BufSizeWarnInfo info;
    map<int16_t, BufSizeWarnInfo>::iterator p = _setBufSizeWarn.find(type);
    if (p == _setBufSizeWarn.end())
    {
        info.sndWarn = false;
        info.sndSize = -1;
        info.rcvWarn = false;
        info.rcvSize = -1;
        _setBufSizeWarn.insert(make_pair(type, info));
    }
    else
    {
        info = p->second;
    }
    return info;
}

ConnectionOptions
IceInternal::Instance::readConnectionOptions(const string& propertyPrefix) const
{
    const PropertiesPtr& properties = _initData.properties;
    ConnectionOptions connectionOptions;

    connectionOptions.connectTimeout =
        chrono::seconds(properties->getIcePropertyAsInt(propertyPrefix + ".ConnectTimeout"));

    connectionOptions.closeTimeout = chrono::seconds(properties->getIcePropertyAsInt(propertyPrefix + ".CloseTimeout"));
    connectionOptions.idleTimeout = chrono::seconds(properties->getIcePropertyAsInt(propertyPrefix + ".IdleTimeout"));
    connectionOptions.enableIdleCheck = properties->getIcePropertyAsInt(propertyPrefix + ".EnableIdleCheck") > 0;

    connectionOptions.inactivityTimeout =
        chrono::seconds(properties->getIcePropertyAsInt(propertyPrefix + ".InactivityTimeout"));

    connectionOptions.maxDispatches = properties->getIcePropertyAsInt(propertyPrefix + ".MaxDispatches");

    return connectionOptions;
}

void
IceInternal::Instance::setSndBufSizeWarn(int16_t type, int size)
{
    lock_guard lock(_setBufSizeWarnMutex);

    BufSizeWarnInfo info = getBufSizeWarnInternal(type);
    info.sndWarn = true;
    info.sndSize = size;
    _setBufSizeWarn[type] = info;
}

void
IceInternal::Instance::setRcvBufSizeWarn(int16_t type, int size)
{
    lock_guard lock(_setBufSizeWarnMutex);

    BufSizeWarnInfo info = getBufSizeWarnInternal(type);
    info.rcvWarn = true;
    info.rcvSize = size;
    _setBufSizeWarn[type] = info;
}

IceInternal::ProcessI::ProcessI(const CommunicatorPtr& communicator) : _communicator(communicator) {}

void
IceInternal::ProcessI::shutdown(const Current&)
{
    _communicator->shutdown();
}

void
IceInternal::ProcessI::writeMessage(string message, int32_t fd, const Current&)
{
    switch (fd)
    {
        case 1:
        {
            consoleOut << message << endl;
            break;
        }
        case 2:
        {
            consoleErr << message << endl;
            break;
        }
    }
}
