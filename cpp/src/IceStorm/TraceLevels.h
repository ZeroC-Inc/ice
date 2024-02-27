//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_STORM_TRACE_LEVELS_H
#define ICE_STORM_TRACE_LEVELS_H

#include <Ice/LoggerF.h>
#include <Ice/PropertiesF.h>

#include <string>

namespace IceStorm
{

class TraceLevels
{
public:

    TraceLevels(const std::string name, const Ice::PropertiesPtr&, Ice::LoggerPtr);

    const int topicMgr;
    const char* topicMgrCat;

    const int topic;
    const char* topicCat;

    const int subscriber;
    const char* subscriberCat;

    const int election;
    const char* electionCat;

    const int replication;
    const char* replicationCat;

    const Ice::LoggerPtr logger;
};

} // End namespace IceStorm

#endif
