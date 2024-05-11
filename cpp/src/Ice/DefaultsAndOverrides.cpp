//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "DefaultsAndOverrides.h"
#include "Ice/LocalException.h"
#include "Ice/LoggerUtil.h"
#include "Ice/Properties.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

IceInternal::DefaultsAndOverrides::DefaultsAndOverrides(const PropertiesPtr& properties, const LoggerPtr& logger)
    : overrideTimeout(false),
      overrideTimeoutValue(-1),
      overrideCompress(false),
      overrideCompressValue(false),
      overrideSecure(false),
      overrideSecureValue(false)
{
    const_cast<string&>(defaultProtocol) = properties->getIceProperty("Ice.Default.Protocol");

    const_cast<string&>(defaultHost) = properties->getIceProperty("Ice.Default.Host");

    string value;

    value = properties->getIceProperty("Ice.Default.SourceAddress");
    if (!value.empty())
    {
        const_cast<Address&>(defaultSourceAddress) = getNumericAddress(value);
        if (!isAddressValid(defaultSourceAddress))
        {
            throw InitializationException(
                __FILE__,
                __LINE__,
                "invalid IP address set for Ice.Default.SourceAddress: `" + value + "'");
        }
    }

    value = properties->getIceProperty("Ice.Override.Timeout");
    if (!value.empty())
    {
        const_cast<bool&>(overrideTimeout) = true;
        const_cast<int32_t&>(overrideTimeoutValue) = properties->getIcePropertyAsInt("Ice.Override.Timeout");
        if (overrideTimeoutValue < 1 && overrideTimeoutValue != -1)
        {
            const_cast<int32_t&>(overrideTimeoutValue) = -1;
            Warning out(logger);
            out << "invalid value for Ice.Override.Timeout `" << properties->getIceProperty("Ice.Override.Timeout")
                << "': defaulting to -1";
        }
    }

    value = properties->getIceProperty("Ice.Override.Compress");
    if (!value.empty())
    {
        const_cast<bool&>(overrideCompress) = true;
        const_cast<bool&>(overrideCompressValue) = properties->getIcePropertyAsInt("Ice.Override.Compress") > 0;
    }

    value = properties->getIceProperty("Ice.Override.Secure");
    if (!value.empty())
    {
        const_cast<bool&>(overrideSecure) = true;
        const_cast<bool&>(overrideSecureValue) = properties->getIcePropertyAsInt("Ice.Override.Secure") > 0;
    }

    const_cast<bool&>(defaultCollocationOptimization) =
        properties->getIcePropertyAsInt("Ice.Default.CollocationOptimized") > 0;

    value = properties->getIceProperty("Ice.Default.EndpointSelection");
    if (value == "Random")
    {
        defaultEndpointSelection = EndpointSelectionType::Random;
    }
    else if (value == "Ordered")
    {
        defaultEndpointSelection = EndpointSelectionType::Ordered;
    }
    else
    {
        throw EndpointSelectionTypeParseException(
            __FILE__,
            __LINE__,
            "illegal value `" + value + "'; expected `Random' or `Ordered'");
    }

    const_cast<int&>(defaultTimeout) = properties->getIcePropertyAsInt("Ice.Default.Timeout");
    if (defaultTimeout < 1 && defaultTimeout != -1)
    {
        const_cast<int32_t&>(defaultTimeout) = 60000;
        Warning out(logger);
        out << "invalid value for Ice.Default.Timeout `" << properties->getIceProperty("Ice.Default.Timeout")
            << "': defaulting to 60000";
    }

    const_cast<int&>(defaultInvocationTimeout) = properties->getIcePropertyAsInt("Ice.Default.InvocationTimeout");
    if (defaultInvocationTimeout < 1 && defaultInvocationTimeout != -1 && defaultInvocationTimeout != -2)
    {
        const_cast<int32_t&>(defaultInvocationTimeout) = -1;
        Warning out(logger);
        out << "invalid value for Ice.Default.InvocationTimeout `"
            << properties->getIceProperty("Ice.Default.InvocationTimeout") << "': defaulting to -1";
    }

    const_cast<int&>(defaultLocatorCacheTimeout) = properties->getIcePropertyAsInt("Ice.Default.LocatorCacheTimeout");
    if (defaultLocatorCacheTimeout < -1)
    {
        const_cast<int32_t&>(defaultLocatorCacheTimeout) = -1;
        Warning out(logger);
        out << "invalid value for Ice.Default.LocatorCacheTimeout `"
            << properties->getIceProperty("Ice.Default.LocatorCacheTimeout") << "': defaulting to -1";
    }

    const_cast<bool&>(defaultPreferSecure) = properties->getIcePropertyAsInt("Ice.Default.PreferSecure") > 0;

    value = properties->getPropertyWithDefault("Ice.Default.EncodingVersion", encodingVersionToString(currentEncoding));
    defaultEncoding = stringToEncodingVersion(value);
    checkSupportedEncoding(defaultEncoding);

    bool slicedFormat = properties->getIcePropertyAsInt("Ice.Default.SlicedFormat") > 0;
    const_cast<FormatType&>(defaultFormat) = slicedFormat ? FormatType::SlicedFormat : FormatType::CompactFormat;
}
