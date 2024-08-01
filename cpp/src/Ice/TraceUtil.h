//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_TRACE_UTIL_H
#define ICE_TRACE_UTIL_H

#include "Ice/InstanceF.h"
#include "Ice/Logger.h"
#include "TraceLevelsF.h"

namespace Ice
{
    class OutputStream;
    class InputStream;
}

namespace IceInternal
{
    class Instance;

    void traceSend(const Ice::OutputStream&, const InstancePtr& instance, const Ice::LoggerPtr&, const TraceLevelsPtr&);
    void traceRecv(const Ice::InputStream&, const Ice::LoggerPtr&, const TraceLevelsPtr&);
    void trace(const char*, const Ice::InputStream&, const Ice::LoggerPtr&, const TraceLevelsPtr&);
    void traceSlicing(const char*, std::string_view, const char*, const Ice::LoggerPtr&);
}

#endif
