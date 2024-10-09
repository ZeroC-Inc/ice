//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#define ICESTORM_SERVICE_API_EXPORTS

#include "Service.h"
#include "../Ice/PluginManagerI.h" // For loadPlugin
#include "../Ice/PropertyNames.h"
#include "../Ice/PropertyUtil.h"
#include "Ice/StringUtil.h"
#include "IceGrid/Registry.h"
#include "Instance.h"
#include "NodeI.h"
#include "Observers.h"
#include "TopicI.h"
#include "TopicManagerI.h"
#include "TraceLevels.h"
#include "TransientTopicI.h"
#include "TransientTopicManagerI.h"
#include "Util.h"

#include <stdexcept>

using namespace std;
using namespace Ice;
using namespace IceStorm;
using namespace IceStormInternal;
using namespace IceStormElection;

namespace
{
    class ServiceI final : public IceStormInternal::Service
    {
    public:
        IceStorm::TopicManagerPrx getTopicManager() const final;

        void start(const std::string&, const Ice::CommunicatorPtr&, const Ice::StringSeq&) final;

        // For IceGrid
        void start(
            const Ice::CommunicatorPtr&,
            const Ice::ObjectAdapterPtr&,
            const Ice::ObjectAdapterPtr&,
            const std::string&,
            const Ice::Identity&);

        void stop() final;

    private:
        void createDbEnv(const Ice::CommunicatorPtr&);

        std::shared_ptr<IceStorm::TopicManagerImpl> _manager;
        std::shared_ptr<IceStorm::TransientTopicManagerImpl> _transientManager;
        std::optional<IceStorm::TopicManagerPrx> _managerProxy;
        std::shared_ptr<IceStorm::Instance> _instance;
    };

    class FinderI final : public IceStorm::Finder
    {
    public:
        FinderI(TopicManagerPrx topicManager) : _topicManager(std::move(topicManager)) {}

        std::optional<TopicManagerPrx> getTopicManager(const Ice::Current&) final { return _topicManager; }

    private:
        const TopicManagerPrx _topicManager;
    };
}

extern "C"
{
    ICESTORM_SERVICE_API ::IceBox::Service* createIceStorm(const CommunicatorPtr&) { return new ServiceI; }
}

shared_ptr<IceStormInternal::Service>
IceStormInternal::Service::create(
    const CommunicatorPtr& communicator,
    const ObjectAdapterPtr& topicAdapter,
    const ObjectAdapterPtr& publishAdapter,
    const string& name,
    const Ice::Identity& id)
{
    shared_ptr<ServiceI> service(new ServiceI);
    service->start(communicator, topicAdapter, publishAdapter, name, id);
    return service;
}

void
ServiceI::start(const string& name, const CommunicatorPtr& communicator, const StringSeq&)
{
    auto properties = communicator->getProperties();

    int id = properties->getPropertyAsIntWithDefault("IceStorm.NodeId", -1);

    // If we are using a replicated deployment and if the topic manager thread pool max size is not set then ensure it
    // is set to some suitably high number. This ensures no deadlocks in the replicated case due to call forwarding
    // from replicas to coordinators.
    if (id != -1 && properties->getProperty("IceStorm.TopicManager.ThreadPool.SizeMax").empty())
    {
        properties->setProperty("IceStorm.TopicManager.ThreadPool.SizeMax", "100");
    }

    auto topicAdapter = communicator->createObjectAdapter("IceStorm.TopicManager");
    auto publishAdapter = communicator->createObjectAdapter("IceStorm.Publish");

    //
    // We use the name of the service for the name of the database environment.
    //
    string instanceName = properties->getPropertyWithDefault("IceStorm.InstanceName", "IceStorm");
    Identity topicManagerId = {"TopicManager", instanceName};

    if (properties->getPropertyAsIntWithDefault("IceStorm.Transient", 0) > 0)
    {
        _instance = make_shared<Instance>(instanceName, name, communicator, publishAdapter, topicAdapter, nullptr);
        try
        {
            auto manager = make_shared<TransientTopicManagerImpl>(_instance);
            _managerProxy = topicAdapter->add<TopicManagerPrx>(manager, topicManagerId);
        }
        catch (const Ice::Exception& ex)
        {
            _instance = 0;

            LoggerOutputBase s;
            s << "exception while starting IceStorm service " << name << ":\n";
            s << ex;
            throw IceBox::FailureException(__FILE__, __LINE__, s.str());
        }
        topicAdapter->activate();
        publishAdapter->activate();
        return;
    }

    if (id == -1) // No replication.
    {
        try
        {
            auto instance =
                make_shared<PersistentInstance>(instanceName, name, communicator, publishAdapter, topicAdapter);
            _manager = TopicManagerImpl::create(instance);
            _instance = std::move(instance);
            _managerProxy = topicAdapter->add<TopicManagerPrx>(_manager->getServant(), topicManagerId);
        }
        catch (const Ice::Exception& ex)
        {
            _instance = 0;

            LoggerOutputBase s;
            s << "exception while starting IceStorm service " << name << ":\n";
            s << ex;

            throw IceBox::FailureException(__FILE__, __LINE__, s.str());
        }
    }
    else
    {
        // Here we want to create a map of id -> election node proxies.
        map<int, NodePrx> nodes;

        string topicManagerAdapterId = properties->getProperty("IceStorm.TopicManager.AdapterId");

        // We support two possible deployments. The first is a manual deployment, the second is IceGrid.
        //
        // Here we check for the manual deployment
        const string prefix = "IceStorm.Nodes.";
        Ice::PropertyDict props = properties->getPropertiesForPrefix(prefix);
        if (!props.empty())
        {
            for (const auto& prop : props)
            {
                try
                {
                    int nodeid = stoi(prop.first.substr(prefix.size()));
                    auto nodePrx = communicator->propertyToProxy<NodePrx>(prop.first);
                    assert(nodePrx);
                    nodes.insert({nodeid, *nodePrx});
                }
                catch (const std::invalid_argument&)
                {
                    Ice::Warning warn(communicator->getLogger());
                    warn << "deployment warning: invalid node id `" << prop.first.substr(prefix.size()) << "'";
                }
            }
        }
        else
        {
            // If adapter id's are defined for the topic manager or node adapters then we consider this an IceGrid
            // based deployment.
            string nodeAdapterId = properties->getProperty("IceStorm.Node.AdapterId");

            // Validate first that the adapter ids match for the node and the topic manager otherwise some other
            // deployment is being used.
            const string suffix = ".TopicManager";
            if (topicManagerAdapterId.empty() || nodeAdapterId.empty() ||
                topicManagerAdapterId.replace(topicManagerAdapterId.find(suffix), suffix.size(), ".Node") !=
                    nodeAdapterId)
            {
                Ice::Error error(communicator->getLogger());
                error << "deployment error: `" << topicManagerAdapterId << "' prefix does not match `" << nodeAdapterId
                      << "'";
                throw IceBox::FailureException(__FILE__, __LINE__, "IceGrid deployment is incorrect");
            }

            // Determine the set of node id and node proxies.
            //
            // This is determined by locating all topic manager replicas, and then working out the node for that
            // replica.
            //
            // We work out the node id by removing the instance name. The node id must follow.
            auto locator = uncheckedCast<IceGrid::LocatorPrx>(communicator->getDefaultLocator().value());
            auto query = locator->getLocalQuery();
            auto replicas = query->findAllReplicas(communicator->stringToProxy(instance "IceStorm/TopicManager"));

            for (const auto& replica : replicas)
            {
                string adapterid = replica->ice_getAdapterId();

                // Replace TopicManager with the node endpoint.
                adapterid = adapterid.replace(adapterid.find(suffix), suffix.size(), ".Node");

                // The adapter id must start with the instance name.
                if (adapterid.find(instanceName) != 0)
                {
                    Ice::Error error(communicator->getLogger());
                    error << "deployment error: `" << adapterid << "' does not start with `" << instanceName << "'";
                    throw IceBox::FailureException(__FILE__, __LINE__, "IceGrid deployment is incorrect");
                }

                // The node id follows. We find the first digit (the start of the node id, and then the end of the
                // digits).
                string::size_type start = instanceName.size();
                while (start < adapterid.size() && !IceInternal::isDigit(adapterid[start]))
                {
                    ++start;
                }
                string::size_type end = start;
                while (end < adapterid.size() && IceInternal::isDigit(adapterid[end]))
                {
                    ++end;
                }
                if (start == end)
                {
                    // We must have at least one digit, otherwise there is some sort of deployment error.
                    Ice::Error error(communicator->getLogger());
                    error << "deployment error: node id does not follow instance name. instance name:" << instanceName
                          << " adapter id: " << adapterid;
                    throw IceBox::FailureException(__FILE__, __LINE__, "IceGrid deployment is incorrect");
                }

                int nodeid = atoi(adapterid.substr(start, end - start).c_str());
                ostringstream os;
                os << "node" << nodeid;
                Ice::Identity ident;
                ident.category = instanceName;
                ident.name = os.str();
                nodes.insert({nodeid, replica->ice_adapterId(adapterid)->ice_identity<NodePrx>(ident)});
            }
        }

        if (nodes.size() < 3)
        {
            Ice::Error error(communicator->getLogger());
            error << "Replication requires at least 3 Nodes";
            throw IceBox::FailureException(__FILE__, __LINE__, "Replication requires at least 3 Nodes");
        }

        try
        {
            // If the node thread pool size is not set then initialize
            // to the number of nodes + 1 and disable thread pool size
            // warnings.
            if (properties->getProperty("IceStorm.Node.ThreadPool.Size").empty())
            {
                ostringstream os;
                os << nodes.size() + 1;
                properties->setProperty("IceStorm.Node.ThreadPool.Size", os.str());
                properties->setProperty("IceStorm.Node.ThreadPool.SizeWarn", "0");
            }
            if (properties->getProperty("IceStorm.Node.MessageSizeMax").empty())
            {
                properties->setProperty("IceStorm.Node.MessageSizeMax", "0"); // No limit on data exchanged internally
            }

            auto nodeAdapter = communicator->createObjectAdapter("IceStorm.Node");
            auto instance = make_shared<PersistentInstance>(
                instanceName,
                name,
                communicator,
                publishAdapter,
                topicAdapter,
                nodeAdapter,
                nodes.at(id));
            _instance = instance;

            _instance->observers()->setMajority(static_cast<unsigned int>(nodes.size()) / 2);

            // Trace replication information.
            auto traceLevels = _instance->traceLevels();
            if (traceLevels->election > 0)
            {
                Ice::Trace out(traceLevels->logger, traceLevels->electionCat);
                out << "I am node " << id << "\n";
                for (const auto& node : nodes)
                {
                    out << "\tnode: " << node.first << " proxy: " << node.second->ice_toString() << "\n";
                }
            }

            if (topicManagerAdapterId.empty())
            {
                // We're not using an IceGrid deployment. Here we need
                // a proxy which is used to create proxies to the
                // replicas later.
                _managerProxy = topicAdapter->createProxy<TopicManagerPrx>(topicManagerId);
            }
            else
            {
                // If we're using IceGrid deployment we need to create
                // indirect proxies.
                _managerProxy = topicAdapter->createIndirectProxy<TopicManagerPrx>(topicManagerId);
            }

            _manager = TopicManagerImpl::create(instance);
            topicAdapter->add(_manager->getServant(), topicManagerId);

            ostringstream os; // The node object identity.
            os << "node" << id;
            Ice::Identity nodeid;
            nodeid.category = instanceName;
            nodeid.name = os.str();

            assert(_manager);
            assert(_managerProxy);

            auto node = make_shared<NodeI>(_instance, _manager, *_managerProxy, id, nodes);
            _instance->setNode(node);
            nodeAdapter->add(node, nodeid);
            nodeAdapter->activate();

            node->start();
        }
        catch (const Ice::Exception& ex)
        {
            _instance = nullptr;

            LoggerOutputBase s;
            s << "exception while starting IceStorm service " << name << ":\n" << ex;
            throw IceBox::FailureException(__FILE__, __LINE__, s.str());
        }
    }

    topicAdapter->add(
        make_shared<FinderI>(topicAdapter->createProxy<TopicManagerPrx>(topicManagerId)),
        stringToIdentity("IceStorm/Finder"));

    topicAdapter->activate();
    publishAdapter->activate();
}

void
ServiceI::start(
    const CommunicatorPtr& communicator,
    const ObjectAdapterPtr& topicAdapter,
    const ObjectAdapterPtr& publishAdapter,
    const string& name,
    const Identity& id)
{
    //
    // This is for IceGrid only and as such we use a transient implementation of IceStorm.
    //

    const string instanceName = "IceStorm";
    _instance = make_shared<Instance>(instanceName, name, communicator, publishAdapter, topicAdapter, nullptr);

    try
    {
        auto manager = make_shared<TransientTopicManagerImpl>(_instance);
        _managerProxy = topicAdapter->add<TopicManagerPrx>(manager, id);
    }
    catch (const Ice::Exception& ex)
    {
        _instance = nullptr;
        LoggerOutputBase s;
        s << "exception while starting IceStorm service " << name << ":\n" << ex;
        throw IceBox::FailureException(__FILE__, __LINE__, s.str());
    }
}

TopicManagerPrx
ServiceI::getTopicManager() const
{
    assert(_managerProxy);
    return *_managerProxy;
}

void
ServiceI::stop()
{
    // Shutdown the instance. This deactivates all OAs.
    _instance->shutdown();

    //
    // It's necessary to reap all destroyed topics on shutdown.
    //
    if (_manager)
    {
        _manager->shutdown();
    }
    if (_transientManager)
    {
        _transientManager->shutdown();
    }

    //
    // Destroy the instance. This step must occur last.
    //
    _instance->destroy();
}
