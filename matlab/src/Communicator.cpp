//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Communicator.h"
#include "Ice/Proxy.h"
#include "Ice/ImplicitContext.h"
#include "Ice/Logger.h"
#include "ice.h"
#include "Future.h"
#include "Util.h"

using namespace std;
using namespace IceMatlab;

extern "C"
{
    mxArray* Ice_Communicator_unref(void* self)
    {
        delete reinterpret_cast<shared_ptr<Ice::Communicator>*>(self);
        return 0;
    }

    mxArray* Ice_Communicator_destroy(void* self)
    {
        deref<Ice::Communicator>(self)->destroy();
        return 0;
    }

    mxArray* Ice_Communicator_destroyAsync(void* self, void** future)
    {
        *future = 0;
        auto c = deref<Ice::Communicator>(self);
        auto f = make_shared<SimpleFuture>();

        thread t(
            [c, f]
            {
                try
                {
                    c->destroy();
                    f->done();
                }
                catch (const std::exception&)
                {
                    f->exception(current_exception());
                }
            });
        t.detach();
        *future = new shared_ptr<SimpleFuture>(move(f));
        return 0;
    }

    mxArray* Ice_Communicator_stringToProxy(void* self, const char* s, void** proxy)
    {
        try
        {
            *proxy = createProxy(deref<Ice::Communicator>(self)->stringToProxy(s));
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_proxyToString(void* self, void* proxy)
    {
        assert(proxy);
        try
        {
            auto p = restoreProxy(proxy);
            return createResultValue(createStringFromUTF8(deref<Ice::Communicator>(self)->proxyToString(p)));
        }
        catch (...)
        {
            return createResultException(convertException(std::current_exception()));
        }
    }

    mxArray* Ice_Communicator_propertyToProxy(void* self, const char* prop, void** proxy)
    {
        try
        {
            *proxy = createProxy(deref<Ice::Communicator>(self)->propertyToProxy(prop));
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_proxyToProperty(void* self, void* proxy, const char* prop)
    {
        assert(proxy);
        try
        {
            auto p = restoreProxy(proxy);
            auto d = deref<Ice::Communicator>(self)->proxyToProperty(p, prop);
            return createResultValue(createStringMap(d));
        }
        catch (...)
        {
            return createResultException(convertException(std::current_exception()));
        }
    }

    mxArray* Ice_Communicator_identityToString(void* self, mxArray* id)
    {
        try
        {
            Ice::Identity ident;
            getIdentity(id, ident);
            return createResultValue(createStringFromUTF8(deref<Ice::Communicator>(self)->identityToString(ident)));
        }
        catch (...)
        {
            return createResultException(convertException(std::current_exception()));
        }
    }

    mxArray* Ice_Communicator_getImplicitContext(void* self, void** ctx)
    {
        try
        {
            auto p = deref<Ice::Communicator>(self)->getImplicitContext();
            *ctx = createShared<Ice::ImplicitContext>(p);
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_getProperties(void* self, void** props)
    {
        try
        {
            auto p = deref<Ice::Communicator>(self)->getProperties();
            *props = new shared_ptr<Ice::Properties>(move(p));
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_getLogger(void* self, void** logger)
    {
        try
        {
            auto l = deref<Ice::Communicator>(self)->getLogger();
            *logger = createShared<Ice::Logger>(l);
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_getDefaultRouter(void* self, void** proxy)
    {
        try
        {
            *proxy = createProxy(deref<Ice::Communicator>(self)->getDefaultRouter());
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_setDefaultRouter(void* self, void* proxy)
    {
        try
        {
            optional<Ice::RouterPrx> p = Ice::uncheckedCast<Ice::RouterPrx>(restoreNullableProxy(proxy));
            deref<Ice::Communicator>(self)->setDefaultRouter(p);
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_getDefaultLocator(void* self, void** proxy)
    {
        try
        {
            *proxy = createProxy(deref<Ice::Communicator>(self)->getDefaultLocator());
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_setDefaultLocator(void* self, void* proxy)
    {
        try
        {
            optional<Ice::LocatorPrx> p = Ice::uncheckedCast<Ice::LocatorPrx>(restoreNullableProxy(proxy));
            deref<Ice::Communicator>(self)->setDefaultLocator(p);
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_flushBatchRequests(void* self, mxArray* mode)
    {
        try
        {
            auto m = static_cast<Ice::CompressBatch>(getEnumerator(mode, "Ice.CompressBatch"));
            deref<Ice::Communicator>(self)->flushBatchRequests(m);
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }

    mxArray* Ice_Communicator_flushBatchRequestsAsync(void* self, mxArray* mode, void** future)
    {
        *future = 0;
        auto f = make_shared<SimpleFuture>();

        try
        {
            auto m = static_cast<Ice::CompressBatch>(getEnumerator(mode, "Ice.CompressBatch"));
            function<void()> token = deref<Ice::Communicator>(self)->flushBatchRequestsAsync(
                m,
                [f](exception_ptr e) { f->exception(e); },
                [f](bool /*sentSynchronously*/) { f->done(); });
            f->token(token);
            *future = new shared_ptr<SimpleFuture>(move(f));
        }
        catch (...)
        {
            return convertException(std::current_exception());
        }
        return 0;
    }
}
