//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "IceUtil/Random.h"
#include "Test.h"
#include "TestHelper.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace std;
using namespace Ice;
using namespace Test;

class Client : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Client::run(int argc, char** argv)
{
    PropertiesPtr properties = createProperties(argc, argv);
    properties->setProperty("Ice.Plugin.IceSSL", "IceSSL:createIceSSL");
    properties->setProperty("IceSSL.Keychain", "client.keychain");
    properties->setProperty("IceSSL.KeychainPassword", "password");
    CommunicatorHolder holder = initialize(argc, argv, properties);
    auto communicator = holder.communicator();
    cout << "testing proxy hash algorithm collisions... " << flush;
    map<int32_t, optional<ObjectPrx>> seenProxy;
    map<int32_t, EndpointPtr> seenEndpoint;
    unsigned int proxyCollisions = 0;
    unsigned int i = 0;
    unsigned int maxCollisions = 10;
    unsigned int maxIterations = 10000;

    for (i = 0; proxyCollisions < maxCollisions && i < maxIterations; ++i)
    {
        ostringstream os;
        os << i << ":tcp -p " << IceUtilInternal::random(65536) << " -t 10" << IceUtilInternal::random(1000000)
           << ":udp -p " << IceUtilInternal::random(65536) << " -h " << IceUtilInternal::random(100);

        ObjectPrx obj(communicator, os.str());
        EndpointSeq endpoints = obj->ice_getEndpoints();
        if (!seenProxy.insert(make_pair(hash<ObjectPrx>{}(obj), obj)).second)
        {
            ++proxyCollisions;
        }
        test(hash<ObjectPrx>{}(obj) == hash<ObjectPrx>{}(obj));
    }
    test(proxyCollisions < maxCollisions);

    //
    // Check the same proxy produce the same hash, even when we recreate the proxy.
    //
    ObjectPrx prx1(communicator, "Glacier2/router:tcp -p 10010");
    ObjectPrx prx2(communicator, "Glacier2/router:ssl -p 10011");
    ObjectPrx prx3(communicator, "Glacier2/router:udp -p 10012");
    ObjectPrx prx4(communicator, "Glacier2/router:tcp -h zeroc.com -p 10010");
    ObjectPrx prx5(communicator, "Glacier2/router:ssl -h zeroc.com -p 10011");
    ObjectPrx prx6(communicator, "Glacier2/router:udp -h zeroc.com -p 10012");
    ObjectPrx prx7(communicator, "Glacier2/router:tcp -p 10010 -t 10000");
    ObjectPrx prx8(communicator, "Glacier2/router:ssl -p 10011 -t 10000");
    ObjectPrx prx9(communicator, "Glacier2/router:tcp -h zeroc.com -p 10010 -t 10000");
    ObjectPrx prx10(communicator, "Glacier2/router:ssl -h zeroc.com -p 10011 -t 10000");

    map<string, size_t> proxyMap;
    proxyMap["prx1"] = hash<ObjectPrx>{}(prx1);
    proxyMap["prx2"] = hash<ObjectPrx>{}(prx2);
    proxyMap["prx3"] = hash<ObjectPrx>{}(prx3);
    proxyMap["prx4"] = hash<ObjectPrx>{}(prx4);
    proxyMap["prx5"] = hash<ObjectPrx>{}(prx5);
    proxyMap["prx6"] = hash<ObjectPrx>{}(prx6);
    proxyMap["prx7"] = hash<ObjectPrx>{}(prx7);
    proxyMap["prx8"] = hash<ObjectPrx>{}(prx8);
    proxyMap["prx9"] = hash<ObjectPrx>{}(prx9);
    proxyMap["prx10"] = hash<ObjectPrx>{}(prx10);

    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:tcp -p 10010")) == proxyMap["prx1"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:ssl -p 10011")) == proxyMap["prx2"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:udp -p 10012")) == proxyMap["prx3"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:tcp -h zeroc.com -p 10010")) == proxyMap["prx4"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:ssl -h zeroc.com -p 10011")) == proxyMap["prx5"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:udp -h zeroc.com -p 10012")) == proxyMap["prx6"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:tcp -p 10010 -t 10000")) == proxyMap["prx7"]);
    test(hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:ssl -p 10011 -t 10000")) == proxyMap["prx8"]);
    test(
        hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:tcp -h zeroc.com -p 10010 -t 10000")) == proxyMap["prx9"]);
    test(
        hash<ObjectPrx>{}(*communicator->stringToProxy("Glacier2/router:ssl -h zeroc.com -p 10011 -t 10000")) ==
        proxyMap["prx10"]);

    cerr << "ok" << endl;
}

DEFINE_TEST(Client)
