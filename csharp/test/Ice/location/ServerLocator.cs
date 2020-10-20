// Copyright (c) ZeroC, Inc. All rights reserved.

using System.Diagnostics;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice.Test.Location
{
    public class ServerLocator : ITestLocator
    {
        private readonly ServerLocatorRegistry _registry;
        private readonly ILocatorRegistryPrx _registryPrx;
        private int _requestCount;

        public ServerLocator(ServerLocatorRegistry registry, ILocatorRegistryPrx registryPrx)
        {
            _registry = registry;
            _registryPrx = registryPrx;
            _requestCount = 0;
        }

        public IObjectPrx? FindAdapterById(string adapter, Current current, CancellationToken cancel)
        {
            ++_requestCount;
            // We add a small delay to make sure locator request queuing gets tested when
            // running the test on a fast machine
            System.Threading.Thread.Sleep(1);

            return _registry.GetIce1Adapter(adapter);
        }

        public IObjectPrx? FindObjectById(Identity id, Current current, CancellationToken cancel)
        {
            ++_requestCount;
            // We add a small delay to make sure locator request queuing gets tested when
            // running the test on a fast machine
            System.Threading.Thread.Sleep(1);

            return _registry.GetIce1Object(id);
        }

        public ILocatorRegistryPrx GetRegistry(Current current, CancellationToken cancel) => _registryPrx;

        public int GetRequestCount(Current current, CancellationToken cancel) => _requestCount;

        public (IEnumerable<EndpointData>, IEnumerable<string>) ResolveLocation(
            string[] location,
            Current current,
            CancellationToken cancel)
        {
            ++_requestCount;
            // We add a small delay to make sure locator request queuing gets tested when
            // running the test on a fast machine
            System.Threading.Thread.Sleep(1);

            return (_registry.GetIce2Adapter(location[0]), location[1..]);
        }

        public (IEnumerable<EndpointData>, IEnumerable<string>) ResolveWellKnownProxy(
            Identity identity,
            Current current,
            CancellationToken cancel)
        {
            ++_requestCount;
            // We add a small delay to make sure locator request queuing gets tested when
            // running the test on a fast machine
            System.Threading.Thread.Sleep(1);

            return _registry.GetIce2Object(identity);
        }
    }
}
