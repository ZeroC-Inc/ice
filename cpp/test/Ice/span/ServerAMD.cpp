//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "TestHelper.h"
#include "TestAMDI.h"

using namespace std;

class ServerAMD : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
ServerAMD::run(int argc, char** argv)
{
    Ice::CommunicatorHolder communicator = initialize(argc, argv);

    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint());
    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    adapter->add(std::make_shared<TestIntfAMDI>(), Ice::stringToIdentity("test"));
    adapter->activate();
    serverReady();
    communicator->waitForShutdown();
}

DEFINE_TEST(ServerAMD)
