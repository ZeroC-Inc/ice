//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GLACIER2_SESSION_ROUTER_I_H
#define GLACIER2_SESSION_ROUTER_I_H

#include <Ice/Ice.h>

#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/Router.h>
#include <Glacier2/Instrumentation.h>

#include <set>

namespace Glacier2
{
    class FilterManager;
    class Instance;
    class RouterI;
    class SessionRouterI;
    class SSLCreateSession;
    class UserPasswordCreateSession;

    class CreateSession : public std::enable_shared_from_this<CreateSession>
    {
    public:
        CreateSession(std::shared_ptr<SessionRouterI>, const std::string&, const Ice::Current&);

        void create();
        void addPendingCallback(std::shared_ptr<CreateSession>);

        void authorized(bool);
        void unexpectedAuthorizeException(std::exception_ptr);

        void sessionCreated(const std::optional<SessionPrx>&);
        void unexpectedCreateSessionException(std::exception_ptr);

        void exception(std::exception_ptr);

        void createException(std::exception_ptr);

        virtual void authorize() = 0;
        virtual void createSession() = 0;
        virtual std::shared_ptr<FilterManager> createFilterManager() = 0;
        virtual void finished(const std::optional<SessionPrx>&) = 0;
        virtual void finished(std::exception_ptr) = 0;

    protected:
        const std::shared_ptr<Instance> _instance;
        const std::shared_ptr<SessionRouterI> _sessionRouter;
        const std::string _user;
        const Ice::Current _current;
        Ice::Context _context;
        std::vector<std::shared_ptr<CreateSession>> _pendingCallbacks;
        std::optional<SessionControlPrx> _control;
        std::shared_ptr<FilterManager> _filterManager;
    };

    class SessionRouterI final : public Router,
                                 public Glacier2::Instrumentation::ObserverUpdater,
                                 public std::enable_shared_from_this<SessionRouterI>
    {
    public:
        SessionRouterI(
            std::shared_ptr<Instance>,
            std::optional<PermissionsVerifierPrx>,
            std::optional<SessionManagerPrx>,
            std::optional<SSLPermissionsVerifierPrx>,
            std::optional<SSLSessionManagerPrx>);
        ~SessionRouterI() override;
        void destroy();

        std::optional<Ice::ObjectPrx> getClientProxy(std::optional<bool>&, const Ice::Current&) const override;
        std::optional<Ice::ObjectPrx> getServerProxy(const Ice::Current&) const override;
        Ice::ObjectProxySeq addProxies(Ice::ObjectProxySeq, const Ice::Current&) override;
        std::string getCategoryForClient(const Ice::Current&) const override;
        void createSessionAsync(
            std::string,
            std::string,
            std::function<void(const std::optional<SessionPrx>&)>,
            std::function<void(std::exception_ptr)>,
            const Ice::Current&) override;
        void createSessionFromSecureConnectionAsync(
            std::function<void(const std::optional<SessionPrx>&)>,
            std::function<void(std::exception_ptr)>,
            const Ice::Current&) override;
        void refreshSessionAsync(std::function<void()>, std::function<void(std::exception_ptr)>, const Ice::Current&)
            override;
        void destroySession(const Ice::Current&) override;
        std::int64_t getSessionTimeout(const Ice::Current&) const override;
        int getACMTimeout(const Ice::Current&) const override;

        void updateSessionObservers() override;

        std::shared_ptr<RouterI>
        getRouter(const std::shared_ptr<Ice::Connection>&, const Ice::Identity&, bool = true) const;

        Ice::ObjectPtr getClientBlobject(const std::shared_ptr<Ice::Connection>&, const Ice::Identity&) const;
        Ice::ObjectPtr getServerBlobject(const std::string&) const;

        void refreshSession(const std::shared_ptr<Ice::Connection>&);
        void destroySession(const std::shared_ptr<Ice::Connection>&);

        int sessionTraceLevel() const { return _sessionTraceLevel; }

    private:
        std::shared_ptr<RouterI>
        getRouterImpl(const std::shared_ptr<Ice::Connection>&, const Ice::Identity&, bool) const;

        void sessionDestroyException(std::exception_ptr);

        bool startCreateSession(const std::shared_ptr<CreateSession>&, const std::shared_ptr<Ice::Connection>&);
        void finishCreateSession(const std::shared_ptr<Ice::Connection>&, const std::shared_ptr<RouterI>&);

        friend class Glacier2::CreateSession;
        friend class Glacier2::UserPasswordCreateSession;
        friend class Glacier2::SSLCreateSession;

        const std::shared_ptr<Instance> _instance;
        const int _sessionTraceLevel;
        const int _rejectTraceLevel;
        const std::optional<PermissionsVerifierPrx> _verifier;
        const std::optional<SessionManagerPrx> _sessionManager;
        const std::optional<SSLPermissionsVerifierPrx> _sslVerifier;
        const std::optional<SSLSessionManagerPrx> _sslSessionManager;

        std::map<std::shared_ptr<Ice::Connection>, std::shared_ptr<RouterI>> _routersByConnection;
        mutable std::map<std::shared_ptr<Ice::Connection>, std::shared_ptr<RouterI>>::const_iterator
            _routersByConnectionHint;

        std::map<std::string, std::shared_ptr<RouterI>> _routersByCategory;
        mutable std::map<std::string, std::shared_ptr<RouterI>>::const_iterator _routersByCategoryHint;

        std::map<std::shared_ptr<Ice::Connection>, std::shared_ptr<CreateSession>> _pending;

        bool _destroy;

        mutable std::mutex _mutex;
    };
}

#endif
