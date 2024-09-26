//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GLACIER2_ROUTER_I_H
#define GLACIER2_ROUTER_I_H

#include "ClientBlobject.h"
#include "Glacier2/Router.h"
#include "Ice/Ice.h"
#include "ServerBlobject.h"

namespace Glacier2
{
    class RoutingTable;
    class RouterI;
    class FilterManager;

    class RouterI final : public Router
    {
    public:
        RouterI(
            std::shared_ptr<Instance>,
            Ice::ConnectionPtr,
            const std::string&,
            std::optional<SessionPrx>,
            const Ice::Identity&,
            std::shared_ptr<FilterManager>,
            const Ice::Context&);

        void destroy(std::function<void(std::exception_ptr)>);

        std::optional<Ice::ObjectPrx> getClientProxy(std::optional<bool>&, const Ice::Current&) const final;
        std::optional<Ice::ObjectPrx> getServerProxy(const Ice::Current&) const final;
        Ice::ObjectProxySeq addProxies(Ice::ObjectProxySeq, const Ice::Current&) final;
        std::string getCategoryForClient(const Ice::Current&) const final;
        void createSessionAsync(
            std::string,
            std::string,
            std::function<void(const std::optional<SessionPrx>& returnValue)>,
            std::function<void(std::exception_ptr)>,
            const Ice::Current&) final;
        void createSessionFromSecureConnectionAsync(
            std::function<void(const std::optional<SessionPrx>& returnValue)>,
            std::function<void(std::exception_ptr)>,
            const Ice::Current&) final;
        void refreshSession(const Ice::Current&) final {}
        void destroySession(const Ice::Current&) final;
        std::int64_t getSessionTimeout(const Ice::Current&) const final;
        int getACMTimeout(const Ice::Current&) const final;

        std::shared_ptr<ClientBlobject> getClientBlobject() const;
        std::shared_ptr<ServerBlobject> getServerBlobject() const;

        std::optional<SessionPrx> getSession() const;

        void updateObserver(const std::shared_ptr<Glacier2::Instrumentation::RouterObserver>&);

        std::string toString() const;

    private:
        const std::shared_ptr<Instance> _instance;
        const std::shared_ptr<RoutingTable> _routingTable;
        const std::optional<Ice::ObjectPrx> _serverProxy;
        const std::shared_ptr<ClientBlobject> _clientBlobject;
        const std::shared_ptr<ServerBlobject> _serverBlobject;
        const Ice::ConnectionPtr _connection;
        const std::string _userId;
        const std::optional<SessionPrx> _session;
        const Ice::Identity _controlId;
        const Ice::Context _context;

        std::shared_ptr<Glacier2::Instrumentation::SessionObserver> _observer;
    };
}

#endif
