//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#import "Communicator.h"
#import "DispatchAdapter.h"
#import "IceUtil.h"
#import "ImplicitContext.h"
#import "Logger.h"
#import "ObjectAdapter.h"
#import "ObjectPrx.h"
#import "Process.h"
#import "Properties.h"
#import "PropertiesAdmin.h"
#import "UnsupportedAdminFacet.h"

#import "Convert.h"
#import "LoggerWrapperI.h"

#include "Ice/DefaultsAndOverrides.h"
#include "Ice/Instance.h"

@implementation ICECommunicator

- (std::shared_ptr<Ice::Communicator>)communicator
{
    return std::static_pointer_cast<Ice::Communicator>(self.cppObject);
}

- (void)destroy
{
    self.communicator->destroy();
}

- (void)shutdown
{
    self.communicator->shutdown();
}

- (void)waitForShutdown
{
    self.communicator->waitForShutdown();
}

- (bool)isShutdown
{
    return self.communicator->isShutdown();
}

- (id)stringToProxy:(NSString*)str error:(NSError**)error
{
    try
    {
        auto prx = self.communicator->stringToProxy(fromNSString(str));
        if (prx)
        {
            return [[ICEObjectPrx alloc] initWithCppObjectPrx:prx.value()];
        }
        return [NSNull null];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (nullable id)propertyToProxy:(NSString*)property error:(NSError* _Nullable* _Nullable)error
{
    try
    {
        auto prx = self.communicator->propertyToProxy(fromNSString(property));
        if (prx)
        {
            return [[ICEObjectPrx alloc] initWithCppObjectPrx:prx.value()];
        }
        return [NSNull null];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (NSDictionary<NSString*, NSString*>*)proxyToProperty:(ICEObjectPrx*)prx
                                              property:(NSString*)property
                                                 error:(NSError* _Nullable* _Nullable)error
{
    return toNSDictionary(self.communicator->proxyToProperty([prx prx], fromNSString(property)));
}

- (ICEObjectAdapter*)createObjectAdapter:(NSString*)name error:(NSError* _Nullable* _Nullable)error
{
    try
    {
        auto oa = self.communicator->createObjectAdapter(fromNSString(name));
        return [ICEObjectAdapter getHandle:oa];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (ICEObjectAdapter*)createObjectAdapterWithEndpoints:(NSString*)name
                                            endpoints:(NSString*)endpoints
                                                error:(NSError* _Nullable* _Nullable)error
{
    try
    {
        auto oa = self.communicator->createObjectAdapterWithEndpoints(fromNSString(name), fromNSString(endpoints));
        return [ICEObjectAdapter getHandle:oa];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (ICEObjectAdapter*)createObjectAdapterWithRouter:(NSString*)name
                                            router:(ICEObjectPrx*)router
                                             error:(NSError* _Nullable* _Nullable)error
{
    try
    {
        assert(router);
        auto oa = self.communicator->createObjectAdapterWithRouter(
            fromNSString(name),
            Ice::uncheckedCast<Ice::RouterPrx>([router prx]).value());
        return [ICEObjectAdapter getHandle:oa];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (ICEImplicitContext*)getImplicitContext
{
    auto implicitContext = self.communicator->getImplicitContext();
    return [ICEImplicitContext getHandle:implicitContext];
}

// id<ICELoggerProtocol> may be either a Swift logger or a wrapper around a C++ logger
- (id<ICELoggerProtocol>)getLogger
{
    auto logger = self.communicator->getLogger();

    auto swiftLogger = std::dynamic_pointer_cast<LoggerWrapperI>(logger);
    if (swiftLogger)
    {
        return swiftLogger->getLogger();
    }

    return [ICELogger getHandle:logger];
}

- (nullable ICEObjectPrx*)getDefaultRouter
{
    std::optional<Ice::RouterPrx> router = self.communicator->getDefaultRouter();
    if (router)
    {
        return [[ICEObjectPrx alloc] initWithCppObjectPrx:router.value()];
    }
    else
    {
        return nil;
    }
}

- (BOOL)setDefaultRouter:(ICEObjectPrx*)router error:(NSError**)error
{
    try
    {
        std::optional<Ice::ObjectPrx> r;
        if (router)
        {
            r = [router prx];
        }
        self.communicator->setDefaultRouter(Ice::uncheckedCast<Ice::RouterPrx>(r));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (nullable ICEObjectPrx*)getDefaultLocator
{
    std::optional<Ice::LocatorPrx> locator = self.communicator->getDefaultLocator();
    if (locator)
    {
        return [[ICEObjectPrx alloc] initWithCppObjectPrx:locator.value()];
    }
    else
    {
        return nil;
    }
}

- (BOOL)setDefaultLocator:(ICEObjectPrx*)locator error:(NSError**)error
{
    try
    {
        std::optional<Ice::ObjectPrx> l;
        if (locator)
        {
            l = [locator prx];
        }
        self.communicator->setDefaultLocator((Ice::uncheckedCast<Ice::LocatorPrx>(l)));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (BOOL)flushBatchRequests:(std::uint8_t)compress error:(NSError**)error
{
    try
    {
        self.communicator->flushBatchRequests(Ice::CompressBatch(compress));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (void)flushBatchRequestsAsync:(std::uint8_t)compress
                      exception:(void (^)(NSError*))exception
                           sent:(void (^_Nullable)(bool))sent
{
    try
    {
        self.communicator->flushBatchRequestsAsync(
            Ice::CompressBatch(compress),
            [exception](std::exception_ptr e)
            {
                @autoreleasepool
                {
                    exception(convertException(e));
                }
            },
            [sent](bool sentSynchronously)
            {
                if (sent)
                {
                    sent(sentSynchronously);
                }
            });
    }
    catch (...)
    {
        // Typically CommunicatorDestroyedException. Note that the callback is called on the
        // thread making the invocation, which is fine since we only use it to fulfill the
        // PromiseKit promise.
        exception(convertException(std::current_exception()));
    }
}

- (nullable ICEObjectPrx*)createAdmin:(ICEObjectAdapter* _Nullable)adminAdapter
                                 name:(NSString*)name
                             category:(NSString*)category
                                error:(NSError**)error
{
    try
    {
        auto ident = Ice::Identity{fromNSString(name), fromNSString(category)};
        auto adapter = adminAdapter ? [adminAdapter objectAdapter] : nullptr;
        auto prx = self.communicator->createAdmin(adapter, ident);
        return [[ICEObjectPrx alloc] initWithCppObjectPrx:prx];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (nullable id)getAdmin:(NSError**)error
{
    try
    {
        auto adminPrx = self.communicator->getAdmin();
        return adminPrx ? [[ICEObjectPrx alloc] initWithCppObjectPrx:adminPrx.value()] : [NSNull null];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (BOOL)addAdminFacet:(id<ICEDispatchAdapter>)dispatchAdapter facet:(NSString*)facet error:(NSError**)error
{
    try
    {
        auto cppDispatcher = std::make_shared<CppDispatcher>(dispatchAdapter);
        self.communicator->addAdminFacet(cppDispatcher, fromNSString(facet));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (id<ICEDispatchAdapter>)removeAdminFacet:(NSString*)facet error:(NSError**)error
{
    try
    {
        // servant can either be a Swift wrapped facet or a builtin admin facet
        return [self facetToDispatchAdapter:self.communicator->removeAdminFacet(fromNSString(facet))];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (nullable id)findAdminFacet:(NSString*)facet error:(NSError**)error
{
    try
    {
        // servant can either be null, a Swift wrapped facet, or a builtin admin facet
        auto servant = self.communicator->findAdminFacet(fromNSString(facet));

        if (!servant)
        {
            return [NSNull null];
        }

        return [self facetToDispatchAdapter:servant];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (nullable NSDictionary<NSString*, id<ICEDispatchAdapter>>*)findAllAdminFacets:(NSError**)error
{
    try
    {
        NSMutableDictionary<NSString*, id<ICEDispatchAdapter>>* facets = [NSMutableDictionary dictionary];

        for (const auto& d : self.communicator->findAllAdminFacets())
        {
            [facets setObject:[self facetToDispatchAdapter:d.second] forKey:toNSString(d.first)];
        }

        return facets;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (ICEProperties*)getProperties
{
    auto props = self.communicator->getProperties();
    return [ICEProperties getHandle:props];
}

- (nullable dispatch_queue_t)getClientDispatchQueue:(NSError* _Nullable* _Nullable)error
{
    try
    {
        return self.communicator->getClientDispatchQueue();
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (nullable dispatch_queue_t)getServerDispatchQueue:(NSError* _Nullable* _Nullable)error
{
    try
    {
        return self.communicator->getServerDispatchQueue();
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (void)getDefaultEncoding:(std::uint8_t*)major minor:(std::uint8_t*)minor
{
    auto defaultEncoding = IceInternal::getInstance(self.communicator)->defaultsAndOverrides()->defaultEncoding;
    *major = defaultEncoding.major;
    *minor = defaultEncoding.minor;
}

- (std::uint8_t)getDefaultFormat
{
    return static_cast<std::uint8_t>(
        IceInternal::getInstance(self.communicator)->defaultsAndOverrides()->defaultFormat);
}

- (id<ICEDispatchAdapter>)facetToDispatchAdapter:(const Ice::ObjectPtr&)servant
{
    if (!servant)
    {
        return nil;
    }

    auto cppDispatcher = std::dynamic_pointer_cast<CppDispatcher>(servant);
    if (cppDispatcher)
    {
        return cppDispatcher->dispatchAdapter();
    }

    Class<ICEAdminFacetFactory> factory = [ICEUtil adminFacetFactory];

    auto process = std::dynamic_pointer_cast<Ice::Process>(servant);
    if (process)
    {
        return [factory createProcess:self handle:[ICEProcess getHandle:process]];
    }

    auto propertiesAdmin = std::dynamic_pointer_cast<Ice::PropertiesAdmin>(servant);
    if (propertiesAdmin)
    {
        return [factory createProperties:self handle:[ICEPropertiesAdmin getHandle:propertiesAdmin]];
    }

    return [factory createUnsupported:self handle:[ICEUnsupportedAdminFacet getHandle:servant]];
}

- (BOOL)initializePlugins:(NSError**)error
{
    try
    {
        self.communicator->getPluginManager()->initializePlugins();
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}
@end
