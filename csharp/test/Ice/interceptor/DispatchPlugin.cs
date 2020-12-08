// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice.Test.Interceptor
{
    public class DispatchPluginFactory : IPluginFactory
    {
        public IPlugin Create(Communicator communicator, string name, string[] args) => new Plugin(communicator);

        internal class Plugin : IPlugin
        {
            private Communicator _communicator;

            public Task ActivateAsync(CancellationToken cancel)
            {
                _communicator.AddDispatchInterceptor(
                    async (request, current, next, cancel) =>
                    {
                        current.Context["DispatchPlugin"] = "1";
                        OutgoingResponseFrame response = await next(request, current, cancel);
                        if (request.Protocol == Protocol.Ice2)
                        {
                            response.AddBinaryContextEntry(100, 100, (ostr, v) => ostr.WriteInt(v));
                        }
                        return response;
                    });

                return Task.CompletedTask;
            }
            internal Plugin(Communicator communicator) => _communicator = communicator;

            public ValueTask DisposeAsync() => default;
        }
    }
}
