//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using System.Threading;
using System.Text;
using System.Linq;
using System.Globalization;
using IceInternal;

namespace Ice
{
    public enum ToStringMode
    {
        Unicode,
        ASCII,
        Compat
    }

    internal sealed class BufSizeWarnInfo
    {
        // Whether send size warning has been emitted
        public bool sndWarn;

        // The send size for which the warning wwas emitted
        public int sndSize;

        // Whether receive size warning has been emitted
        public bool rcvWarn;

        // The receive size for which the warning wwas emitted
        public int rcvSize;
    }

    public sealed partial class Communicator : IDisposable
    {
        private class ObserverUpdater : Instrumentation.IObserverUpdater
        {
            public ObserverUpdater(Communicator communicator) => _communicator = communicator;

            public void UpdateConnectionObservers() => _communicator.UpdateConnectionObservers();

            public void UpdateThreadObservers() => _communicator.UpdateThreadObservers();

            private readonly Communicator _communicator;
        }

        public Action<Action, Connection?>? Dispatcher { get; }
        /// <summary>
        /// Get the logger for this communicator.
        /// </summary>
        /// <returns>This communicator's logger.
        ///
        /// </returns>
        public ILogger Logger { get; internal set; }

        public Instrumentation.ICommunicatorObserver? Observer { get; }

        public Action? ThreadStart { get; private set; }

        public Action? ThreadStop { get; private set; }

        public ToStringMode ToStringMode { get; }

        internal int CacheMessageBuffers { get; }
        internal int ClassGraphDepthMax { get; } // No mutex lock, immutable.
        internal ACMConfig ClientACM { get; }
        internal DefaultsAndOverrides DefaultsAndOverrides { get; private set; }
        internal int MessageSizeMax { get; } // No mutex lock, immutable.
        internal INetworkProxy? NetworkProxy { get; }
        internal bool PreferIPv6 { get; }
        internal int ProtocolSupport { get; }
        internal ACMConfig ServerACM { get; }
        internal TraceLevels TraceLevels { get; private set; }

        private static string[] _emptyArgs = Array.Empty<string>();
        private static readonly string[] _suffixes =
        {
            "EndpointSelection",
            "ConnectionCached",
            "PreferSecure",
            "LocatorCacheTimeout",
            "InvocationTimeout",
            "Locator",
            "Router",
            "CollocationOptimized",
            "Context\\..*"
        };
        private static readonly object _staticLock = new object();

        private const int StateActive = 0;
        private const int StateDestroyInProgress = 1;
        private const int StateDestroyed = 2;

        private ObjectAdapter? _adminAdapter;
        private readonly bool _adminEnabled = false;
        private readonly HashSet<string> _adminFacetFilter = new HashSet<string>();
        private readonly Dictionary<string, (object servant, Disp disp)> _adminFacets =
            new Dictionary<string, (object servant, Disp disp)>();
        private Identity? _adminIdentity;
        private AsyncIOThread? _asyncIOThread;
        private IceInternal.ThreadPool _clientThreadPool;
        private readonly Func<int, string>? _compactIdResolver;
        private ILocatorPrx? _defaultLocator;
        private IRouterPrx? _defaultRouter;
        private EndpointFactoryManager _endpointFactoryManager;
        private EndpointHostResolver _endpointHostResolver;
        private readonly ImplicitContext? _implicitContext; // Immutable
        private LocatorManager _locatorManager;
        private ObjectAdapterFactory _objectAdapterFactory;
        private static bool _oneOffDone = false;
        private OutgoingConnectionFactory _outgoingConnectionFactory;
        private static bool _printProcessIdDone = false;
        private RequestHandlerFactory? _requestHandlerFactory;
        private readonly int[] _retryIntervals;
        private RetryQueue _retryQueue;
        private RouterManager _routerManager;
        private IceInternal.ThreadPool? _serverThreadPool;
        private readonly Dictionary<short, BufSizeWarnInfo> _setBufSizeWarn = new Dictionary<short, BufSizeWarnInfo>();
        private int _state;
        private IceInternal.Timer _timer;
        private string[] _typeIdNamespaces = { "Ice.TypeId" };

        public Communicator(Dictionary<string, string>? properties,
                            Func<int, string>? compactIdResolver = null,
                            Action<Action, Connection?>? dispatcher = null,
                            ILogger? logger = null,
                            Instrumentation.ICommunicatorObserver? observer = null,
                            Action? threadStart = null,
                            Action? threadStop = null,
                            string[]? typeIdNamespaces = null) :
            this(ref _emptyArgs,
                 null,
                 properties,
                 compactIdResolver,
                 dispatcher,
                 logger,
                 observer,
                 threadStart,
                 threadStop,
                 typeIdNamespaces)
        {
        }

        public Communicator(ref string[] args,
                            Dictionary<string, string>? properties,
                            Func<int, string>? compactIdResolver = null,
                            Action<Action, Connection?>? dispatcher = null,
                            ILogger? logger = null,
                            Instrumentation.ICommunicatorObserver? observer = null,
                            Action? threadStart = null,
                            Action? threadStop = null,
                            string[]? typeIdNamespaces = null) :
            this(ref args,
                 null,
                 properties,
                 compactIdResolver,
                 dispatcher,
                 logger,
                 observer,
                 threadStart,
                 threadStop,
                 typeIdNamespaces)
        {
        }

        public Communicator(NameValueCollection? appSettings = null,
                            Dictionary<string, string>? properties = null,
                            Func<int, string>? compactIdResolver = null,
                            Action<Action, Connection?>? dispatcher = null,
                            ILogger? logger = null,
                            Instrumentation.ICommunicatorObserver? observer = null,
                            Action? threadStart = null,
                            Action? threadStop = null,
                            string[]? typeIdNamespaces = null) :
            this(ref _emptyArgs,
                 appSettings,
                 properties,
                 compactIdResolver,
                 dispatcher,
                 logger,
                 observer,
                 threadStart,
                 threadStop,
                 typeIdNamespaces)
        {
        }

        public Communicator(ref string[] args,
                            NameValueCollection? appSettings,
                            Dictionary<string, string>? properties = null,
                            Func<int, string>? compactIdResolver = null,
                            Action<Action, Connection?>? dispatcher = null,
                            ILogger? logger = null,
                            Instrumentation.ICommunicatorObserver? observer = null,
                            Action? threadStart = null,
                            Action? threadStop = null,
                            string[]? typeIdNamespaces = null)
        {
            _state = StateActive;
            _compactIdResolver = compactIdResolver;
            Dispatcher = dispatcher;
            Logger = logger ?? Util.getProcessLogger();
            Observer = observer;
            ThreadStart = threadStart;
            ThreadStop = threadStop;
            _typeIdNamespaces = typeIdNamespaces ?? new string[] { "Ice.TypeId" };

            if (properties == null)
            {
                properties = new Dictionary<string, string>();
            }
            else
            {
                // clone properties as we don't want to modify the properties given to
                // this constructor
                properties = new Dictionary<string, string>(properties);
            }

            if (appSettings != null)
            {
                foreach (var key in appSettings.AllKeys)
                {
                    string[]? values = appSettings.GetValues(key);
                    if (values == null)
                    {
                        properties[key] = "";
                    }
                    else
                    {
                        // TODO: this join is not sufficient to create a string
                        // compatible with GetPropertyAsList
                        properties[key] = string.Join(",", values);
                    }
                }
            }

            if (!properties.ContainsKey("Ice.ProgramName"))
            {
                properties["Ice.ProgramName"] = AppDomain.CurrentDomain.FriendlyName;
            }

            properties.ParseIceArgs(ref args);
            SetProperties(properties);

            try
            {
                lock (_staticLock)
                {
                    if (!_oneOffDone)
                    {
                        string? stdOut = GetProperty("Ice.StdOut");

                        System.IO.StreamWriter? outStream = null;

                        if (stdOut != null)
                        {
                            try
                            {
                                outStream = System.IO.File.AppendText(stdOut);
                            }
                            catch (System.IO.IOException ex)
                            {
                                FileException fe = new Ice.FileException(ex);
                                fe.path = stdOut;
                                throw fe;
                            }
                            outStream.AutoFlush = true;
                            Console.Out.Close();
                            Console.SetOut(outStream);
                        }

                        string? stdErr = GetProperty("Ice.StdErr");
                        if (stdErr != null)
                        {
                            if (stdErr.Equals(stdOut))
                            {
                                Console.SetError(outStream);
                            }
                            else
                            {
                                System.IO.StreamWriter errStream;
                                try
                                {
                                    errStream = System.IO.File.AppendText(stdErr);
                                }
                                catch (System.IO.IOException ex)
                                {
                                    Ice.FileException fe = new Ice.FileException(ex);
                                    fe.path = stdErr;
                                    throw fe;
                                }
                                errStream.AutoFlush = true;
                                Console.Error.Close();
                                Console.SetError(errStream);
                            }
                        }

                        _oneOffDone = true;
                    }
                }

                if (logger == null)
                {
                    string? logfile = GetProperty("Ice.LogFile");
                    string? programName = GetProperty("Ice.ProgramName");
                    Debug.Assert(programName != null);
                    if (logfile != null)
                    {
                        Logger = new FileLoggerI(programName, logfile);
                    }
                    else if (Util.getProcessLogger() is LoggerI)
                    {
                        //
                        // Ice.ConsoleListener is enabled by default.
                        //
                        Logger = new TraceLoggerI(programName, (GetPropertyAsInt("Ice.ConsoleListener") ?? 1) > 0);
                    }
                    // else already set to process logger
                }

                TraceLevels = new TraceLevels(this);

                DefaultsAndOverrides = new DefaultsAndOverrides(this, Logger);

                ClientACM = new ACMConfig(this, Logger, "Ice.ACM.Client",
                                           new ACMConfig(this, Logger, "Ice.ACM", new ACMConfig(false)));

                ServerACM = new ACMConfig(this, Logger, "Ice.ACM.Server",
                                           new ACMConfig(this, Logger, "Ice.ACM", new ACMConfig(true)));

                {
                    int num = GetPropertyAsInt("Ice.MessageSizeMax") ?? 1024;
                    if (num < 1 || num > 0x7fffffff / 1024)
                    {
                        MessageSizeMax = 0x7fffffff;
                    }
                    else
                    {
                        MessageSizeMax = num * 1024; // Property is in kilobytes, MessageSizeMax in bytes
                    }
                }

                {
                    var num = GetPropertyAsInt("Ice.ClassGraphDepthMax") ?? 100;
                    if (num < 1 || num > 0x7fffffff)
                    {
                        ClassGraphDepthMax = 0x7fffffff;
                    }
                    else
                    {
                        ClassGraphDepthMax = num;
                    }
                }

                ToStringMode = Enum.Parse<ToStringMode>(GetProperty("Ice.ToStringMode") ?? "Unicode");

                CacheMessageBuffers = GetPropertyAsInt("Ice.CacheMessageBuffers") ?? 2;

                _implicitContext = ImplicitContext.Create(GetProperty("Ice.ImplicitContext"));
                _routerManager = new RouterManager();

                _locatorManager = new LocatorManager(this);

                string[]? arr = GetPropertyAsList("Ice.RetryIntervals");

                if (arr == null)
                {
                    _retryIntervals = new int[] { 0 };
                }
                else
                {
                    _retryIntervals = new int[arr.Length];
                    for (int i = 0; i < arr.Length; i++)
                    {
                        int v = int.Parse(arr[i], CultureInfo.InvariantCulture);
                        //
                        // If -1 is the first value, no retry and wait intervals.
                        //
                        if (i == 0 && v == -1)
                        {
                            _retryIntervals = Array.Empty<int>();
                            break;
                        }

                        _retryIntervals[i] = v > 0 ? v : 0;
                    }
                }

                _requestHandlerFactory = new RequestHandlerFactory(this);

                bool isIPv6Supported = Network.isIPv6Supported();
                bool ipv4 = (GetPropertyAsInt("Ice.IPv4") ?? 1) > 0;
                bool ipv6 = (GetPropertyAsInt("Ice.IPv6") ?? (isIPv6Supported ? 1 : 0)) > 0;
                if (!ipv4 && !ipv6)
                {
                    throw new InitializationException("Both IPV4 and IPv6 support cannot be disabled.");
                }
                else if (ipv4 && ipv6)
                {
                    ProtocolSupport = Network.EnableBoth;
                }
                else if (ipv4)
                {
                    ProtocolSupport = Network.EnableIPv4;
                }
                else
                {
                    ProtocolSupport = Network.EnableIPv6;
                }
                PreferIPv6 = GetPropertyAsInt("Ice.PreferIPv6Address") > 0;

                NetworkProxy = CreateNetworkProxy(ProtocolSupport);

                _endpointFactoryManager = new EndpointFactoryManager(this);

                ProtocolInstance tcpInstance = new ProtocolInstance(this, TCPEndpointType.value, "tcp", false);
                _endpointFactoryManager.add(new TcpEndpointFactory(tcpInstance));

                ProtocolInstance udpInstance = new ProtocolInstance(this, UDPEndpointType.value, "udp", false);
                _endpointFactoryManager.add(new UdpEndpointFactory(udpInstance));

                ProtocolInstance wsInstance = new ProtocolInstance(this, WSEndpointType.value, "ws", false);
                _endpointFactoryManager.add(new WSEndpointFactory(wsInstance, TCPEndpointType.value));

                ProtocolInstance wssInstance = new ProtocolInstance(this, WSSEndpointType.value, "wss", true);
                _endpointFactoryManager.add(new WSEndpointFactory(wssInstance, SSLEndpointType.value));

                _outgoingConnectionFactory = new OutgoingConnectionFactory(this);

                _objectAdapterFactory = new ObjectAdapterFactory(this);

                _retryQueue = new RetryQueue(this);

                if (GetPropertyAsInt("Ice.PreloadAssemblies") > 0)
                {
                    AssemblyUtil.preloadAssemblies();
                }

                //
                // Load plug-ins.
                //
                Debug.Assert(_serverThreadPool == null);
                LoadPlugins(ref args);

                //
                // Initialize the endpoint factories once all the plugins are loaded. This gives
                // the opportunity for the endpoint factories to find underyling factories.
                //
                _endpointFactoryManager.initialize();

                //
                // Create Admin facets, if enabled.
                //
                // Note that any logger-dependent admin facet must be created after we load all plugins,
                // since one of these plugins can be a Logger plugin that sets a new logger during loading
                //

                if (GetProperty("Ice.Admin.Enabled") == null)
                {
                    _adminEnabled = GetProperty("Ice.Admin.Endpoints") != null;
                }
                else
                {
                    _adminEnabled = GetPropertyAsInt("Ice.Admin.Enabled") > 0;
                }

                _adminFacetFilter = new HashSet<string>(
                    (GetPropertyAsList("Ice.Admin.Facets") ?? Array.Empty<string>()).Distinct());

                if (_adminEnabled)
                {
                    //
                    // Process facet
                    //
                    string processFacetName = "Process";
                    if (_adminFacetFilter.Count == 0 || _adminFacetFilter.Contains(processFacetName))
                    {
                        ProcessTraits traits = default;
                        IProcess process = new IceInternal.Process(this);
                        Disp disp = (current, incoming) => traits.Dispatch(process, current, incoming);
                        _adminFacets.Add(processFacetName, (process, disp));
                    }

                    //
                    // Logger facet
                    //
                    string loggerFacetName = "Logger";
                    if (_adminFacetFilter.Count == 0 || _adminFacetFilter.Contains(loggerFacetName))
                    {
                        ILoggerAdminLogger loggerAdminLogger = new LoggerAdminLogger(this, Logger);
                        Logger = loggerAdminLogger;
                        LoggerAdminTraits traits = default;
                        ILoggerAdmin servant = loggerAdminLogger.getFacet();
                        Disp disp = (incoming, current) => traits.Dispatch(servant, incoming, current);
                        _adminFacets.Add(loggerFacetName, (servant, disp));
                    }

                    //
                    // Properties facet
                    //
                    string propertiesFacetName = "Properties";
                    PropertiesAdmin? propsAdmin = null;
                    if (_adminFacetFilter.Count == 0 || _adminFacetFilter.Contains(propertiesFacetName))
                    {
                        propsAdmin = new PropertiesAdmin(this);
                        PropertiesAdminTraits traits = default;
                        Disp disp = (current, incoming) => traits.Dispatch(propsAdmin, current, incoming);
                        _adminFacets.Add(propertiesFacetName, (propsAdmin, disp));
                    }

                    //
                    // Metrics facet
                    //
                    string metricsFacetName = "Metrics";
                    if (_adminFacetFilter.Count == 0 || _adminFacetFilter.Contains(metricsFacetName))
                    {
                        var communicatorObserver = new CommunicatorObserverI(this, Logger);
                        Observer = communicatorObserver;
                        IceMX.MetricsAdminTraits traits = default;
                        var metricsAdmin = communicatorObserver.getFacet();
                        Disp disp = (current, incoming) => traits.Dispatch(metricsAdmin, current, incoming);
                        _adminFacets.Add(metricsFacetName, (metricsAdmin, disp));

                        //
                        // Make sure the admin plugin receives property updates.
                        //
                        if (propsAdmin != null)
                        {
                            propsAdmin.addUpdateCallback((Dictionary<string, string> updates) =>
                            {
                                communicatorObserver.getFacet().updated(updates);
                            });
                        }
                    }
                }

                //
                // Set observer updater
                //
                if (Observer != null)
                {
                    Observer.setObserverUpdater(new ObserverUpdater(this));
                }

                //
                // Create threads.
                //
                try
                {
                    _timer = new IceInternal.Timer(this, IceInternal.Util.stringToThreadPriority(
                                                   GetProperty("Ice.ThreadPriority")));
                }
                catch (System.Exception ex)
                {
                    Logger.error($"cannot create thread for timer:\n{ex}");
                    throw;
                }

                try
                {
                    _endpointHostResolver = new EndpointHostResolver(this);
                }
                catch (System.Exception ex)
                {
                    Logger.error($"cannot create thread for endpoint host resolver:\n{ex}");
                    throw;
                }
                _clientThreadPool = new IceInternal.ThreadPool(this, "Ice.ThreadPool.Client", 0);

                //
                // The default router/locator may have been set during the loading of plugins.
                // Therefore we make sure it is not already set before checking the property.
                //
                if (GetDefaultRouter() == null)
                {
                    IRouterPrx? router = GetPropertyAsProxy("Ice.Default.Router", IRouterPrx.Factory);
                    if (router != null)
                    {
                        SetDefaultRouter(router);
                    }
                }

                if (GetDefaultLocator() == null)
                {
                    ILocatorPrx? locator = GetPropertyAsProxy("Ice.Default.Locator", ILocatorPrx.Factory);
                    if (locator != null)
                    {
                        SetDefaultLocator(locator);
                    }
                }

                //
                // Show process id if requested (but only once).
                //
                lock (this)
                {
                    if (!_printProcessIdDone && GetPropertyAsInt("Ice.PrintProcessId") > 0)
                    {
                        using var p = System.Diagnostics.Process.GetCurrentProcess();
                        Console.WriteLine(p.Id);
                        _printProcessIdDone = true;
                    }
                }

                //
                // Server thread pool initialization is lazy in serverThreadPool().
                //

                //
                // An application can set Ice.InitPlugins=0 if it wants to postpone
                // initialization until after it has interacted directly with the
                // plug-ins.
                //
                if ((GetPropertyAsInt("Ice.InitPlugins") ?? 1) > 0)
                {
                    InitializePlugins();
                }

                //
                // This must be done last as this call creates the Ice.Admin object adapter
                // and eventually registers a process proxy with the Ice locator (allowing
                // remote clients to invoke on Ice.Admin facets as soon as it's registered).
                //
                if ((GetPropertyAsInt("Ice.Admin.DelayCreation") ?? 0) <= 0)
                {
                    GetAdmin();
                }
            }
            catch (System.Exception)
            {
                Destroy();
                throw;
            }
        }

        /// <summary>
        /// Add a new facet to the Admin object.
        /// Adding a servant with a facet that is already registered
        /// throws AlreadyRegisteredException.
        ///
        /// </summary>
        /// <param name="servant">The servant that implements the new Admin facet.
        /// </param>
        /// <param name="facet">The name of the new Admin facet.</param>
        public void AddAdminFacet<T, Traits>(T servant, string facet) where Traits : struct, IInterfaceTraits<T>
        {
            Traits traits = default;
            Disp disp = (incoming, current) => traits.Dispatch(servant, incoming, current);
            Debug.Assert(servant != null);
            AddAdminFacet(servant, disp, facet);
        }

        public void AddAdminFacet(object servant, Disp disp, string facet)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                if (_adminFacetFilter.Count > 0 && !_adminFacetFilter.Contains(facet))
                {
                    throw new ArgumentException($"facet `{facet}' not allow by Ice.Admin.Facets configuration", nameof(facet));
                }

                if (_adminFacets.ContainsKey(facet))
                {
                    throw new ArgumentException($"A facet `{facet}' is already registered", nameof(facet));
                }
                _adminFacets.Add(facet, (servant, disp));
                if (_adminAdapter != null)
                {
                    _adminAdapter.Add(disp, _adminIdentity, facet);
                }
            }
        }

        /// <summary>
        /// Add the Admin object with all its facets to the provided object adapter.
        /// If Ice.Admin.ServerId is set and the provided object adapter has a Locator,
        /// createAdmin registers the Admin's Process facet with the Locator's LocatorRegistry.
        ///
        /// createAdmin call only be called once; subsequent calls raise InitializationException.
        ///
        /// </summary>
        /// <param name="adminAdapter">The object adapter used to host the Admin object; if null and
        /// Ice.Admin.Endpoints is set, create, activate and use the Ice.Admin object adapter.
        ///
        /// </param>
        /// <param name="adminIdentity">The identity of the Admin object.
        ///
        /// </param>
        /// <returns>A proxy to the main ("") facet of the Admin object. Never returns a null proxy.
        ///
        /// </returns>
        public IObjectPrx CreateAdmin(ObjectAdapter? adminAdapter, Identity adminIdentity)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                if (_adminAdapter != null)
                {
                    throw new InitializationException("Admin already created");
                }

                if (!_adminEnabled)
                {
                    throw new InitializationException("Admin is disabled");
                }

                _adminIdentity = adminIdentity;
                if (adminAdapter == null)
                {
                    if (GetProperty("Ice.Admin.Endpoints") != null)
                    {
                        adminAdapter = _objectAdapterFactory.createObjectAdapter("Ice.Admin", null);
                    }
                    else
                    {
                        throw new InitializationException("Ice.Admin.Endpoints is not set");
                    }
                }
                else
                {
                    _adminAdapter = adminAdapter;
                }
                Debug.Assert(_adminAdapter != null);
                AddAllAdminFacets();
            }

            if (adminAdapter == null)
            {
                try
                {
                    _adminAdapter.Activate();
                }
                catch (LocalException)
                {
                    //
                    // We cleanup _adminAdapter, however this error is not recoverable
                    // (can't call again getAdmin() after fixing the problem)
                    // since all the facets (servants) in the adapter are lost
                    //
                    _adminAdapter.Destroy();
                    lock (this)
                    {
                        _adminAdapter = null;
                    }
                    throw;
                }
            }
            SetServerProcessProxy(_adminAdapter, adminIdentity);
            return _adminAdapter.CreateProxy(adminIdentity);
        }

        /// <summary>
        /// Create a new object adapter.
        /// The endpoints for the object
        /// adapter are taken from the property name.Endpoints.
        ///
        /// It is legal to create an object adapter with the empty string as
        /// its name. Such an object adapter is accessible via bidirectional
        /// connections or by collocated invocations that originate from the
        /// same communicator as is used by the adapter.
        ///
        /// Attempts to create a named object adapter for which no configuration
        /// can be found raise InitializationException.
        ///
        /// </summary>
        /// <param name="name">The object adapter name.
        ///
        /// </param>
        /// <returns>The new object adapter.
        ///
        /// </returns>
        public ObjectAdapter CreateObjectAdapter(string name)
        {
            return ObjectAdapterFactory().createObjectAdapter(name, null);
        }

        /// <summary>
        /// Create a new object adapter with endpoints.
        /// This operation sets
        /// the property name.Endpoints, and then calls
        /// createObjectAdapter. It is provided as a convenience
        /// function.
        ///
        /// Calling this operation with an empty name will result in a
        /// UUID being generated for the name.
        ///
        /// </summary>
        /// <param name="name">The object adapter name.
        ///
        /// </param>
        /// <param name="endpoints">The endpoints for the object adapter.
        ///
        /// </param>
        /// <returns>The new object adapter.
        ///
        /// </returns>
        public ObjectAdapter CreateObjectAdapterWithEndpoints(string name, string endpoints)
        {
            if (name.Length == 0)
            {
                name = Guid.NewGuid().ToString();
            }

            SetProperty($"{name}.Endpoints", endpoints);
            return ObjectAdapterFactory().createObjectAdapter(name, null);
        }

        /// <summary>
        /// Create a new object adapter with a router.
        /// This operation
        /// creates a routed object adapter.
        ///
        /// Calling this operation with an empty name will result in a
        /// UUID being generated for the name.
        ///
        /// </summary>
        /// <param name="name">The object adapter name.
        ///
        /// </param>
        /// <param name="router">The router.
        ///
        /// </param>
        /// <returns>The new object adapter.
        ///
        /// </returns>
        public ObjectAdapter CreateObjectAdapterWithRouter(string name, IRouterPrx router)
        {
            if (name.Length == 0)
            {
                name = Guid.NewGuid().ToString();
            }

            //
            // We set the proxy properties here, although we still use the proxy supplied.
            //
            Dictionary<string, string> properties = router.ToProperty($"{name}.Router");
            foreach (KeyValuePair<string, string> entry in properties)
            {
                SetProperty(entry.Key, entry.Value);
            }

            return ObjectAdapterFactory().createObjectAdapter(name, router);
        }

        public Reference CreateReference(string s, string? propertyPrefix = null)
        {
            const string delim = " \t\n\r";

            int beg;
            int end = 0;

            beg = IceUtilInternal.StringUtil.findFirstNotOf(s, delim, end);
            if (beg == -1)
            {
                throw new FormatException($"no non-whitespace characters found in `{s}'");
            }

            //
            // Extract the identity, which may be enclosed in single
            // or double quotation marks.
            //
            string idstr;
            end = IceUtilInternal.StringUtil.checkQuote(s, beg);
            if (end == -1)
            {
                throw new FormatException($"mismatched quotes around identity in `{s} '");
            }
            else if (end == 0)
            {
                end = IceUtilInternal.StringUtil.findFirstOf(s, delim + ":@", beg);
                if (end == -1)
                {
                    end = s.Length;
                }
                idstr = s.Substring(beg, end - beg);
            }
            else
            {
                beg++; // Skip leading quote
                idstr = s.Substring(beg, end - beg);
                end++; // Skip trailing quote
            }

            if (beg == end)
            {
                throw new FormatException($"no identity in `{s}'");
            }

            //
            // Parsing the identity may raise FormatException.
            //
            Identity ident = Identity.Parse(idstr);

            string facet = "";
            InvocationMode mode = InvocationMode.Twoway;
            bool secure = false;
            EncodingVersion encoding = DefaultsAndOverrides.defaultEncoding;
            ProtocolVersion protocol = Util.Protocol_1_0;
            string adapter;

            while (true)
            {
                beg = IceUtilInternal.StringUtil.findFirstNotOf(s, delim, end);
                if (beg == -1)
                {
                    break;
                }

                if (s[beg] == ':' || s[beg] == '@')
                {
                    break;
                }

                end = IceUtilInternal.StringUtil.findFirstOf(s, delim + ":@", beg);
                if (end == -1)
                {
                    end = s.Length;
                }

                if (beg == end)
                {
                    break;
                }

                string option = s.Substring(beg, end - beg);
                if (option.Length != 2 || option[0] != '-')
                {
                    throw new FormatException("expected a proxy option but found `{option}' in `{s}'");
                }

                //
                // Check for the presence of an option argument. The
                // argument may be enclosed in single or double
                // quotation marks.
                //
                string? argument = null;
                int argumentBeg = IceUtilInternal.StringUtil.findFirstNotOf(s, delim, end);
                if (argumentBeg != -1)
                {
                    char ch = s[argumentBeg];
                    if (ch != '@' && ch != ':' && ch != '-')
                    {
                        beg = argumentBeg;
                        end = IceUtilInternal.StringUtil.checkQuote(s, beg);
                        if (end == -1)
                        {
                            throw new FormatException($"mismatched quotes around value for {option} option in `{s}'");
                        }
                        else if (end == 0)
                        {
                            end = IceUtilInternal.StringUtil.findFirstOf(s, delim + ":@", beg);
                            if (end == -1)
                            {
                                end = s.Length;
                            }
                            argument = s.Substring(beg, end - beg);
                        }
                        else
                        {
                            beg++; // Skip leading quote
                            argument = s.Substring(beg, end - beg);
                            end++; // Skip trailing quote
                        }
                    }
                }

                //
                // If any new options are added here,
                // IceInternal::Reference::toString() and its derived classes must be updated as well.
                //
                switch (option[1])
                {
                    case 'f':
                        {
                            if (argument == null)
                            {
                                throw new FormatException($"no argument provided for -f option in `{s}'");
                            }

                            facet = IceUtilInternal.StringUtil.unescapeString(argument, 0, argument.Length, "");
                            break;
                        }

                    case 't':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -t option in `{s}'");
                            }
                            mode = InvocationMode.Twoway;
                            break;
                        }

                    case 'o':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -o option in `{s}'");
                            }
                            mode = InvocationMode.Oneway;
                            break;
                        }

                    case 'O':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -O option in `{s}'");
                            }
                            mode = InvocationMode.BatchOneway;
                            break;
                        }

                    case 'd':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -d option in `{s}'");
                            }
                            mode = InvocationMode.Datagram;
                            break;
                        }

                    case 'D':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -D option in `{s}'");
                            }
                            mode = InvocationMode.BatchDatagram;
                            break;
                        }

                    case 's':
                        {
                            if (argument != null)
                            {
                                throw new FormatException(
                                    $"unexpected argument `{argument}' provided for -s option in `{s}'");
                            }
                            secure = true;
                            break;
                        }

                    case 'e':
                        {
                            if (argument == null)
                            {
                                throw new FormatException($"no argument provided for -e option in `{s}'");
                            }

                            encoding = Util.stringToEncodingVersion(argument);
                            break;
                        }

                    case 'p':
                        {
                            if (argument == null)
                            {
                                throw new FormatException($"no argument provided for -p option `{s}'");
                            }

                            protocol = Util.stringToProtocolVersion(argument);
                            break;
                        }

                    default:
                        {
                            throw new FormatException("unknown option `{option}' in `{s}'");
                        }
                }
            }

            if (beg == -1)
            {
                return CreateReference(ident, facet, mode, secure, protocol, encoding, Array.Empty<Endpoint>(),
                    null, propertyPrefix);
            }

            List<Endpoint> endpoints = new List<Endpoint>();

            if (s[beg] == ':')
            {
                List<string> unknownEndpoints = new List<string>();
                end = beg;

                while (end < s.Length && s[end] == ':')
                {
                    beg = end + 1;

                    end = beg;
                    while (true)
                    {
                        end = s.IndexOf(':', end);
                        if (end == -1)
                        {
                            end = s.Length;
                            break;
                        }
                        else
                        {
                            bool quoted = false;
                            int quote = beg;
                            while (true)
                            {
                                quote = s.IndexOf('\"', quote);
                                if (quote == -1 || end < quote)
                                {
                                    break;
                                }
                                else
                                {
                                    quote = s.IndexOf('\"', ++quote);
                                    if (quote == -1)
                                    {
                                        break;
                                    }
                                    else if (end < quote)
                                    {
                                        quoted = true;
                                        break;
                                    }
                                    ++quote;
                                }
                            }
                            if (!quoted)
                            {
                                break;
                            }
                            ++end;
                        }
                    }

                    string es = s.Substring(beg, end - beg);
                    Endpoint? endp = EndpointFactoryManager().create(es, false);
                    if (endp != null)
                    {
                        endpoints.Add(endp);
                    }
                    else
                    {
                        unknownEndpoints.Add(es);
                    }
                }
                if (endpoints.Count == 0)
                {
                    Debug.Assert(unknownEndpoints.Count > 0);
                    throw new FormatException($"invalid endpoint `{unknownEndpoints[0]}' in `{s}'");
                }
                else if (unknownEndpoints.Count != 0 && (GetPropertyAsInt("Ice.Warn.Endpoints") ?? 1) > 0)
                {
                    StringBuilder msg = new StringBuilder("Proxy contains unknown endpoints:");
                    int sz = unknownEndpoints.Count;
                    for (int idx = 0; idx < sz; ++idx)
                    {
                        msg.Append(" `");
                        msg.Append(unknownEndpoints[idx]);
                        msg.Append("'");
                    }
                    Logger.warning(msg.ToString());
                }

                Endpoint[] ep = endpoints.ToArray();
                return CreateReference(ident, facet, mode, secure, protocol, encoding, ep, null, propertyPrefix);
            }
            else if (s[beg] == '@')
            {
                beg = IceUtilInternal.StringUtil.findFirstNotOf(s, delim, beg + 1);
                if (beg == -1)
                {
                    throw new ArgumentException($"missing adapter id in `{s}'");
                }

                string adapterstr;
                end = IceUtilInternal.StringUtil.checkQuote(s, beg);
                if (end == -1)
                {
                    throw new ArgumentException($"mismatched quotes around adapter id in `{s}'");
                }
                else if (end == 0)
                {
                    end = IceUtilInternal.StringUtil.findFirstOf(s, delim, beg);
                    if (end == -1)
                    {
                        end = s.Length;
                    }
                    adapterstr = s.Substring(beg, end - beg);
                }
                else
                {
                    beg++; // Skip leading quote
                    adapterstr = s.Substring(beg, end - beg);
                    end++; // Skip trailing quote
                }

                if (end != s.Length && IceUtilInternal.StringUtil.findFirstNotOf(s, delim, end) != -1)
                {
                    throw new ArgumentException(
                        $"invalid trailing characters after `{s.Substring(0, end + 1)}' in `{s}'");
                }

                adapter = IceUtilInternal.StringUtil.unescapeString(adapterstr, 0, adapterstr.Length, "");

                if (adapter.Length == 0)
                {
                    throw new ArgumentException($"empty adapter id in `{s}'");
                }
                return CreateReference(ident, facet, mode, secure, protocol, encoding, Array.Empty<Endpoint>(),
                    adapter, propertyPrefix);
            }

            throw new ArgumentException($"malformed proxy `{s}'");
        }

        public Reference CreateReference(Identity ident, InputStream s)
        {
            //
            // Don't read the identity here. Operations calling this
            // constructor read the identity, and pass it as a parameter.
            //

            //
            // For compatibility with the old FacetPath.
            //
            string[] facetPath = s.ReadStringSeq();
            string facet;
            if (facetPath.Length > 0)
            {
                if (facetPath.Length > 1)
                {
                    throw new ProxyUnmarshalException();
                }
                facet = facetPath[0];
            }
            else
            {
                facet = "";
            }

            int mode = s.ReadByte();
            if (mode < 0 || mode > (int)InvocationMode.Last)
            {
                throw new ProxyUnmarshalException();
            }

            bool secure = s.ReadBool();

            ProtocolVersion protocol;
            EncodingVersion encoding;
            if (!s.Encoding.Equals(Util.Encoding_1_0))
            {
                byte major = s.ReadByte();
                byte minor = s.ReadByte();
                protocol = new ProtocolVersion(major, minor);

                major = s.ReadByte();
                minor = s.ReadByte();
                encoding = new EncodingVersion(major, minor);
            }
            else
            {
                protocol = Util.Protocol_1_0;
                encoding = Util.Encoding_1_0;
            }

            Endpoint[] endpoints;
            string adapterId = "";

            int sz = s.ReadSize();
            if (sz > 0)
            {
                endpoints = new Endpoint[sz];
                for (int i = 0; i < sz; i++)
                {
                    endpoints[i] = EndpointFactoryManager().read(s);
                }
            }
            else
            {
                endpoints = Array.Empty<Endpoint>();
                adapterId = s.ReadString();
            }

            return CreateReference(ident, facet, (InvocationMode)mode, secure, protocol, encoding, endpoints, adapterId,
                                   null);
        }

        /// <summary>
        /// Destroy the communicator.
        /// This operation calls shutdown
        /// implicitly.  Calling destroy cleans up memory, and shuts down
        /// this communicator's client functionality and destroys all object
        /// adapters. Subsequent calls to destroy are ignored.
        /// </summary>
        public void Destroy()
        {
            lock (this)
            {
                //
                // If destroy is in progress, wait for it to be done. This
                // is necessary in case destroy() is called concurrently
                // by multiple threads.
                //
                while (_state == StateDestroyInProgress)
                {
                    Monitor.Wait(this);
                }

                if (_state == StateDestroyed)
                {
                    return;
                }
                _state = StateDestroyInProgress;
            }

            //
            // Shutdown and destroy all the incoming and outgoing Ice
            // connections and wait for the connections to be finished.
            //
            _objectAdapterFactory.shutdown();
            _outgoingConnectionFactory.destroy();

            _objectAdapterFactory.destroy();
            _outgoingConnectionFactory.waitUntilFinished();

            _retryQueue.destroy(); // Must be called before destroying thread pools.

            if (Observer != null)
            {
                Observer.setObserverUpdater(null);
            }

            if (Logger is ILoggerAdminLogger)
            {
                ((ILoggerAdminLogger)Logger).destroy();
            }

            //
            // Now, destroy the thread pools. This must be done *only* after
            // all the connections are finished (the connections destruction
            // can require invoking callbacks with the thread pools).
            //
            if (_serverThreadPool != null)
            {
                _serverThreadPool.destroy();
            }
            _clientThreadPool.destroy();

            if (_asyncIOThread != null)
            {
                _asyncIOThread.destroy();
            }
            _endpointHostResolver.destroy();

            //
            // Wait for all the threads to be finished.
            //
            _timer.destroy();
            _clientThreadPool.joinWithAllThreads();
            if (_serverThreadPool != null)
            {
                _serverThreadPool.joinWithAllThreads();
            }
            if (_asyncIOThread != null)
            {
                _asyncIOThread.joinWithThread();
            }
            _endpointHostResolver.joinWithThread();
            _routerManager.destroy();
            _locatorManager.destroy();
            _endpointFactoryManager.destroy();

            if (GetPropertyAsInt("Ice.Warn.UnusedProperties") > 0)
            {
                List<string> unusedProperties = GetUnusedProperties();
                if (unusedProperties.Count != 0)
                {
                    StringBuilder message = new StringBuilder("The following properties were set but never read:");
                    foreach (string s in unusedProperties)
                    {
                        message.Append("\n    ");
                        message.Append(s);
                    }
                    Logger.warning(message.ToString());
                }
            }

            //
            // Destroy last so that a Logger plugin can receive all log/traces before its destruction.
            //
            List<(string Name, IPlugin Plugin)> plugins;
            lock (this)
            {
                plugins = new List<(string Name, IPlugin Plugin)>(_plugins);
            }
            plugins.Reverse();
            foreach (var p in plugins)
            {
                try
                {
                    p.Plugin.destroy();
                }
                catch (System.Exception ex)
                {
                    Util.getProcessLogger().warning(
                        $"unexpected exception raised by plug-in `{p.Name}' destruction:\n{ex}");
                }
            }

            lock (this)
            {
                _serverThreadPool = null;
                _asyncIOThread = null;

                _requestHandlerFactory = null;

                _adminAdapter = null;
                _adminFacets.Clear();

                _state = StateDestroyed;
                Monitor.PulseAll(this);
            }

            {
                if (Logger != null && Logger is FileLoggerI)
                {
                    ((FileLoggerI)Logger).destroy();
                }
            }
        }

        public void Dispose() => Destroy();

        /// <summary>
        /// Returns a facet of the Admin object.
        /// </summary>
        /// <param name="facet">The name of the Admin facet.
        /// </param>
        /// <returns>The servant associated with this Admin facet, or
        /// null if no facet is registered with the given name.</returns>
        public (object servant, Disp disp) FindAdminFacet(string facet)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                (object servant, Disp disp) result;
                if (!_adminFacets.TryGetValue(facet, out result))
                {
                    return default;
                }
                return result;
            }
        }

        /// <summary>
        /// Returns a map of all facets of the Admin object.
        /// </summary>
        /// <returns>A collection containing all the facet names and
        /// servants of the Admin object.
        ///
        /// </returns>
        public Dictionary<string, (object servant, Disp disp)> FindAllAdminFacets()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return new Dictionary<string, (object servant, Disp disp)>(_adminFacets);
            }
        }

        /// <summary>
        /// Get a proxy to the main facet of the Admin object.
        /// GetAdmin also creates the Admin object and creates and activates the Ice.Admin object
        /// adapter to host this Admin object if Ice.Admin.Enpoints is set. The identity of the Admin
        /// object created by getAdmin is {value of Ice.Admin.InstanceName}/admin, or {UUID}/admin
        /// when Ice.Admin.InstanceName is not set.
        ///
        /// If Ice.Admin.DelayCreation is 0 or not set, getAdmin is called by the communicator
        /// initialization, after initialization of all plugins.
        ///
        /// </summary>
        /// <returns>A proxy to the main ("") facet of the Admin object, or a null proxy if no
        /// Admin object is configured.</returns>
        public IObjectPrx? GetAdmin()
        {
            ObjectAdapter adminAdapter;
            Identity adminIdentity;

            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                if (_adminAdapter != null)
                {
                    Debug.Assert(_adminIdentity != null);
                    return _adminAdapter.CreateProxy(_adminIdentity.Value);
                }
                else if (_adminEnabled)
                {
                    if (GetProperty("Ice.Admin.Endpoints") != null)
                    {
                        adminAdapter = _objectAdapterFactory.createObjectAdapter("Ice.Admin", null);
                    }
                    else
                    {
                        return null;
                    }
                    adminIdentity = new Identity("admin", GetProperty("Ice.Admin.InstanceName") ?? "");
                    if (adminIdentity.Category.Length == 0)
                    {
                        adminIdentity.Category = Guid.NewGuid().ToString();
                    }

                    _adminIdentity = adminIdentity;
                    _adminAdapter = adminAdapter;
                    AddAllAdminFacets();
                    // continue below outside synchronization
                }
                else
                {
                    return null;
                }
            }

            try
            {
                adminAdapter.Activate();
            }
            catch (LocalException)
            {
                // We cleanup _adminAdapter, however this error is not recoverable
                // (can't call again getAdmin() after fixing the problem)
                // since all the facets (servants) in the adapter are lost
                adminAdapter.Destroy();
                lock (this)
                {
                    _adminAdapter = null;
                }
                throw;
            }

            SetServerProcessProxy(adminAdapter, adminIdentity);
            return adminAdapter.CreateProxy(adminIdentity);
        }

        /// <summary>
        /// Get the default locator this communicator.
        /// </summary>
        /// <returns>The default locator for this communicator.</returns>
        public ILocatorPrx? GetDefaultLocator()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _defaultLocator;
            }
        }

        /// <summary>
        /// Get the default router this communicator.
        /// </summary>
        /// <returns>The default router for this communicator.</returns>
        public IRouterPrx? GetDefaultRouter()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                return _defaultRouter;
            }
        }

        /// <summary>
        /// Get the implicit context associated with this communicator.
        /// </summary>
        /// <returns>The implicit context associated with this communicator;
        /// returns null when the property Ice.ImplicitContext is not set
        /// or is set to None.</returns>
        public IImplicitContext? GetImplicitContext() => _implicitContext;

        /// <summary>
        /// Check whether communicator has been shut down.
        /// </summary>
        /// <returns>True if the communicator has been shut down; false otherwise.</returns>
        public bool IsShutdown()
        {
            try
            {
                return ObjectAdapterFactory().isShutdown();
            }
            catch (CommunicatorDestroyedException)
            {
                return true;
            }
        }

        public LocatorManager LocatorManager()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _locatorManager;
            }
        }

        public ObjectAdapterFactory ObjectAdapterFactory()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _objectAdapterFactory;
            }
        }

        /// <summary>
        /// Remove the following facet to the Admin object.
        /// Removing a facet that was not previously registered throws
        /// NotRegisteredException.
        ///
        /// </summary>
        /// <param name="facet">The name of the Admin facet.
        /// </param>
        /// <returns>The servant associated with this Admin facet.</returns>
        public (object servant, Disp disp) RemoveAdminFacet(string facet)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                (object servant, Disp disp) result = default;
                if (!_adminFacets.TryGetValue(facet, out result))
                {
                    throw new NotRegisteredException("facet", facet);
                }
                _adminFacets.Remove(facet);
                if (_adminAdapter != null)
                {
                    Debug.Assert(_adminIdentity != null);
                    _adminAdapter.Remove(_adminIdentity.Value, facet);
                }
                return result;
            }
        }

        public RouterManager RouterManager()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _routerManager;
            }
        }

        /// <summary>
        /// Set a default Ice locator for this communicator.
        /// All newly
        /// created proxy and object adapters will use this default
        /// locator. To disable the default locator, null can be used.
        /// Note that this operation has no effect on existing proxies or
        /// object adapters.
        ///
        /// You can also set a locator for an individual proxy by calling the
        /// operation ice_locator on the proxy, or for an object adapter
        /// by calling ObjectAdapter.setLocator on the object adapter.
        ///
        /// </summary>
        /// <param name="locator">The default locator to use for this communicator.</param>
        public void SetDefaultLocator(ILocatorPrx? locator)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                _defaultLocator = locator;
            }
        }

        /// <summary>
        /// Set a default router for this communicator.
        /// All newly
        /// created proxies will use this default router. To disable the
        /// default router, null can be used. Note that this
        /// operation has no effect on existing proxies.
        ///
        /// You can also set a router for an individual proxy
        /// by calling the operation ice_router on the proxy.
        ///
        /// </summary>
        /// <param name="router">The default router to use for this communicator.
        ///
        /// </param>
        public void SetDefaultRouter(IRouterPrx router)
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                _defaultRouter = router;
            }
        }

        /// <summary>
        /// Shuts down this communicator's server functionality, which
        /// includes the deactivation of all object adapters.
        /// Attempts to use a
        /// deactivated object adapter raise ObjectAdapterDeactivatedException.
        /// Subsequent calls to shutdown are ignored.
        ///
        /// After shutdown returns, no new requests are processed. However, requests
        /// that have been started before shutdown was called might still be active.
        /// You can use waitForShutdown to wait for the completion of all
        /// requests.
        /// </summary>
        public void Shutdown()
        {
            try
            {
                ObjectAdapterFactory().shutdown();
            }
            catch (CommunicatorDestroyedException)
            {
                // Ignore
            }
        }

        public IceInternal.Timer Timer()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _timer;
            }
        }

        /// <summary>
        /// Wait until the application has called shutdown (or destroy).
        /// On the server side, this operation blocks the calling thread
        /// until all currently-executing operations have completed.
        /// On the client side, the operation simply blocks until another
        /// thread has called shutdown or destroy.
        ///
        /// A typical use of this operation is to call it from the main thread,
        /// which then waits until some other thread calls shutdown.
        /// After shut-down is complete, the main thread returns and can do some
        /// cleanup work before it finally calls destroy to shut down
        /// the client functionality, and then exits the application.
        ///
        /// </summary>
        public void WaitForShutdown()
        {
            try
            {
                ObjectAdapterFactory().waitForShutdown();
            }
            catch (CommunicatorDestroyedException)
            {
                // Ignore
            }
        }

        internal AsyncIOThread AsyncIOThread()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                if (_asyncIOThread == null) // Lazy initialization.
                {
                    _asyncIOThread = new AsyncIOThread(this);
                }

                return _asyncIOThread;
            }
        }

        internal int CheckRetryAfterException(LocalException ex, Reference @ref, ref int cnt)
        {
            ILogger logger = Logger;

            if (@ref.getMode() == InvocationMode.BatchOneway || @ref.getMode() == InvocationMode.BatchDatagram)
            {
                Debug.Assert(false); // batch no longer implemented anyway
                throw ex;
            }

            if (ex is ObjectNotExistException)
            {
                ObjectNotExistException one = (ObjectNotExistException)ex;
                RouterInfo? ri = @ref.getRouterInfo();
                if (ri != null && one.operation.Equals("ice_add_proxy"))
                {
                    //
                    // If we have a router, an ObjectNotExistException with an
                    // operation name "ice_add_proxy" indicates to the client
                    // that the router isn't aware of the proxy (for example,
                    // because it was evicted by the router). In this case, we
                    // must *always* retry, so that the missing proxy is added
                    // to the router.
                    //

                    ri.clearCache(@ref);

                    if (TraceLevels.retry >= 1)
                    {
                        string s = "retrying operation call to add proxy to router\n" + ex;
                        logger.trace(TraceLevels.retryCat, s);
                    }
                    return 0; // We must always retry, so we don't look at the retry count.
                }
                else if (@ref.isIndirect())
                {
                    //
                    // We retry ObjectNotExistException if the reference is
                    // indirect.
                    //

                    if (@ref.isWellKnown())
                    {
                        @ref.getLocatorInfo()?.clearCache(@ref);
                    }
                }
                else
                {
                    //
                    // For all other cases, we don't retry ObjectNotExistException.
                    //
                    throw ex;
                }
            }
            else if (ex is RequestFailedException)
            {
                throw ex;
            }

            //
            // There is no point in retrying an operation that resulted in a
            // MarshalException. This must have been raised locally (because if
            // it happened in a server it would result in an UnknownLocalException
            // instead), which means there was a problem in this process that will
            // not change if we try again.
            //
            if (ex is MarshalException)
            {
                throw ex;
            }

            //
            // Don't retry if the communicator is destroyed, object adapter is deactivated,
            // or connection is manually closed.
            //
            if (ex is CommunicatorDestroyedException ||
                ex is ObjectAdapterDeactivatedException ||
                ex is ConnectionManuallyClosedException)
            {
                throw ex;
            }

            //
            // Don't retry invocation timeouts.
            //
            if (ex is InvocationTimeoutException || ex is InvocationCanceledException)
            {
                throw ex;
            }

            ++cnt;
            Debug.Assert(cnt > 0);

            int interval;
            if (cnt == (_retryIntervals.Length + 1) && ex is Ice.CloseConnectionException)
            {
                //
                // A close connection exception is always retried at least once, even if the retry
                // limit is reached.
                //
                interval = 0;
            }
            else if (cnt > _retryIntervals.Length)
            {
                if (TraceLevels.retry >= 1)
                {
                    string s = "cannot retry operation call because retry limit has been exceeded\n" + ex;
                    logger.trace(TraceLevels.retryCat, s);
                }
                throw ex;
            }
            else
            {
                interval = _retryIntervals[cnt - 1];
            }

            if (TraceLevels.retry >= 1)
            {
                string s = "retrying operation call";
                if (interval > 0)
                {
                    s += " in " + interval + "ms";
                }
                s += " because of exception\n" + ex;
                logger.trace(TraceLevels.retryCat, s);
            }

            return interval;
        }

        internal IceInternal.ThreadPool ClientThreadPool()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _clientThreadPool;
            }
        }

        internal Reference CreateReference(Identity ident, string facet, Reference tmpl, Endpoint[] endpoints)
        {
            return CreateReference(ident, facet, tmpl.getMode(), tmpl.getSecure(), tmpl.getProtocol(), tmpl.getEncoding(),
                          endpoints, null, null);
        }

        internal Reference CreateReference(Identity ident, string facet, Reference tmpl, string adapterId)
        {
            //
            // Create new reference
            //
            return CreateReference(ident, facet, tmpl.getMode(), tmpl.getSecure(), tmpl.getProtocol(), tmpl.getEncoding(),
                          Array.Empty<Endpoint>(), adapterId, null);
        }

        internal Reference CreateReference(Identity ident, Connection connection)
        {
            //
            // Create new reference
            //
            return new FixedReference(
                this,
                ident,
                "", // Facet
                ((Endpoint)connection.Endpoint).datagram() ? InvocationMode.Datagram : InvocationMode.Twoway,
                ((Endpoint)connection.Endpoint).secure(),
                Util.Protocol_1_0,
                DefaultsAndOverrides.defaultEncoding,
                connection,
                -1,
                null,
                null);
        }

        internal EndpointFactoryManager EndpointFactoryManager()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _endpointFactoryManager;
            }
        }

        internal EndpointHostResolver EndpointHostResolver()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _endpointHostResolver;
            }
        }

        internal BufSizeWarnInfo GetBufSizeWarn(short type)
        {
            lock (_setBufSizeWarn)
            {
                BufSizeWarnInfo info;
                if (!_setBufSizeWarn.ContainsKey(type))
                {
                    info = new BufSizeWarnInfo();
                    info.sndWarn = false;
                    info.sndSize = -1;
                    info.rcvWarn = false;
                    info.rcvSize = -1;
                    _setBufSizeWarn.Add(type, info);
                }
                else
                {
                    info = _setBufSizeWarn[type];
                }
                return info;
            }
        }

        internal OutgoingConnectionFactory OutgoingConnectionFactory()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _outgoingConnectionFactory;
            }
        }

        internal RequestHandlerFactory RequestHandlerFactory()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                Debug.Assert(_requestHandlerFactory != null);
                return _requestHandlerFactory;
            }
        }

        //
        // Return the C# class associated with this Slice type-id
        // Used for both non-local Slice classes and exceptions
        //
        internal Type? ResolveClass(string id)
        {
            // First attempt corresponds to no cs:namespace metadata in the
            // enclosing top-level module
            //
            string className = TypeToClass(id);
            Type? c = AssemblyUtil.findType(className);

            //
            // If this fails, look for helper classes in the typeIdNamespaces namespace(s)
            //
            if (c == null && _typeIdNamespaces != null)
            {
                foreach (var ns in _typeIdNamespaces)
                {
                    Type? helper = AssemblyUtil.findType(ns + "." + className);
                    if (helper != null)
                    {
                        try
                        {
                            c = helper.GetProperty("targetClass").PropertyType;
                            break; // foreach
                        }
                        catch (System.Exception)
                        {
                        }
                    }
                }
            }

            //
            // Ensure the class is instantiable.
            //
            if (c != null && !c.IsAbstract && !c.IsInterface)
            {
                return c;
            }

            return null;
        }

        internal string ResolveCompactId(int compactId)
        {
            string[] defaultVal = { "IceCompactId" };
            var compactIdNamespaces = new List<string>(defaultVal);

            if (_typeIdNamespaces != null)
            {
                compactIdNamespaces.AddRange(_typeIdNamespaces);
            }

            string result = "";

            foreach (var ns in compactIdNamespaces)
            {
                string className = ns + ".TypeId_" + compactId;
                try
                {
                    Type? c = AssemblyUtil.findType(className);
                    if (c != null)
                    {
                        result = (string)c.GetField("typeId").GetValue(null);
                        break; // foreach
                    }
                }
                catch (System.Exception)
                {
                }
            }
            return result;
        }

        internal RetryQueue RetryQueue()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }
                return _retryQueue;
            }
        }

        internal IceInternal.ThreadPool ServerThreadPool()
        {
            lock (this)
            {
                if (_state == StateDestroyed)
                {
                    throw new CommunicatorDestroyedException();
                }

                if (_serverThreadPool == null) // Lazy initialization.
                {
                    if (_state == StateDestroyInProgress)
                    {
                        throw new CommunicatorDestroyedException();
                    }
                    _serverThreadPool = new IceInternal.ThreadPool(this, "Ice.ThreadPool.Server",
                        GetPropertyAsInt("Ice.ServerIdleTime") ?? 0);
                }

                return _serverThreadPool;
            }
        }

        internal void SetRcvBufSizeWarn(short type, int size)
        {
            lock (_setBufSizeWarn)
            {
                BufSizeWarnInfo info = GetBufSizeWarn(type);
                info.rcvWarn = true;
                info.rcvSize = size;
                _setBufSizeWarn[type] = info;
            }
        }

        internal void SetServerProcessProxy(ObjectAdapter adminAdapter, Identity adminIdentity)
        {
            IObjectPrx? admin = adminAdapter.CreateProxy(adminIdentity);
            ILocatorPrx? locator = adminAdapter.GetLocator();
            string? serverId = GetProperty("Ice.Admin.ServerId");

            if (locator != null && serverId != null)
            {
                IProcessPrx process = IProcessPrx.UncheckedCast(admin.Clone(facet: "Process"));
                try
                {
                    //
                    // Note that as soon as the process proxy is registered, the communicator might be
                    // shutdown by a remote client and admin facets might start receiving calls.
                    //
                    locator.GetRegistry().SetServerProcessProxy(serverId, process);
                }
                catch (ServerNotFoundException)
                {
                    if (TraceLevels.location >= 1)
                    {
                        StringBuilder s = new StringBuilder();
                        s.Append("couldn't register server `" + serverId + "' with the locator registry:\n");
                        s.Append("the server is not known to the locator registry");
                        Logger.trace(TraceLevels.locationCat, s.ToString());
                    }

                    throw new InitializationException("Locator knows nothing about server `" + serverId + "'");
                }
                catch (LocalException ex)
                {
                    if (TraceLevels.location >= 1)
                    {
                        StringBuilder s = new StringBuilder();
                        s.Append("couldn't register server `" + serverId + "' with the locator registry:\n" + ex);
                        Logger.trace(TraceLevels.locationCat, s.ToString());
                    }
                    throw; // TODO: Shall we raise a special exception instead of a non obvious local exception?
                }

                if (TraceLevels.location >= 1)
                {
                    StringBuilder s = new StringBuilder();
                    s.Append("registered server `" + serverId + "' with the locator registry");
                    Logger.trace(TraceLevels.locationCat, s.ToString());
                }
            }
        }

        internal void SetSndBufSizeWarn(short type, int size)
        {
            lock (_setBufSizeWarn)
            {
                BufSizeWarnInfo info = GetBufSizeWarn(type);
                info.sndWarn = true;
                info.sndSize = size;
                _setBufSizeWarn[type] = info;
            }
        }

        internal void SetThreadHook(Action threadStart, Action threadStop)
        {
            //
            // No locking, as it can only be called during plug-in loading
            //
            ThreadStart = threadStart;
            ThreadStop = threadStop;
        }

        internal void UpdateConnectionObservers()
        {
            try
            {
                _outgoingConnectionFactory.updateConnectionObservers();
                _objectAdapterFactory.updateConnectionObservers();
            }
            catch (CommunicatorDestroyedException)
            {
            }
        }

        internal void UpdateThreadObservers()
        {
            try
            {
                _clientThreadPool.updateObservers();
                if (_serverThreadPool != null)
                {
                    _serverThreadPool.updateObservers();
                }
                _objectAdapterFactory.updateThreadObservers();
                _endpointHostResolver.updateObserver();

                if (_asyncIOThread != null)
                {
                    _asyncIOThread.updateObserver();
                }
                Debug.Assert(Observer != null);
                _timer.updateObserver(Observer);
            }
            catch (CommunicatorDestroyedException)
            {
            }
        }

        private static string TypeToClass(string id)
        {
            if (!id.StartsWith("::", StringComparison.Ordinal))
            {
                throw new MarshalException("expected type id but received `" + id + "'");
            }
            return id.Substring(2).Replace("::", ".");
        }

        private void AddAllAdminFacets()
        {
            lock (this)
            {
                Debug.Assert(_adminAdapter != null);
                foreach (var entry in _adminFacets)
                {
                    if (_adminFacetFilter.Count == 0 || _adminFacetFilter.Contains(entry.Key))
                    {
                        _adminAdapter.Add(entry.Value.disp, _adminIdentity, entry.Key);
                    }
                }
            }
        }

        private void CheckForUnknownProperties(string prefix)
        {
            //
            // Do not warn about unknown properties if Ice prefix, ie Ice, Glacier2, etc
            //
            foreach (string name in PropertyNames.clPropNames)
            {
                if (prefix.StartsWith(string.Format("{0}.", name), StringComparison.Ordinal))
                {
                    return;
                }
            }

            List<string> unknownProps = new List<string>();
            Dictionary<string, string> props = GetProperties(forPrefix: $"{prefix}.");
            foreach (string prop in props.Keys)
            {
                bool valid = false;
                for (int i = 0; i < _suffixes.Length; ++i)
                {
                    string pattern = "^" + Regex.Escape(prefix + ".") + _suffixes[i] + "$";
                    if (new Regex(pattern).Match(prop).Success)
                    {
                        valid = true;
                        break;
                    }
                }

                if (!valid)
                {
                    unknownProps.Add(prop);
                }
            }

            if (unknownProps.Count != 0)
            {
                StringBuilder message = new StringBuilder("found unknown properties for proxy '");
                message.Append(prefix);
                message.Append("':");
                foreach (string s in unknownProps)
                {
                    message.Append("\n    ");
                    message.Append(s);
                }
                Logger.warning(message.ToString());
            }
        }

        private INetworkProxy? CreateNetworkProxy(int protocolSupport)
        {
            string? proxyHost = GetProperty("Ice.SOCKSProxyHost");
            if (proxyHost != null)
            {
                if (protocolSupport == Network.EnableIPv6)
                {
                    throw new InitializationException("IPv6 only is not supported with SOCKS4 proxies");
                }
                return new SOCKSNetworkProxy(proxyHost, GetPropertyAsInt("Ice.SOCKSProxyPort") ?? 1080);
            }

            proxyHost = GetProperty("Ice.HTTPProxyHost");
            if (proxyHost != null)
            {
                return new HTTPNetworkProxy(proxyHost, GetPropertyAsInt("Ice.HTTPProxyPort") ?? 1080);
            }

            return null;
        }

        private Reference CreateReference(
            Identity ident,
            string facet,
            InvocationMode mode,
            bool secure,
            ProtocolVersion protocol,
            EncodingVersion encoding,
            Endpoint[] endpoints,
            string? adapterId,
            string? propertyPrefix)
        {
            //
            // Default local proxy options.
            //
            LocatorInfo? locatorInfo = null;
            if (_defaultLocator != null)
            {
                if (!_defaultLocator.IceReference.getEncoding().Equals(encoding))
                {
                    locatorInfo = LocatorManager().get(_defaultLocator.Clone(encodingVersion: encoding));
                }
                else
                {
                    locatorInfo = LocatorManager().get(_defaultLocator);
                }
            }
            RouterInfo? routerInfo = null;
            if (_defaultRouter != null)
            {
                routerInfo = RouterManager().get(_defaultRouter);
            }
            bool collocOptimized = DefaultsAndOverrides.defaultCollocationOptimization;
            bool cacheConnection = true;
            bool preferSecure = DefaultsAndOverrides.defaultPreferSecure;
            EndpointSelectionType endpointSelection = DefaultsAndOverrides.defaultEndpointSelection;
            int locatorCacheTimeout = DefaultsAndOverrides.defaultLocatorCacheTimeout;
            int invocationTimeout = DefaultsAndOverrides.defaultInvocationTimeout;
            Dictionary<string, string>? context = null;

            //
            // Override the defaults with the proxy properties if a property prefix is defined.
            //
            if (propertyPrefix != null && propertyPrefix.Length > 0)
            {
                //
                // Warn about unknown properties.
                //
                if ((GetPropertyAsInt("Ice.Warn.UnknownProperties") ?? 1) > 0)
                {
                    CheckForUnknownProperties(propertyPrefix);
                }

                string property = $"{propertyPrefix}.Locator";
                ILocatorPrx? locator = GetPropertyAsProxy(property, ILocatorPrx.Factory);
                if (locator != null)
                {
                    if (!locator.IceReference.getEncoding().Equals(encoding))
                    {
                        locatorInfo = LocatorManager().get(locator.Clone(encodingVersion: encoding));
                    }
                    else
                    {
                        locatorInfo = LocatorManager().get(locator);
                    }
                }

                property = $"{propertyPrefix}.Router";
                IRouterPrx? router = GetPropertyAsProxy(property, IRouterPrx.Factory);
                if (router != null)
                {
                    if (propertyPrefix.EndsWith(".Router", StringComparison.Ordinal))
                    {
                        Logger.warning($"`{property}={GetProperty(property)}': cannot set a router on a router; setting ignored");
                    }
                    else
                    {
                        routerInfo = RouterManager().get(router);
                    }
                }

                property = $"{propertyPrefix}.CollocationOptimized";
                collocOptimized = (GetPropertyAsInt(property) ?? (collocOptimized ? 1 : 0)) > 0;

                property = $"{propertyPrefix}.ConnectionCached";
                cacheConnection = (GetPropertyAsInt(property) ?? (cacheConnection ? 1 : 0)) > 0;

                property = $"{propertyPrefix}.PreferSecure";
                preferSecure = (GetPropertyAsInt(property) ?? (preferSecure ? 1 : 0)) > 0;

                property = propertyPrefix + ".EndpointSelection";
                string? val = GetProperty(property);
                if (val != null)
                {
                    endpointSelection = Enum.Parse<EndpointSelectionType>(val);
                }

                property = $"{propertyPrefix}.LocatorCacheTimeout";
                val = GetProperty(property);
                if (val != null)
                {
                    locatorCacheTimeout = GetPropertyAsInt(property) ?? locatorCacheTimeout;
                    if (locatorCacheTimeout < -1)
                    {
                        locatorCacheTimeout = -1;
                        Logger.warning($"invalid value for {property} `{val}': defaulting to -1");
                    }
                }

                property = $"{propertyPrefix}.InvocationTimeout";
                val = GetProperty(property);
                if (val != null)
                {
                    invocationTimeout = GetPropertyAsInt(property) ?? invocationTimeout;
                    if (invocationTimeout < 1 && invocationTimeout != -1)
                    {
                        invocationTimeout = -1;
                        Logger.warning($"invalid value for {property} `{val}': defaulting to -1");
                    }
                }

                property = $"{propertyPrefix}.Context.";
                context = GetProperties(forPrefix: property).ToDictionary(e => e.Key.Substring(property.Length),
                                                                          e => e.Value);
            }

            //
            // Create new reference
            //
            return new RoutableReference(this,
                                         ident,
                                         facet,
                                         mode,
                                         secure,
                                         protocol,
                                         encoding,
                                         endpoints,
                                         adapterId,
                                         locatorInfo,
                                         routerInfo,
                                         collocOptimized,
                                         cacheConnection,
                                         preferSecure,
                                         endpointSelection,
                                         locatorCacheTimeout,
                                         invocationTimeout,
                                         context);
        }
    }
}
