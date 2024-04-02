//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICESTORM_REPLICA_H
#define ICESTORM_REPLICA_H

#include "Election.h"
#include "Ice/Ice.h"

#include <set>

namespace IceStormElection
{
    struct GroupNodeInfo
    {
        explicit GroupNodeInfo(int i);
        GroupNodeInfo(int i, LogUpdate l, std::optional<Ice::ObjectPrx> o = std::nullopt);

        bool operator<(const GroupNodeInfo& rhs) const;
        bool operator==(const GroupNodeInfo& rhs) const;

        const int id;
        const LogUpdate llu;
        const std::optional<Ice::ObjectPrx> observer;
    };

    class Replica
    {
    public:
        virtual LogUpdate getLastLogUpdate() const = 0;
        virtual void sync(const Ice::ObjectPrx&) = 0;
        virtual void initMaster(const std::set<IceStormElection::GroupNodeInfo>&, const LogUpdate&) = 0;
        virtual std::optional<Ice::ObjectPrx> getObserver() const = 0;
        virtual std::optional<Ice::ObjectPrx> getSync() const = 0;
    };
}

#endif // RELICA_H
