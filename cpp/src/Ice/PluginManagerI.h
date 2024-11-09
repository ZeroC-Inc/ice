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

    class PluginManagerI final : public PluginManager
    {
    public:
        // Register a plugin factory (internal).
        static void registerPluginFactory(std::string, PluginFactory, bool);

        void initializePlugins() final;
        StringSeq getPlugins() final;
        PluginPtr getPlugin(std::string_view) final;
        void addPlugin(std::string, PluginPtr) final;
        void destroy() noexcept final;

        // Constructs the plugin manager (internal).
        PluginManagerI(const CommunicatorPtr&);

        // Loads all the plugins and returns the number of plugins loaded (internal).
        size_t loadPlugins(int& argc, const char* argv[]);

    private:
        void loadPlugin(const std::string&, const std::string&, StringSeq&);
        PluginPtr findPlugin(std::string_view) const;

        struct PluginInfo
        {
            std::string name;
            PluginPtr plugin;
        };
        typedef std::vector<PluginInfo> PluginInfoList;

        CommunicatorPtr _communicator;
        PluginInfoList _plugins;
        bool _initialized;
        std::mutex _mutex;
        static const char* const _kindOfObject;
    };
}

#endif
