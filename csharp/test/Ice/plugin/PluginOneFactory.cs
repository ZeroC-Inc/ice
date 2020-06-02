//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using ZeroC.Ice;

namespace ZeroC.Ice.Test.Plugin
{
    public class PluginOneFactory : IPluginFactory
    {
        public IPlugin Create(Communicator communicator, string name, string[] args) => new PluginOne(communicator);

        internal class PluginOne : BasePlugin
        {
            public PluginOne(Communicator communicator) : base(communicator)
            {
            }

            public override void Initialize()
            {
                var other = (BasePlugin?)_communicator.GetPlugin("PluginTwo");
                TestHelper.Assert(other != null);
                _other = other;
                TestHelper.Assert(!_other.isInitialized());
                _initialized = true;
            }

            public override void Destroy()
            {
                _destroyed = true;
                TestHelper.Assert(_other!.isDestroyed());
            }
        }
    }
}
