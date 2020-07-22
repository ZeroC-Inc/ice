//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Threading.Tasks;

namespace ZeroC.Ice.Test.Plugin
{
    public class PluginFactory : IPluginFactory
    {
        public IPlugin Create(Communicator communicator, string name, string[] args) => new Plugin(args);

        internal class Plugin : IPlugin
        {
            private readonly string[] _args;
            private bool _destroyed;
            private bool _initialized;

            public Plugin(string[] args) => _args = args;

            public void Initialize()
            {
                _initialized = true;
                TestHelper.Assert(_args.Length == 3);
                TestHelper.Assert(_args[0] == "C:\\Program Files\\");
                TestHelper.Assert(_args[1] == "--DatabasePath");
                TestHelper.Assert(_args[2] == "C:\\Program Files\\Application\\db");
            }

            public ValueTask DisposeAsync()
            {
                _destroyed = true;
                return new ValueTask(Task.CompletedTask);
            }

            ~Plugin()
            {
                if (!_initialized)
                {
                    Console.WriteLine("Plugin not initialized");
                }
                if (!_destroyed)
                {
                    Console.WriteLine("Plugin not destroyed");
                }
            }
        }
    }
}
