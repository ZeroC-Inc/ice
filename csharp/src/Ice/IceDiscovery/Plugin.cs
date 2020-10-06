// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using ZeroC.Ice;

namespace ZeroC.IceDiscovery
{
    internal sealed class Plugin : IPlugin
    {
        private readonly Communicator _communicator;
        private ILocatorPrx? _defaultLocator;
        private ILocatorPrx? _locator;
        private ObjectAdapter? _locatorAdapter;
        private ObjectAdapter? _multicastAdapter;
        private ObjectAdapter? _replyAdapter;

        public async ValueTask DisposeAsync()
        {
            if (_multicastAdapter != null)
            {
                await _multicastAdapter.DisposeAsync().ConfigureAwait(false);
            }

            if (_replyAdapter != null)
            {
                await _replyAdapter.DisposeAsync().ConfigureAwait(false);
            }

            if (_locatorAdapter != null)
            {
                await _locatorAdapter.DisposeAsync().ConfigureAwait(false);
            }

            if (IObjectPrx.Equals(_communicator.DefaultLocator, _locator))
            {
                // Restore original default locator proxy, if the user didn't change it in the meantime
                _communicator.DefaultLocator = _defaultLocator;
            }
        }

        public void Initialize(PluginInitializationContext context)
        {
            string address = _communicator.GetProperty("IceDiscovery.Address") ??
                (_communicator.PreferIPv6 ? "ff15::1" : "239.255.0.1");

            int port = _communicator.GetPropertyAsInt("IceDiscovery.Port") ?? 4061;
            string intf = _communicator.GetProperty("IceDiscovery.Interface") ?? "";

            if (_communicator.GetProperty("IceDiscovery.Multicast.Endpoints") == null)
            {
                if (intf.Length > 0)
                {
                    _communicator.SetProperty("IceDiscovery.Multicast.Endpoints",
                                              $"udp -h \"{address}\" -p {port} --interface \"{intf}\"");
                }
                else
                {
                    _communicator.SetProperty("IceDiscovery.Multicast.Endpoints", $"udp -h \"{address}\" -p {port}");
                }
            }

            string lookupEndpoints = _communicator.GetProperty("IceDiscovery.Lookup") ?? "";
            if (lookupEndpoints.Length == 0)
            {
                int ipVersion = _communicator.PreferIPv6 ? Network.EnableIPv6 : Network.EnableIPv4;
                List<string> interfaces = Network.GetInterfacesForMulticast(intf, ipVersion);
                foreach (string p in interfaces)
                {
                    if (p != interfaces[0])
                    {
                        lookupEndpoints += ":";
                    }
                    lookupEndpoints += $"udp -h \"{address}\" -p {port} --interface \"{p}\"";
                }
            }

            if (_communicator.GetProperty("IceDiscovery.Reply.Endpoints") == null)
            {
                _communicator.SetProperty("IceDiscovery.Reply.Endpoints",
                    intf.Length == 0 ? "udp -h *" : $"udp -h \"{intf}\"");
            }

            if (_communicator.GetProperty("IceDiscovery.Locator.Endpoints") == null)
            {
                _communicator.SetProperty("IceDiscovery.Locator.AdapterId", Guid.NewGuid().ToString());
            }

            _multicastAdapter = _communicator.CreateObjectAdapter("IceDiscovery.Multicast");
            _replyAdapter = _communicator.CreateObjectAdapter("IceDiscovery.Reply");
            _locatorAdapter = _communicator.CreateObjectAdapter("IceDiscovery.Locator");

            // Setup locator registry.
            var locatorRegistry = new LocatorRegistry();
            ILocatorRegistryPrx locatorRegistryPrx =
                _locatorAdapter.AddWithUUID(locatorRegistry, ILocatorRegistryPrx.Factory);

            ILookupPrx lookupPrx =
                ILookupPrx.Parse($"IceDiscovery/Lookup -d:{lookupEndpoints}", _communicator).Clone(clearRouter: true);

            // Add lookup Ice object
            var lookup = new Lookup(locatorRegistry, lookupPrx, _communicator, _replyAdapter);
            _multicastAdapter.Add("IceDiscovery/Lookup", lookup);

            // Setup locator on the communicator.
            _locator = _locatorAdapter.AddWithUUID(new Locator(lookup, locatorRegistryPrx), ILocatorPrx.Factory);
            _defaultLocator = _communicator.DefaultLocator;
            _communicator.DefaultLocator = _locator;

            _multicastAdapter.Activate();
            _replyAdapter.Activate();
            _locatorAdapter.Activate();
        }

        internal Plugin(Communicator communicator) => _communicator = communicator;
    }

    /// <summary>The IceDiscovery plug-in's factory.</summary>
    public sealed class PluginFactory : IPluginFactory
    {
        /// <inheritdoc/>
        public IPlugin Create(Communicator communicator, string name, string[] args) => new Plugin(communicator);
    }
}
