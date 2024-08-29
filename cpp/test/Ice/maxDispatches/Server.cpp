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
    initData.properties->setProperty("Ice.ThreadPool.Server.Size", "10"); // plenty of threads to handle the requests
    Ice::CommunicatorHolder communicator = initialize(argc, argv, initData);

    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint());
    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    adapter->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter->activate();

    communicator->getProperties()->setProperty("TestAdapterMax10.Endpoints", getTestEndpoint(1));
    communicator->getProperties()->setProperty("TestAdapterMax10.Connection.MaxDispatches", "10");
    adapter = communicator->createObjectAdapter("TestAdapterMax10");
    adapter->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter->activate();

    communicator->getProperties()->setProperty("TestAdapterMax1.Endpoints", getTestEndpoint(2));
    communicator->getProperties()->setProperty("TestAdapterMax1.Connection.MaxDispatches", "1");
    adapter = communicator->createObjectAdapter("TestAdapterMax1");
    adapter->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter->activate();

    communicator->getProperties()->setProperty("TestAdapterSerialize.Endpoints", getTestEndpoint(3));
    communicator->getProperties()->setProperty("TestAdapterSerialize.ThreadPool.Size", "10");
    communicator->getProperties()->setProperty("TestAdapterSerialize.ThreadPool.Serialize", "1");
    adapter = communicator->createObjectAdapter("TestAdapterSerialize");
    adapter->add(std::make_shared<TestIntfI>(), Ice::stringToIdentity("test"));
    adapter->activate();

    serverReady();
    communicator->waitForShutdown();
}

DEFINE_TEST(Server)
