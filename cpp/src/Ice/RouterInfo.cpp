//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/RouterInfo.h>
#include <Ice/Router.h>
#include <Ice/LocalException.h>
#include <Ice/Connection.h> // For ice_connection()->timeout().
#include <Ice/Reference.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

IceInternal::RouterManager::RouterManager() :
    _tableHint(_table.end())
{
}

void
IceInternal::RouterManager::destroy()
{
    lock_guard lock(_mutex);
    for_each(_table.begin(), _table.end(),
             [](const pair<RouterPrxPtr, RouterInfoPtr> it)
             {
                 it.second->destroy();
             });
    _table.clear();
    _tableHint = _table.end();
}

RouterInfoPtr
IceInternal::RouterManager::get(const RouterPrxPtr& rtr)
{
    if(!rtr)
    {
        return nullptr;
    }

    RouterPrxPtr router = rtr->ice_router(0); // The router cannot be routed.

    lock_guard lock(_mutex);

    RouterInfoTable::iterator p = _table.end();

    if(_tableHint != _table.end())
    {
        if(targetEqualTo(_tableHint->first, router))
        {
            p = _tableHint;
        }
    }

    if(p == _table.end())
    {
        p = _table.find(router);
    }

    if(p == _table.end())
    {
        _tableHint = _table.insert(_tableHint, pair<const RouterPrxPtr, RouterInfoPtr>(router, new RouterInfo(router)));
    }
    else
    {
        _tableHint = p;
    }

    return _tableHint->second;
}

RouterInfoPtr
IceInternal::RouterManager::erase(const RouterPrxPtr& rtr)
{
    RouterInfoPtr info;
    if(rtr)
    {
        RouterPrxPtr router = Ice::uncheckedCast<RouterPrx>(rtr->ice_router(nullptr)); // The router cannot be routed.
        lock_guard lock(_mutex);

        RouterInfoTable::iterator p = _table.end();
        if(_tableHint != _table.end() && targetEqualTo(_tableHint->first, router))
        {
            p = _tableHint;
            _tableHint = _table.end();
        }

        if(p == _table.end())
        {
            p = _table.find(router);
        }

        if(p != _table.end())
        {
            info = p->second;
            _table.erase(p);
        }
    }

    return info;
}

IceInternal::RouterInfo::RouterInfo(const RouterPrxPtr& router) : _router(router), _hasRoutingTable(false)
{
    assert(_router);
}

void
IceInternal::RouterInfo::destroy()
{
    lock_guard lock(_mutex);

    _clientEndpoints.clear();
    _adapter = 0;
    _identities.clear();
}

bool
IceInternal::RouterInfo::operator==(const RouterInfo& rhs) const
{
    return Ice::targetEqualTo(_router, rhs._router);
}

bool
IceInternal::RouterInfo::operator<(const RouterInfo& rhs) const
{
    return Ice::targetLess(_router, rhs._router);
}

vector<EndpointIPtr>
IceInternal::RouterInfo::getClientEndpoints()
{
    {
        lock_guard lock(_mutex);
        if(!_clientEndpoints.empty())
        {
            return _clientEndpoints;
        }
    }

    optional<bool> hasRoutingTable;
    Ice::ObjectPrxPtr proxy = _router->getClientProxy(hasRoutingTable);
    return setClientEndpoints(proxy, hasRoutingTable ? hasRoutingTable.value() : true);
}

void
IceInternal::RouterInfo::getClientProxyResponse(const Ice::ObjectPrxPtr& proxy,
                                                const optional<bool>& hasRoutingTable,
                                                const GetClientEndpointsCallbackPtr& callback)
{
    callback->setEndpoints(setClientEndpoints(proxy, hasRoutingTable ? hasRoutingTable.value() : true));
}

void
IceInternal::RouterInfo::getClientProxyException(std::exception_ptr ex,
                                                 const GetClientEndpointsCallbackPtr& callback)
{
    callback->setException(ex);
}

void
IceInternal::RouterInfo::getClientEndpoints(const GetClientEndpointsCallbackPtr& callback)
{
    vector<EndpointIPtr> clientEndpoints;
    {
        lock_guard lock(_mutex);
        clientEndpoints = _clientEndpoints;
    }

    if(!clientEndpoints.empty())
    {
        callback->setEndpoints(clientEndpoints);
        return;
    }

    RouterInfoPtr self = shared_from_this();
    _router->getClientProxyAsync(
        [self, callback](const Ice::ObjectPrxPtr& proxy, optional<bool> hasRoutingTable)
        {
            self->getClientProxyResponse(proxy, hasRoutingTable, callback);
        },
        [self, callback](exception_ptr e)
        {
            self->getClientProxyException(e, callback);
        });
}

vector<EndpointIPtr>
IceInternal::RouterInfo::getServerEndpoints()
{
    Ice::ObjectPrxPtr serverProxy = _router->getServerProxy();
    if(!serverProxy)
    {
        throw NoEndpointException(__FILE__, __LINE__);
    }
    serverProxy = serverProxy->ice_router(0); // The server proxy cannot be routed.
    return serverProxy->_getReference()->getEndpoints();
}

bool
IceInternal::RouterInfo::addProxyAsync(
    const Ice::ObjectPrxPtr& proxy,
    function<void()> response,
    function<void(exception_ptr)> ex)
{
    assert(proxy);
    {
        lock_guard lock(_mutex);
        if(!_hasRoutingTable)
        {
            return true; // The router implementation doesn't maintain a routing table.
        }
        else if(_identities.find(proxy->ice_getIdentity()) != _identities.end())
        {
            //
            // Only add the proxy to the router if it's not already in our local map.
            //
            return true;
        }
    }

    Ice::ObjectProxySeq proxies;
    proxies.push_back(proxy);

    RouterInfoPtr self = shared_from_this();
    _router->addProxiesAsync(
        proxies,
        [response, self, proxy](Ice::ObjectProxySeq evictedProxies)
        {
            self->addAndEvictProxies(proxy, evictedProxies);
            response();
        },
        ex);
    return false;
}

void
IceInternal::RouterInfo::setAdapter(const ObjectAdapterPtr& adapter)
{
    lock_guard lock(_mutex);
    _adapter = adapter;
}

ObjectAdapterPtr
IceInternal::RouterInfo::getAdapter() const
{
    lock_guard lock(_mutex);
    return _adapter;
}

void
IceInternal::RouterInfo::clearCache(const ReferencePtr& ref)
{
    lock_guard lock(_mutex);
    _identities.erase(ref->getIdentity());
}

vector<EndpointIPtr>
IceInternal::RouterInfo::setClientEndpoints(const Ice::ObjectPrxPtr& proxy, bool hasRoutingTable)
{
    lock_guard lock(_mutex);
    if(_clientEndpoints.empty())
    {
        _hasRoutingTable = hasRoutingTable;
        if(!proxy)
        {
            //
            // If getClientProxy() return nil, use router endpoints.
            //
            _clientEndpoints = _router->_getReference()->getEndpoints();
        }
        else
        {
            Ice::ObjectPrxPtr clientProxy = proxy->ice_router(0); // The client proxy cannot be routed.

            //
            // In order to avoid creating a new connection to the router,
            // we must use the same timeout as the already existing
            // connection.
            //
            if(_router->ice_getConnection())
            {
                clientProxy = clientProxy->ice_timeout(_router->ice_getConnection()->timeout());
            }

            _clientEndpoints = clientProxy->_getReference()->getEndpoints();
        }
    }
    return _clientEndpoints;
}

void
IceInternal::RouterInfo::addAndEvictProxies(const Ice::ObjectPrxPtr& proxy, const Ice::ObjectProxySeq& evictedProxies)
{
    lock_guard lock(_mutex);

    //
    // Check if the proxy hasn't already been evicted by a concurrent addProxies call.
    // If it's the case, don't add it to our local map.
    //
    multiset<Identity>::iterator p = _evictedIdentities.find(proxy->ice_getIdentity());
    if(p != _evictedIdentities.end())
    {
        _evictedIdentities.erase(p);
    }
    else
    {
        //
        // If we successfully added the proxy to the router,
        // we add it to our local map.
        //
        _identities.insert(proxy->ice_getIdentity());
    }

    //
    // We also must remove whatever proxies the router evicted.
    //
    for(Ice::ObjectProxySeq::const_iterator q = evictedProxies.begin(); q != evictedProxies.end(); ++q)
    {
        if(_identities.erase((*q)->ice_getIdentity()) == 0)
        {
            //
            // It's possible for the proxy to not have been
            // added yet in the local map if two threads
            // concurrently call addProxies.
            //
            _evictedIdentities.insert((*q)->ice_getIdentity());
        }
    }
}
