//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Communicator.h"
#include "BatchRequestInterceptor.h"
#include "Dispatcher.h"
#include "Ice/DisableWarnings.h"
#include "Ice/Initialize.h"
#include "Ice/LocalExceptions.h"
#include "Ice/Locator.h"
#include "Ice/ObjectAdapter.h"
#include "Ice/Properties.h"
#include "Ice/Router.h"
#include "Ice/ValueFactory.h"
#include "ImplicitContext.h"
#include "Logger.h"
#include "ObjectAdapter.h"
#include "Operation.h"
#include "Properties.h"
#include "PropertiesAdmin.h"
#include "Proxy.h"
#include "Thread.h"
#include "Types.h"
#include "Util.h"
#include "ValueFactoryManager.h"

#include <pythread.h>

#include <future>

using namespace std;
using namespace IcePy;

#if defined(__GNUC__) && ((__GNUC__ >= 8))
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

static unsigned long _mainThreadId;

using CommunicatorMap = map<Ice::CommunicatorPtr, PyObject*>;
static CommunicatorMap _communicatorMap;

namespace IcePy
{
    struct CommunicatorObject
    {
        PyObject_HEAD Ice::CommunicatorPtr* communicator;
        PyObject* wrapper;
        std::future<void>* shutdownFuture;
        std::exception_ptr* shutdownException;
        bool shutdown;
        DispatcherPtr* dispatcher;
    };
}

extern "C" CommunicatorObject*
communicatorNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    assert(type && type->tp_alloc);
    CommunicatorObject* self = reinterpret_cast<CommunicatorObject*>(type->tp_alloc(type, 0));
    if (!self)
    {
        return nullptr;
    }
    self->communicator = 0;
    self->wrapper = 0;
    self->shutdownFuture = nullptr;
    self->shutdownException = nullptr;
    self->shutdown = false;
    self->dispatcher = 0;
    return self;
}

extern "C" int
communicatorInit(CommunicatorObject* self, PyObject* args, PyObject* /*kwds*/)
{
    //
    // The argument options are:
    //
    // Ice.initialize()
    // Ice.initialize(args)
    // Ice.initialize(initData)
    // Ice.initialize(configFile)
    // Ice.initialize(args, initData)
    // Ice.initialize(args, configFile)
    //

    PyObject* arg1 = 0;
    PyObject* arg2 = 0;
    if (!PyArg_ParseTuple(args, "|OO", &arg1, &arg2))
    {
        return -1;
    }

    PyObject* argList = 0;
    PyObject* initData = 0;
    PyObject* configFile = 0;

    if (arg1 == Py_None)
    {
        arg1 = 0;
    }

    if (arg2 == Py_None)
    {
        arg2 = 0;
    }

    PyObject* initDataType = lookupType("Ice.InitializationData");

    if (arg1)
    {
        if (PyList_Check(arg1))
        {
            argList = arg1;
        }
        else if (PyObject_IsInstance(arg1, initDataType))
        {
            initData = arg1;
        }
        else if (checkString(arg1))
        {
            configFile = arg1;
        }
        else
        {
            PyErr_Format(
                PyExc_ValueError,
                "initialize expects an argument list, Ice.InitializationData or a configuration filename");
            return -1;
        }
    }

    if (arg2)
    {
        if (PyList_Check(arg2))
        {
            if (argList)
            {
                PyErr_Format(PyExc_ValueError, "unexpected list argument to initialize");
                return -1;
            }
            argList = arg2;
        }
        else if (PyObject_IsInstance(arg2, initDataType))
        {
            if (initData)
            {
                PyErr_Format(PyExc_ValueError, "unexpected Ice.InitializationData argument to initialize");
                return -1;
            }
            initData = arg2;
        }
        else if (checkString(arg2))
        {
            if (configFile)
            {
                PyErr_Format(PyExc_ValueError, "unexpected string argument to initialize");
                return -1;
            }
            configFile = arg2;
        }
        else
        {
            PyErr_Format(
                PyExc_ValueError,
                "initialize expects an argument list, Ice.InitializationData or a configuration filename");
            return -1;
        }
    }

    if (initData && configFile)
    {
        PyErr_Format(PyExc_ValueError, "initialize accepts either Ice.InitializationData or a configuration filename");
        return -1;
    }

    Ice::StringSeq seq;
    if (argList && !listToStringSeq(argList, seq))
    {
        return -1;
    }

    Ice::InitializationData data;
    DispatcherPtr dispatcherWrapper;

    try
    {
        if (initData)
        {
            PyObjectHandle properties{getAttr(initData, "properties", false)};
            PyObjectHandle logger{getAttr(initData, "logger", false)};
            PyObjectHandle threadStart{getAttr(initData, "threadStart", false)};
            PyObjectHandle threadStop{getAttr(initData, "threadStop", false)};
            PyObjectHandle batchRequestInterceptor{getAttr(initData, "batchRequestInterceptor", false)};
            PyObjectHandle dispatcher{getAttr(initData, "dispatcher", false)};

            if (properties.get())
            {
                //
                // Get the properties implementation.
                //
                PyObjectHandle impl{getAttr(properties.get(), "_impl", false)};
                assert(impl.get());
                data.properties = getProperties(impl.get());
            }

            if (logger.get())
            {
                data.logger = make_shared<LoggerWrapper>(logger.get());
            }

            if (threadStart.get() || threadStop.get())
            {
                auto threadHook = make_shared<ThreadHook>(threadStart.get(), threadStop.get());
                data.threadStart = [threadHook]() { threadHook->start(); };
                data.threadStop = [threadHook]() { threadHook->stop(); };
            }

            // TODO: rename dispatch to executor
            if (dispatcher.get())
            {
                dispatcherWrapper = make_shared<Dispatcher>(dispatcher.get());
                data.executor =
                    [dispatcherWrapper](function<void()> call, const shared_ptr<Ice::Connection>& connection)
                { dispatcherWrapper->dispatch(call, connection); };
            }

            if (batchRequestInterceptor.get())
            {
                auto batchRequestInterceptorWrapper =
                    make_shared<BatchRequestInterceptorWrapper>(batchRequestInterceptor.get());
                data.batchRequestInterceptor =
                    [batchRequestInterceptorWrapper](const Ice::BatchRequest& req, int count, int size)
                { batchRequestInterceptorWrapper->enqueue(req, count, size); };
            }
        }

        //
        // We always supply our own implementation of ValueFactoryManager.
        //
        data.valueFactoryManager = ValueFactoryManager::create();

        if (!data.properties)
        {
            data.properties = Ice::createProperties();
        }

        if (configFile)
        {
            data.properties->load(getString(configFile));
        }

        if (argList)
        {
            data.properties = Ice::createProperties(seq, data.properties);
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return -1;
    }

    //
    // Remaining command line options are passed to the communicator
    // as an argument vector in case they contain plug-in properties.
    //
    int argc = static_cast<int>(seq.size());
    char** argv = new char*[static_cast<size_t>(argc) + 1];
    int i = 0;
    for (Ice::StringSeq::const_iterator s = seq.begin(); s != seq.end(); ++s, ++i)
    {
        argv[i] = strdup(s->c_str());
    }
    argv[argc] = 0;

    data.compactIdResolver = resolveCompactId;

    // Always accept cycles in Python
    data.properties->setProperty("Ice.AcceptClassCycles", "1");

    Ice::CommunicatorPtr communicator;
    try
    {
        AllowThreads allowThreads;
        if (argList)
        {
            communicator = Ice::initialize(argc, argv, data);
        }
        else
        {
            communicator = Ice::initialize(data);
        }
    }
    catch (...)
    {
        for (i = 0; i < argc; ++i)
        {
            free(argv[i]);
        }
        delete[] argv;

        setPythonException(current_exception());
        return -1;
    }

    //
    // Replace the contents of the given argument list with the filtered arguments.
    //
    if (argList)
    {
        PyList_SetSlice(argList, 0, PyList_Size(argList), 0); // Clear the list.

        for (i = 0; i < argc; ++i)
        {
            PyObjectHandle str{Py_BuildValue("s", argv[i])};
            PyList_Append(argList, str.get());
        }
    }

    for (i = 0; i < argc; ++i)
    {
        free(argv[i]);
    }
    delete[] argv;

    self->communicator = new Ice::CommunicatorPtr(communicator);
    _communicatorMap.insert(CommunicatorMap::value_type(communicator, reinterpret_cast<PyObject*>(self)));

    if (dispatcherWrapper)
    {
        self->dispatcher = new DispatcherPtr(dispatcherWrapper);
        dispatcherWrapper->setCommunicator(communicator);
    }

    return 0;
}

extern "C" void
communicatorDealloc(CommunicatorObject* self)
{
    if (self->communicator)
    {
        CommunicatorMap::iterator p = _communicatorMap.find(*self->communicator);
        //
        // find() can fail if an error occurred during communicator initialization.
        //
        if (p != _communicatorMap.end())
        {
            _communicatorMap.erase(p);
        }
    }

    delete self->communicator;
    delete self->shutdownException;
    delete self->shutdownFuture;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

extern "C" PyObject*
communicatorDestroy(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);

    auto vfm = dynamic_pointer_cast<ValueFactoryManager>((*self->communicator)->getValueFactoryManager());
    assert(vfm);

    try
    {
        AllowThreads allowThreads; // Release Python's global interpreter lock to avoid a potential deadlock.
        (*self->communicator)->destroy();
    }
    catch (...)
    {
        setPythonException(current_exception());
    }

    vfm->destroy();

    if (self->dispatcher)
    {
        (*self->dispatcher)->setCommunicator(nullptr); // Break cyclic reference.
    }

    //
    // Break cyclic reference between this object and its Python wrapper.
    //
    Py_XDECREF(self->wrapper);
    self->wrapper = 0;

    if (PyErr_Occurred())
    {
        return nullptr;
    }
    else
    {
        return Py_None;
    }
}

extern "C" PyObject*
communicatorShutdown(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    try
    {
        AllowThreads allowThreads; // Release Python's global interpreter lock to avoid a potential deadlock.
        (*self->communicator)->shutdown();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorWaitForShutdown(CommunicatorObject* self, PyObject* args)
{
    //
    // This method differs somewhat from the standard Ice API because of
    // signal issues. This method expects an integer timeout value, and
    // returns a boolean to indicate whether it was successful. When
    // called from the main thread, the timeout is used to allow control
    // to return to the caller (the Python interpreter) periodically.
    // When called from any other thread, we call waitForShutdown directly
    // and ignore the timeout.
    //
    int timeout = 0;
    if (!PyArg_ParseTuple(args, "i", &timeout))
    {
        return nullptr;
    }

    assert(timeout > 0);
    assert(self->communicator);

    //
    // Do not call waitForShutdown from the main thread, because it prevents
    // signals (such as keyboard interrupts) from being delivered to Python.
    //
    if (PyThread_get_thread_ident() == _mainThreadId)
    {
        if (!self->shutdown)
        {
            if (self->shutdownFuture == nullptr)
            {
                self->shutdownFuture = new std::future<void>();
                *self->shutdownFuture =
                    std::async(std::launch::async, [&self] { (*self->communicator)->waitForShutdown(); });
            }

            {
                AllowThreads allowThreads; // Release Python's global interpreter lock during blocking calls.
                if (self->shutdownFuture->wait_for(std::chrono::milliseconds(timeout)) == std::future_status::timeout)
                {
                    return Py_False;
                }
            }

            self->shutdown = true;
            try
            {
                self->shutdownFuture->get();
            }
            catch (...)
            {
                self->shutdownException = new std::exception_ptr(std::current_exception());
            }
        }

        assert(self->shutdown);
        if (self->shutdownException)
        {
            setPythonException(*self->shutdownException);
            return nullptr;
        }
    }
    else
    {
        try
        {
            AllowThreads allowThreads; // Release Python's global interpreter lock during blocking calls.
            (*self->communicator)->waitForShutdown();
        }
        catch (...)
        {
            setPythonException(current_exception());
            return nullptr;
        }
    }

    return Py_True;
}

extern "C" PyObject*
communicatorIsShutdown(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    bool isShutdown;
    try
    {
        isShutdown = (*self->communicator)->isShutdown();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return isShutdown ? Py_True : Py_False;
}

extern "C" PyObject*
communicatorStringToProxy(CommunicatorObject* self, PyObject* args)
{
    PyObject* strObj;
    if (!PyArg_ParseTuple(args, "O", &strObj))
    {
        return nullptr;
    }

    string str;
    if (!getStringArg(strObj, "str", str))
    {
        return nullptr;
    }

    assert(self->communicator);
    optional<Ice::ObjectPrx> proxy;
    try
    {
        proxy = (*self->communicator)->stringToProxy(str);
        if (proxy)
        {
            return createProxy(proxy.value(), *self->communicator);
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorProxyToString(CommunicatorObject* self, PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
    {
        return nullptr;
    }

    optional<Ice::ObjectPrx> proxy;
    if (!getProxyArg(obj, "proxyToString", "obj", proxy))
    {
        return nullptr;
    }

    string str;

    assert(self->communicator);
    try
    {
        str = (*self->communicator)->proxyToString(proxy);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return createString(str);
}

extern "C" PyObject*
communicatorPropertyToProxy(CommunicatorObject* self, PyObject* args)
{
    PyObject* strObj;
    if (!PyArg_ParseTuple(args, "O", &strObj))
    {
        return nullptr;
    }

    string str;
    if (!getStringArg(strObj, "property", str))
    {
        return nullptr;
    }

    assert(self->communicator);
    optional<Ice::ObjectPrx> proxy;
    try
    {
        proxy = (*self->communicator)->propertyToProxy(str);
        if (proxy)
        {
            return createProxy(proxy.value(), *self->communicator);
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorProxyToProperty(CommunicatorObject* self, PyObject* args)
{
    //
    // We don't want to accept None here, so we can specify ProxyType and force
    // the caller to supply a proxy object.
    //
    PyObject* proxyObj;
    PyObject* strObj;
    if (!PyArg_ParseTuple(args, "O!O", &ProxyType, &proxyObj, &strObj))
    {
        return nullptr;
    }

    Ice::ObjectPrx proxy = getProxy(proxyObj);
    string str;
    if (!getStringArg(strObj, "property", str))
    {
        return nullptr;
    }

    assert(self->communicator);
    Ice::PropertyDict dict;
    try
    {
        dict = (*self->communicator)->proxyToProperty(proxy, str);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObjectHandle result{PyDict_New()};
    if (result.get())
    {
        for (Ice::PropertyDict::iterator p = dict.begin(); p != dict.end(); ++p)
        {
            PyObjectHandle key{createString(p->first)};
            PyObjectHandle val{createString(p->second)};
            if (!val.get() || PyDict_SetItem(result.get(), key.get(), val.get()) < 0)
            {
                return nullptr;
            }
        }
    }

    return result.release();
}

extern "C" PyObject*
communicatorIdentityToString(CommunicatorObject* self, PyObject* args)
{
    PyObject* identityType = lookupType("Ice.Identity");
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", identityType, &obj))
    {
        return nullptr;
    }

    Ice::Identity id;
    if (!getIdentity(obj, id))
    {
        return nullptr;
    }
    string str;

    assert(self->communicator);
    try
    {
        str = (*self->communicator)->identityToString(id);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return createString(str);
}

extern "C" PyObject*
communicatorFlushBatchRequests(CommunicatorObject* self, PyObject* args)
{
    PyObject* compressBatchType = lookupType("Ice.CompressBatch");
    PyObject* compressBatch;
    if (!PyArg_ParseTuple(args, "O!", compressBatchType, &compressBatch))
    {
        return nullptr;
    }

    PyObjectHandle v{getAttr(compressBatch, "_value", false)};
    assert(v.get());
    Ice::CompressBatch cb = static_cast<Ice::CompressBatch>(PyLong_AsLong(v.get()));

    assert(self->communicator);
    try
    {
        AllowThreads allowThreads; // Release Python's global interpreter lock to avoid a potential deadlock.
        (*self->communicator)->flushBatchRequests(cb);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorFlushBatchRequestsAsync(CommunicatorObject* self, PyObject* args, PyObject* /*kwds*/)
{
    PyObject* compressBatchType = lookupType("Ice.CompressBatch");
    PyObject* compressBatch;
    if (!PyArg_ParseTuple(args, "O!", compressBatchType, &compressBatch))
    {
        return nullptr;
    }

    PyObjectHandle v{getAttr(compressBatch, "_value", false)};
    assert(v.get());
    Ice::CompressBatch compress = static_cast<Ice::CompressBatch>(PyLong_AsLong(v.get()));

    assert(self->communicator);
    const string op = "flushBatchRequests";

    auto callback = make_shared<FlushAsyncCallback>(op);
    function<void()> cancel;
    try
    {
        cancel = (*self->communicator)
                     ->flushBatchRequestsAsync(
                         compress,
                         [callback](exception_ptr ex) { callback->exception(ex); },
                         [callback](bool sentSynchronously) { callback->sent(sentSynchronously); });
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObjectHandle asyncInvocationContextObj{createAsyncInvocationContext(std::move(cancel), *self->communicator)};
    if (!asyncInvocationContextObj.get())
    {
        return nullptr;
    }

    PyObjectHandle future{createFuture(op, asyncInvocationContextObj.get())};
    if (!future.get())
    {
        return nullptr;
    }
    callback->setFuture(future.get());
    return future.release();
}

extern "C" PyObject*
communicatorCreateAdmin(CommunicatorObject* self, PyObject* args)
{
    PyObject* adapter;
    PyObject* identityType = lookupType("Ice.Identity");
    PyObject* id;
    if (!PyArg_ParseTuple(args, "OO!", &adapter, identityType, &id))
    {
        return nullptr;
    }

    Ice::ObjectAdapterPtr oa;

    PyObject* adapterType = lookupType("Ice.ObjectAdapter");
    if (adapter != Py_None && !PyObject_IsInstance(adapter, adapterType))
    {
        PyErr_Format(PyExc_ValueError, "expected ObjectAdapter or None");
        return nullptr;
    }

    if (adapter != Py_None)
    {
        oa = unwrapObjectAdapter(adapter);
    }

    Ice::Identity identity;
    if (!getIdentity(id, identity))
    {
        return nullptr;
    }

    assert(self->communicator);
    optional<Ice::ObjectPrx> proxy;
    try
    {
        proxy = (*self->communicator)->createAdmin(oa, identity);
        assert(proxy);

        return createProxy(proxy.value(), *self->communicator);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }
}

extern "C" PyObject*
communicatorGetAdmin(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    optional<Ice::ObjectPrx> proxy;
    try
    {
        proxy = (*self->communicator)->getAdmin();
        if (proxy)
        {
            return createProxy(proxy.value(), *self->communicator);
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorAddAdminFacet(CommunicatorObject* self, PyObject* args)
{
    PyObject* objectType = lookupType("Ice.Object");
    PyObject* servant;
    PyObject* facetObj;
    if (!PyArg_ParseTuple(args, "O!O", objectType, &servant, &facetObj))
    {
        return nullptr;
    }

    string facet;
    if (!getStringArg(facetObj, "facet", facet))
    {
        return nullptr;
    }

    ServantWrapperPtr wrapper = createServantWrapper(servant);
    if (PyErr_Occurred())
    {
        return nullptr;
    }

    assert(self->communicator);
    try
    {
        (*self->communicator)->addAdminFacet(wrapper, facet);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorFindAdminFacet(CommunicatorObject* self, PyObject* args)
{
    PyObject* facetObj;
    if (!PyArg_ParseTuple(args, "O", &facetObj))
    {
        return nullptr;
    }

    string facet;
    if (!getStringArg(facetObj, "facet", facet))
    {
        return nullptr;
    }

    assert(self->communicator);
    try
    {
        //
        // The facet being found may not be implemented by a Python servant
        // (e.g., it could be the Process or Properties facet), in which case
        // we return None.
        //
        Ice::ObjectPtr obj = (*self->communicator)->findAdminFacet(facet);
        if (obj)
        {
            ServantWrapperPtr wrapper = dynamic_pointer_cast<ServantWrapper>(obj);
            if (wrapper)
            {
                return wrapper->getObject();
            }

            Ice::NativePropertiesAdminPtr props = dynamic_pointer_cast<Ice::NativePropertiesAdmin>(obj);
            if (props)
            {
                return createNativePropertiesAdmin(props);
            }

            // If the facet isn't supported in Python, just return an Ice.Object.
            PyTypeObject* objectType = reinterpret_cast<PyTypeObject*>(lookupType("Ice.Object"));
            return objectType->tp_alloc(objectType, 0);
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorFindAllAdminFacets(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    Ice::FacetMap facetMap;
    try
    {
        facetMap = (*self->communicator)->findAllAdminFacets();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObjectHandle result{PyDict_New()};
    if (!result.get())
    {
        return nullptr;
    }

    PyTypeObject* objectType = reinterpret_cast<PyTypeObject*>(lookupType("Ice.Object"));
    PyObjectHandle plainObject{objectType->tp_alloc(objectType, 0)};

    for (Ice::FacetMap::const_iterator p = facetMap.begin(); p != facetMap.end(); ++p)
    {
        PyObjectHandle obj = plainObject;

        ServantWrapperPtr wrapper = dynamic_pointer_cast<ServantWrapper>(p->second);
        if (wrapper)
        {
            obj = wrapper->getObject();
        }
        else
        {
            Ice::NativePropertiesAdminPtr props = dynamic_pointer_cast<Ice::NativePropertiesAdmin>(p->second);
            if (props)
            {
                obj = createNativePropertiesAdmin(props);
            }
        }

        if (PyDict_SetItemString(result.get(), const_cast<char*>(p->first.c_str()), obj.get()) < 0)
        {
            return nullptr;
        }
    }

    return result.release();
}

extern "C" PyObject*
communicatorRemoveAdminFacet(CommunicatorObject* self, PyObject* args)
{
    PyObject* facetObj;
    if (!PyArg_ParseTuple(args, "O", &facetObj))
    {
        return nullptr;
    }

    string facet;
    if (!getStringArg(facetObj, "facet", facet))
    {
        return nullptr;
    }

    assert(self->communicator);
    try
    {
        //
        // The facet being removed may not be implemented by a Python servant
        // (e.g., it could be the Process or Properties facet), in which case
        // we return None.
        //
        Ice::ObjectPtr obj = (*self->communicator)->removeAdminFacet(facet);
        assert(obj);
        ServantWrapperPtr wrapper = dynamic_pointer_cast<ServantWrapper>(obj);
        if (wrapper)
        {
            return wrapper->getObject();
        }
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorSetWrapper(CommunicatorObject* self, PyObject* args)
{
    PyObject* wrapper;
    if (!PyArg_ParseTuple(args, "O", &wrapper))
    {
        return nullptr;
    }

    assert(!self->wrapper);
    self->wrapper = wrapper;
    Py_INCREF(self->wrapper);

    return Py_None;
}

extern "C" PyObject*
communicatorGetWrapper(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->wrapper);
    Py_INCREF(self->wrapper);
    return self->wrapper;
}

extern "C" PyObject*
communicatorGetProperties(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    Ice::PropertiesPtr properties;
    try
    {
        properties = (*self->communicator)->getProperties();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return createProperties(properties);
}

extern "C" PyObject*
communicatorGetLogger(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    Ice::LoggerPtr logger;
    try
    {
        logger = (*self->communicator)->getLogger();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    //
    // The communicator's logger can either be a C++ object (such as
    // the default logger supplied by the Ice run time), or a C++
    // wrapper around a Python implementation. If the latter, we
    // return it directly. Otherwise, we create a Python object
    // that delegates to the C++ object.
    //
    LoggerWrapperPtr wrapper = dynamic_pointer_cast<LoggerWrapper>(logger);
    if (wrapper)
    {
        PyObject* obj = wrapper->getObject();
        Py_INCREF(obj);
        return obj;
    }

    return createLogger(logger);
}

extern "C" PyObject*
communicatorGetValueFactoryManager(CommunicatorObject* self, PyObject* /*args*/)
{
    auto vfm = dynamic_pointer_cast<ValueFactoryManager>((*self->communicator)->getValueFactoryManager());
    return vfm->getObject();
}

extern "C" PyObject*
communicatorGetImplicitContext(CommunicatorObject* self, PyObject* /*args*/)
{
    Ice::ImplicitContextPtr implicitContext = (*self->communicator)->getImplicitContext();

    if (implicitContext == 0)
    {
        return Py_None;
    }

    return createImplicitContext(implicitContext);
}

extern "C" PyObject*
communicatorCreateObjectAdapter(CommunicatorObject* self, PyObject* args)
{
    PyObject* strObj;
    if (!PyArg_ParseTuple(args, "O", &strObj))
    {
        return nullptr;
    }

    string name;
    if (!getStringArg(strObj, "name", name))
    {
        return nullptr;
    }

    assert(self->communicator);
    Ice::ObjectAdapterPtr adapter;
    try
    {
        adapter = (*self->communicator)->createObjectAdapter(name);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObject* obj = createObjectAdapter(adapter);
    if (!obj)
    {
        try
        {
            adapter->deactivate();
        }
        catch (const Ice::Exception&)
        {
        }
    }

    return obj;
}

extern "C" PyObject*
communicatorCreateObjectAdapterWithEndpoints(CommunicatorObject* self, PyObject* args)
{
    PyObject* nameObj;
    PyObject* endpointsObj;
    if (!PyArg_ParseTuple(args, "OO", &nameObj, &endpointsObj))
    {
        return nullptr;
    }

    string name;
    string endpoints;
    if (!getStringArg(nameObj, "name", name))
    {
        return nullptr;
    }
    if (!getStringArg(endpointsObj, "endpoints", endpoints))
    {
        return nullptr;
    }

    assert(self->communicator);
    Ice::ObjectAdapterPtr adapter;
    try
    {
        adapter = (*self->communicator)->createObjectAdapterWithEndpoints(name, endpoints);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObject* obj = createObjectAdapter(adapter);
    if (!obj)
    {
        try
        {
            adapter->deactivate();
        }
        catch (const Ice::Exception&)
        {
        }
    }

    return obj;
}

extern "C" PyObject*
communicatorCreateObjectAdapterWithRouter(CommunicatorObject* self, PyObject* args)
{
    PyObject* nameObj;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "OO", &nameObj, &p))
    {
        return nullptr;
    }

    string name;
    if (!getStringArg(nameObj, "name", name))
    {
        return nullptr;
    }

    optional<Ice::ObjectPrx> proxy;
    if (!getProxyArg(p, "createObjectAdapterWithRouter", "rtr", proxy, "Ice.RouterPrx"))
    {
        return nullptr;
    }

    optional<Ice::RouterPrx> router = Ice::uncheckedCast<Ice::RouterPrx>(proxy);

    assert(self->communicator);
    Ice::ObjectAdapterPtr adapter;
    try
    {
        AllowThreads allowThreads; // Release Python's global interpreter lock to avoid a potential deadlock.
        adapter = (*self->communicator)->createObjectAdapterWithRouter(name, router.value());
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    PyObject* obj = createObjectAdapter(adapter);
    if (!obj)
    {
        try
        {
            adapter->deactivate();
        }
        catch (const Ice::Exception&)
        {
        }
    }

    return obj;
}

extern "C" PyObject*
communicatorGetDefaultRouter(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    optional<Ice::RouterPrx> router;
    try
    {
        router = (*self->communicator)->getDefaultRouter();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    if (!router)
    {
        return Py_None;
    }

    PyObject* routerProxyType = lookupType("Ice.RouterPrx");
    assert(routerProxyType);
    return createProxy(router.value(), *self->communicator, routerProxyType);
}

extern "C" PyObject*
communicatorSetDefaultRouter(CommunicatorObject* self, PyObject* args)
{
    PyObject* p;
    if (!PyArg_ParseTuple(args, "O", &p))
    {
        return nullptr;
    }

    optional<Ice::ObjectPrx> proxy;
    if (!getProxyArg(p, "setDefaultRouter", "rtr", proxy, "Ice.RouterPrx"))
    {
        return nullptr;
    }

    optional<Ice::RouterPrx> router = Ice::uncheckedCast<Ice::RouterPrx>(proxy);

    assert(self->communicator);
    try
    {
        (*self->communicator)->setDefaultRouter(router);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

extern "C" PyObject*
communicatorGetDefaultLocator(CommunicatorObject* self, PyObject* /*args*/)
{
    assert(self->communicator);
    optional<Ice::LocatorPrx> locator;
    try
    {
        locator = (*self->communicator)->getDefaultLocator();
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    if (!locator)
    {
        return Py_None;
    }

    PyObject* locatorProxyType = lookupType("Ice.LocatorPrx");
    assert(locatorProxyType);
    return createProxy(locator.value(), *self->communicator, locatorProxyType);
}

extern "C" PyObject*
communicatorSetDefaultLocator(CommunicatorObject* self, PyObject* args)
{
    PyObject* p;
    if (!PyArg_ParseTuple(args, "O", &p))
    {
        return nullptr;
    }

    optional<Ice::ObjectPrx> proxy;
    if (!getProxyArg(p, "setDefaultLocator", "loc", proxy, "Ice.LocatorPrx"))
    {
        return nullptr;
    }

    optional<Ice::LocatorPrx> locator = Ice::uncheckedCast<Ice::LocatorPrx>(proxy);

    assert(self->communicator);
    try
    {
        (*self->communicator)->setDefaultLocator(locator);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return Py_None;
}

static PyMethodDef CommunicatorMethods[] = {
    {"destroy", reinterpret_cast<PyCFunction>(communicatorDestroy), METH_NOARGS, PyDoc_STR("destroy() -> None")},
    {"shutdown", reinterpret_cast<PyCFunction>(communicatorShutdown), METH_NOARGS, PyDoc_STR("shutdown() -> None")},
    {"waitForShutdown",
     reinterpret_cast<PyCFunction>(communicatorWaitForShutdown),
     METH_VARARGS,
     PyDoc_STR("waitForShutdown() -> None")},
    {"isShutdown",
     reinterpret_cast<PyCFunction>(communicatorIsShutdown),
     METH_NOARGS,
     PyDoc_STR("isShutdown() -> bool")},
    {"stringToProxy",
     reinterpret_cast<PyCFunction>(communicatorStringToProxy),
     METH_VARARGS,
     PyDoc_STR("stringToProxy(str) -> Ice.ObjectPrx")},
    {"proxyToString",
     reinterpret_cast<PyCFunction>(communicatorProxyToString),
     METH_VARARGS,
     PyDoc_STR("proxyToString(Ice.ObjectPrx) -> string")},
    {"propertyToProxy",
     reinterpret_cast<PyCFunction>(communicatorPropertyToProxy),
     METH_VARARGS,
     PyDoc_STR("propertyToProxy(str) -> Ice.ObjectPrx")},
    {"proxyToProperty",
     reinterpret_cast<PyCFunction>(communicatorProxyToProperty),
     METH_VARARGS,
     PyDoc_STR("proxyToProperty(Ice.ObjectPrx, str) -> dict")},
    {"identityToString",
     reinterpret_cast<PyCFunction>(communicatorIdentityToString),
     METH_VARARGS,
     PyDoc_STR("identityToString(Ice.Identity) -> string")},
    {"createObjectAdapter",
     reinterpret_cast<PyCFunction>(communicatorCreateObjectAdapter),
     METH_VARARGS,
     PyDoc_STR("createObjectAdapter(name) -> Ice.ObjectAdapter")},
    {"createObjectAdapterWithEndpoints",
     reinterpret_cast<PyCFunction>(communicatorCreateObjectAdapterWithEndpoints),
     METH_VARARGS,
     PyDoc_STR("createObjectAdapterWithEndpoints(name, endpoints) -> Ice.ObjectAdapter")},
    {"createObjectAdapterWithRouter",
     reinterpret_cast<PyCFunction>(communicatorCreateObjectAdapterWithRouter),
     METH_VARARGS,
     PyDoc_STR("createObjectAdapterWithRouter(name, router) -> Ice.ObjectAdapter")},
    {"getValueFactoryManager",
     reinterpret_cast<PyCFunction>(communicatorGetValueFactoryManager),
     METH_NOARGS,
     PyDoc_STR("getValueFactoryManager() -> Ice.ValueFactoryManager")},
    {"getImplicitContext",
     reinterpret_cast<PyCFunction>(communicatorGetImplicitContext),
     METH_NOARGS,
     PyDoc_STR("getImplicitContext() -> Ice.ImplicitContext")},
    {"getProperties",
     reinterpret_cast<PyCFunction>(communicatorGetProperties),
     METH_NOARGS,
     PyDoc_STR("getProperties() -> Ice.Properties")},
    {"getLogger",
     reinterpret_cast<PyCFunction>(communicatorGetLogger),
     METH_NOARGS,
     PyDoc_STR("getLogger() -> Ice.Logger")},
    {"getDefaultRouter",
     reinterpret_cast<PyCFunction>(communicatorGetDefaultRouter),
     METH_NOARGS,
     PyDoc_STR("getDefaultRouter() -> proxy")},
    {"setDefaultRouter",
     reinterpret_cast<PyCFunction>(communicatorSetDefaultRouter),
     METH_VARARGS,
     "setDefaultRouter(proxy) -> None"},
    {"getDefaultLocator",
     reinterpret_cast<PyCFunction>(communicatorGetDefaultLocator),
     METH_NOARGS,
     PyDoc_STR("getDefaultLocator() -> proxy")},
    {"setDefaultLocator",
     reinterpret_cast<PyCFunction>(communicatorSetDefaultLocator),
     METH_VARARGS,
     PyDoc_STR("setDefaultLocator(proxy) -> None")},
    {"flushBatchRequests",
     reinterpret_cast<PyCFunction>(communicatorFlushBatchRequests),
     METH_VARARGS,
     PyDoc_STR("flushBatchRequests(compress) -> None")},
    {"flushBatchRequestsAsync",
     reinterpret_cast<PyCFunction>(communicatorFlushBatchRequestsAsync),
     METH_VARARGS,
     PyDoc_STR("flushBatchRequestsAsync(compress) -> Ice.Future")},
    {"createAdmin",
     reinterpret_cast<PyCFunction>(communicatorCreateAdmin),
     METH_VARARGS,
     PyDoc_STR("createAdmin(adminAdapter, adminIdentity) -> Ice.ObjectPrx")},
    {"getAdmin",
     reinterpret_cast<PyCFunction>(communicatorGetAdmin),
     METH_NOARGS,
     PyDoc_STR("getAdmin() -> Ice.ObjectPrx")},
    {"addAdminFacet",
     reinterpret_cast<PyCFunction>(communicatorAddAdminFacet),
     METH_VARARGS,
     PyDoc_STR("addAdminFacet(servant, facet) -> None")},
    {"findAdminFacet",
     reinterpret_cast<PyCFunction>(communicatorFindAdminFacet),
     METH_VARARGS,
     PyDoc_STR("findAdminFacet(facet) -> Ice.Object")},
    {"findAllAdminFacets",
     reinterpret_cast<PyCFunction>(communicatorFindAllAdminFacets),
     METH_NOARGS,
     PyDoc_STR("findAllAdminFacets() -> dictionary")},
    {"removeAdminFacet",
     reinterpret_cast<PyCFunction>(communicatorRemoveAdminFacet),
     METH_VARARGS,
     PyDoc_STR("removeAdminFacet(facet) -> Ice.Object")},
    {"_setWrapper",
     reinterpret_cast<PyCFunction>(communicatorSetWrapper),
     METH_VARARGS,
     PyDoc_STR("internal function")},
    {"_getWrapper", reinterpret_cast<PyCFunction>(communicatorGetWrapper), METH_NOARGS, PyDoc_STR("internal function")},
    {0, 0} /* sentinel */
};

namespace IcePy
{
    PyTypeObject CommunicatorType = {
        /* The ob_type field must be initialized in the module init function
         * to be portable to Windows without using C++. */
        PyVarObject_HEAD_INIT(0, 0) "IcePy.Communicator", /* tp_name */
        sizeof(CommunicatorObject),                       /* tp_basicsize */
        0,                                                /* tp_itemsize */
        /* methods */
        reinterpret_cast<destructor>(communicatorDealloc), /* tp_dealloc */
        0,                                                 /* tp_print */
        0,                                                 /* tp_getattr */
        0,                                                 /* tp_setattr */
        0,                                                 /* tp_reserved */
        0,                                                 /* tp_repr */
        0,                                                 /* tp_as_number */
        0,                                                 /* tp_as_sequence */
        0,                                                 /* tp_as_mapping */
        0,                                                 /* tp_hash */
        0,                                                 /* tp_call */
        0,                                                 /* tp_str */
        0,                                                 /* tp_getattro */
        0,                                                 /* tp_setattro */
        0,                                                 /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                                /* tp_flags */
        0,                                                 /* tp_doc */
        0,                                                 /* tp_traverse */
        0,                                                 /* tp_clear */
        0,                                                 /* tp_richcompare */
        0,                                                 /* tp_weaklistoffset */
        0,                                                 /* tp_iter */
        0,                                                 /* tp_iternext */
        CommunicatorMethods,                               /* tp_methods */
        0,                                                 /* tp_members */
        0,                                                 /* tp_getset */
        0,                                                 /* tp_base */
        0,                                                 /* tp_dict */
        0,                                                 /* tp_descr_get */
        0,                                                 /* tp_descr_set */
        0,                                                 /* tp_dictoffset */
        reinterpret_cast<initproc>(communicatorInit),      /* tp_init */
        0,                                                 /* tp_alloc */
        reinterpret_cast<newfunc>(communicatorNew),        /* tp_new */
        0,                                                 /* tp_free */
        0,                                                 /* tp_is_gc */
    };
}

bool
IcePy::initCommunicator(PyObject* module)
{
    _mainThreadId = PyThread_get_thread_ident();

    if (PyType_Ready(&CommunicatorType) < 0)
    {
        return false;
    }
    PyTypeObject* type = &CommunicatorType; // Necessary to prevent GCC's strict-alias warnings.
    if (PyModule_AddObject(module, "Communicator", reinterpret_cast<PyObject*>(type)) < 0)
    {
        return false;
    }

    return true;
}

Ice::CommunicatorPtr
IcePy::getCommunicator(PyObject* obj)
{
    assert(PyObject_IsInstance(obj, reinterpret_cast<PyObject*>(&CommunicatorType)));
    CommunicatorObject* cobj = reinterpret_cast<CommunicatorObject*>(obj);
    return *cobj->communicator;
}

PyObject*
IcePy::createCommunicator(const Ice::CommunicatorPtr& communicator)
{
    CommunicatorMap::iterator p = _communicatorMap.find(communicator);
    if (p != _communicatorMap.end())
    {
        Py_INCREF(p->second);
        return p->second;
    }

    CommunicatorObject* obj = communicatorNew(&CommunicatorType, 0, 0);
    if (obj)
    {
        obj->communicator = new Ice::CommunicatorPtr(communicator);
    }
    return (PyObject*)obj;
}

PyObject*
IcePy::getCommunicatorWrapper(const Ice::CommunicatorPtr& communicator)
{
    CommunicatorMap::iterator p = _communicatorMap.find(communicator);
    assert(p != _communicatorMap.end());
    CommunicatorObject* obj = reinterpret_cast<CommunicatorObject*>(p->second);
    if (obj->wrapper)
    {
        Py_INCREF(obj->wrapper);
        return obj->wrapper;
    }
    else
    {
        //
        // Communicator must have been destroyed already.
        //
        return Py_None;
    }
}

extern "C" PyObject*
IcePy_identityToString(PyObject* /*self*/, PyObject* args)
{
    PyObject* identityType = lookupType("Ice.Identity");
    PyObject* obj;
    PyObject* mode = 0;
    if (!PyArg_ParseTuple(args, "O!O", identityType, &obj, &mode))
    {
        return nullptr;
    }

    Ice::Identity id;
    if (!getIdentity(obj, id))
    {
        return nullptr;
    }

    Ice::ToStringMode toStringMode = Ice::ToStringMode::Unicode;
    if (mode != Py_None && PyObject_HasAttrString(mode, "value"))
    {
        PyObjectHandle modeValue{getAttr(mode, "value", true)};
        toStringMode = static_cast<Ice::ToStringMode>(PyLong_AsLong(modeValue.get()));
    }

    string str;

    try
    {
        str = identityToString(id, toStringMode);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return createString(str);
}

extern "C" PyObject*
IcePy_stringToIdentity(PyObject* /*self*/, PyObject* obj)
{
    string str;
    if (!getStringArg(obj, "str", str))
    {
        return nullptr;
    }

    Ice::Identity id;
    try
    {
        id = Ice::stringToIdentity(str);
    }
    catch (...)
    {
        setPythonException(current_exception());
        return nullptr;
    }

    return createIdentity(id);
}
