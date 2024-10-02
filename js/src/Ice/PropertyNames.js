// Copyright (c) ZeroC, Inc.

// Generated by makeprops.py from PropertyNames.xml

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

import { Property } from "./Property.js";
export const PropertyNames = {};
const IceProps =
[
    new Property("Ice.BackgroundLocatorCacheUpdates", false, "0", false),
    new Property("Ice.BatchAutoFlush", false, "", true),
    new Property("Ice.BatchAutoFlushSize", false, "1024", false),
    new Property("Ice.ClassGraphDepthMax", false, "10", false),
    new Property("Ice.Connection.Client.CloseTimeout", false, "10", false),
    new Property("Ice.Connection.Client.ConnectTimeout", false, "10", false),
    new Property("Ice.Connection.Client.EnableIdleCheck", false, "1", false),
    new Property("Ice.Connection.Client.IdleTimeout", false, "60", false),
    new Property("Ice.Connection.Client.InactivityTimeout", false, "300", false),
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
    new Property("Ice.ImplicitContext", false, "None", false),
    new Property("Ice.MessageSizeMax", false, "1024", false),
    new Property("Ice.Override.Secure", false, "", false),
    new Property("Ice.RetryIntervals", false, "0", false),
    new Property("Ice.ToStringMode", false, "Unicode", false),
    new Property("Ice.Trace.Locator", false, "0", false),
    new Property("Ice.Trace.Network", false, "0", false),
    new Property("Ice.Trace.Protocol", false, "0", false),
    new Property("Ice.Trace.Retry", false, "0", false),
    new Property("Ice.Trace.Slicing", false, "0", false),
    new Property("Ice.Warn.Connections", false, "0", false),
    new Property("Ice.Warn.Dispatch", false, "1", false),
    new Property("Ice.Warn.Endpoints", false, "1", false),
    new Property("Ice.Warn.UnusedProperties", false, "0", false),
];

PropertyNames.validProps = new Map();
PropertyNames.validProps.set("Ice", IceProps);
