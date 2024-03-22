//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_OBJECT_ADAPTER_H
#define ICE_OBJECT_ADAPTER_H

#include "CommunicatorF.h"
#include "Endpoint.h"
#include "FacetMap.h"
#include "ServantLocator.h"

#ifdef ICE_SWIFT
#    include <dispatch/dispatch.h>
#endif

#include <memory>
#include <optional>

namespace Ice
{
    class LocatorPrx;
    class ObjectPrx;

    /**
     * The object adapter provides an up-call interface from the Ice run time to the implementation of Ice objects. The
     * object adapter is responsible for receiving requests from endpoints, and for mapping between servants,
     * identities, and proxies.
     * @see Communicator
     * @see ServantLocator
     * \headerfile Ice/Ice.h
     */
    class ObjectAdapter
    {
    public:
        virtual ~ObjectAdapter() = default;

        /**
         * Get the name of this object adapter.
         * @return This object adapter's name.
         */
        virtual std::string getName() const noexcept = 0;

        /**
         * Get the communicator this object adapter belongs to.
         * @return This object adapter's communicator.
         * @see Communicator
         */
        virtual CommunicatorPtr getCommunicator() const noexcept = 0;

        /**
         * Activate all endpoints that belong to this object adapter. After activation, the object adapter can dispatch
         * requests received through its endpoints.
         * @see #hold
         * @see #deactivate
         */
        virtual void activate() = 0;

        /**
         * Temporarily hold receiving and dispatching requests. The object adapter can be reactivated with the
         * {@link #activate} operation. <p class="Note"> Holding is not immediate, i.e., after {@link #hold} returns,
         * the object adapter might still be active for some time. You can use {@link #waitForHold} to wait until
         * holding is complete.
         * @see #activate
         * @see #deactivate
         * @see #waitForHold
         */
        virtual void hold() = 0;

        /**
         * Wait until the object adapter holds requests. Calling {@link #hold} initiates holding of requests, and
         * {@link #waitForHold} only returns when holding of requests has been completed.
         * @see #hold
         * @see #waitForDeactivate
         * @see Communicator#waitForShutdown
         */
        virtual void waitForHold() = 0;

        /**
         * Deactivate all endpoints that belong to this object adapter. After deactivation, the object adapter stops
         * receiving requests through its endpoints. Object adapters that have been deactivated must not be reactivated
         * again, and cannot be used otherwise. Attempts to use a deactivated object adapter raise
         * {@link ObjectAdapterDeactivatedException} however, attempts to {@link #deactivate} an already deactivated
         * object adapter are ignored and do nothing. Once deactivated, it is possible to destroy the adapter to clean
         * up resources and then create and activate a new adapter with the same name. <p class="Note"> After {@link
         * #deactivate} returns, no new requests are processed by the object adapter. However, requests that have been
         * started before {@link #deactivate} was called might still be active. You can use {@link #waitForDeactivate}
         * to wait for the completion of all requests for this object adapter.
         * @see #activate
         * @see #hold
         * @see #waitForDeactivate
         * @see Communicator#shutdown
         */
        virtual void deactivate() noexcept = 0;

        /**
         * Wait until the object adapter has deactivated. Calling {@link #deactivate} initiates object adapter
         * deactivation, and {@link #waitForDeactivate} only returns when deactivation has been completed.
         * @see #deactivate
         * @see #waitForHold
         * @see Communicator#waitForShutdown
         */
        virtual void waitForDeactivate() noexcept = 0;

        /**
         * Check whether object adapter has been deactivated.
         * @return Whether adapter has been deactivated.
         * @see Communicator#shutdown
         */
        virtual bool isDeactivated() const noexcept = 0;

        /**
         * Destroys the object adapter and cleans up all resources held by the object adapter. If the object adapter has
         * not yet been deactivated, destroy implicitly initiates the deactivation and waits for it to finish.
         * Subsequent calls to destroy are ignored. Once destroy has returned, it is possible to create another object
         * adapter with the same name.
         * @see #deactivate
         * @see #waitForDeactivate
         * @see Communicator#destroy
         */
        virtual void destroy() noexcept = 0;

        /**
         * Add a servant to this object adapter's Active Servant Map. Note that one servant can implement several Ice
         * objects by registering the servant with multiple identities. Adding a servant with an identity that is in the
         * map already throws {@link AlreadyRegisteredException}.
         * @param servant The servant to add.
         * @param id The identity of the Ice object that is implemented by the servant.
         * @return A proxy that matches the given identity and this object adapter.
         * @see Identity
         * @see #addFacet
         * @see #addWithUUID
         * @see #remove
         * @see #find
         */
        virtual ObjectPrx add(const ObjectPtr& servant, const Identity& id) = 0;

        /**
         * Like {@link #add}, but with a facet. Calling <code>add(servant, id)</code> is equivalent to calling
         * {@link #addFacet} with an empty facet.
         * @param servant The servant to add.
         * @param id The identity of the Ice object that is implemented by the servant.
         * @param facet The facet. An empty facet means the default facet.
         * @return A proxy that matches the given identity, facet, and this object adapter.
         * @see Identity
         * @see #add
         * @see #addFacetWithUUID
         * @see #removeFacet
         * @see #findFacet
         */
        virtual ObjectPrx addFacet(const ObjectPtr& servant, const Identity& id, const std::string& facet) = 0;

        /**
         * Add a servant to this object adapter's Active Servant Map, using an automatically generated UUID as its
         * identity. Note that the generated UUID identity can be accessed using the proxy's
         * <code>ice_getIdentity</code> operation.
         * @param servant The servant to add.
         * @return A proxy that matches the generated UUID identity and this object adapter.
         * @see Identity
         * @see #add
         * @see #addFacetWithUUID
         * @see #remove
         * @see #find
         */
        virtual ObjectPrx addWithUUID(const ObjectPtr& servant) = 0;

        /**
         * Like {@link #addWithUUID}, but with a facet. Calling <code>addWithUUID(servant)</code> is equivalent to
         * calling
         * {@link #addFacetWithUUID} with an empty facet.
         * @param servant The servant to add.
         * @param facet The facet. An empty facet means the default facet.
         * @return A proxy that matches the generated UUID identity, facet, and this object adapter.
         * @see Identity
         * @see #addFacet
         * @see #addWithUUID
         * @see #removeFacet
         * @see #findFacet
         */
        virtual ObjectPrx addFacetWithUUID(const ObjectPtr& servant, const std::string& facet) = 0;

        /**
         * Add a default servant to handle requests for a specific category. Adding a default servant for a category for
         * which a default servant is already registered throws {@link AlreadyRegisteredException}. To dispatch
         * operation calls on servants, the object adapter tries to find a servant for a given Ice object identity and
         * facet in the following order: <ol> <li>The object adapter tries to find a servant for the identity and facet
         * in the Active Servant Map.</li> <li>If no servant has been found in the Active Servant Map, the object
         * adapter tries to find a default servant for the category component of the identity.</li> <li>If no servant
         * has been found by any of the preceding steps, the object adapter tries to find a default servant for an empty
         * category, regardless of the category contained in the identity.</li> <li>If no servant has been found by any
         * of the preceding steps, the object adapter gives up and the caller receives {@link ObjectNotExistException}
         * or {@link FacetNotExistException}.</li>
         * </ol>
         * @param servant The default servant.
         * @param category The category for which the default servant is registered. An empty category means it will
         * handle all categories.
         * @see #removeDefaultServant
         * @see #findDefaultServant
         */
        virtual void addDefaultServant(const ObjectPtr& servant, const std::string& category) = 0;

        /**
         * Remove a servant (that is, the default facet) from the object adapter's Active Servant Map.
         * @param id The identity of the Ice object that is implemented by the servant. If the servant implements
         * multiple Ice objects, {@link #remove} has to be called for all those Ice objects. Removing an identity that
         * is not in the map throws {@link NotRegisteredException}.
         * @return The removed servant.
         * @see Identity
         * @see #add
         * @see #addWithUUID
         */
        virtual ObjectPtr remove(const Identity& id) = 0;

        /**
         * Like {@link #remove}, but with a facet. Calling <code>remove(id)</code> is equivalent to calling
         * {@link #removeFacet} with an empty facet.
         * @param id The identity of the Ice object that is implemented by the servant.
         * @param facet The facet. An empty facet means the default facet.
         * @return The removed servant.
         * @see Identity
         * @see #addFacet
         * @see #addFacetWithUUID
         */
        virtual ObjectPtr removeFacet(const Identity& id, const std::string& facet) = 0;

        /**
         * Remove all facets with the given identity from the Active Servant Map. The operation completely removes the
         * Ice object, including its default facet. Removing an identity that is not in the map throws
         * {@link NotRegisteredException}.
         * @param id The identity of the Ice object to be removed.
         * @return A collection containing all the facet names and servants of the removed Ice object.
         * @see #remove
         * @see #removeFacet
         */
        virtual FacetMap removeAllFacets(const Identity& id) = 0;

        /**
         * Remove the default servant for a specific category. Attempting to remove a default servant for a category
         * that is not registered throws {@link NotRegisteredException}.
         * @param category The category of the default servant to remove.
         * @return The default servant.
         * @see #addDefaultServant
         * @see #findDefaultServant
         */
        virtual ObjectPtr removeDefaultServant(const std::string& category) = 0;

        /**
         * Look up a servant in this object adapter's Active Servant Map by the identity of the Ice object it
         * implements. <p class="Note">This operation only tries to look up a servant in the Active Servant Map. It does
         * not attempt to find a servant by using any installed {@link ServantLocator}.
         * @param id The identity of the Ice object for which the servant should be returned.
         * @return The servant that implements the Ice object with the given identity, or null if no such servant has
         * been found.
         * @see Identity
         * @see #findFacet
         * @see #findByProxy
         */
        virtual ObjectPtr find(const Identity& id) const = 0;

        /**
         * Like {@link #find}, but with a facet. Calling <code>find(id)</code> is equivalent to calling {@link
         * #findFacet} with an empty facet.
         * @param id The identity of the Ice object for which the servant should be returned.
         * @param facet The facet. An empty facet means the default facet.
         * @return The servant that implements the Ice object with the given identity and facet, or null if no such
         * servant has been found.
         * @see Identity
         * @see #find
         * @see #findByProxy
         */
        virtual ObjectPtr findFacet(const Identity& id, const std::string& facet) const = 0;

        /**
         * Find all facets with the given identity in the Active Servant Map.
         * @param id The identity of the Ice object for which the facets should be returned.
         * @return A collection containing all the facet names and servants that have been found, or an empty map if
         * there is no facet for the given identity.
         * @see #find
         * @see #findFacet
         */
        virtual FacetMap findAllFacets(const Identity& id) const = 0;

        /**
         * Look up a servant in this object adapter's Active Servant Map, given a proxy.
         * <p class="Note">This operation only tries to lookup a servant in the Active Servant Map. It does not attempt
         * to find a servant by using any installed {@link ServantLocator}.
         * @param proxy The proxy for which the servant should be returned.
         * @return The servant that matches the proxy, or null if no such servant has been found.
         * @see #find
         * @see #findFacet
         */
        virtual ObjectPtr findByProxy(const ObjectPrx& proxy) const = 0;

        /**
         * Add a Servant Locator to this object adapter. Adding a servant locator for a category for which a servant
         * locator is already registered throws {@link AlreadyRegisteredException}. To dispatch operation calls on
         * servants, the object adapter tries to find a servant for a given Ice object identity and facet in the
         * following order: <ol> <li>The object adapter tries to find a servant for the identity and facet in the Active
         * Servant Map.</li> <li>If no servant has been found in the Active Servant Map, the object adapter tries to
         * find a servant locator for the category component of the identity. If a locator is found, the object adapter
         * tries to find a servant using this locator.</li> <li>If no servant has been found by any of the preceding
         * steps, the object adapter tries to find a locator for an empty category, regardless of the category contained
         * in the identity. If a locator is found, the object adapter tries to find a servant using this locator.</li>
         * <li>If no servant has been found by any of the preceding steps, the object adapter gives up and the caller
         * receives {@link ObjectNotExistException} or {@link FacetNotExistException}.</li>
         * </ol>
         * <p class="Note">Only one locator for the empty category can be installed.
         * @param locator The locator to add.
         * @param category The category for which the Servant Locator can locate servants, or an empty string if the
         * Servant Locator does not belong to any specific category.
         * @see Identity
         * @see #removeServantLocator
         * @see #findServantLocator
         * @see ServantLocator
         */
        virtual void
        addServantLocator(const std::shared_ptr<ServantLocator>& locator, const std::string& category) = 0;

        /**
         * Remove a Servant Locator from this object adapter.
         * @param category The category for which the Servant Locator can locate servants, or an empty string if the
         * Servant Locator does not belong to any specific category.
         * @return The Servant Locator, or throws {@link NotRegisteredException} if no Servant Locator was found for the
         * given category.
         * @see Identity
         * @see #addServantLocator
         * @see #findServantLocator
         * @see ServantLocator
         */
        virtual std::shared_ptr<ServantLocator> removeServantLocator(const std::string& category) = 0;

        /**
         * Find a Servant Locator installed with this object adapter.
         * @param category The category for which the Servant Locator can locate servants, or an empty string if the
         * Servant Locator does not belong to any specific category.
         * @return The Servant Locator, or null if no Servant Locator was found for the given category.
         * @see Identity
         * @see #addServantLocator
         * @see #removeServantLocator
         * @see ServantLocator
         */
        virtual std::shared_ptr<ServantLocator> findServantLocator(const std::string& category) const = 0;

        /**
         * Find the default servant for a specific category.
         * @param category The category of the default servant to find.
         * @return The default servant or null if no default servant was registered for the category.
         * @see #addDefaultServant
         * @see #removeDefaultServant
         */
        virtual ObjectPtr findDefaultServant(const std::string& category) const = 0;

        /**
         * Get the dispatcher associated with this object adapter. This object dispatches incoming requests to the
         * servants managed by this object adapter, and takes into account the servant locators.
         * @return The dispatcher. This shared_ptr is never null.
         * @remarks You can add this dispatcher as a servant (including default servant) in another object adapter.
         */
        virtual ObjectPtr dispatcher() const noexcept = 0;

        /**
         * Create a proxy for the object with the given identity. If this object adapter is configured with an adapter
         * id, the return value is an indirect proxy that refers to the adapter id. If a replica group id is also
         * defined, the return value is an indirect proxy that refers to the replica group id. Otherwise, if no adapter
         * id is defined, the return value is a direct proxy containing this object adapter's published endpoints.
         * @param id The object's identity.
         * @return A proxy for the object with the given identity.
         * @see Identity
         */
        virtual ObjectPrx createProxy(const Identity& id) const = 0;

        /**
         * Create a direct proxy for the object with the given identity. The returned proxy contains this object
         * adapter's published endpoints.
         * @param id The object's identity.
         * @return A proxy for the object with the given identity.
         * @see Identity
         */
        virtual ObjectPrx createDirectProxy(const Identity& id) const = 0;

        /**
         * Create an indirect proxy for the object with the given identity. If this object adapter is configured with an
         * adapter id, the return value refers to the adapter id. Otherwise, the return value contains only the object
         * identity.
         * @param id The object's identity.
         * @return A proxy for the object with the given identity.
         * @see Identity
         */
        virtual ObjectPrx createIndirectProxy(const Identity& id) const = 0;

        /**
         * Set an Ice locator for this object adapter. By doing so, the object adapter will register itself with the
         * locator registry when it is activated for the first time. Furthermore, the proxies created by this object
         * adapter will contain the adapter identifier instead of its endpoints. The adapter identifier must be
         * configured using the AdapterId property.
         * @param loc The locator used by this object adapter.
         * @see #createDirectProxy
         * @see Locator
         * @see LocatorRegistry
         */
        virtual void setLocator(const std::optional<LocatorPrx>& loc) = 0;

        /**
         * Get the Ice locator used by this object adapter.
         * @return The locator used by this object adapter, or null if no locator is used by this object adapter.
         * @see Locator
         * @see #setLocator
         */
        virtual std::optional<LocatorPrx> getLocator() const noexcept = 0;

        /**
         * Get the set of endpoints configured with this object adapter.
         * @return The set of endpoints.
         * @see Endpoint
         */
        virtual EndpointSeq getEndpoints() const noexcept = 0;

        /**
         * Refresh the set of published endpoints. The run time re-reads the PublishedEndpoints property if it is set
         * and re-reads the list of local interfaces if the adapter is configured to listen on all endpoints. This
         * operation is useful to refresh the endpoint information that is published in the proxies that are created by
         * an object adapter if the network interfaces used by a host changes.
         */
        virtual void refreshPublishedEndpoints() = 0;

        /**
         * Get the set of endpoints that proxies created by this object adapter will contain.
         * @return The set of published endpoints.
         * @see #refreshPublishedEndpoints
         * @see Endpoint
         */
        virtual EndpointSeq getPublishedEndpoints() const noexcept = 0;

        /**
         * Set of the endpoints that proxies created by this object adapter will contain.
         * @param newEndpoints The new set of endpoints that the object adapter will embed in proxies.
         * @see #refreshPublishedEndpoints
         * @see Endpoint
         */
        virtual void setPublishedEndpoints(const EndpointSeq& newEndpoints) = 0;

#ifdef ICE_SWIFT
        virtual dispatch_queue_t getDispatchQueue() const = 0;
#endif
    };

    using ObjectAdapterPtr = std::shared_ptr<ObjectAdapter>;
}

#endif
