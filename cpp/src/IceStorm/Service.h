// Copyright (c) ZeroC, Inc.

#ifndef ICESTORM_SERVICE_H
#define ICESTORM_SERVICE_H

#include "IceBox/IceBox.h"
#include "IceStorm/IceStorm.h"

// These IceStorm APIs are exported because they are used by IceGrid
#ifndef ICESTORM_SERVICE_API
#    if defined(ICESTORM_SERVICE_API_EXPORTS)
#        define ICESTORM_SERVICE_API ICE_DECLSPEC_EXPORT
#    else
#        define ICESTORM_SERVICE_API ICE_DECLSPEC_IMPORT
#    endif
#endif

#if defined(_MSC_VER) && !defined(ICESTORM_SERVICE_API_EXPORTS)
#    pragma comment(lib, ICE_LIBNAME("IceStormService")) // Automatically link with IceStormService[D].lib
#endif

// This API is internal to Ice, and should not be used by external applications.
namespace IceStormInternal
{
    class Service : public IceBox::Service
    {
    public:
        ICESTORM_SERVICE_API static std::shared_ptr<Service> create(
            const Ice::CommunicatorPtr&,
            const Ice::ObjectAdapterPtr&,
            const Ice::ObjectAdapterPtr&,
            const std::string&,
            const Ice::Identity&);

        [[nodiscard]] ICESTORM_SERVICE_API virtual IceStorm::TopicManagerPrx getTopicManager() const = 0;
    };
}

#endif
