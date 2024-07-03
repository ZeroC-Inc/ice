//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "TestHelper.h"
#include "TestI.h"

using namespace std;

class Server : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Server::run(int argc, char** argv)
{
    Ice::InitializationData initData;
    initData.properties = createTestProperties(argc, argv);
    // We configure a low idle timeout to make sure we send heartbeats frequently. It's the sending
    // of the heartbeats that schedules the inactivity timer task.
    initData.properties->setProperty("Ice.Connection.IdleTimeout", "1");
    initData.properties->setProperty("TestAdapter.Connection.InactivityTimeout", "5");
    initData.properties->setProperty("TestAdapter3s.Connection.InactivityTimeout", "3");

    Ice::CommunicatorHolder communicator = initialize(argc, argv, initData);
    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint());
    communicator->getProperties()->setProperty("TestAdapter3s.Endpoints", getTestEndpoint(1));

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    adapter->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter->activate();

    Ice::ObjectAdapterPtr adapter3s = communicator->createObjectAdapter("TestAdapter3s");
    adapter3s->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter3s->activate();

    serverReady();
    communicator->waitForShutdown();
}

DEFINE_TEST(Server)
