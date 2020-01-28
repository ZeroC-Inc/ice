//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Linq;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;

using Ice;

namespace IceBox
{
    //
    // NOTE: the class isn't final on purpose to allow users to extend it.
    //
    public class ServiceManager : IServiceManager
    {
        public ServiceManager(Communicator communicator, string[] args)
        {
            _communicator = communicator;
            _logger = _communicator.Logger;

            if (_communicator.GetProperty("Ice.Admin.Enabled") == null)
            {
                _adminEnabled = _communicator.GetProperty("Ice.Admin.Endpoints") != null;
            }
            else
            {
                _adminEnabled = _communicator.GetPropertyAsInt("Ice.Admin.Enabled") > 0;
            }

            if (_adminEnabled)
            {
                _adminFacetFilter = new HashSet<string>(
                    _communicator.GetPropertyAsList("Ice.Admin.Facets") ?? Array.Empty<string>());
            }

            _argv = args;
            _traceServiceObserver = _communicator.GetPropertyAsInt("IceBox.Trace.ServiceObserver") ?? 0;
        }

        public void StartService(string name, Current current)
        {
            ServiceInfo? info;
            lock (this)
            {
                //
                // Search would be more efficient if services were contained in
                // a map, but order is required for shutdown.
                //
                info = _services.Find(service => service.Name.Equals(name));
                if (info == null)
                {
                    throw new NoSuchServiceException();
                }

                if (info.Status != ServiceStatus.Stopped)
                {
                    throw new AlreadyStartedException();
                }
                info.Status = ServiceStatus.Starting;
                _pendingStatusChanges = true;
            }

            bool started = false;
            try
            {
                Debug.Assert(info.Service != null);
                Communicator? communicator = info.Communicator ?? _sharedCommunicator;
                Debug.Assert(communicator != null);
                info.Service.start(info.Name, communicator, info.Args);
                started = true;
            }
            catch (System.Exception ex)
            {
                _logger.Warning($"ServiceManager: exception while starting service {info.Name}:\n{ex}");
            }

            lock (this)
            {
                if (started)
                {
                    info.Status = ServiceStatus.Started;
                    ServicesStarted(new List<string>() { name }, _observers.Keys);
                }
                else
                {
                    info.Status = ServiceStatus.Stopped;
                }
                _pendingStatusChanges = false;
                System.Threading.Monitor.PulseAll(this);
            }
        }

        public void StopService(string name, Current current)
        {
            ServiceInfo? info;
            lock (this)
            {
                //
                // Search would be more efficient if services were contained in
                // a map, but order is required for shutdown.
                //
                info = _services.Find(service => service.Name.Equals(name));
                if (info == null)
                {
                    throw new NoSuchServiceException();
                }

                if (info.Status != ServiceStatus.Started)
                {
                    throw new AlreadyStoppedException();
                }
                _pendingStatusChanges = true;
            }

            bool stopped = false;
            try
            {
                Debug.Assert(info.Service != null);
                info.Service.stop();
                stopped = true;
            }
            catch (System.Exception ex)
            {
                _logger.Warning($"ServiceManager: exception while stopping service {info.Name}\n{ex}");
            }

            lock (this)
            {
                if (stopped)
                {
                    info.Status = ServiceStatus.Stopped;
                    ServicesStopped(new List<string>() { name }, _observers.Keys);
                }
                else
                {
                    info.Status = ServiceStatus.Started;
                }
                _pendingStatusChanges = false;
                System.Threading.Monitor.PulseAll(this);
            }
        }

        public void AddObserver(IServiceObserverPrx observer, Ice.Current current)
        {
            var activeServices = new List<string>();

            //
            // Null observers and duplicate registrations are ignored
            //
            lock (this)
            {
                if (observer != null)
                {
                    try
                    {
                        _observers.Add(observer, true);
                    }
                    catch (ArgumentException)
                    {
                        return;
                    }

                    if (_traceServiceObserver >= 1)
                    {
                        _logger.Trace("IceBox.ServiceObserver", $"Added service observer {observer}");
                    }

                    foreach (ServiceInfo info in _services)
                    {
                        if (info.Status == ServiceStatus.Started)
                        {
                            activeServices.Add(info.Name);
                        }
                    }
                }
            }

            if (activeServices.Count > 0)
            {
                observer!.ServicesStartedAsync(activeServices.ToArray()).ContinueWith((t) => ObserverCompleted(observer, t),
                    TaskScheduler.Current);
            }
        }

        public void Shutdown(Current current) => _communicator.Shutdown();

        public int Run()
        {
            try
            {
                //
                // Create an object adapter. Services probably should NOT share
                // this object adapter, as the endpoint(s) for this object adapter
                // will most likely need to be firewalled for security reasons.
                //
                ObjectAdapter? adapter = null;
                if (_communicator.GetProperty("IceBox.ServiceManager.Endpoints") != null)
                {
                    adapter = _communicator.CreateObjectAdapter("IceBox.ServiceManager");
                    adapter.Add(this, new Identity("ServiceManager",
                        _communicator.GetProperty("IceBox.InstanceName") ?? "IceBox"));
                }

                //
                // Parse the property set with the prefix "IceBox.Service.". These
                // properties should have the following format:
                //
                // IceBox.Service.Foo=<assembly>:Package.Foo [args]
                //
                // We parse the service properties specified in IceBox.LoadOrder
                // first, then the ones from remaining services.
                //
                string prefix = "IceBox.Service.";
                Dictionary<string, string> services = _communicator.GetProperties(forPrefix: prefix);

                if (services.Count == 0)
                {
                    throw new InvalidOperationException("ServiceManager: configuration must include at least one IceBox service");
                }

                string[] loadOrder = (_communicator.GetPropertyAsList("IceBox.LoadOrder") ?? Array.Empty<string>()).Where(
                    s => s.Length > 0).ToArray();
                var servicesInfo = new List<StartServiceInfo>();
                foreach (string name in loadOrder)
                {
                    string key = prefix + name;
                    if (!services.TryGetValue(key, out string? value))
                    {
                        throw new InvalidOperationException($"ServiceManager: no service definition for `{name}'");
                    }
                    servicesInfo.Add(new StartServiceInfo(name, value, _argv));
                    services.Remove(key);
                }

                foreach (KeyValuePair<string, string> entry in services)
                {
                    servicesInfo.Add(new StartServiceInfo(entry.Key.Substring(prefix.Length), entry.Value, _argv));
                }

                //
                // Check if some services are using the shared communicator in which
                // case we create the shared communicator now with a property set that
                // is the union of all the service properties (from services that use
                // the shared communicator).
                //
                if (_communicator.GetProperties(forPrefix: "IceBox.UseSharedCommunicator.").Count > 0)
                {
                    Dictionary<string, string> properties = CreateServiceProperties("SharedCommunicator");
                    foreach (StartServiceInfo service in servicesInfo)
                    {
                        if ((_communicator.GetPropertyAsInt($"IceBox.UseSharedCommunicator.{service.Name}") ?? 0) <= 0)
                        {
                            continue;
                        }

                        //
                        // Load the service properties using the shared communicator properties as the default properties.
                        //
                        properties.ParseIceArgs(ref service.Args);

                        //
                        // Parse <service>.* command line options (the Ice command line options
                        // were parsed by the call to createProperties above).
                        //
                        properties.ParseArgs(ref service.Args, service.Name);
                    }

                    string facetNamePrefix = "IceBox.SharedCommunicator.";
                    bool addFacets = ConfigureAdmin(properties, facetNamePrefix);

                    _sharedCommunicator = new Communicator(properties);

                    if (addFacets)
                    {
                        // Add all facets created on shared communicator to the IceBox communicator
                        // but renamed <prefix>.<facet-name>, except for the Process facet which is
                        // never added.
                        foreach (KeyValuePair<string, (object servant, Disp disp)> p in _sharedCommunicator.FindAllAdminFacets())
                        {
                            if (!p.Key.Equals("Process"))
                            {
                                _communicator.AddAdminFacet(p.Value.servant, p.Value.disp, facetNamePrefix + p.Key);
                            }
                        }
                    }
                }

                foreach (StartServiceInfo s in servicesInfo)
                {
                    StartService(s.Name, s.EntryPoint, s.Args);
                }

                //
                // Start Admin (if enabled) and/or deprecated IceBox.ServiceManager OA
                //
                _communicator.AddAdminFacet(this, this.Dispatch, "IceBox.ServiceManager");
                _communicator.GetAdmin();
                if (adapter != null)
                {
                    adapter.Activate();
                }

                //
                // We may want to notify external scripts that the services
                // have started and that IceBox is "ready".
                // This is done by defining the property IceBox.PrintServicesReady=bundleName
                //
                // bundleName is whatever you choose to call this set of
                // services. It will be echoed back as "bundleName ready".
                //
                // This must be done after start() has been invoked on the
                // services.
                //
                string? bundleName = _communicator.GetProperty("IceBox.PrintServicesReady");
                if (bundleName != null)
                {
                    Console.Out.WriteLine(bundleName + " ready");
                }

                _communicator.WaitForShutdown();
            }
            catch (CommunicatorDestroyedException)
            {
                // Expected if the communicator is shutdown
            }
            catch (ObjectAdapterDeactivatedException)
            {
                // Expected if the mmunicator is shutdown
            }
            catch (System.Exception ex)
            {
                _logger.Error("ServiceManager: caught exception:\n" + ex.ToString());
                return 1;
            }
            finally
            {
                //
                // Invoke stop() on the services.
                //
                StopAll();
            }

            return 0;
        }

        private void StartService(string service, string entryPoint, string[] args)
        {
            lock (this)
            {
                //
                // Extract the assembly name and the class name.
                //
                int sepPos = entryPoint.IndexOf(':');
                if (sepPos != -1)
                {
                    const string driveLetters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                    if (entryPoint.Length > 3 &&
                       sepPos == 1 &&
                       driveLetters.IndexOf(entryPoint[0]) != -1 &&
                       (entryPoint[2] == '\\' || entryPoint[2] == '/'))
                    {
                        sepPos = entryPoint.IndexOf(':', 3);
                    }
                }
                if (sepPos == -1)
                {
                    throw new FormatException($"invalid entry point format `{entryPoint}");
                }

                System.Reflection.Assembly? serviceAssembly = null;
                string assemblyName = entryPoint.Substring(0, sepPos);
                string className = entryPoint.Substring(sepPos + 1);

                try
                {
                    //
                    // First try to load the assembly using Assembly.Load, which will succeed
                    // if a fully-qualified name is provided or if a partial name has been qualified
                    // in configuration. If that fails, try Assembly.LoadFrom(), which will succeed
                    // if a file name is configured or a partial name is configured and DEVPATH is used.
                    //
                    try
                    {
                        serviceAssembly = System.Reflection.Assembly.Load(assemblyName);
                    }
                    catch (System.Exception ex)
                    {
                        try
                        {
                            serviceAssembly = System.Reflection.Assembly.LoadFrom(assemblyName);
                        }
                        catch (System.Exception)
                        {
#pragma warning disable CA2200 // Rethrow to preserve stack details.
                            throw ex;
#pragma warning restore CA2200 // Rethrow to preserve stack details.
                        }
                    }
                }
                catch (System.Exception ex)
                {
                    throw new InvalidOperationException(
                        $"ServiceManager: unable to load service '{entryPoint}': error loading assembly: {assemblyName}", ex);
                }

                //
                // Instantiate the class.
                //
                Type? c = null;
                try
                {
                    c = serviceAssembly.GetType(className, true);
                }
                catch (System.Exception ex)
                {
                    throw new InvalidOperationException(
                        $"ServiceManager: unable to load service '{entryPoint}': cannot find the service class `{className}'", ex);
                }

                var info = new ServiceInfo(service, ServiceStatus.Stopped, args);

                ILogger? logger = null;
                //
                // If IceBox.UseSharedCommunicator.<name> is defined, create a
                // communicator for the service. The communicator inherits
                // from the shared communicator properties. If it's not
                // defined, add the service properties to the shared
                // commnunicator property set.
                //
                Communicator communicator;
                if (_communicator.GetPropertyAsInt($"IceBox.UseSharedCommunicator.{service}") > 0)
                {
                    Debug.Assert(_sharedCommunicator != null);
                    communicator = _sharedCommunicator;
                }
                else
                {
                    //
                    // Create the service properties. We use the communicator properties as the default
                    // properties if IceBox.InheritProperties is set.
                    //
                    Dictionary<string, string> properties = CreateServiceProperties(service);
                    if (info.Args.Length > 0)
                    {
                        //
                        // Create the service properties with the given service arguments. This should
                        // read the service config file if it's specified with --Ice.Config.
                        //
                        properties.ParseIceArgs(ref info.Args);

                        //
                        // Next, parse the service "<service>.*" command line options (the Ice command
                        // line options were parsed by the createProperties above)
                        //
                        properties.ParseArgs(ref info.Args, service);
                    }

                    //
                    // Clone the logger to assign a new prefix. If one of the built-in loggers is configured
                    // don't set any logger.
                    //
                    if (properties.TryGetValue("Ice.LogFile", out string? logFile))
                    {
                        logger = _logger.CloneWithPrefix(properties.GetValueOrDefault("Ice.ProgramName") ?? "");
                    }

                    //
                    // If Admin is enabled on the IceBox communicator, for each service that does not set
                    // Ice.Admin.Enabled, we set Ice.Admin.Enabled=1 to have this service create facets; then
                    // we add these facets to the IceBox Admin object as IceBox.Service.<service>.<facet>.
                    //
                    string serviceFacetNamePrefix = "IceBox.Service." + service + ".";
                    bool addFacets = ConfigureAdmin(properties, serviceFacetNamePrefix);

                    //
                    // Remaining command line options are passed to the communicator. This is
                    // necessary for Ice plug-in properties (e.g.: IceSSL).
                    //
                    info.Communicator = new Communicator(ref info.Args, properties, logger: logger);
                    communicator = info.Communicator;

                    if (addFacets)
                    {
                        // Add all facets created on the service communicator to the IceBox communicator
                        // but renamed IceBox.Service.<service>.<facet-name>, except for the Process facet
                        // which is never added
                        foreach (KeyValuePair<string, (object servant, Disp disp)> p in communicator.FindAllAdminFacets())
                        {
                            if (!p.Key.Equals("Process"))
                            {
                                _communicator.AddAdminFacet(p.Value.servant, p.Value.disp, serviceFacetNamePrefix + p.Key);
                            }
                        }
                    }
                }

                try
                {
                    //
                    // Instantiate the service.
                    //
                    IService? s;
                    try
                    {
                        //
                        // If the service class provides a constructor that accepts an Ice.Communicator argument,
                        // use that in preference to the default constructor.
                        //
                        var parameterTypes = new Type[1];
                        parameterTypes[0] = typeof(Communicator);
                        System.Reflection.ConstructorInfo? ci = c.GetConstructor(parameterTypes);
                        if (ci != null)
                        {
                            object[] parameters = new object[1];
                            parameters[0] = _communicator;
                            s = (IService)ci.Invoke(parameters);
                        }
                        else
                        {
                            //
                            // Fall back to the default constructor.
                            //
                            s = (IService?)IceInternal.AssemblyUtil.CreateInstance(c);
                        }
                    }
                    catch (System.Exception ex)
                    {
                        throw new InvalidOperationException($"ServiceManager: unable to load service '{entryPoint}", ex);
                    }

                    info.Service = s ?? throw new InvalidOperationException(
                            $"ServiceManager: unable to load service '{entryPoint}': " +
                            $"no default constructor for `{className}'");

                    try
                    {
                        info.Service.start(service, communicator, info.Args);
                    }
                    catch (System.Exception ex)
                    {
                        throw new InvalidOperationException($"ServiceManager: exception while starting service {service}", ex);
                    }

                    info.Status = ServiceStatus.Started;
                    _services.Add(info);
                }
                catch (System.Exception)
                {
                    if (info.Communicator != null)
                    {
                        DestroyServiceCommunicator(service, info.Communicator);
                    }

                    throw;
                }

            }
        }

        private void StopAll()
        {
            lock (this)
            {
                //
                // First wait for any active startService/stopService calls to complete.
                //
                while (_pendingStatusChanges)
                {
                    System.Threading.Monitor.Wait(this);
                }

                //
                // For each service, we call stop on the service and flush its database environment to
                // the disk. Services are stopped in the reverse order of the order they were started.
                //
                _services.Reverse();
                var stoppedServices = new List<string>();
                foreach (ServiceInfo info in _services)
                {
                    if (info.Status == ServiceStatus.Started)
                    {
                        try
                        {
                            Debug.Assert(info.Service != null);
                            info.Service.stop();
                            stoppedServices.Add(info.Name);
                        }
                        catch (System.Exception ex)
                        {
                            _logger.Warning($"IceBox.ServiceManager: exception while stopping service {info.Name}:\n{ex}");
                        }
                    }

                    if (info.Communicator != null)
                    {
                        DestroyServiceCommunicator(info.Name, info.Communicator);
                    }
                }

                if (_sharedCommunicator != null)
                {
                    RemoveAdminFacets("IceBox.SharedCommunicator.");

                    try
                    {
                        _sharedCommunicator.Destroy();
                    }
                    catch (System.Exception ex)
                    {
                        _logger.Warning($"ServiceManager: exception while destroying shared communicator:\n{ex}");
                    }
                    _sharedCommunicator = null;
                }

                _services.Clear();
                ServicesStopped(stoppedServices, _observers.Keys);
            }
        }

        private void ServicesStarted(List<string> services, Dictionary<IServiceObserverPrx, bool>.KeyCollection observers)
        {
            //
            // Must be called with 'this' unlocked
            //

            if (services.Count > 0)
            {
                string[] servicesArray = services.ToArray();

                foreach (IServiceObserverPrx observer in observers)
                {
                    observer.ServicesStartedAsync(servicesArray).ContinueWith((t) => ObserverCompleted(observer, t),
                        TaskScheduler.Current);
                }
            }
        }

        private void ServicesStopped(List<string> services, Dictionary<IServiceObserverPrx, bool>.KeyCollection observers)
        {
            //
            // Must be called with 'this' unlocked
            //

            if (services.Count > 0)
            {
                string[] servicesArray = services.ToArray();

                foreach (IServiceObserverPrx observer in observers)
                {
                    observer.ServicesStoppedAsync(servicesArray).ContinueWith((t) => ObserverCompleted(observer, t),
                        TaskScheduler.Current);
                }
            }
        }

        private void
        ObserverCompleted(IServiceObserverPrx observer, Task t)
        {
            try
            {
                t.Wait();
            }
            catch (AggregateException ae)
            {
                lock (this)
                {
                    if (_observers.Remove(observer))
                    {
                        ObserverRemoved(observer, ae.InnerException);
                    }
                }
            }
        }

        private void ObserverRemoved(IServiceObserverPrx observer, System.Exception ex)
        {
            if (_traceServiceObserver >= 1)
            {
                //
                // CommunicatorDestroyedException may occur during shutdown. The observer notification has
                // been sent, but the communicator was destroyed before the reply was received. We do not
                // log a message for this exception.
                //
                if (!(ex is CommunicatorDestroyedException))
                {
                    _logger.Trace("IceBox.ServiceObserver",
                                  $"Removed service observer {observer}\nafter catching {ex}");
                }
            }
        }

        private enum ServiceStatus
        {
            Stopping,
            Stopped,
            Starting,
            Started
        }

        private class ServiceInfo
        {
            internal ServiceInfo(string name, ServiceStatus status, string[] args)
            {
                Name = name;
                Status = status;
                Args = args;
            }
            internal readonly string Name;
            internal ServiceStatus Status;
            internal string[] Args;
            internal IService? Service;
            internal Communicator? Communicator;
        }

        private class StartServiceInfo
        {
            internal StartServiceInfo(string service, string value, string[] serverArgs)
            {
                //
                // Separate the entry point from the arguments.
                //
                Name = service;

                try
                {
                    Args = IceUtilInternal.Options.split(value);
                }
                catch (FormatException ex)
                {
                    throw new ArgumentException($"ServiceManager: invalid arguments for service `{Name}'", ex);
                }

                Debug.Assert(Args.Length > 0);

                EntryPoint = Args[0];
                Args = Args.Skip(1).Concat(serverArgs.Where(arg => arg.StartsWith($"--{service}."))).ToArray();
            }

            internal string Name;
            internal string EntryPoint;
            internal string[] Args;
        }

        private Dictionary<string, string> CreateServiceProperties(string service)
        {
            Dictionary<string, string> properties;
            if ((_communicator.GetPropertyAsInt("IceBox.InheritProperties") ?? 0) > 0)
            {
                // Inherit all except Ice.Admin.xxx properties
                properties = _communicator.GetProperties().Where(p => !p.Key.StartsWith("Ice.Admin.")).ToDictionary(
                    p => p.Key, p => p.Value);
            }
            else
            {
                properties = new Dictionary<string, string>();
            }

            string? programName = _communicator.GetProperty("Ice.ProgramName");
            properties["Ice.ProgramName"] = programName == null ? service : $"{programName}-{service}";
            return properties;
        }

        private void DestroyServiceCommunicator(string service, Communicator communicator)
        {
            if (communicator != null)
            {
                try
                {
                    communicator.Shutdown();
                    communicator.WaitForShutdown();
                }
                catch (CommunicatorDestroyedException)
                {
                    //
                    // Ignore, the service might have already destroyed
                    // the communicator for its own reasons.
                    //
                }
                catch (System.Exception ex)
                {
                    _logger.Warning($"ServiceManager: exception while shutting down communicator for service {service}\n{ex}");
                }

                RemoveAdminFacets("IceBox.Service." + service + ".");
                communicator.Destroy();
            }
        }

        private bool ConfigureAdmin(Dictionary<string, string> properties, string prefix)
        {
            if (_adminEnabled && !properties.ContainsKey("Ice.Admin.Enabled"))
            {
                var facetNames = new List<string>();
                Debug.Assert(_adminFacetFilter != null);
                foreach (string p in _adminFacetFilter)
                {
                    if (p.StartsWith(prefix))
                    {
                        facetNames.Add(p.Substring(prefix.Length));
                    }
                }

                if (_adminFacetFilter.Count == 0 || facetNames.Count > 0)
                {
                    properties["Ice.Admin.Enabled"] = "1";

                    if (facetNames.Count > 0)
                    {
                        // TODO: need String.Join with escape!
                        properties["Ice.Admin.Facets"] = string.Join(" ", facetNames.ToArray());
                    }
                    return true;
                }
            }
            return false;
        }

        private void RemoveAdminFacets(string prefix)
        {
            try
            {
                foreach (string p in _communicator.FindAllAdminFacets().Keys)
                {
                    if (p.StartsWith(prefix))
                    {
                        _communicator.RemoveAdminFacet(p);
                    }
                }
            }
            catch (CommunicatorDestroyedException)
            {
                // Ignored
            }
            catch (ObjectAdapterDeactivatedException)
            {
                // Ignored
            }
        }

        private readonly Communicator _communicator;
        private readonly bool _adminEnabled = false;
        private readonly HashSet<string>? _adminFacetFilter = null;
        private Communicator? _sharedCommunicator = null;
        private readonly ILogger _logger;
        private readonly string[] _argv; // Filtered server argument vector
        private readonly List<ServiceInfo> _services = new List<ServiceInfo>();
        private bool _pendingStatusChanges = false;
        private readonly Dictionary<IServiceObserverPrx, bool> _observers = new Dictionary<IServiceObserverPrx, bool>();
        private readonly int _traceServiceObserver = 0;
    }

}
