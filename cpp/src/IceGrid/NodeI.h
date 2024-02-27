//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_GRID_NODE_I_H
#define ICE_GRID_NODE_I_H

#include <IceUtil/Timer.h>
#include <IcePatch2/FileServer.h>
#include <IceGrid/Internal.h>
#include <IceGrid/PlatformInfo.h>
#include <IceGrid/UserAccountMapper.h>
#include <IceGrid/FileCache.h>
#include <set>
#include <exception>

namespace IceGrid
{

class Activator;
class NodeSessionManager;
class ServerCommand;
class ServerI;
class TraceLevels;

class NodeI : public Node, public std::enable_shared_from_this<NodeI>
{
public:

    class Update final : public std::enable_shared_from_this<Update>
    {
    public:

        using UpdateFunction = std::function<void(std::function<void()>, std::function<void(std::exception_ptr)>)>;

        Update(UpdateFunction, const std::shared_ptr<NodeI>&, const NodeObserverPrxPtr&);
        bool send();

    private:

        UpdateFunction _func;
        std::shared_ptr<NodeI> _node;
        NodeObserverPrxPtr _observer;
    };

    NodeI(const std::shared_ptr<Ice::ObjectAdapter>&, NodeSessionManager&, const std::shared_ptr<Activator>&,
          const IceUtil::TimerPtr&, const std::shared_ptr<TraceLevels>&, const NodePrxPtr&,
          const std::string&, const UserAccountMapperPrxPtr&, const std::string&);

    void loadServerAsync(std::shared_ptr<InternalServerDescriptor>, std::string,
                         std::function<void(const ServerPrxPtr &, const AdapterPrxDict &, int, int)>,
                         std::function<void(std::exception_ptr)>,
                         const Ice::Current &) override;

    void loadServerWithoutRestartAsync(std::shared_ptr<InternalServerDescriptor>, std::string,
                                       std::function<void(const ServerPrxPtr&,
                                                          const AdapterPrxDict&, int, int)>,
                                       std::function<void(std::exception_ptr)>,
                                       const Ice::Current &) override;

    void destroyServerAsync(std::string, std::string, int, std::string,
                            std::function<void()>,
                            std::function<void(std::exception_ptr)>,
                            const Ice::Current&) override;

    void destroyServerWithoutRestartAsync(std::string, std::string, int, std::string,
                                          std::function<void()>,
                                          std::function<void(std::exception_ptr)>,
                                          const ::Ice::Current& current) override;

    void patchAsync(
        PatcherFeedbackPrxPtr feedback,
        std::string application,
        std::string server,
        std::shared_ptr<InternalDistributionDescriptor> appDistrib,
        bool shutdown,
        std::function<void()> response,
        std::function<void(std::exception_ptr)> exception,
        const Ice::Current&) override;

    void registerWithReplica(InternalRegistryPrxPtr, const Ice::Current&) override;

    void replicaInit(InternalRegistryPrxSeq, const Ice::Current&) override;
    void replicaAdded(InternalRegistryPrxPtr, const Ice::Current&) override;
    void replicaRemoved(InternalRegistryPrxPtr, const Ice::Current&) override;

    std::string getName(const Ice::Current&) const override;
    std::string getHostname(const Ice::Current&) const override;
    LoadInfo getLoad(const Ice::Current&) const override;
    int getProcessorSocketCount(const Ice::Current&) const override;
    void shutdown(const Ice::Current&) const override;

    std::int64_t getOffsetFromEnd(std::string, int, const Ice::Current&) const override;
    bool read(std::string, std::int64_t, int, std::int64_t&, Ice::StringSeq&, const Ice::Current&) const override;

    void shutdown();

    IceUtil::TimerPtr getTimer() const;
    std::shared_ptr<Ice::Communicator> getCommunicator() const;
    std::shared_ptr<Ice::ObjectAdapter> getAdapter() const;
    std::shared_ptr<Activator> getActivator() const;
    std::shared_ptr<TraceLevels> getTraceLevels() const;
    UserAccountMapperPrxPtr getUserAccountMapper() const;
    PlatformInfo& getPlatformInfo();
    std::shared_ptr<FileCache> getFileCache() const;
    NodePrxPtr getProxy() const;
    const PropertyDescriptorSeq& getPropertiesOverride() const;
    const std::string& getInstanceName() const;

    std::string getOutputDir() const;
    bool getRedirectErrToOut() const;
    bool allowEndpointsOverride() const;

    NodeSessionPrxPtr registerWithRegistry(const InternalRegistryPrxPtr&);
    void checkConsistency(const NodeSessionPrxPtr&);
    NodeSessionPrxPtr getMasterNodeSession() const;

    void addObserver(const NodeSessionPrxPtr&, const NodeObserverPrxPtr&);
    void removeObserver(const NodeSessionPrxPtr&);
    void observerUpdateServer(const ServerDynamicInfo&);
    void observerUpdateAdapter(const AdapterDynamicInfo&);

    void queueUpdate(const NodeObserverPrxPtr&, Update::UpdateFunction);
    void dequeueUpdate(const NodeObserverPrxPtr&, const std::shared_ptr<Update>&, bool);

    void addServer(const std::shared_ptr<ServerI>&, const std::string&);
    void removeServer(const std::shared_ptr<ServerI>&, const std::string&);

    Ice::Identity createServerIdentity(const std::string&) const;
    std::string getServerAdminCategory() const;

    bool canRemoveServerDirectory(const std::string&);

private:

    std::vector<std::shared_ptr<ServerCommand>> checkConsistencyNoSync(const Ice::StringSeq&);
    void patch(const IcePatch2::FileServerPrxPtr&, const std::string&, const std::vector<std::string>&);

    std::set<std::shared_ptr<ServerI>> getApplicationServers(const std::string&) const;
    std::string getFilePath(const std::string&) const;

    void loadServer(std::shared_ptr<InternalServerDescriptor>, std::string, bool,
                    std::function<void(const ServerPrxPtr &, const AdapterPrxDict &, int, int)>&&,
                    std::function<void(std::exception_ptr)>&&,
                    const Ice::Current&);

    void destroyServer(std::string, std::string, int, std::string, bool,
                       std::function<void()>,
                       std::function<void(std::exception_ptr)>,
                       const Ice::Current&);

    const std::shared_ptr<Ice::Communicator> _communicator;
    const std::shared_ptr<Ice::ObjectAdapter> _adapter;
    NodeSessionManager& _sessions;
    const std::shared_ptr<Activator> _activator;
    const IceUtil::TimerPtr _timer;
    const std::shared_ptr<TraceLevels> _traceLevels;
    const std::string _name;
    const NodePrxPtr _proxy;
    const std::string _outputDir;
    const bool _redirectErrToOut;
    const bool _allowEndpointsOverride;
    const int _waitTime;
    const std::string _instanceName;
    const UserAccountMapperPrxPtr _userAccountMapper;
    PlatformInfo _platform;
    const std::string _dataDir;
    const std::string _serversDir;
    const std::string _tmpDir;
    const std::shared_ptr<FileCache> _fileCache;
    PropertyDescriptorSeq _propertiesOverride;

    unsigned long _serial;
    bool _consistencyCheckDone;

    std::mutex _observerMutex;
    std::map<NodeSessionPrxPtr, NodeObserverPrxPtr> _observers;
    std::map<std::string, ServerDynamicInfo> _serversDynamicInfo;
    std::map<std::string, AdapterDynamicInfo> _adaptersDynamicInfo;

    std::map<NodeObserverPrxPtr, std::deque<std::shared_ptr<Update>>> _observerUpdates;

    mutable std::mutex _serversMutex;
    std::map<std::string, std::set<std::shared_ptr<ServerI>> > _serversByApplication;
    std::set<std::string> _patchInProgress;

    std::mutex _mutex;
    std::condition_variable _condVar;
};

}

#endif
