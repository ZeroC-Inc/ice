//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TOPIC_MANAGER_I_H
#define TOPIC_MANAGER_I_H

#include <IceStorm/IceStorm.h>

#include <IceStorm/Replica.h>
#include <IceStorm/Election.h>
#include <IceStorm/Instrumentation.h>
#include <IceStorm/Util.h>

namespace IceStorm
{

class PersistentInstance;
class TopicImpl;

//
// TopicManager implementation.
//
class TopicManagerImpl final : public IceStormElection::Replica,
                               public IceStorm::Instrumentation::ObserverUpdater,
                               public std::enable_shared_from_this<TopicManagerImpl>
{
public:

    static std::shared_ptr<TopicManagerImpl> create(const std::shared_ptr<PersistentInstance>&);

    // TopicManager methods.
    TopicPrxPtr create(const std::string&);
    TopicPrxPtr retrieve(const std::string&);
    TopicDict retrieveAll();

    // Observer methods.
    void observerInit(const IceStormElection::LogUpdate&, const IceStormElection::TopicContentSeq&);
    void observerCreateTopic(const IceStormElection::LogUpdate&, const std::string&);
    void observerDestroyTopic(const IceStormElection::LogUpdate&, const std::string&);
    void observerAddSubscriber(const IceStormElection::LogUpdate&, const std::string&,
                               const IceStorm::SubscriberRecord&);
    void observerRemoveSubscriber(const IceStormElection::LogUpdate&, const std::string&, const Ice::IdentitySeq&);

    // Sync methods.
    void getContent(IceStormElection::LogUpdate&, IceStormElection::TopicContentSeq&);

    // Replica methods.
    IceStormElection::LogUpdate getLastLogUpdate() const final;
    void sync(const Ice::ObjectPrx&) final;
    void initMaster(const std::set<IceStormElection::GroupNodeInfo>&, const IceStormElection::LogUpdate&) final;
    std::optional<Ice::ObjectPrx> getObserver() const final;
    std::optional<Ice::ObjectPrx> getSync() const final;

    void reap();

    void shutdown();

    std::shared_ptr<Ice::Object> getServant() const;

private:

    TopicManagerImpl(std::shared_ptr<PersistentInstance>);

    void updateTopicObservers() override;
    void updateSubscriberObservers() override;

    TopicPrx installTopic(
        const std::string&,
        const Ice::Identity&, bool,
        const IceStorm::SubscriberRecordSeq& = IceStorm::SubscriberRecordSeq());

    const std::shared_ptr<PersistentInstance> _instance;

    std::map<std::string, std::shared_ptr<TopicImpl>> _topics;

    std::shared_ptr<Ice::Object> _managerImpl;
    std::optional<Ice::ObjectPrx> _observer;
    std::optional<Ice::ObjectPrx> _sync;

    LLUMap _lluMap;
    SubscriberMap _subscriberMap;

    std::recursive_mutex _mutex;
};

} // End namespace IceStorm

#endif
