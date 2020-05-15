//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using ZeroC.Ice;

public class PluginThreeFailFactory : IPluginFactory
{
    public IPlugin Create(Communicator communicator, string name, string[] args) => new PluginThreeFail(communicator);

    internal class PluginThreeFail : BasePluginFail
    {
        public PluginThreeFail(Communicator communicator) : base(communicator)
        {
        }

        public override void Initialize() => throw new PluginInitializeFailException();

        public override void Destroy() => TestHelper.Assert(false);

        ~PluginThreeFail()
        {
            if (_initialized)
            {
                Console.WriteLine("PluginThreeFail was initialized");
            }
            if (_destroyed)
            {
                Console.WriteLine("PluginThreeFail was destroyed");
            }
        }
    }
}
