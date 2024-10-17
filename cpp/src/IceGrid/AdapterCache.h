//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICEGRID_ADAPTERCACHE_H
#define ICEGRID_ADAPTERCACHE_H

#include "Cache.h"
#include "IceGrid/Registry.h"
#include "Internal.h"

#include <optional>
#include <set>

namespace IceGrid
{
    class AdapterCache;
    class AdapterEntry;
    class ServerAdapterEntry;
    class ServerEntry;
    class SynchronizationCallback;

    using ServerEntrySeq = std::vector<std::shared_ptr<ServerEntry>>;

    struct LocatorAdapterInfo
    {
        std::string id;
        std::optional<AdapterPrx> proxy;
        std::chrono::seconds activationTimeout;
        std::chrono::seconds deactivationTimeout;
    };
    using LocatorAdapterInfoSeq = std::vector<LocatorAdapterInfo>;

    class GetAdapterInfoResult
    {
    public:
        void add(const ServerAdapterEntry*);
        AdapterInfoSeq get();

    private:
        AdapterInfoSeq _adapters;
        std::vector<std::optional<std::future<std::optional<Ice::ObjectPrx>>>> _results;
    };

    class AdapterEntry
    {
    public:
        AdapterEntry(AdapterCache&, const std::string&, const std::string&);

        virtual bool addSyncCallback(const std::shared_ptr<SynchronizationCallback>&, const std::set<std::string>&) = 0;

        virtual void getLocatorAdapterInfo(
            LocatorAdapterInfoSeq&,
            int&,
            bool&,
            bool&,
            std::string&,
            const std::set<std::string>&) = 0;
        virtual float getLeastLoadedNodeLoad(LoadSample) const = 0;
        virtual AdapterInfoSeq getAdapterInfoNoEndpoints() const = 0;
        virtual std::shared_ptr<GetAdapterInfoResult> getAdapterInfoAsync() const = 0;
        virtual std::optional<AdapterPrx> getProxy(const std::string&, bool) const = 0;

        virtual bool canRemove();

        std::string getId() const;
        std::string getApplication() const;

    protected:
        AdapterCache& _cache;
        const std::string _id;
        std::string _application;
    };

    class ServerAdapterEntry final : public AdapterEntry
    {
    public:
        ServerAdapterEntry(
            AdapterCache&,
            const std::string&,
            const std::string&,
            const std::string&,
            int,
            const std::shared_ptr<ServerEntry>&);

        bool addSyncCallback(const std::shared_ptr<SynchronizationCallback>&, const std::set<std::string>&) final;

        void
        getLocatorAdapterInfo(LocatorAdapterInfoSeq&, int&, bool&, bool&, std::string&, const std::set<std::string>&)
            final;

        float getLeastLoadedNodeLoad(LoadSample) const final;
        AdapterInfoSeq getAdapterInfoNoEndpoints() const final;
        std::shared_ptr<GetAdapterInfoResult> getAdapterInfoAsync() const final;
        std::optional<AdapterPrx> getProxy(const std::string&, bool) const final;

        void getLocatorAdapterInfo(LocatorAdapterInfoSeq&) const;
        const std::string& getReplicaGroupId() const { return _replicaGroupId; }
        int getPriority() const;

        std::string getServerId() const;
        std::string getNodeName() const;

    private:
        const std::string _replicaGroupId;
        const int _priority;
        const std::shared_ptr<ServerEntry> _server;
    };

    class ReplicaGroupEntry final : public AdapterEntry
    {
    public:
        ReplicaGroupEntry(
            AdapterCache&,
            const std::string&,
            const std::string&,
            const std::shared_ptr<LoadBalancingPolicy>&,
            const std::string&);

        bool addSyncCallback(const std::shared_ptr<SynchronizationCallback>&, const std::set<std::string>&) final;

        void
        getLocatorAdapterInfo(LocatorAdapterInfoSeq&, int&, bool&, bool&, std::string&, const std::set<std::string>&)
            final;
        float getLeastLoadedNodeLoad(LoadSample) const final;
        AdapterInfoSeq getAdapterInfoNoEndpoints() const final;
        std::shared_ptr<GetAdapterInfoResult> getAdapterInfoAsync() const final;
        std::optional<AdapterPrx> getProxy(const std::string&, bool) const final { return std::nullopt; }

        void addReplica(const std::string&, const std::shared_ptr<ServerAdapterEntry>&);
        bool removeReplica(const std::string&);

        void update(const std::string&, const std::shared_ptr<LoadBalancingPolicy>&, const std::string&);
        bool hasAdaptersFromOtherApplications() const;

        const std::string& getFilter() const { return _filter; }

    private:
        std::shared_ptr<LoadBalancingPolicy> _loadBalancing;
        int _loadBalancingNReplicas;
        LoadSample _loadSample;
        std::string _filter;
        std::vector<std::shared_ptr<ServerAdapterEntry>> _replicas;
        int _lastReplica;
        bool _requestInProgress;

        mutable std::mutex _mutex;
        std::condition_variable _condVar;
    };

    class AdapterCache : public CacheByString<AdapterEntry>
    {
    public:
        AdapterCache(const Ice::CommunicatorPtr&);

        void addServerAdapter(const AdapterDescriptor&, const std::shared_ptr<ServerEntry>&, const std::string&);
        void addReplicaGroup(const ReplicaGroupDescriptor&, const std::string&);

        std::shared_ptr<AdapterEntry> get(const std::string&) const;

        void removeServerAdapter(const std::string&);
        void removeReplicaGroup(const std::string&);

    protected:
        virtual std::shared_ptr<AdapterEntry> addImpl(const std::string&, const std::shared_ptr<AdapterEntry>&);
        virtual void removeImpl(const std::string&);

    private:
        const Ice::CommunicatorPtr _communicator;
    };

};

#endif
