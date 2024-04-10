//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Test.h"
#include "TestHelper.h"

using namespace std;
using namespace Ice;
using namespace Test;

// The client and server have the same idle timeout (1s) and the server has EnableIdleCheck = 1.
// We verify that the server's idle check does not abort the connection as long as this connection receives heartbeats,
// even when the heartbeats are not read off the connection in a timely manner.
// To verify this situation, we use an OA with a 1-thread thread pool and use this unique thread for a long synchronous
// dispatch (sleep).
void
testIdleCheckDoesNotAbortConnectionWhenThreadPoolIsExhausted(const TestIntfPrx& p)
{
    cout << "testing that the idle check does not abort a connection that receives heartbeats... " << flush;
    p->ice_ping();
    ConnectionPtr connection = p->ice_getCachedConnection();
    test(connection);
    p->sleep(2000);                                   // the implementation in the server sleeps for 2,000ms
    test(p->ice_getCachedConnection() == connection); // we still have the same connection
    cout << "ok" << endl;
}

// We verify that the idle check aborts the connection when the connection (here server connection) remains idle for
// longer than idle timeout. Here, the server has an idle timeout of 1s and EnableIdleCheck = 1. We intentionally
// misconfigure the client with an idle timeout of 3s to send heartbeats every 1.5s, which is too long to prevent the
// server from aborting the connection.
void
testConnectionAbortedByIdleCheck(const string& proxyString, const PropertiesPtr& properties)
{
    cout << "testing that the idle check aborts a connection that does not receive anything for 1s... " << flush;

    // Create a new communicator with the desired properties.
    Ice::InitializationData initData;
    initData.properties = properties->clone();
    initData.properties->setProperty("Ice.IdleTimeout", "3");
    initData.properties->setProperty("Ice.Warn.Connections", "0");
    Ice::CommunicatorHolder holder = initialize(initData);
    TestIntfPrx p(holder.communicator(), proxyString);

    p->ice_ping();
    ConnectionPtr connection = p->ice_getCachedConnection();
    test(connection);

    // The idle check on the server side aborts the connection because it doesn't get a heartbeat in a timely fashion.
    try
    {
        p->sleep(2000); // the implementation in the server sleeps for 2,000ms
        test(false);    // we expect the server to abort the connection after about 1 second.
    }
    catch (const ConnectionLostException&)
    {
        // Expected
    }
    cout << "ok" << endl;
}

void
allTests(TestHelper* helper)
{
    CommunicatorPtr communicator = helper->communicator();
    string proxyString = "test: " + helper->getTestEndpoint();
    TestIntfPrx p(communicator, proxyString);

    testIdleCheckDoesNotAbortConnectionWhenThreadPoolIsExhausted(p);
    testConnectionAbortedByIdleCheck(proxyString, communicator->getProperties());

    p->shutdown();
}
