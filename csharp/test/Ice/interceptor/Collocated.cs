//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Interceptor
{
    public class Collocated : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            string pluginPath =
                string.Format("msbuild/plugin/{0}/Plugin.dll",
                    Path.GetFileName(Path.GetDirectoryName(Assembly.GetExecutingAssembly().CodeBase)));
            await using Communicator communicator = Initialize(
                ref args,
                new Dictionary<string, string>()
                {
                    {
                        "Ice.Plugin.InvocationPlugin",
                        $"{pluginPath }:ZeroC.Ice.Test.Interceptor.InvocationPluginFactory"
                    },
                    {
                        "Ice.Plugin.DispatchPlugin",
                        $"{pluginPath }:ZeroC.Ice.Test.Interceptor.DispatchPluginFactory"
                    }
                });
            communicator.SetProperty("TestAdapter.Endpoints", GetTestEndpoint(0));
            ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter");
            adapter.Add("test", new MyObject());
            await DispatchInterceptors.ActivateAsync(adapter);
            AllTests.Run(this);
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Collocated>(args);
    }
}
