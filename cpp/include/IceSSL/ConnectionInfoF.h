//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __IceSSL_ConnectionInfoF_h__
#define __IceSSL_ConnectionInfoF_h__

#include <IceUtil/PushDisableWarnings.h>
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/ValueF.h>
#include <Ice/Exception.h>
#include <Ice/LocalObject.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Comparable.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Optional.h>
#include <IceUtil/UndefSysMacros.h>

#ifndef ICESSL_API
#   if defined(ICE_STATIC_LIBS)
#       define ICESSL_API /**/
#   elif defined(ICESSL_API_EXPORTS)
#       define ICESSL_API ICE_DECLSPEC_EXPORT
#   else
#       define ICESSL_API ICE_DECLSPEC_IMPORT
#   endif
#endif

#ifdef ICE_CPP11_MAPPING // C++11 mapping

namespace IceSSL
{

class ConnectionInfo;

}

/// \cond STREAM
namespace Ice
{

}
/// \endcond

/// \cond INTERNAL
namespace IceSSL
{

using ConnectionInfoPtr = ::std::shared_ptr<ConnectionInfo>;

}
/// \endcond

#else // C++98 mapping

namespace IceSSL
{

class ConnectionInfo;
/// \cond INTERNAL
ICESSL_API ::Ice::LocalObject* upCast(ConnectionInfo*);
/// \endcond
typedef ::IceInternal::Handle< ConnectionInfo> ConnectionInfoPtr;

}

/// \cond STREAM
namespace Ice
{

}
/// \endcond

#endif

#include <IceUtil/PopDisableWarnings.h>
#endif
