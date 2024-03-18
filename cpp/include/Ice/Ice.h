//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_ICE_H
#define ICE_ICE_H

#include "Config.h"

#include "Comparable.h"
#include "InputStream.h"
#include "MarshaledResult.h"
#include "Object.h"
#include "OutputStream.h"
#include "Proxy.h"
#include "UserException.h"
#include "Value.h"

#ifndef ICE_BUILDING_GENERATED_CODE

// We don't need to see the following headers when building the generated code.

#    include "Communicator.h"
#    include "Connection.h"
#    include "IconvStringConverter.h"
#    include "ImplicitContext.h"
#    include "Initialize.h"
#    include "Instrumentation.h"
#    include "LocalException.h"
#    include "Logger.h"
#    include "LoggerUtil.h"
#    include "NativePropertiesAdmin.h"
#    include "ObjectAdapter.h"
#    include "Plugin.h"
#    include "Properties.h"
#    include "ProxyFunctions.h"
#    include "RegisterPlugins.h"
#    include "ServantLocator.h"
#    include "SlicedData.h"
#    include "StringConverter.h"
#    include "UUID.h"

// Generated header files:
#    include "Ice/EndpointTypes.h"
#    include "Ice/Locator.h"
#    include "Ice/Metrics.h"
#    include "Ice/Process.h"
#    include "Ice/PropertiesAdmin.h"
#    include "Ice/RemoteLogger.h"
#    include "Ice/Router.h"

#    if !defined(__APPLE__) || TARGET_OS_IPHONE == 0
#        include "CtrlCHandler.h"
#        include "Service.h"
#    endif
#endif

#endif
