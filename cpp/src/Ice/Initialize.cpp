//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/ArgVector.h>
#include "Ice/Communicator.h"
#include <Ice/Properties.h>
#include <Ice/Initialize.h>
#include <Ice/LocalException.h>
#include <Ice/LoggerI.h>
#include <Ice/Instance.h>
#include <Ice/PluginManagerI.h>
#include <Ice/StringUtil.h>
#include <Ice/StringConverter.h>
#include "CheckIdentity.h"

#include <mutex>
#include <stdexcept>

using namespace std;
using namespace Ice;
using namespace IceInternal;

namespace
{
    mutex globalMutex;
    Ice::LoggerPtr processLogger;
}

StringSeq
Ice::argsToStringSeq(int argc, const char* const argv[])
{
    StringSeq result;
    for (int i = 0; i < argc; i++)
    {
        result.push_back(argv[i]);
    }
    return result;
}

#ifdef _WIN32

StringSeq
Ice::argsToStringSeq(int /*argc*/, const wchar_t* const argv[])
{
    //
    // Don't need to use a wide string converter argv is expected to
    // come from Windows API.
    //
    const StringConverterPtr converter = getProcessStringConverter();
    StringSeq args;
    for (int i = 0; argv[i] != 0; i++)
    {
        args.push_back(wstringToString(argv[i], converter));
    }
    return args;
}

#endif

void
Ice::stringSeqToArgs(const StringSeq& args, int& argc, const char* argv[])
{
    //
    // Shift all elements in argv which are present in args to the
    // beginning of argv. We record the original value of argc so
    // that we can know later if we've shifted the array.
    //
    const int argcOrig = argc;
    int i = 0;
    while (i < argc)
    {
        if (find(args.begin(), args.end(), argv[i]) == args.end())
        {
            for (int j = i; j < argc - 1; j++)
            {
                argv[j] = argv[j + 1];
            }
            --argc;
        }
        else
        {
            ++i;
        }
    }

    //
    // Make sure that argv[argc] == 0, the ISO C++ standard requires this.
    // We can only do this if we've shifted the array, otherwise argv[argc]
    // may point to an invalid address.
    //
    if (argv && argcOrig != argc)
    {
        argv[argc] = 0;
    }
}

#ifdef _WIN32
void
Ice::stringSeqToArgs(const StringSeq& args, int& argc, const wchar_t* argv[])
{
    //
    // Don't need to use a wide string converter argv is expected to
    // come from Windows API.
    //
    const StringConverterPtr converter = getProcessStringConverter();

    //
    // Shift all elements in argv which are present in args to the
    // beginning of argv. We record the original value of argc so
    // that we can know later if we've shifted the array.
    //
    const int argcOrig = argc;
    int i = 0;
    while (i < argc)
    {
        if (find(args.begin(), args.end(), wstringToString(argv[i], converter)) == args.end())
        {
            for (int j = i; j < argc - 1; j++)
            {
                argv[j] = argv[j + 1];
            }
            --argc;
        }
        else
        {
            ++i;
        }
    }

    //
    // Make sure that argv[argc] == 0, the ISO C++ standard requires this.
    // We can only do this if we've shifted the array, otherwise argv[argc]
    // may point to an invalid address.
    //
    if (argv && argcOrig != argc)
    {
        argv[argc] = 0;
    }
}
#endif

PropertiesPtr
Ice::createProperties()
{
    return make_shared<Properties>();
}

PropertiesPtr
Ice::createProperties(StringSeq& args, const PropertiesPtr& defaults)
{
    return make_shared<Properties>(args, defaults);
}

PropertiesPtr
Ice::createProperties(int& argc, const char* argv[], const PropertiesPtr& defaults)
{
    StringSeq args = argsToStringSeq(argc, argv);
    PropertiesPtr properties = createProperties(args, defaults);
    stringSeqToArgs(args, argc, argv);
    return properties;
}

#ifdef _WIN32
PropertiesPtr
Ice::createProperties(int& argc, const wchar_t* argv[], const PropertiesPtr& defaults)
{
    StringSeq args = argsToStringSeq(argc, argv);
    PropertiesPtr properties = createProperties(args, defaults);
    stringSeqToArgs(args, argc, argv);
    return properties;
}
#endif

Ice::ThreadHookPlugin::ThreadHookPlugin(
    const CommunicatorPtr& communicator,
    function<void()> threadStart,
    function<void()> threadStop)
{
    if (communicator == nullptr)
    {
        throw PluginInitializationException(__FILE__, __LINE__, "Communicator cannot be null");
    }

    IceInternal::InstancePtr instance = IceInternal::getInstance(communicator);
    instance->setThreadHook(std::move(threadStart), std::move(threadStop));
}

void
Ice::ThreadHookPlugin::initialize()
{
}

void
Ice::ThreadHookPlugin::destroy()
{
}

namespace
{
    inline void checkIceVersion(int version)
    {
#ifndef ICE_IGNORE_VERSION

#    if ICE_INT_VERSION % 100 > 50
        //
        // Beta version: exact match required
        //
        if (ICE_INT_VERSION != version)
        {
            throw VersionMismatchException(__FILE__, __LINE__);
        }
#    else

        //
        // Major and minor version numbers must match.
        //
        if (ICE_INT_VERSION / 100 != version / 100)
        {
            throw VersionMismatchException(__FILE__, __LINE__);
        }

        //
        // Reject beta caller
        //
        if (version % 100 > 50)
        {
            throw VersionMismatchException(__FILE__, __LINE__);
        }

        //
        // The caller's patch level cannot be greater than library's patch level. (Patch level changes are
        // backward-compatible, but not forward-compatible.)
        //
        if (version % 100 > ICE_INT_VERSION % 100)
        {
            throw VersionMismatchException(__FILE__, __LINE__);
        }

#    endif
#endif
    }
}

Ice::CommunicatorPtr
Ice::initialize(int& argc, const char* argv[], const InitializationData& initializationData, int version)
{
    checkIceVersion(version);

    InitializationData initData = initializationData;
    initData.properties = createProperties(argc, argv, initData.properties);

    CommunicatorPtr communicator = Communicator::create(initData);
    communicator->finishSetup(argc, argv);
    return communicator;
}

Ice::CommunicatorPtr
Ice::initialize(int& argc, const char* argv[], ICE_CONFIG_FILE_STRING configFile, int version)
{
    InitializationData initData;
    initData.properties = createProperties();
    initData.properties->load(configFile);
    return initialize(argc, argv, initData, version);
}

#ifdef _WIN32
Ice::CommunicatorPtr
Ice::initialize(int& argc, const wchar_t* argv[], const InitializationData& initializationData, int version)
{
    Ice::StringSeq args = argsToStringSeq(argc, argv);
    CommunicatorPtr communicator = initialize(args, initializationData, version);
    stringSeqToArgs(args, argc, argv);
    return communicator;
}

Ice::CommunicatorPtr
Ice::initialize(int& argc, const wchar_t* argv[], ICE_CONFIG_FILE_STRING configFile, int version)
{
    InitializationData initData;
    initData.properties = createProperties();
    initData.properties->load(configFile);
    return initialize(argc, argv, initData, version);
}
#endif

Ice::CommunicatorPtr
Ice::initialize(StringSeq& args, const InitializationData& initializationData, int version)
{
    IceInternal::ArgVector av(args);
    CommunicatorPtr communicator = initialize(av.argc, av.argv, initializationData, version);
    args = argsToStringSeq(av.argc, av.argv);
    return communicator;
}

Ice::CommunicatorPtr
Ice::initialize(StringSeq& args, ICE_CONFIG_FILE_STRING configFile, int version)
{
    InitializationData initData;
    initData.properties = createProperties();
    initData.properties->load(configFile);
    return initialize(args, initData, version);
}

Ice::CommunicatorPtr
Ice::initialize(const InitializationData& initData, int version)
{
    //
    // We can't simply call the other initialize() because this one does NOT read
    // the config file, while the other one always does.
    //
    checkIceVersion(version);

    CommunicatorPtr communicator = Communicator::create(initData);
    int argc = 0;
    const char* argv[] = {0};
    communicator->finishSetup(argc, argv);
    return communicator;
}

Ice::CommunicatorPtr
Ice::initialize(ICE_CONFIG_FILE_STRING configFile, int version)
{
    InitializationData initData;
    initData.properties = createProperties();
    initData.properties->load(configFile);
    return initialize(initData, version);
}

LoggerPtr
Ice::getProcessLogger()
{
    lock_guard lock(globalMutex);

    if (processLogger == nullptr)
    {
        //
        // TODO: Would be nice to be able to use process name as prefix by default.
        //
        processLogger = make_shared<LoggerI>("", "", true);
    }
    return processLogger;
}

void
Ice::setProcessLogger(const LoggerPtr& logger)
{
    lock_guard lock(globalMutex);
    processLogger = logger;
}

void
Ice::registerPluginFactory(const std::string& name, PluginFactory factory, bool loadOnInitialize)
{
    lock_guard lock(globalMutex);
    PluginManagerI::registerPluginFactory(name, factory, loadOnInitialize);
}

//
// CommunicatorHolder
//

Ice::CommunicatorHolder::CommunicatorHolder() {}

Ice::CommunicatorHolder::CommunicatorHolder(shared_ptr<Communicator> communicator)
    : _communicator(std::move(communicator))
{
}

Ice::CommunicatorHolder&
Ice::CommunicatorHolder::operator=(shared_ptr<Communicator> communicator)
{
    if (_communicator)
    {
        _communicator->destroy();
    }
    _communicator = std::move(communicator);
    return *this;
}

Ice::CommunicatorHolder&
Ice::CommunicatorHolder::operator=(CommunicatorHolder&& other) noexcept
{
    if (_communicator)
    {
        _communicator->destroy();
    }
    _communicator = std::move(other._communicator);
    return *this;
}

Ice::CommunicatorHolder::~CommunicatorHolder()
{
    if (_communicator)
    {
        _communicator->destroy();
    }
}

Ice::CommunicatorHolder::operator bool() const { return _communicator != nullptr; }

const Ice::CommunicatorPtr&
Ice::CommunicatorHolder::communicator() const
{
    return _communicator;
}

const Ice::CommunicatorPtr&
Ice::CommunicatorHolder::operator->() const
{
    return _communicator;
}

Ice::CommunicatorPtr
Ice::CommunicatorHolder::release()
{
    return std::move(_communicator);
}

InstancePtr
IceInternal::getInstance(const CommunicatorPtr& communicator)
{
    return communicator->_instance;
}

IceUtil::TimerPtr
IceInternal::getInstanceTimer(const CommunicatorPtr& communicator)
{
    return communicator->_instance->timer();
}

Identity
Ice::stringToIdentity(const string& s)
{
    Identity ident;

    //
    // Find unescaped separator; note that the string may contain an escaped
    // backslash before the separator.
    //
    string::size_type slash = string::npos;
    string::size_type pos = 0;
    while ((pos = s.find('/', pos)) != string::npos)
    {
        string::size_type escapes = 0;
        while (static_cast<int>(pos - escapes) > 0 && s[pos - escapes - 1] == '\\')
        {
            escapes++;
        }

        //
        // We ignore escaped escapes
        //
        if (escapes % 2 == 0)
        {
            if (slash == string::npos)
            {
                slash = pos;
            }
            else
            {
                //
                // Extra unescaped slash found.
                //
                throw IdentityParseException(__FILE__, __LINE__, "unescaped '/' in identity `" + s + "'");
            }
        }
        pos++;
    }

    if (slash == string::npos)
    {
        try
        {
            ident.name = unescapeString(s, 0, s.size(), "/");
        }
        catch (const invalid_argument& ex)
        {
            throw IdentityParseException(__FILE__, __LINE__, "invalid identity name `" + s + "': " + ex.what());
        }
    }
    else
    {
        try
        {
            ident.category = unescapeString(s, 0, slash, "/");
        }
        catch (const invalid_argument& ex)
        {
            throw IdentityParseException(__FILE__, __LINE__, "invalid category in identity `" + s + "': " + ex.what());
        }

        if (slash + 1 < s.size())
        {
            try
            {
                ident.name = unescapeString(s, slash + 1, s.size(), "/");
            }
            catch (const invalid_argument& ex)
            {
                throw IdentityParseException(__FILE__, __LINE__, "invalid name in identity `" + s + "': " + ex.what());
            }
        }
    }

    checkIdentity(ident, __FILE__, __LINE__);
    return ident;
}

string
Ice::identityToString(const Identity& ident, ToStringMode toStringMode)
{
    checkIdentity(ident, __FILE__, __LINE__);
    if (ident.category.empty())
    {
        return escapeString(ident.name, "/", toStringMode);
    }
    else
    {
        return escapeString(ident.category, "/", toStringMode) + '/' + escapeString(ident.name, "/", toStringMode);
    }
}
