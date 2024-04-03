//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "InstrumentationI.h"
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
    initData.properties->setProperty("Ice.Admin.Endpoints", "default");
    initData.properties->setProperty("Ice.Admin.InstanceName", "client");
    initData.properties->setProperty("Ice.Admin.DelayCreation", "1");
    initData.properties->setProperty("Ice.Warn.Connections", "0");
    CommunicatorObserverIPtr observer = make_shared<CommunicatorObserverI>();
    initData.observer = observer;
    Ice::CommunicatorHolder communicator = initialize(argc, argv, initData);

    MetricsPrx allTests(Test::TestHelper*, const CommunicatorObserverIPtr&);
    MetricsPrx metrics = allTests(this, observer);
    metrics->shutdown();
}

DEFINE_TEST(Client)
