//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Test.h"
#include "TestHelper.h"

#include <vector>

using namespace std;
using namespace Ice;
using namespace Test;

// Verify that we can create connectionCount connections and send a ping on each connection.
void
testCreateConnections(TestIntfPrx p, int connectionCount, function<void()> postCloseDelay)
{
    cout << "testing the creation of " << connectionCount << " connections... " << flush;
    vector<ConnectionPtr> connectionList;
    for (int i = 0; i < connectionCount; ++i)
    {
        p = p->ice_connectionId("connection-" + to_string(i));
        p->ice_ping();
        connectionList.push_back(p->ice_getCachedConnection());
    }

    // Close all connections
    for (auto& connection : connectionList)
    {
        connection->close().get();
    }

    if (postCloseDelay)
    {
        postCloseDelay();
    }

    cout << "ok" << endl;
}

// Verify that we can create connectionCount connections and send a ping on each connection.
void
testCreateConnectionsWithMax(TestIntfPrx p, int max, function<void()> postCloseDelay)
{
    cout << "testing the creation of " << max << " connections with connection lost at " << (max + 1) << "... "
         << flush;
    vector<ConnectionPtr> connectionList;
    for (int i = 0; i < max; ++i)
    {
        p = p->ice_connectionId("connection-" + to_string(i));
        p->ice_ping();
        connectionList.push_back(p->ice_getCachedConnection());
    }

    p = p->ice_connectionId("connection-" + to_string(max));

    try
    {
        p->ice_ping();
        test(false);
    }
    catch (const Ice::ConnectionLostException&)
    {
        // Expected
    }

    // Close all connections
    for (auto& connection : connectionList)
    {
        connection->close().get();
    }

    if (postCloseDelay)
    {
        postCloseDelay();
    }

    cout << "ok" << endl;
}

// Verify that we can create max connections, then connection lost, then recover.
void
testCreateConnectionsWithMaxAndRecovery(TestIntfPrx p, int max, function<void()> postCloseDelay)
{
    cout << "testing the creation of " << max << " connections with connection lost at " << (max + 1)
         << " then recovery... " << flush;
    vector<ConnectionPtr> connectionList;
    for (int i = 0; i < max; ++i)
    {
        p = p->ice_connectionId("connection-" + to_string(i));
        p->ice_ping();
        connectionList.push_back(p->ice_getCachedConnection());
    }

    p = p->ice_connectionId("connection-" + to_string(max));

    try
    {
        p->ice_ping();
        test(false);
    }
    catch (const Ice::ConnectionLostException&)
    {
        // Expected
    }

    // Close one connection
    connectionList.front()->close().get();
    connectionList.erase(connectionList.begin());

    // Try again
    const int maxRetries = 3;
    for (int i = 0; i < maxRetries; ++i)
    {
        try
        {
            p->ice_ping();
        }
        catch (const Ice::ConnectionLostException& ex)
        {
            // Retry after 50ms to give the server time to close connections if it’s slow.
            if (i < maxRetries)
            {
                this_thread::sleep_for(50ms);
                continue;
            }
            cout << "Unexpected exception: " << ex << endl;
            test(false);
        }
    }

    connectionList.push_back(p->ice_getCachedConnection());

    // Close all connections
    for (auto& connection : connectionList)
    {
        connection->close().get();
    }

    if (postCloseDelay)
    {
        postCloseDelay();
    }
    cout << "ok" << endl;
}

void
allTests(TestHelper* helper)
{
    CommunicatorPtr communicator = helper->communicator();
    string proxyString = "test: " + helper->getTestEndpoint();
    TestIntfPrx p(communicator, proxyString);

    string proxyStringMax10 = "test: " + helper->getTestEndpoint(1);
    TestIntfPrx pMax10(communicator, proxyStringMax10);

    string transport = helper->getTestProtocol();

    // When the transport is WS or WSS, we need to wait a little bit: the server closes the connection after it
    // gets a transport frame from the client.
    function<void()> postCloseDelay;
    if (transport.find("ws") == 0)
    {
        postCloseDelay = []() { this_thread::sleep_for(50ms); };
    }

    testCreateConnections(p, 100, postCloseDelay);
    testCreateConnectionsWithMax(pMax10, 10, postCloseDelay);
    testCreateConnectionsWithMaxAndRecovery(pMax10, 10, postCloseDelay);

    p->shutdown();
}
