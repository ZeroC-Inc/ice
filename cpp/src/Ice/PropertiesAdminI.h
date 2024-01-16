//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_PROPERTIES_ADMIN_I_H
#define ICE_PROPERTIES_ADMIN_I_H

#include <IceUtil/RecMutex.h>
#include <Ice/Properties.h>
#include <Ice/PropertiesAdmin.h>
#include <Ice/NativePropertiesAdmin.h>
#include <Ice/LoggerF.h>

#ifdef ICE_CPP11_MAPPING
#include <list>
#endif

namespace IceInternal
{

class PropertiesAdminI final : public Ice::PropertiesAdmin,
                               public Ice::NativePropertiesAdmin,
                               public std::enable_shared_from_this<PropertiesAdminI>,
                               private IceUtil::RecMutex
{
public:

    PropertiesAdminI(const InstancePtr&);

#ifdef ICE_CPP11_MAPPING
    std::string getProperty(std::string, const Ice::Current&) final;
    Ice::PropertyDict getPropertiesForPrefix(std::string, const Ice::Current&) final;
    void setProperties(::Ice::PropertyDict, const Ice::Current&) final;
#else
    std::string getProperty(const std::string&, const Ice::Current&) final;
    Ice::PropertyDict getPropertiesForPrefix(const std::string&, const Ice::Current&) final;
    void setProperties(const Ice::PropertyDict&, const Ice::Current&) final;
#endif

    std::function<void()> addUpdateCallback(std::function<void(const Ice::PropertyDict&)>) final;

private:

    void removeUpdateCallback(std::list<std::function<void(const Ice::PropertyDict&)>>::iterator);

    const Ice::PropertiesPtr _properties;
    const Ice::LoggerPtr _logger;

    std::list<std::function<void(const Ice::PropertyDict&)>> _updateCallbacks;
};
ICE_DEFINE_SHARED_PTR(PropertiesAdminIPtr, PropertiesAdminI);

}

#endif
