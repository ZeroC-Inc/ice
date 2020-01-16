//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;

public class PluginFactory : Ice.IPluginFactory
{
    public Ice.IPlugin create(Ice.Communicator communicator, string name, string[] args) => new Plugin(args);

    internal class Plugin : Ice.IPlugin
    {
        public Plugin(string[] args) => _args = args;

        public void initialize()
        {
            _initialized = true;
            test(_args.Length == 3);
            test(_args[0] == "C:\\Program Files\\");
            test(_args[1] == "--DatabasePath");
            test(_args[2] == "C:\\Program Files\\Application\\db");
        }

        public void destroy() => _destroyed = true;

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

        private static void test(bool b)
        {
            if (!b)
            {
                throw new Exception();
            }
        }

        private bool _initialized = false;
        private bool _destroyed = false;
        private string[] _args;
    }
}
