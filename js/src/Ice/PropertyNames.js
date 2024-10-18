
// Copyright (c) ZeroC, Inc.

// Generated by makeprops.py from PropertyNames.xml

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

import { Property, PropertyArray } from "./Property.js";
export const PropertyNames = {};

PropertyNames.ProxyProps = new PropertyArray("Proxy", false, [
    new Property("EndpointSelection", false, "", false, null),
    new Property("ConnectionCached", false, "", false, null),
    new Property("PreferSecure", false, "", false, null),
    new Property("LocatorCacheTimeout", false, "", false, null),
    new Property("InvocationTimeout", false, "", false, null),
    new Property("Locator", false, "", false, null),
    new Property("Router", false, "", false, null),
    new Property(/^Context\../, true, "", false, null)
]);

PropertyNames.ConnectionProps = new PropertyArray("Connection", true, [
    new Property("CloseTimeout", false, "10", false, null),
    new Property("ConnectTimeout", false, "10", false, null),
    new Property("EnableIdleCheck", false, "1", false, null),
    new Property("IdleTimeout", false, "60", false, null),
    new Property("InactivityTimeout", false, "300", false, null)
]);

PropertyNames.ThreadPoolProps = new PropertyArray("ThreadPool", true, [
]);

PropertyNames.ObjectAdapterProps = new PropertyArray("ObjectAdapter", true, [
    new Property("PublishedEndpoints", false, "", false, null),
    new Property("Router", false, "", false, PropertyNames.ProxyProps),
    new Property("ProxyOptions", false, "", false, null),
    new Property("MessageSizeMax", false, "", false, null)
]);

PropertyNames.LMDBProps = new PropertyArray("LMDB", true, [
]);

PropertyNames.IceProps = new PropertyArray("Ice", false, [
    new Property("BackgroundLocatorCacheUpdates", false, "0", false, null),
    new Property("BatchAutoFlush", false, "", true, null),
    new Property("BatchAutoFlushSize", false, "1024", false, null),
    new Property("ClassGraphDepthMax", false, "10", false, null),
    new Property("Connection.Client", false, "", false, PropertyNames.ConnectionProps),
    new Property("Default.EncodingVersion", false, "1.1", false, null),
    new Property("Default.EndpointSelection", false, "Random", false, null),
    new Property("Default.Host", false, "", false, null),
    new Property("Default.Locator", false, "", false, PropertyNames.ProxyProps),
    new Property("Default.LocatorCacheTimeout", false, "-1", false, null),
    new Property("Default.InvocationTimeout", false, "-1", false, null),
    new Property("Default.PreferSecure", false, "0", false, null),
    new Property("Default.Protocol", false, "tcp", false, null),
    new Property("Default.Router", false, "", false, PropertyNames.ProxyProps),
    new Property("Default.SlicedFormat", false, "0", false, null),
    new Property("Default.SourceAddress", false, "", false, null),
    new Property("ImplicitContext", false, "None", false, null),
    new Property("MessageSizeMax", false, "1024", false, null),
    new Property("Override.Secure", false, "", false, null),
    new Property("RetryIntervals", false, "0", false, null),
    new Property("ToStringMode", false, "Unicode", false, null),
    new Property("Trace.Locator", false, "0", false, null),
    new Property("Trace.Network", false, "0", false, null),
    new Property("Trace.Protocol", false, "0", false, null),
    new Property("Trace.Retry", false, "0", false, null),
    new Property("Trace.Slicing", false, "0", false, null),
    new Property("Warn.Connections", false, "0", false, null),
    new Property("Warn.Dispatch", false, "1", false, null),
    new Property("Warn.Endpoints", false, "1", false, null),
    new Property("Warn.UnusedProperties", false, "0", false, null)
]);

PropertyNames.IceMXProps = new PropertyArray("IceMX", false, [
]);

PropertyNames.IceDiscoveryProps = new PropertyArray("IceDiscovery", false, [
]);

PropertyNames.IceLocatorDiscoveryProps = new PropertyArray("IceLocatorDiscovery", false, [
]);

PropertyNames.IceBoxProps = new PropertyArray("IceBox", false, [
]);

PropertyNames.IceBoxAdminProps = new PropertyArray("IceBoxAdmin", false, [
]);

PropertyNames.IceBridgeProps = new PropertyArray("IceBridge", false, [
]);

PropertyNames.IceGridAdminProps = new PropertyArray("IceGridAdmin", false, [
]);

PropertyNames.IceGridProps = new PropertyArray("IceGrid", false, [
]);

PropertyNames.IceSSLProps = new PropertyArray("IceSSL", false, [
]);

PropertyNames.IceStormProps = new PropertyArray("IceStorm", false, [
]);

PropertyNames.IceStormAdminProps = new PropertyArray("IceStormAdmin", false, [
]);

PropertyNames.IceBTProps = new PropertyArray("IceBT", false, [
]);

PropertyNames.Glacier2Props = new PropertyArray("Glacier2", false, [
]);

PropertyNames.validProps = [
    PropertyNames.IceProps,
    PropertyNames.IceMXProps,
    PropertyNames.IceDiscoveryProps,
    PropertyNames.IceLocatorDiscoveryProps,
    PropertyNames.IceBoxProps,
    PropertyNames.IceBoxAdminProps,
    PropertyNames.IceBridgeProps,
    PropertyNames.IceGridAdminProps,
    PropertyNames.IceGridProps,
    PropertyNames.IceSSLProps,
    PropertyNames.IceStormProps,
    PropertyNames.IceStormAdminProps,
    PropertyNames.IceBTProps,
    PropertyNames.Glacier2Props,
];
