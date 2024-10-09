
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
    new Property("CollocationOptimized", false, "", false, null),
    new Property(/^Context\../, true, "", false, null)
]);

PropertyNames.ConnectionProps = new PropertyArray("Connection", true, [
    new Property("CloseTimeout", false, "10", false, null),
    new Property("ConnectTimeout", false, "10", false, null),
    new Property("EnableIdleCheck", false, "1", false, null),
    new Property("IdleTimeout", false, "60", false, null),
    new Property("InactivityTimeout", false, "300", false, null)
]);

PropertyNames.ObjectAdapterProps = new PropertyArray("ObjectAdapter", true, [
    new Property("Endpoints", false, "", false, null),
    new Property("PublishedEndpoints", false, "", false, null),
    new Property("Router", false, "", false, PropertyNames.ProxyProps),
    new Property("ProxyOptions", false, "", false, null),
    new Property("MessageSizeMax", false, "", false, null)
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
    new Property("Warn.UnknownProperties", false, "1", false, null),
    new Property("Warn.UnusedProperties", false, "0", false, null)
]);

PropertyNames.validProps = new Map([
    ["Ice", PropertyNames.IceProps],
    ["IceMX", null],
    ["IceDiscovery", null],
    ["IceLocatorDiscovery", null],
    ["IceBox", null],
    ["IceBoxAdmin", null],
    ["IceBridge", null],
    ["IceGridAdmin", null],
    ["IceGrid", null],
    ["IceSSL", null],
    ["IceStorm", null],
    ["IceStormAdmin", null],
    ["IceBT", null],
    ["Glacier2", null]
])
