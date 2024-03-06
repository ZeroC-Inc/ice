//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __Ice_Communicator_h__
#define __Ice_Communicator_h__

#include <IceUtil/PushDisableWarnings.h>
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/ValueF.h>
#include <Ice/Exception.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Comparable.h>
#include <Ice/Proxy.h>
#include <Ice/Object.h>
#include <Ice/Value.h>
#include <Ice/Incoming.h>
#include <Ice/FactoryTableInit.h>
#include <optional>
#include <Ice/ExceptionHelpers.h>
#include <Ice/LoggerF.h>
#include <Ice/InstrumentationF.h>
#include <Ice/ObjectAdapterF.h>
#include <Ice/ValueFactory.h>
#include <Ice/PluginF.h>
#include <Ice/ImplicitContextF.h>
#include <Ice/Current.h>
#include <Ice/Properties.h>
#include <Ice/FacetMap.h>
#include <Ice/Connection.h>
#include <IceUtil/UndefSysMacros.h>

#ifndef ICE_API
#   if defined(ICE_STATIC_LIBS)
#       define ICE_API /**/
#   elif defined(ICE_API_EXPORTS)
#       define ICE_API ICE_DECLSPEC_EXPORT
#   else
#       define ICE_API ICE_DECLSPEC_IMPORT
#   endif
#endif

namespace Ice
{

class Communicator;

}

namespace Ice
{

/**
 * The output mode for xxxToString method such as identityToString and proxyToString. The actual encoding format for
 * the string is the same for all modes: you don't need to specify an encoding format or mode when reading such a
 * string.
 */
enum class ToStringMode : unsigned char
{
    /**
     * Characters with ordinal values greater than 127 are kept as-is in the resulting string. Non-printable ASCII
     * characters with ordinal values 127 and below are encoded as \\t, \\n (etc.) or \\unnnn.
     */
    Unicode,
    /**
     * Characters with ordinal values greater than 127 are encoded as universal character names in the resulting
     * string: \\unnnn for BMP characters and \\Unnnnnnnn for non-BMP characters. Non-printable ASCII characters
     * with ordinal values 127 and below are encoded as \\t, \\n (etc.) or \\unnnn.
     */
    ASCII,
    /**
     * Characters with ordinal values greater than 127 are encoded as a sequence of UTF-8 bytes using octal escapes.
     * Characters with ordinal values 127 and below are encoded as \\t, \\n (etc.) or an octal escape. Use this mode
     * to generate strings compatible with Ice 3.6 and earlier.
     */
    Compat
};

}

namespace Ice
{

/**
 * The central object in Ice. One or more communicators can be instantiated for an Ice application. Communicator
 * instantiation is language-specific, and not specified in Slice code.
 * @see Logger
 * @see ObjectAdapter
 * @see Properties
 * @see ValueFactory
 * \headerfile Ice/Ice.h
 */
class ICE_CLASS(ICE_API) Communicator
{
public:

    ICE_MEMBER(ICE_API) virtual ~Communicator();

    /**
     * Destroy the communicator. This operation calls {@link #shutdown} implicitly. Calling {@link #destroy} cleans up
     * memory, and shuts down this communicator's client functionality and destroys all object adapters. Subsequent
     * calls to {@link #destroy} are ignored.
     * @see #shutdown
     * @see ObjectAdapter#destroy
     */
    virtual void destroy() noexcept = 0;

    /**
     * Shuts down this communicator's server functionality, which includes the deactivation of all object adapters.
     * Attempts to use a deactivated object adapter raise ObjectAdapterDeactivatedException. Subsequent calls to
     * shutdown are ignored.
     * After shutdown returns, no new requests are processed. However, requests that have been started before shutdown
     * was called might still be active. You can use {@link #waitForShutdown} to wait for the completion of all
     * requests.
     * @see #destroy
     * @see #waitForShutdown
     * @see ObjectAdapter#deactivate
     */
    virtual void shutdown() noexcept = 0;

    /**
     * Wait until the application has called {@link #shutdown} (or {@link #destroy}). On the server side, this
     * operation blocks the calling thread until all currently-executing operations have completed. On the client
     * side, the operation simply blocks until another thread has called {@link #shutdown} or {@link #destroy}.
     * A typical use of this operation is to call it from the main thread, which then waits until some other thread
     * calls {@link #shutdown}. After shut-down is complete, the main thread returns and can do some cleanup work
     * before it finally calls {@link #destroy} to shut down the client functionality, and then exits the application.
     * @see #shutdown
     * @see #destroy
     * @see ObjectAdapter#waitForDeactivate
     */
    virtual void waitForShutdown() noexcept = 0;

    /**
     * Check whether communicator has been shut down.
     * @return True if the communicator has been shut down; false otherwise.
     * @see #shutdown
     */
    virtual bool isShutdown() const noexcept = 0;

    /**
     * Convert a stringified proxy into a proxy.
     * For example, <code>MyCategory/MyObject:tcp -h some_host -p 10000</code> creates a proxy that refers to the Ice
     * object having an identity with a name "MyObject" and a category "MyCategory", with the server running on host
     * "some_host", port 10000. If the stringified proxy does not parse correctly, the operation throws one of
     * ProxyParseException, EndpointParseException, or IdentityParseException. Refer to the Ice manual for a detailed
     * description of the syntax supported by stringified proxies.
     * @param str The stringified proxy to convert into a proxy.
     * @return The proxy, or nullopt if <code>str</code> is an empty string.
     * @see #proxyToString
     */
    virtual std::optional<ObjectPrx> stringToProxy(const ::std::string& str) const = 0;

    /**
     * Convert a proxy into a string.
     * @param obj The proxy to convert into a stringified proxy.
     * @return The stringified proxy, or an empty string if
     * <code>obj</code> is nil.
     * @see #stringToProxy
     */
    virtual ::std::string proxyToString(const std::optional<ObjectPrx>& obj) const = 0;

    /**
     * Convert a set of proxy properties into a proxy. The "base" name supplied in the <code>property</code> argument
     * refers to a property containing a stringified proxy, such as <code>MyProxy=id:tcp -h localhost -p 10000</code>.
     * Additional properties configure local settings for the proxy, such as <code>MyProxy.PreferSecure=1</code>. The
     * "Properties" appendix in the Ice manual describes each of the supported proxy properties.
     * @param property The base property name.
     * @return The proxy.
     */
    virtual std::optional<ObjectPrx> propertyToProxy(const ::std::string& property) const = 0;

    /**
     * Convert a proxy to a set of proxy properties.
     * @param proxy The proxy.
     * @param property The base property name.
     * @return The property set.
     */
    virtual ::Ice::PropertyDict proxyToProperty(const std::optional<ObjectPrx>& proxy, const ::std::string& property) const = 0;

    /**
     * Convert an identity into a string.
     * @param ident The identity to convert into a string.
     * @return The "stringified" identity.
     */
    virtual ::std::string identityToString(const Identity& ident) const = 0;

    /**
     * Create a new object adapter. The endpoints for the object adapter are taken from the property
     * <code><em>name</em>.Endpoints</code>.
     * It is legal to create an object adapter with the empty string as its name. Such an object adapter is accessible
     * via bidirectional connections or by collocated invocations that originate from the same communicator as is used
     * by the adapter. Attempts to create a named object adapter for which no configuration can be found raise
     * InitializationException.
     * @param name The object adapter name.
     * @return The new object adapter.
     * @see #createObjectAdapterWithEndpoints
     * @see ObjectAdapter
     * @see Properties
     */
    virtual ::std::shared_ptr<::Ice::ObjectAdapter> createObjectAdapter(const ::std::string& name) = 0;

    /**
     * Create a new object adapter with endpoints. This operation sets the property
     * <code><em>name</em>.Endpoints</code>, and then calls {@link #createObjectAdapter}. It is provided as a
     * convenience function. Calling this operation with an empty name will result in a UUID being generated for the
     * name.
     * @param name The object adapter name.
     * @param endpoints The endpoints for the object adapter.
     * @return The new object adapter.
     * @see #createObjectAdapter
     * @see ObjectAdapter
     * @see Properties
     */
    virtual ::std::shared_ptr<::Ice::ObjectAdapter> createObjectAdapterWithEndpoints(const ::std::string& name, const ::std::string& endpoints) = 0;

    /**
     * Create a new object adapter with a router. This operation creates a routed object adapter.
     * Calling this operation with an empty name will result in a UUID being generated for the name.
     * @param name The object adapter name.
     * @param rtr The router.
     * @return The new object adapter.
     * @see #createObjectAdapter
     * @see ObjectAdapter
     * @see Properties
     */
    virtual ::std::shared_ptr<::Ice::ObjectAdapter> createObjectAdapterWithRouter(const ::std::string& name, const RouterPrx& rtr) = 0;

    /**
     * Get the implicit context associated with this communicator.
     * @return The implicit context associated with this communicator; returns null when the property Ice.ImplicitContext
     * is not set or is set to None.
     */
    virtual ::std::shared_ptr<::Ice::ImplicitContext> getImplicitContext() const noexcept = 0;

    /**
     * Get the properties for this communicator.
     * @return This communicator's properties.
     * @see Properties
     */
    virtual ::std::shared_ptr<::Ice::Properties> getProperties() const noexcept = 0;

    /**
     * Get the logger for this communicator.
     * @return This communicator's logger.
     * @see Logger
     */
    virtual ::std::shared_ptr<::Ice::Logger> getLogger() const noexcept = 0;

    /**
     * Get the observer resolver object for this communicator.
     * @return This communicator's observer resolver object.
     */
    virtual ::std::shared_ptr<::Ice::Instrumentation::CommunicatorObserver> getObserver() const noexcept = 0;

    /**
     * Get the default router for this communicator.
     * @return The default router for this communicator.
     * @see #setDefaultRouter
     * @see Router
     */
    virtual std::optional<RouterPrx> getDefaultRouter() const = 0;

    /**
     * Set a default router for this communicator. All newly created proxies will use this default router. To disable
     * the default router, null can be used. Note that this operation has no effect on existing proxies.
     * You can also set a router for an individual proxy by calling the operation <code>ice_router</code> on the
     * proxy.
     * @param rtr The default router to use for this communicator.
     * @see #getDefaultRouter
     * @see #createObjectAdapterWithRouter
     * @see Router
     */
    virtual void setDefaultRouter(const std::optional<RouterPrx>& rtr) = 0;

    /**
     * Get the default locator for this communicator.
     * @return The default locator for this communicator.
     * @see #setDefaultLocator
     * @see Locator
     */
    virtual std::optional<Ice::LocatorPrx> getDefaultLocator() const = 0;

    /**
     * Set a default Ice locator for this communicator. All newly created proxy and object adapters will use this
     * default locator. To disable the default locator, null can be used. Note that this operation has no effect on
     * existing proxies or object adapters.
     * You can also set a locator for an individual proxy by calling the operation <code>ice_locator</code> on the
     * proxy, or for an object adapter by calling {@link ObjectAdapter#setLocator} on the object adapter.
     * @param loc The default locator to use for this communicator.
     * @see #getDefaultLocator
     * @see Locator
     * @see ObjectAdapter#setLocator
     */
    virtual void setDefaultLocator(const std::optional<LocatorPrx>& loc) = 0;

    /**
     * Get the plug-in manager for this communicator.
     * @return This communicator's plug-in manager.
     * @see PluginManager
     */
    virtual ::std::shared_ptr<::Ice::PluginManager> getPluginManager() const = 0;

    /**
     * Get the value factory manager for this communicator.
     * @return This communicator's value factory manager.
     * @see ValueFactoryManager
     */
    virtual ::std::shared_ptr<::Ice::ValueFactoryManager> getValueFactoryManager() const noexcept = 0;

    /**
     * Flush any pending batch requests for this communicator. This means all batch requests invoked on fixed proxies
     * for all connections associated with the communicator. Any errors that occur while flushing a connection are
     * ignored.
     * @param compress Specifies whether or not the queued batch requests should be compressed before being sent over
     * the wire.
     */
    ICE_MEMBER(ICE_API) void flushBatchRequests(CompressBatch compress);

    /**
     * Flush any pending batch requests for this communicator. This means all batch requests invoked on fixed proxies
     * for all connections associated with the communicator. Any errors that occur while flushing a connection are
     * ignored.
     * @param compress Specifies whether or not the queued batch requests should be compressed before being sent over
     * the wire.
     * @param exception The exception callback.
     * @param sent The sent callback.
     * @return A function that can be called to cancel the invocation locally.
     */
    virtual ::std::function<void()>
    flushBatchRequestsAsync(CompressBatch compress,
                            ::std::function<void(::std::exception_ptr)> exception,
                            ::std::function<void(bool)> sent = nullptr) = 0;

    /**
     * Flush any pending batch requests for this communicator. This means all batch requests invoked on fixed proxies
     * for all connections associated with the communicator. Any errors that occur while flushing a connection are
     * ignored.
     * @param compress Specifies whether or not the queued batch requests should be compressed before being sent over
     * the wire.
     * @return The future object for the invocation.
     */
    ICE_MEMBER(ICE_API) std::future<void> flushBatchRequestsAsync(CompressBatch compress);

    /**
     * Add the Admin object with all its facets to the provided object adapter. If <code>Ice.Admin.ServerId</code> is
     * set and the provided object adapter has a {@link Locator}, createAdmin registers the Admin's Process facet with
     * the {@link Locator}'s {@link LocatorRegistry}. createAdmin must only be called once; subsequent calls raise
     * InitializationException.
     * @param adminAdapter The object adapter used to host the Admin object; if null and Ice.Admin.Endpoints is set,
     * create, activate and use the Ice.Admin object adapter.
     * @param adminId The identity of the Admin object.
     * @return A proxy to the main ("") facet of the Admin object.
     * @see #getAdmin
     */
    virtual ObjectPrx createAdmin(const ::std::shared_ptr<ObjectAdapter>& adminAdapter, const Identity& adminId) = 0;

    /**
     * Get a proxy to the main facet of the Admin object. getAdmin also creates the Admin object and creates and
     * activates the Ice.Admin object adapter to host this Admin object if Ice.Admin.Endpoints is set. The identity of
     * the Admin object created by getAdmin is {value of Ice.Admin.InstanceName}/admin, or {UUID}/admin when
     * <code>Ice.Admin.InstanceName</code> is not set. If Ice.Admin.DelayCreation is 0 or not set, getAdmin is called
     * by the communicator initialization, after initialization of all plugins.
     * @return A proxy to the main ("") facet of the Admin object, or nullopt if no Admin object is configured.
     * @see #createAdmin
     */
    virtual std::optional<ObjectPrx> getAdmin() const = 0;

    /**
     * Add a new facet to the Admin object. Adding a servant with a facet that is already registered throws
     * AlreadyRegisteredException.
     * @param servant The servant that implements the new Admin facet.
     * @param facet The name of the new Admin facet.
     */
    virtual void addAdminFacet(const ::std::shared_ptr<Object>& servant, const ::std::string& facet) = 0;

    /**
     * Remove the following facet to the Admin object. Removing a facet that was not previously registered throws
     * NotRegisteredException.
     * @param facet The name of the Admin facet.
     * @return The servant associated with this Admin facet.
     */
    virtual ::std::shared_ptr<::Ice::Object> removeAdminFacet(const ::std::string& facet) = 0;

    /**
     * Returns a facet of the Admin object.
     * @param facet The name of the Admin facet.
     * @return The servant associated with this Admin facet, or null if no facet is registered with the given name.
     */
    virtual ::std::shared_ptr<::Ice::Object> findAdminFacet(const ::std::string& facet) = 0;

    /**
     * Returns a map of all facets of the Admin object.
     * @return A collection containing all the facet names and servants of the Admin object.
     * @see #findAdminFacet
     */
    virtual ::Ice::FacetMap findAllAdminFacets() = 0;

#ifdef ICE_SWIFT
    /**
     * Returns the client dispatch queue.
     * @return The dispatch queue associated wih this Communicator's
     * client thread pool.
     */
    virtual dispatch_queue_t getClientDispatchQueue() const = 0;

    /**
     * Returns the server dispatch queue.
     * @return The dispatch queue associated wih the Communicator's
     * server thread pool.
     */
    virtual dispatch_queue_t getServerDispatchQueue() const = 0;
#endif

    virtual void postToClientThreadPool(::std::function<void()> call) = 0;
};

}

/// \cond STREAM
namespace Ice
{

}
/// \endcond

/// \cond INTERNAL
namespace Ice
{

using CommunicatorPtr = ::std::shared_ptr<Communicator>;

}
/// \endcond

#include <IceUtil/PopDisableWarnings.h>
#endif
