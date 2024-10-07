// Copyright (c) ZeroC, Inc.

// Generated by makeprops.py from PropertyNames.xml

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

namespace Ice.Internal;

public sealed class PropertyNames
{
    internal static PropertyArray ProxyProps = new(
        "Proxy",
        [
            new(@"EndpointSelection", false, "", false, null, false),
            new(@"ConnectionCached", false, "", false, null, false),
            new(@"PreferSecure", false, "", false, null, false),
            new(@"LocatorCacheTimeout", false, "", false, null, false),
            new(@"InvocationTimeout", false, "", false, null, false),
            new(@"Locator", false, "", false, null, false),
            new(@"Router", false, "", false, null, false),
            new(@"CollocationOptimized", false, "", false, null, false),
            new(@"^Context\.[^\s]+$", true, "", false, null, false),
        ]);

    internal static PropertyArray ConnectionProps = new(
        "Connection",
        [
            new(@"CloseTimeout", false, "10", false, null, false),
            new(@"ConnectTimeout", false, "10", false, null, false),
            new(@"EnableIdleCheck", false, "1", false, null, false),
            new(@"IdleTimeout", false, "60", false, null, false),
            new(@"InactivityTimeout", false, "300", false, null, false),
            new(@"MaxDispatches", false, "100", false, null, false),
        ]);

    internal static PropertyArray ThreadPoolProps = new(
        "ThreadPool",
        [
            new(@"Size", false, "1", false, null, false),
            new(@"SizeMax", false, "", false, null, false),
            new(@"SizeWarn", false, "0", false, null, false),
            new(@"StackSize", false, "0", false, null, false),
            new(@"Serialize", false, "0", false, null, false),
            new(@"ThreadIdleTime", false, "60", false, null, false),
            new(@"ThreadPriority", false, "", false, null, false),
        ]);

    internal static PropertyArray ObjectAdapterProps = new(
        "ObjectAdapter",
        [
            new(@"AdapterId", false, "", false, null, false),
            new(@"Connection", false, "", false, ConnectionProps, false),
            new(@"Endpoints", false, "", false, null, false),
            new(@"Locator", false, "", false, ProxyProps, false),
            new(@"PublishedEndpoints", false, "", false, null, false),
            new(@"PublishedHost", false, "", false, null, false),
            new(@"ReplicaGroupId", false, "", false, null, false),
            new(@"Router", false, "", false, ProxyProps, false),
            new(@"ProxyOptions", false, "", false, null, false),
            new(@"ThreadPool", false, "", false, ThreadPoolProps, false),
            new(@"MaxConnections", false, "0", false, null, false),
            new(@"MessageSizeMax", false, "", false, null, false),
        ]);

    internal static PropertyArray IceProps = new(
        "Ice",
        [
            new(@"AcceptClassCycles", false, "0", false, null, false),
            new(@"Admin", false, "", false, ObjectAdapterProps, false),
            new(@"Admin.DelayCreation", false, "0", false, null, false),
            new(@"Admin.Enabled", false, "", false, null, false),
            new(@"Admin.Facets", false, "", false, null, false),
            new(@"Admin.InstanceName", false, "", false, null, false),
            new(@"Admin.Logger.KeepLogs", false, "100", false, null, false),
            new(@"Admin.Logger.KeepTraces", false, "100", false, null, false),
            new(@"Admin.Logger.Properties", false, "", false, null, false),
            new(@"Admin.ServerId", false, "", false, null, false),
            new(@"BackgroundLocatorCacheUpdates", false, "0", false, null, false),
            new(@"BatchAutoFlush", false, "", true, null, false),
            new(@"BatchAutoFlushSize", false, "1024", false, null, false),
            new(@"ClassGraphDepthMax", false, "10", false, null, false),
            new(@"Compression.Level", false, "1", false, null, false),
            new(@"Config", false, "", false, null, false),
            new(@"Connection.Client", false, "", false, ConnectionProps, false),
            new(@"Connection.Server", false, "", false, ConnectionProps, false),
            new(@"ConsoleListener", false, "1", false, null, false),
            new(@"Default.CollocationOptimized", false, "1", false, null, false),
            new(@"Default.EncodingVersion", false, "1.1", false, null, false),
            new(@"Default.EndpointSelection", false, "Random", false, null, false),
            new(@"Default.Host", false, "", false, null, false),
            new(@"Default.Locator", false, "", false, ProxyProps, false),
            new(@"Default.LocatorCacheTimeout", false, "-1", false, null, false),
            new(@"Default.InvocationTimeout", false, "-1", false, null, false),
            new(@"Default.PreferSecure", false, "0", false, null, false),
            new(@"Default.Protocol", false, "tcp", false, null, false),
            new(@"Default.Router", false, "", false, ProxyProps, false),
            new(@"Default.SlicedFormat", false, "0", false, null, false),
            new(@"Default.SourceAddress", false, "", false, null, false),
            new(@"HTTPProxyHost", false, "", false, null, false),
            new(@"HTTPProxyPort", false, "1080", false, null, false),
            new(@"ImplicitContext", false, "None", false, null, false),
            new(@"InitPlugins", false, "1", false, null, false),
            new(@"IPv4", false, "1", false, null, false),
            new(@"IPv6", false, "1", false, null, false),
            new(@"LogFile", false, "", false, null, false),
            new(@"MessageSizeMax", false, "1024", false, null, false),
            new(@"Override.Compress", false, "", false, null, false),
            new(@"Override.Secure", false, "", false, null, false),
            new(@"^Plugin\.[^\s]+$", true, "", false, null, false),
            new(@"PluginLoadOrder", false, "", false, null, false),
            new(@"PreferIPv6Address", false, "0", false, null, false),
            new(@"PreloadAssemblies", false, "0", false, null, false),
            new(@"PrintAdapterReady", false, "", false, null, false),
            new(@"PrintProcessId", false, "", false, null, false),
            new(@"ProgramName", false, "", false, null, false),
            new(@"RetryIntervals", false, "0", false, null, false),
            new(@"ServerIdleTime", false, "0", false, null, false),
            new(@"SOCKSProxyHost", false, "", false, null, false),
            new(@"SOCKSProxyPort", false, "1080", false, null, false),
            new(@"StdErr", false, "", false, null, false),
            new(@"StdOut", false, "", false, null, false),
            new(@"ThreadPool.Client", false, "", false, ThreadPoolProps, false),
            new(@"ThreadPool.Server", false, "", false, ThreadPoolProps, false),
            new(@"ThreadPriority", false, "", false, null, false),
            new(@"ToStringMode", false, "Unicode", false, null, false),
            new(@"Trace.Admin.Properties", false, "0", false, null, false),
            new(@"Trace.Admin.Logger", false, "0", false, null, false),
            new(@"Trace.Locator", false, "0", false, null, false),
            new(@"Trace.Network", false, "0", false, null, false),
            new(@"Trace.Protocol", false, "0", false, null, false),
            new(@"Trace.Retry", false, "0", false, null, false),
            new(@"Trace.Slicing", false, "0", false, null, false),
            new(@"Trace.ThreadPool", false, "0", false, null, false),
            new(@"UDP.RcvSize", false, "", false, null, false),
            new(@"UDP.SndSize", false, "", false, null, false),
            new(@"TCP.Backlog", false, "", false, null, false),
            new(@"TCP.RcvSize", false, "", false, null, false),
            new(@"TCP.SndSize", false, "", false, null, false),
            new(@"Warn.AMICallback", false, "1", false, null, false),
            new(@"Warn.Connections", false, "0", false, null, false),
            new(@"Warn.Datagrams", false, "0", false, null, false),
            new(@"Warn.Dispatch", false, "1", false, null, false),
            new(@"Warn.Endpoints", false, "1", false, null, false),
            new(@"Warn.UnusedProperties", false, "0", false, null, false),
            new(@"CacheMessageBuffers", false, "2", false, null, false),
        ]);

    internal static PropertyArray IceMXProps = new(
        "IceMX",
        [
            new(@"^Metrics\.[^\s]+\.GroupBy$", true, "", false, null, false),
            new(@"^Metrics\.[^\s]+\.Map$", true, "", false, null, false),
            new(@"^Metrics\.[^\s]+\.RetainDetached$", true, "10", false, null, false),
            new(@"^Metrics\.[^\s]+\.Accept$", true, "", false, null, false),
            new(@"^Metrics\.[^\s]+\.Reject$", true, "", false, null, false),
            new(@"^Metrics\.[^\s]+$", true, "", false, null, false),
        ]);

    internal static PropertyArray IceDiscoveryProps = new(
        "IceDiscovery",
        [
            new(@"Multicast", false, "", false, ObjectAdapterProps, false),
            new(@"Reply", false, "", false, ObjectAdapterProps, false),
            new(@"Locator", false, "", false, ObjectAdapterProps, false),
            new(@"Lookup", false, "", false, null, false),
            new(@"Timeout", false, "300", false, null, false),
            new(@"RetryCount", false, "3", false, null, false),
            new(@"LatencyMultiplier", false, "1", false, null, false),
            new(@"Address", false, "", false, null, false),
            new(@"Port", false, "4061", false, null, false),
            new(@"Interface", false, "", false, null, false),
            new(@"DomainId", false, "", false, null, false),
        ]);

    internal static PropertyArray IceLocatorDiscoveryProps = new(
        "IceLocatorDiscovery",
        [
            new(@"Reply", false, "", false, ObjectAdapterProps, false),
            new(@"Locator", false, "", false, ObjectAdapterProps, false),
            new(@"Lookup", false, "", false, null, false),
            new(@"Timeout", false, "300", false, null, false),
            new(@"RetryCount", false, "3", false, null, false),
            new(@"RetryDelay", false, "2000", false, null, false),
            new(@"Address", false, "", false, null, false),
            new(@"Port", false, "4061", false, null, false),
            new(@"Interface", false, "", false, null, false),
            new(@"InstanceName", false, "IceLocatorDiscovery", false, null, false),
            new(@"Trace.Lookup", false, "0", false, null, false),
        ]);

    internal static PropertyArray IceBoxProps = new(
        "IceBox",
        [
            new(@"InheritProperties", false, "", false, null, false),
            new(@"LoadOrder", false, "", false, null, false),
            new(@"PrintServicesReady", false, "", false, null, false),
            new(@"^Service\.[^\s]+$", true, "", false, null, false),
            new(@"Trace.ServiceObserver", false, "", false, null, false),
            new(@"^UseSharedCommunicator\.[^\s]+$", true, "", false, null, false),
        ]);

    internal static PropertyArray IceSSLProps = new(
        "IceSSL",
        [
            new(@"CAs", false, "", false, null, false),
            new(@"CertStore", false, "My", false, null, false),
            new(@"CertStoreLocation", false, "CurrentUser", false, null, false),
            new(@"CertFile", false, "", false, null, false),
            new(@"CheckCertName", false, "0", false, null, false),
            new(@"CheckCRL", false, "0", false, null, false),
            new(@"DefaultDir", false, "", false, null, false),
            new(@"FindCert", false, "", false, null, false),
            new(@"Password", false, "", false, null, false),
            new(@"Trace.Security", false, "0", false, null, false),
            new(@"TrustOnly", false, "", false, null, false),
            new(@"TrustOnly.Client", false, "", false, null, false),
            new(@"TrustOnly.Server", false, "", false, null, false),
            new(@"^TrustOnly\.Server\.[^\s]+$", true, "", false, null, false),
            new(@"UsePlatformCAs", false, "0", false, null, false),
            new(@"VerifyPeer", false, "2", false, null, false),
        ]);

    internal static PropertyArray[] validProps =
    [
        IceProps,
        IceMXProps,
        IceDiscoveryProps,
        IceLocatorDiscoveryProps,
        IceBoxProps,
        IceSSLProps,
    ];

    internal static string[] clPropNames =
    [
        "Ice",
        "IceMX",
        "IceDiscovery",
        "IceLocatorDiscovery",
        "IceBox",
        "IceBoxAdmin",
        "IceBridge",
        "IceGridAdmin",
        "IceGrid",
        "IceSSL",
        "IceStorm",
        "IceStormAdmin",
        "IceBT",
        "Glacier2",
    ];
}
