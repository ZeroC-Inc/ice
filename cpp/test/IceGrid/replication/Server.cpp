//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <TestI.h>
#include <TestHelper.h>

using namespace std;

class Server final : public Test::TestHelper
{
public:

    void run(int, char**) override;
};

void
Server::run(int argc, char** argv)
{
    Ice::CommunicatorHolder communicatorHolder = initialize(argc, argv);
    auto adapter = communicatorHolder->createObjectAdapter("TestAdapter");
    adapter->add(make_shared<TestI>(), Ice::stringToIdentity(communicatorHolder->getProperties()->getProperty("Identity")));
    try
    {
        adapter->activate();
    }
    catch(const Ice::ObjectAdapterDeactivatedException&)
    {
    }
    communicatorHolder->waitForShutdown();
}

DEFINE_TEST(Server)
