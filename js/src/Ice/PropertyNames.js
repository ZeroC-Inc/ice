// Copyright (c) ZeroC, Inc. All rights reserved.

// Generated by makeprops.py from PropertyNames.xml, Thu Aug  1 10:41:04 2024

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

/* eslint comma-dangle: "off" */
/* eslint array-bracket-newline: "off" */
/* eslint no-useless-escape: "off" */

import { Property } from "./Property.js";
export const PropertyNames = {};
const IceProps =
[
    new Property("Ice.AcceptClassCycles", false, "0", false),
    new Property("Ice.Admin.AdapterId", false, "", false),
    new Property("Ice.Admin.Connection.CloseTimeout", false, "10", false),
    new Property("Ice.Admin.Connection.ConnectTimeout", false, "10", false),
    new Property("Ice.Admin.Connection.EnableIdleCheck", false, "1", false),
    new Property("Ice.Admin.Connection.IdleTimeout", false, "60", false),
    new Property("Ice.Admin.Connection.InactivityTimeout", false, "300", false),
    new Property("Ice.Admin.Connection.MaxDispatches", false, "0", false),
    new Property("Ice.Admin.Connection", false, "", false),
    new Property("Ice.Admin.Endpoints", false, "", false),
    new Property("Ice.Admin.Locator.EndpointSelection", false, "", false),
    new Property("Ice.Admin.Locator.ConnectionCached", false, "", false),
    new Property("Ice.Admin.Locator.PreferSecure", false, "", false),
    new Property("Ice.Admin.Locator.LocatorCacheTimeout", false, "", false),
    new Property("Ice.Admin.Locator.InvocationTimeout", false, "", false),
    new Property("Ice.Admin.Locator.Locator", false, "", false),
    new Property("Ice.Admin.Locator.Router", false, "", false),
    new Property("Ice.Admin.Locator.CollocationOptimized", false, "", false),
    new Property(/^Ice\.Admin\.Locator\.Context\../, true, "", false),
    new Property("Ice.Admin.Locator", false, "", false),
    new Property("Ice.Admin.PublishedEndpoints", false, "", false),
    new Property("Ice.Admin.ReplicaGroupId", false, "", false),
    new Property("Ice.Admin.Router.EndpointSelection", false, "", false),
    new Property("Ice.Admin.Router.ConnectionCached", false, "", false),
    new Property("Ice.Admin.Router.PreferSecure", false, "", false),
    new Property("Ice.Admin.Router.LocatorCacheTimeout", false, "", false),
    new Property("Ice.Admin.Router.InvocationTimeout", false, "", false),
    new Property("Ice.Admin.Router.Locator", false, "", false),
    new Property("Ice.Admin.Router.Router", false, "", false),
    new Property("Ice.Admin.Router.CollocationOptimized", false, "", false),
    new Property(/^Ice\.Admin\.Router\.Context\../, true, "", false),
    new Property("Ice.Admin.Router", false, "", false),
    new Property("Ice.Admin.ProxyOptions", false, "", false),
    new Property("Ice.Admin.ThreadPool.Size", false, "1", false),
    new Property("Ice.Admin.ThreadPool.SizeMax", false, "", false),
    new Property("Ice.Admin.ThreadPool.SizeWarn", false, "0", false),
    new Property("Ice.Admin.ThreadPool.StackSize", false, "0", false),
    new Property("Ice.Admin.ThreadPool.Serialize", false, "0", false),
    new Property("Ice.Admin.ThreadPool.ThreadIdleTime", false, "60", false),
    new Property("Ice.Admin.ThreadPool.ThreadPriority", false, "", false),
    new Property("Ice.Admin.MessageSizeMax", false, "", false),
    new Property("Ice.Admin.DelayCreation", false, "0", false),
    new Property("Ice.Admin.Enabled", false, "", false),
    new Property("Ice.Admin.Facets", false, "", false),
    new Property("Ice.Admin.InstanceName", false, "", false),
    new Property("Ice.Admin.Logger.KeepLogs", false, "100", false),
    new Property("Ice.Admin.Logger.KeepTraces", false, "100", false),
    new Property("Ice.Admin.Logger.Properties", false, "", false),
    new Property("Ice.Admin.ServerId", false, "", false),
    new Property("Ice.BackgroundLocatorCacheUpdates", false, "0", false),
    new Property("Ice.BatchAutoFlush", false, "", true),
    new Property("Ice.BatchAutoFlushSize", false, "1024", false),
    new Property("Ice.ChangeUser", false, "", false),
    new Property("Ice.ClassGraphDepthMax", false, "10", false),
    new Property("Ice.Compression.Level", false, "1", false),
    new Property("Ice.Config", false, "", false),
    new Property("Ice.Connection.CloseTimeout", false, "10", false),
    new Property("Ice.Connection.ConnectTimeout", false, "10", false),
    new Property("Ice.Connection.EnableIdleCheck", false, "1", false),
    new Property("Ice.Connection.IdleTimeout", false, "60", false),
    new Property("Ice.Connection.InactivityTimeout", false, "300", false),
    new Property("Ice.Connection.MaxDispatches", false, "0", false),
    new Property("Ice.Connection", false, "", false),
    new Property("Ice.ConsoleListener", false, "1", false),
    new Property("Ice.Default.CollocationOptimized", false, "1", false),
    new Property("Ice.Default.EncodingVersion", false, "1.1", false),
    new Property("Ice.Default.EndpointSelection", false, "Random", false),
    new Property("Ice.Default.Host", false, "", false),
    new Property("Ice.Default.Locator.EndpointSelection", false, "", false),
    new Property("Ice.Default.Locator.ConnectionCached", false, "", false),
    new Property("Ice.Default.Locator.PreferSecure", false, "", false),
    new Property("Ice.Default.Locator.LocatorCacheTimeout", false, "", false),
    new Property("Ice.Default.Locator.InvocationTimeout", false, "", false),
    new Property("Ice.Default.Locator.Locator", false, "", false),
    new Property("Ice.Default.Locator.Router", false, "", false),
    new Property("Ice.Default.Locator.CollocationOptimized", false, "", false),
    new Property(/^Ice\.Default\.Locator\.Context\../, true, "", false),
    new Property("Ice.Default.Locator", false, "", false),
    new Property("Ice.Default.LocatorCacheTimeout", false, "-1", false),
    new Property("Ice.Default.InvocationTimeout", false, "-1", false),
    new Property("Ice.Default.Package", false, "", false),
    new Property("Ice.Default.PreferSecure", false, "0", false),
    new Property("Ice.Default.Protocol", false, "tcp", false),
    new Property("Ice.Default.Router.EndpointSelection", false, "", false),
    new Property("Ice.Default.Router.ConnectionCached", false, "", false),
    new Property("Ice.Default.Router.PreferSecure", false, "", false),
    new Property("Ice.Default.Router.LocatorCacheTimeout", false, "", false),
    new Property("Ice.Default.Router.InvocationTimeout", false, "", false),
    new Property("Ice.Default.Router.Locator", false, "", false),
    new Property("Ice.Default.Router.Router", false, "", false),
    new Property("Ice.Default.Router.CollocationOptimized", false, "", false),
    new Property(/^Ice\.Default\.Router\.Context\../, true, "", false),
    new Property("Ice.Default.Router", false, "", false),
    new Property("Ice.Default.SlicedFormat", false, "0", false),
    new Property("Ice.Default.SourceAddress", false, "", false),
    new Property("Ice.Default.Timeout", false, "60000", false),
    new Property("Ice.EventLog.Source", false, "", false),
    new Property("Ice.FactoryAssemblies", false, "", false),
    new Property("Ice.HTTPProxyHost", false, "", false),
    new Property("Ice.HTTPProxyPort", false, "1080", false),
    new Property("Ice.ImplicitContext", false, "None", false),
    new Property("Ice.InitPlugins", false, "1", false),
    new Property("Ice.IPv4", false, "1", false),
    new Property("Ice.IPv6", false, "1", false),
    new Property("Ice.LogFile", false, "", false),
    new Property("Ice.LogFile.SizeMax", false, "0", false),
    new Property("Ice.LogStdErr.Convert", false, "1", false),
    new Property("Ice.MessageSizeMax", false, "1024", false),
    new Property("Ice.Nohup", false, "1", false),
    new Property("Ice.Override.CloseTimeout", false, "", false),
    new Property("Ice.Override.Compress", false, "", false),
    new Property("Ice.Override.ConnectTimeout", false, "", false),
    new Property("Ice.Override.Timeout", false, "", false),
    new Property("Ice.Override.Secure", false, "", false),
    new Property(/^Ice\.Package\../, true, "", false),
    new Property(/^Ice\.Plugin\../, true, "", false),
    new Property("Ice.PluginLoadOrder", false, "", false),
    new Property("Ice.PreferIPv6Address", false, "0", false),
    new Property("Ice.PreloadAssemblies", false, "0", false),
    new Property("Ice.PrintAdapterReady", false, "", false),
    new Property("Ice.PrintProcessId", false, "", false),
    new Property("Ice.PrintStackTraces", false, "0", false),
    new Property("Ice.ProgramName", false, "", false),
    new Property("Ice.RetryIntervals", false, "0", false),
    new Property("Ice.ServerIdleTime", false, "0", false),
    new Property("Ice.SOCKSProxyHost", false, "", false),
    new Property("Ice.SOCKSProxyPort", false, "1080", false),
    new Property("Ice.StdErr", false, "", false),
    new Property("Ice.StdOut", false, "", false),
    new Property("Ice.SyslogFacility", false, "LOG_USER", false),
    new Property("Ice.ThreadPool.Client.Size", false, "1", false),
    new Property("Ice.ThreadPool.Client.SizeMax", false, "", false),
    new Property("Ice.ThreadPool.Client.SizeWarn", false, "0", false),
    new Property("Ice.ThreadPool.Client.StackSize", false, "0", false),
    new Property("Ice.ThreadPool.Client.Serialize", false, "0", false),
    new Property("Ice.ThreadPool.Client.ThreadIdleTime", false, "60", false),
    new Property("Ice.ThreadPool.Client.ThreadPriority", false, "", false),
    new Property("Ice.ThreadPool.Server.Size", false, "1", false),
    new Property("Ice.ThreadPool.Server.SizeMax", false, "", false),
    new Property("Ice.ThreadPool.Server.SizeWarn", false, "0", false),
    new Property("Ice.ThreadPool.Server.StackSize", false, "0", false),
    new Property("Ice.ThreadPool.Server.Serialize", false, "0", false),
    new Property("Ice.ThreadPool.Server.ThreadIdleTime", false, "60", false),
    new Property("Ice.ThreadPool.Server.ThreadPriority", false, "", false),
    new Property("Ice.ThreadPriority", false, "", false),
    new Property("Ice.ToStringMode", false, "Unicode", false),
    new Property("Ice.Trace.Admin.Properties", false, "0", false),
    new Property("Ice.Trace.Admin.Logger", false, "0", false),
    new Property("Ice.Trace.Locator", false, "0", false),
    new Property("Ice.Trace.Network", false, "0", false),
    new Property("Ice.Trace.Protocol", false, "0", false),
    new Property("Ice.Trace.Retry", false, "0", false),
    new Property("Ice.Trace.Slicing", false, "0", false),
    new Property("Ice.Trace.ThreadPool", false, "0", false),
    new Property("Ice.UDP.RcvSize", false, "", false),
    new Property("Ice.UDP.SndSize", false, "", false),
    new Property("Ice.TCP.Backlog", false, "", false),
    new Property("Ice.TCP.RcvSize", false, "", false),
    new Property("Ice.TCP.SndSize", false, "", false),
    new Property("Ice.UseApplicationClassLoader", false, "", false),
    new Property("Ice.UseOSLog", false, "0", false),
    new Property("Ice.UseSyslog", false, "0", false),
    new Property("Ice.UseSystemdJournal", false, "0", false),
    new Property("Ice.Warn.AMICallback", false, "1", false),
    new Property("Ice.Warn.Connections", false, "0", false),
    new Property("Ice.Warn.Datagrams", false, "0", false),
    new Property("Ice.Warn.Dispatch", false, "1", false),
    new Property("Ice.Warn.Endpoints", false, "1", false),
    new Property("Ice.Warn.UnknownProperties", false, "1", false),
    new Property("Ice.Warn.UnusedProperties", false, "0", false),
    new Property("Ice.CacheMessageBuffers", false, "2", false),
    new Property("Ice.ThreadInterruptSafe", false, "", false),
];

PropertyNames.validProps = new Map();
PropertyNames.validProps.set("Ice", IceProps);
