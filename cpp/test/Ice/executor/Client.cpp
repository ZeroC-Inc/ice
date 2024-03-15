//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <TestHelper.h>
#include <Test.h>
#include "Executor.h"

using namespace std;

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
    //
    // Limit the send buffer size, this test relies on the socket
    // send() blocking after sending a given amount of data.
    //
    initData.properties->setProperty("Ice.TCP.SndSize", "50000");

    auto executor = Executor::create();
    initData.executor = [=](function<void()> call, const shared_ptr<Ice::Connection>& conn)
    { executor->execute(make_shared<ExecutorCall>(call), conn); };
    // The communicator must be destroyed before the executor is terminated.
    {
        Ice::CommunicatorHolder communicator = initialize(argc, argv, initData);

        void allTests(Test::TestHelper*);
        allTests(this);
    }
    executor->terminate();
}

DEFINE_TEST(Client)
