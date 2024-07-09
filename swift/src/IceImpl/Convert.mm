// Copyright (c) ZeroC, Inc.
#import "Convert.h"
#import "IceUtil.h"
#import "LocalExceptionFactory.h"

#include <cstdlib>
#include <cxxabi.h>
#include <sstream>
#include <typeinfo>

namespace
{
    inline std::string cxxDescription(const Ice::LocalException& e)
    {
        std::ostringstream os;
        os << e;
        return os.str();
    }
}

NSError*
convertException(std::exception_ptr exc)
{
    assert(exc);
    Class<ICELocalExceptionFactory> factory = [ICEUtil localExceptionFactory];

    try
    {
        rethrow_exception(exc);
    }
    catch (const Ice::AlreadyRegisteredException& e)
    {
        return [factory registeredException:toNSString(e.ice_id())
                               kindOfObject:toNSString(e.kindOfObject())
                                   objectId:toNSString(e.id())
                                    message:toNSString(e.what())
                             cxxDescription:toNSString(cxxDescription(e))
                                       file:toNSString(e.ice_file())
                                       line:e.ice_line()];
    }
    catch (const Ice::NotRegisteredException& e)
    {
        return [factory registeredException:toNSString(e.ice_id())
                               kindOfObject:toNSString(e.kindOfObject())
                                   objectId:toNSString(e.id())
                                    message:toNSString(e.what())
                             cxxDescription:toNSString(cxxDescription(e))
                                       file:toNSString(e.ice_file())
                                       line:e.ice_line()];
    }
    catch (const Ice::ConnectionAbortedException& e)
    {
        return [factory connectionClosedException:toNSString(e.ice_id())
                              closedByApplication:e.closedByApplication()
                                          message:toNSString(e.what())
                                   cxxDescription:toNSString(cxxDescription(e))
                                             file:toNSString(e.ice_file())
                                             line:e.ice_line()];
    }
    catch (const Ice::ConnectionClosedException& e)
    {
        return [factory connectionClosedException:toNSString(e.ice_id())
                              closedByApplication:e.closedByApplication()
                                          message:toNSString(e.what())
                                   cxxDescription:toNSString(cxxDescription(e))
                                             file:toNSString(e.ice_file())
                                             line:e.ice_line()];
    }
    catch (const Ice::RequestFailedException& e)
    {
        return [factory requestFailedException:toNSString(e.ice_id())
                                          name:toNSString(e.id().name)
                                      category:toNSString(e.id().category)
                                         facet:toNSString(e.facet())
                                     operation:toNSString(e.operation())
                                       message:toNSString(e.what())
                                cxxDescription:toNSString(cxxDescription(e))
                                          file:toNSString(e.ice_file())
                                          line:e.ice_line()];
    }
    catch (const Ice::LocalException& e)
    {
        return [factory localException:toNSString(e.ice_id())
                               message:toNSString(e.what())
                        cxxDescription:toNSString(cxxDescription(e))
                                  file:toNSString(e.ice_file())
                                  line:e.ice_line()];
    }
    catch (const std::exception& e)
    {
        int status = 0;
        const char* mangled = typeid(e).name();
        char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);

        NSError* error = nullptr;
        if (status == 0) // success
        {
            error = [factory cxxException:toNSString(demangled) message:toNSString(e.what())];
            std::free(demangled);
        }
        else
        {
            error = [factory cxxException:toNSString(mangled) message:toNSString(e.what())];
            assert(demangled == nullptr);
        }
        return error;
    }
    catch (...)
    {
        return [factory cxxException:toNSString("unknown C++ exception") message:toNSString("(no message)")];
    }
}

NSObject*
toObjC(const std::shared_ptr<Ice::Endpoint>& endpoint)
{
    return [ICEEndpoint getHandle:endpoint];
}

void
fromObjC(id object, std::shared_ptr<Ice::Endpoint>& endpoint)
{
    ICEEndpoint* endpt = object;
    endpoint = object == [NSNull null] ? nullptr : [endpt endpoint];
}
