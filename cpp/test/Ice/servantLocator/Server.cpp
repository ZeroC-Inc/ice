//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <ServantLocatorI.h>
#include <TestHelper.h>
#include <TestI.h>

using namespace std;
using namespace Ice;

class ServantLocatorI : public Test::ServantLocatorI
{
public:

    ServantLocatorI(const string& category) : Test::ServantLocatorI(category)
    {
    }

protected:

    virtual shared_ptr<::Ice::Object>
    newServantAndCookie(shared_ptr<void>& cookie) const
    {
        cookie = make_shared<Cookie>();
        return make_shared<TestI>();
    }

    virtual void
    checkCookie(const shared_ptr<void>& cookie) const
    {
        auto co = static_pointer_cast<Cookie>(cookie);
        test(co);
        test(co->message() == "blahblah");
    }

    virtual void
    throwTestIntfUserException() const
    {
        throw Test::TestIntfUserException();
    }
};

class TestActivationI : public Test::TestActivation
{
public:

    void activateServantLocator(bool activate, const Ice::Current& current)
    {
        if(activate)
        {
            current.adapter->addServantLocator(std::make_shared<ServantLocatorI>(""), "");
            current.adapter->addServantLocator(std::make_shared<ServantLocatorI>("category"), "category");
        }
        else
        {
            ServantLocatorPtr locator = current.adapter->removeServantLocator("");
            locator->deactivate("");
            locator = current.adapter->removeServantLocator("category");
            locator->deactivate("category");
        }
    }
};

class Server : public Test::TestHelper
{
public:

    void run(int, char**);
};

void
Server::run(int argc, char** argv)
{
    Ice::CommunicatorHolder communicator = initialize(argc, argv);
    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint());
    communicator->getProperties()->setProperty("Ice.Warn.Dispatch", "0");

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");

    adapter->addServantLocator(std::make_shared<ServantLocatorI>(""), "");
    adapter->addServantLocator(std::make_shared<ServantLocatorI>("category"), "category");
    adapter->add(std::make_shared<TestI>(), Ice::stringToIdentity("asm"));
    adapter->add(std::make_shared<TestActivationI>(), Ice::stringToIdentity("test/activation"));
    adapter->activate();
    serverReady();
    adapter->waitForDeactivate();
}

DEFINE_TEST(Server)
