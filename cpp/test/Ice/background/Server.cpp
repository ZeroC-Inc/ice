//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Configuration.h"
#include "Ice/Ice.h"
#include "PluginI.h"
#include "TestHelper.h"
#include "TestI.h"

#include "Ice/Locator.h"
#include "Ice/Router.h"

#ifdef _MSC_VER
#    pragma comment(lib, ICE_LIBNAME("testtransport"))
#endif

using namespace std;
using namespace Test;
using namespace Ice;

extern "C"
{
    Plugin* createTestTransport(const CommunicatorPtr&, const std::string&, const StringSeq&);
};

class LocatorI final : public Locator
{
public:
    void findAdapterByIdAsync(
        string,
        function<void(const optional<ObjectPrx>&)> response,
        function<void(exception_ptr)>,
        const Current& current) const final
    {
        _controller->checkCallPause(current);
        CommunicatorPtr communicator = current.adapter->getCommunicator();
        response(current.adapter->createDirectProxy(stringToIdentity("dummy")));
    }

    void findObjectByIdAsync(
        Identity id,
        function<void(const optional<ObjectPrx>&)> response,
        function<void(exception_ptr)>,
        const Current& current) const final
    {
        _controller->checkCallPause(current);
        CommunicatorPtr communicator = current.adapter->getCommunicator();
        response(current.adapter->createDirectProxy(id));
    }

    optional<LocatorRegistryPrx> getRegistry(const Current&) const { return nullopt; }

    LocatorI(const BackgroundControllerIPtr& controller) : _controller(controller) {}

private:
    BackgroundControllerIPtr _controller;
};

class RouterI final : public Router
{
public:
    optional<ObjectPrx> getClientProxy(optional<bool>& hasRoutingTable, const Current& current) const
    {
        hasRoutingTable = true;
        _controller->checkCallPause(current);
        return nullopt;
    }

    optional<ObjectPrx> getServerProxy(const Current& current) const final
    {
        _controller->checkCallPause(current);
        return nullopt;
    }

    ObjectProxySeq addProxies(ObjectProxySeq, const Current&) final { return ObjectProxySeq(); }

    RouterI(const BackgroundControllerIPtr& controller) { _controller = controller; }

private:
    BackgroundControllerIPtr _controller;
};

class Server : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Server::run(int argc, char** argv)
{
#ifdef ICE_STATIC_LIBS
    registerPluginFactory("Test", createTestTransport, false);
#endif
    PropertiesPtr properties = createTestProperties(argc, argv);

    //
    // This test kills connections, so we don't want warnings.
    //
    properties->setProperty("Ice.Warn.Connections", "0");

    properties->setProperty("Ice.MessageSizeMax", "50000");

    //
    // This test relies on filling the TCP send/recv buffer, so
    // we rely on a fixed value for these buffers.
    //
    properties->setProperty("Ice.TCP.RcvSize", "50000");

    //
    // Setup the test transport plug-in.
    //
    properties->setProperty("Ice.Plugin.Test", "TestTransport:createTestTransport");
    string defaultProtocol = properties->getPropertyWithDefault("Ice.Default.Protocol", "tcp");
    properties->setProperty("Ice.Default.Protocol", "test-" + defaultProtocol);

    CommunicatorHolder communicator = initialize(argc, argv, properties);

    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
    communicator->getProperties()->setProperty("ControllerAdapter.Endpoints", getTestEndpoint(1, "tcp"));
    communicator->getProperties()->setProperty("ControllerAdapter.ThreadPool.Size", "1");

    ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    ObjectAdapterPtr adapter2 = communicator->createObjectAdapter("ControllerAdapter");

    shared_ptr<PluginI> plugin = dynamic_pointer_cast<PluginI>(communicator->getPluginManager()->getPlugin("Test"));
    assert(plugin);
    ConfigurationPtr configuration = plugin->getConfiguration();
    BackgroundControllerIPtr backgroundController = make_shared<BackgroundControllerI>(adapter, configuration);

    adapter->add(make_shared<BackgroundI>(backgroundController), stringToIdentity("background"));
    adapter->add(make_shared<LocatorI>(backgroundController), stringToIdentity("locator"));
    adapter->add(make_shared<RouterI>(backgroundController), stringToIdentity("router"));
    adapter->activate();

    adapter2->add(backgroundController, stringToIdentity("backgroundController"));
    adapter2->activate();

    communicator->waitForShutdown();
}

DEFINE_TEST(Server)
