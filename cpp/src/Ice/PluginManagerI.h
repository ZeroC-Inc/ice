//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_PLUGIN_MANAGER_I_H
#define ICE_PLUGIN_MANAGER_I_H

#include "Ice/BuiltinSequences.h"
#include "Ice/CommunicatorF.h"
#include "Ice/InstanceF.h"
#include "Ice/Plugin.h"

#include <map>
#include <mutex>

namespace Ice
{
    typedef Ice::Plugin* (*PluginFactory)(const Ice::CommunicatorPtr&, const std::string&, const Ice::StringSeq&);

    class PluginManagerI : public PluginManager
    {
    public:
        static void registerPluginFactory(const std::string&, PluginFactory, bool);

        virtual void initializePlugins();
        virtual StringSeq getPlugins() noexcept;
        virtual PluginPtr getPlugin(const std::string&);
        virtual void addPlugin(const std::string&, const PluginPtr&);
        virtual void destroy() noexcept;
        PluginManagerI(const CommunicatorPtr&);

    private:
        friend class IceInternal::Instance;

        void loadPlugins(int&, const char*[]);
        void loadPlugin(const std::string&, const std::string&, StringSeq&);

        PluginPtr findPlugin(const std::string&) const;

        CommunicatorPtr _communicator;

        struct PluginInfo
        {
            std::string name;
            PluginPtr plugin;
        };
        typedef std::vector<PluginInfo> PluginInfoList;

        PluginInfoList _plugins;
        bool _initialized;
        std::mutex _mutex;
        static const char* const _kindOfObject;
    };
}

#endif
