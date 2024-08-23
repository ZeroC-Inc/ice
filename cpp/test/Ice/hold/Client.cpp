//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "Test.h"
#include "TestHelper.h"

using namespace std;
using namespace Test;

class Client : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Client::run(int argc, char** argv)
{
    Ice::InitializationData initData;
    initData.properties = createTestProperties(argc, argv);
    initData.properties->setProperty("Ice.Connection.Client.IdleTimeout", "1");
    Ice::CommunicatorHolder communicator = initialize(argc, argv, initData);
    void allTests(Test::TestHelper*);
    allTests(this);
}

DEFINE_TEST(Client)
