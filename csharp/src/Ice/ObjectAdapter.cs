// Copyright (c) ZeroC, Inc.

namespace Ice;

public interface ObjectAdapter
{
    /// <summary>
    /// Get the name of this object adapter.
    /// </summary>
    /// <returns>This object adapter's name.</returns>
    string getName();

    /// <summary>
    /// Get the communicator this object adapter belongs to.
    /// </summary>
    /// <returns>This object adapter's communicator.
    ///  </returns>
    Communicator getCommunicator();

    /// <summary>
    /// Activate all endpoints that belong to this object adapter.
    /// After activation, the object adapter can dispatch
    ///  requests received through its endpoints.
    /// </summary>
    void activate();

    /// <summary>
    /// Temporarily hold receiving and dispatching requests.
    /// The object adapter can be reactivated with the
    ///  activate operation. &lt;p class="Note"&gt; Holding is not immediate, i.e., after hold returns, the
    ///  object adapter might still be active for some time. You can use waitForHold to wait until holding is
    ///  complete.
    /// </summary>
    void hold();

    /// <summary>
    /// Wait until the object adapter holds requests.
    /// Calling hold initiates holding of requests, and
    ///  waitForHold only returns when holding of requests has been completed.
    /// </summary>
    void waitForHold();

    /// <summary>
    /// Deactivate all endpoints that belong to this object adapter.
    /// After deactivation, the object adapter stops
    ///  receiving requests through its endpoints. Object adapters that have been deactivated must not be reactivated
    ///  again, and cannot be used otherwise. Attempts to use a deactivated object adapter raise
    ///  ObjectAdapterDeactivatedException however, attempts to deactivate an already deactivated
    ///  object adapter are ignored and do nothing. Once deactivated, it is possible to destroy the adapter to clean up
    ///  resources and then create and activate a new adapter with the same name.
    ///  &lt;p class="Note"&gt; After deactivate returns, no new requests are processed by the object adapter.
    ///  However, requests that have been started before deactivate was called might still be active. You can
    ///  use waitForDeactivate to wait for the completion of all requests for this object adapter.
    /// </summary>
    void deactivate();

    /// <summary>
    /// Wait until the object adapter has deactivated.
    /// Calling deactivate initiates object adapter
    ///  deactivation, and waitForDeactivate only returns when deactivation has been completed.
    /// </summary>

    void waitForDeactivate();

    /// <summary>
    /// Check whether object adapter has been deactivated.
    /// </summary>
    /// <returns>Whether adapter has been deactivated.
    ///  </returns>
    bool isDeactivated();

    /// <summary>
    /// Destroys the object adapter and cleans up all resources held by the object adapter.
    /// If the object adapter has
    ///  not yet been deactivated, destroy implicitly initiates the deactivation and waits for it to finish. Subsequent
    ///  calls to destroy are ignored. Once destroy has returned, it is possible to create another object adapter with
    ///  the same name.
    /// </summary>
    void destroy();

    /// <summary>
    /// Add a servant to this object adapter's Active Servant Map.
    /// Note that one servant can implement several Ice
    ///  objects by registering the servant with multiple identities. Adding a servant with an identity that is in the
    ///  map already throws AlreadyRegisteredException.
    /// </summary>
    ///  <param name="servant">The servant to add.
    ///  </param>
    /// <param name="id">The identity of the Ice object that is implemented by the servant.
    ///  </param>
    /// <returns>A proxy that matches the given identity and this object adapter.
    ///  </returns>
    ObjectPrx add(Object servant, Identity id);

    /// <summary>
    /// Like add, but with a facet.
    /// Calling add(servant, id) is equivalent to calling
    ///  addFacet with an empty facet.
    /// </summary>
    ///  <param name="servant">The servant to add.
    ///  </param>
    /// <param name="id">The identity of the Ice object that is implemented by the servant.
    ///  </param>
    /// <param name="facet">The facet. An empty facet means the default facet.
    ///  </param>
    /// <returns>A proxy that matches the given identity, facet, and this object adapter.
    ///  </returns>
    ObjectPrx addFacet(Object servant, Identity id, string facet);

    /// <summary>
    /// Add a servant to this object adapter's Active Servant Map, using an automatically generated UUID as its
    ///  identity.
    /// Note that the generated UUID identity can be accessed using the proxy's ice_getIdentity
    ///  operation.
    /// </summary>
    ///  <param name="servant">The servant to add.
    ///  </param>
    /// <returns>A proxy that matches the generated UUID identity and this object adapter.
    ///  </returns>
    ObjectPrx addWithUUID(Object servant);

    /// <summary>
    /// Like addWithUUID, but with a facet.
    /// Calling addWithUUID(servant) is equivalent to calling
    ///  addFacetWithUUID with an empty facet.
    /// </summary>
    ///  <param name="servant">The servant to add.
    ///  </param>
    /// <param name="facet">The facet. An empty facet means the default facet.
    ///  </param>
    /// <returns>A proxy that matches the generated UUID identity, facet, and this object adapter.
    ///  </returns>
    ObjectPrx addFacetWithUUID(Object servant, string facet);

    /// <summary>
    /// Add a default servant to handle requests for a specific category.
    /// Adding a default servant for a category for
    ///  which a default servant is already registered throws AlreadyRegisteredException. To dispatch operation
    ///  calls on servants, the object adapter tries to find a servant for a given Ice object identity and facet in the
    ///  following order:
    ///
    ///  The object adapter tries to find a servant for the identity and facet in the Active Servant Map.
    ///  If no servant has been found in the Active Servant Map, the object adapter tries to find a default servant
    ///  for the category component of the identity.
    ///  If no servant has been found by any of the preceding steps, the object adapter tries to find a default
    ///  servant for an empty category, regardless of the category contained in the identity.
    ///  If no servant has been found by any of the preceding steps, the object adapter gives up and the caller
    ///  receives ObjectNotExistException or FacetNotExistException.
    ///
    /// </summary>
    ///  <param name="servant">The default servant.
    ///  </param>
    /// <param name="category">The category for which the default servant is registered. An empty category means it will
    ///  handle all categories.
    ///  </param>
    void addDefaultServant(Object servant, string category);

    /// <summary>
    /// Remove a servant (that is, the default facet) from the object adapter's Active Servant Map.
    /// </summary>
    /// <param name="id">The identity of the Ice object that is implemented by the servant. If the servant implements multiple
    ///  Ice objects, remove has to be called for all those Ice objects. Removing an identity that is not in
    ///  the map throws NotRegisteredException.
    ///  </param>
    /// <returns>The removed servant.
    ///  </returns>
    Object remove(Identity id);

    /// <summary>
    /// Like remove, but with a facet.
    /// Calling remove(id) is equivalent to calling
    ///  removeFacet with an empty facet.
    /// </summary>
    ///  <param name="id">The identity of the Ice object that is implemented by the servant.
    ///  </param>
    /// <param name="facet">The facet. An empty facet means the default facet.
    ///  </param>
    /// <returns>The removed servant.
    ///  </returns>
    Object removeFacet(Identity id, string facet);

    /// <summary>
    /// Remove all facets with the given identity from the Active Servant Map.
    /// The operation completely removes the Ice
    ///  object, including its default facet. Removing an identity that is not in the map throws
    ///  NotRegisteredException.
    /// </summary>
    ///  <param name="id">The identity of the Ice object to be removed.
    ///  </param>
    /// <returns>A collection containing all the facet names and servants of the removed Ice object.
    ///  </returns>
    System.Collections.Generic.Dictionary<string, Object> removeAllFacets(Identity id);

    /// <summary>
    /// Remove the default servant for a specific category.
    /// Attempting to remove a default servant for a category that
    ///  is not registered throws NotRegisteredException.
    /// </summary>
    ///  <param name="category">The category of the default servant to remove.
    ///  </param>
    /// <returns>The default servant.
    ///  </returns>
    Object removeDefaultServant(string category);

    /// <summary>
    /// Look up a servant in this object adapter's Active Servant Map by the identity of the Ice object it implements.
    /// &lt;p class="Note"&gt;This operation only tries to look up a servant in the Active Servant Map. It does not attempt
    ///  to find a servant by using any installed ServantLocator.
    /// </summary>
    ///  <param name="id">The identity of the Ice object for which the servant should be returned.
    ///  </param>
    /// <returns>The servant that implements the Ice object with the given identity, or null if no such servant has been
    ///  found.
    ///  </returns>
    Object find(Identity id);

    /// <summary>
    /// Like find, but with a facet.
    /// Calling find(id) is equivalent to calling findFacet
    ///  with an empty facet.
    /// </summary>
    ///  <param name="id">The identity of the Ice object for which the servant should be returned.
    ///  </param>
    /// <param name="facet">The facet. An empty facet means the default facet.
    ///  </param>
    /// <returns>The servant that implements the Ice object with the given identity and facet, or null if no such
    ///  servant has been found.
    ///  </returns>
    Object findFacet(Identity id, string facet);

    /// <summary>
    /// Find all facets with the given identity in the Active Servant Map.
    /// </summary>
    /// <param name="id">The identity of the Ice object for which the facets should be returned.
    ///  </param>
    /// <returns>A collection containing all the facet names and servants that have been found, or an empty map if there
    ///  is no facet for the given identity.
    ///  </returns>
    System.Collections.Generic.Dictionary<string, Object> findAllFacets(Identity id);

    /// <summary>
    /// Look up a servant in this object adapter's Active Servant Map, given a proxy.
    /// &lt;p class="Note"&gt;This operation only tries to lookup a servant in the Active Servant Map. It does not attempt to
    ///  find a servant by using any installed ServantLocator.
    /// </summary>
    ///  <param name="proxy">The proxy for which the servant should be returned.
    ///  </param>
    /// <returns>The servant that matches the proxy, or null if no such servant has been found.
    ///  </returns>
    Object findByProxy(ObjectPrx proxy);

    /// <summary>
    /// Add a Servant Locator to this object adapter.
    /// Adding a servant locator for a category for which a servant
    ///  locator is already registered throws AlreadyRegisteredException. To dispatch operation calls on
    ///  servants, the object adapter tries to find a servant for a given Ice object identity and facet in the following
    ///  order:
    ///
    ///  The object adapter tries to find a servant for the identity and facet in the Active Servant Map.
    ///  If no servant has been found in the Active Servant Map, the object adapter tries to find a servant locator
    ///  for the category component of the identity. If a locator is found, the object adapter tries to find a servant
    ///  using this locator.
    ///  If no servant has been found by any of the preceding steps, the object adapter tries to find a locator for
    ///  an empty category, regardless of the category contained in the identity. If a locator is found, the object
    ///  adapter tries to find a servant using this locator.
    ///  If no servant has been found by any of the preceding steps, the object adapter gives up and the caller
    ///  receives ObjectNotExistException or FacetNotExistException.
    ///
    ///  &lt;p class="Note"&gt;Only one locator for the empty category can be installed.
    /// </summary>
    ///  <param name="locator">The locator to add.
    ///  </param>
    /// <param name="category">The category for which the Servant Locator can locate servants, or an empty string if the
    ///  Servant Locator does not belong to any specific category.
    ///  </param>
    void addServantLocator(ServantLocator locator, string category);

    /// <summary>
    /// Remove a Servant Locator from this object adapter.
    /// </summary>
    /// <param name="category">The category for which the Servant Locator can locate servants, or an empty string if the
    ///  Servant Locator does not belong to any specific category.
    ///  </param>
    /// <returns>The Servant Locator, or throws NotRegisteredException if no Servant Locator was found for the
    ///  given category.
    ///  </returns>
    ServantLocator removeServantLocator(string category);

    /// <summary>
    /// Find a Servant Locator installed with this object adapter.
    /// </summary>
    /// <param name="category">The category for which the Servant Locator can locate servants, or an empty string if the
    ///  Servant Locator does not belong to any specific category.
    ///  </param>
    /// <returns>The Servant Locator, or null if no Servant Locator was found for the given category.
    ///  </returns>
    ServantLocator findServantLocator(string category);

    /// <summary>
    /// Find the default servant for a specific category.
    /// </summary>
    /// <param name="category">The category of the default servant to find.
    ///  </param>
    /// <returns>The default servant or null if no default servant was registered for the category.
    ///  </returns>
    Object findDefaultServant(string category);

    /// <summary>
    /// Get the dispatcher associated with this object adapter. This object dispatches incoming requests to the
    /// servants managed by this object adapter, and takes into account the servant locators.
    /// </summary>
    /// <value>The dispatcher.</value>
    /// <remarks>You can add this dispatcher as a servant (including default servant) in another object adapter.
    /// </remarks>
    Object dispatcher { get; }

    /// <summary>
    /// Create a proxy for the object with the given identity.
    /// If this object adapter is configured with an adapter id,
    ///  the return value is an indirect proxy that refers to the adapter id. If a replica group id is also defined, the
    ///  return value is an indirect proxy that refers to the replica group id. Otherwise, if no adapter id is defined,
    ///  the return value is a direct proxy containing this object adapter's published endpoints.
    /// </summary>
    ///  <param name="id">The object's identity.
    ///  </param>
    /// <returns>A proxy for the object with the given identity.
    ///  </returns>
    ObjectPrx createProxy(Identity id);

    /// <summary>
    /// Create a direct proxy for the object with the given identity.
    /// The returned proxy contains this object adapter's
    ///  published endpoints.
    /// </summary>
    ///  <param name="id">The object's identity.
    ///  </param>
    /// <returns>A proxy for the object with the given identity.
    ///  </returns>
    ObjectPrx createDirectProxy(Identity id);

    /// <summary>
    /// Create an indirect proxy for the object with the given identity.
    /// If this object adapter is configured with an
    ///  adapter id, the return value refers to the adapter id. Otherwise, the return value contains only the object
    ///  identity.
    /// </summary>
    ///  <param name="id">The object's identity.
    ///  </param>
    /// <returns>A proxy for the object with the given identity.
    ///  </returns>
    ObjectPrx createIndirectProxy(Identity id);

    /// <summary>
    /// Set an Ice locator for this object adapter.
    /// By doing so, the object adapter will register itself with the
    ///  locator registry when it is activated for the first time. Furthermore, the proxies created by this object
    ///  adapter will contain the adapter identifier instead of its endpoints. The adapter identifier must be configured
    ///  using the AdapterId property.
    /// </summary>
    ///  <param name="loc">The locator used by this object adapter.
    ///  </param>
    void setLocator(LocatorPrx loc);

    /// <summary>
    /// Get the Ice locator used by this object adapter.
    /// </summary>
    /// <returns>The locator used by this object adapter, or null if no locator is used by this object adapter.
    ///  </returns>
    LocatorPrx getLocator();

    /// <summary>
    /// Get the set of endpoints configured with this object adapter.
    /// </summary>
    /// <returns>The set of endpoints.
    ///  </returns>
    Endpoint[] getEndpoints();

    /// <summary>
    /// Refresh the set of published endpoints.
    /// The run time re-reads the PublishedEndpoints property if it is set and
    ///  re-reads the list of local interfaces if the adapter is configured to listen on all endpoints. This operation
    ///  is useful to refresh the endpoint information that is published in the proxies that are created by an object
    ///  adapter if the network interfaces used by a host changes.
    /// </summary>
    void refreshPublishedEndpoints();

    /// <summary>
    /// Get the set of endpoints that proxies created by this object adapter will contain.
    /// </summary>
    /// <returns>The set of published endpoints.
    ///  </returns>
    Endpoint[] getPublishedEndpoints();

    /// <summary>
    /// Set of the endpoints that proxies created by this object adapter will contain.
    /// </summary>
    /// <param name="newEndpoints">The new set of endpoints that the object adapter will embed in proxies.
    ///  </param>
    void setPublishedEndpoints(Endpoint[] newEndpoints);
}
