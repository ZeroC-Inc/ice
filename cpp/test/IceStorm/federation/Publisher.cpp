//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <TestHelper.h>
#include <Event.h>

using namespace std;
using namespace Ice;
using namespace IceStorm;
using namespace Test;

class Publisher final : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Publisher::run(int argc, char** argv)
{
    Ice::CommunicatorHolder communicator = initialize(argc, argv);
    auto properties = communicator->getProperties();
    auto managerProxy = properties->getProperty("IceStormAdmin.TopicManager.Default");
    if (managerProxy.empty())
    {
        ostringstream os;
        os << argv[0] << ": `IceStormAdmin.TopicManager.Default' is not set";
        throw runtime_error(os.str());
    }

    auto base = communicator->stringToProxy(managerProxy);
    auto manager = checkedCast<IceStorm::TopicManagerPrx>(base);
    if (!manager)
    {
        ostringstream os;
        os << argv[0] << ": `" << managerProxy << "' is not running";
        throw runtime_error(os.str());
    }

    auto fed1 = manager->retrieve("fed1");
    auto fed2 = manager->retrieve("fed2");
    auto fed3 = manager->retrieve("fed3");

    auto eventFed1 = uncheckedCast<EventPrx>(fed1->getPublisher()->ice_oneway());
    auto eventFed2 = uncheckedCast<EventPrx>(fed2->getPublisher()->ice_oneway());
    auto eventFed3 = uncheckedCast<EventPrx>(fed3->getPublisher()->ice_oneway());

    Ice::Context context;

    context["cost"] = "0";
    for (int i = 0; i < 10; ++i)
    {
        eventFed1->pub("fed1:0", context);
    }

    context["cost"] = "10";
    for (int i = 0; i < 10; ++i)
    {
        eventFed1->pub("fed1:10", context);
    }

    context["cost"] = "15";
    for (int i = 0; i < 10; ++i)
    {
        eventFed1->pub("fed1:15", context);
    }

    context["cost"] = "0";
    for (int i = 0; i < 10; ++i)
    {
        eventFed2->pub("fed2:0", context);
    }

    context["cost"] = "5";
    for (int i = 0; i < 10; ++i)
    {
        eventFed2->pub("fed2:5", context);
    }

    context["cost"] = "0";
    for (int i = 0; i < 10; ++i)
    {
        eventFed3->pub("fed3:0", context);
    }

    //
    // Before we exit, we ping all proxies as twoway, to make sure
    // that all oneways are delivered.
    //
    eventFed1->ice_twoway()->ice_ping();
    eventFed2->ice_twoway()->ice_ping();
    eventFed3->ice_twoway()->ice_ping();
}

DEFINE_TEST(Publisher)
