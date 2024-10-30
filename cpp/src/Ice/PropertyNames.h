// Copyright (c) ZeroC, Inc.

// Generated by makeprops.py from PropertyNames.xml

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

#ifndef ICE_INTERNAL_PROPERTY_NAMES_H
#define ICE_INTERNAL_PROPERTY_NAMES_H

#include "Ice/Config.h"

#include <array>
#include <string>

namespace IceInternal
{
    struct PropertyArray;
    struct Property
    {
        const char* pattern;
        const char* defaultValue;
        const bool usesRegex;
        const bool deprecated;
        const PropertyArray* propertyArray;

    };

    struct PropertyArray
    {
        const char* name;
        const bool prefixOnly;
        const Property* properties;
        const int length;
        const bool isOptIn;
    };

    class PropertyNames
    {
    public:
        static const PropertyArray ProxyProps;
        static const PropertyArray ConnectionProps;
        static const PropertyArray ThreadPoolProps;
        static const PropertyArray ObjectAdapterProps;
        static const PropertyArray LMDBProps;
        static const PropertyArray IceProps;
        static const PropertyArray IceMXProps;
        static const PropertyArray IceDiscoveryProps;
        static const PropertyArray IceLocatorDiscoveryProps;
        static const PropertyArray IceBoxProps;
        static const PropertyArray IceBoxAdminProps;
        static const PropertyArray IceBridgeProps;
        static const PropertyArray IceGridAdminProps;
        static const PropertyArray IceGridProps;
        static const PropertyArray IceSSLProps;
        static const PropertyArray IceStormProps;
        static const PropertyArray IceStormAdminProps;
        static const PropertyArray IceBTProps;
        static const PropertyArray Glacier2Props;
        static const PropertyArray DataStormProps;

        static const std::array<PropertyArray, 15> validProps;
    };
}

#endif
