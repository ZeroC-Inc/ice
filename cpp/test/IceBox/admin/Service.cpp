//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>
#include <IceBox/IceBox.h>
#include <TestI.h>

using namespace std;
using namespace Ice;

class ServiceI : public ::IceBox::Service
{
public:

    ServiceI(const CommunicatorPtr&);
    virtual ~ServiceI();

    virtual void start(const string&,
                       const CommunicatorPtr&,
                       const StringSeq&);

    virtual void stop();
};

extern "C"
{

//
// Factory function
//
ICE_DECLSPEC_EXPORT ::IceBox::Service*
create(const shared_ptr<Communicator>& communicator)
{
    return new ServiceI(communicator);
}

}

ServiceI::ServiceI(const CommunicatorPtr& serviceManagerCommunicator)
{
    TestFacetIPtr facet = std::make_shared<TestFacetI>();

    //
    // Install a custom admin facet.
    //
    serviceManagerCommunicator->addAdminFacet(facet, "TestFacet");

    //
    // The TestFacetI servant also implements PropertiesAdminUpdateCallback.
    // Set the callback on the admin facet.
    //
    ObjectPtr propFacet = serviceManagerCommunicator->findAdminFacet("IceBox.Service.TestService.Properties");
    NativePropertiesAdminPtr admin = ICE_DYNAMIC_CAST(NativePropertiesAdmin, propFacet);
    assert(admin);

    admin->addUpdateCallback([facet](const Ice::PropertyDict& changes) { facet->updated(changes); });
}

ServiceI::~ServiceI()
{
}

void
ServiceI::start(const string&, const CommunicatorPtr&, const StringSeq&)
{
}

void
ServiceI::stop()
{
}
