//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TRANSIENT_TOPIC_MANAGER_I_H
#define TRANSIENT_TOPIC_MANAGER_I_H

#include <IceStorm/IceStormInternal.h>

namespace IceStorm
{
    class Instance;
    class TransientTopicImpl;

    class TransientTopicManagerImpl final : public TopicManagerInternal
    {
    public:
        TransientTopicManagerImpl(std::shared_ptr<Instance>);

        // TopicManager methods.
        std::optional<TopicPrx> create(std::string, const Ice::Current&) final;
        std::optional<TopicPrx> retrieve(std::string, const Ice::Current&) final;
        TopicDict retrieveAll(const Ice::Current&) final;
        std::optional<IceStormElection::NodePrx> getReplicaNode(const Ice::Current&) const final;

        void reap();
        void shutdown();

    private:
        const std::shared_ptr<Instance> _instance;
        std::map<std::string, std::shared_ptr<TransientTopicImpl>> _topics;

        std::mutex _mutex;
    };

} // End namespace IceStorm

#endif
