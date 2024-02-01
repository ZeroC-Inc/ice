//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __Ice_ImplicitContext_h__
#define __Ice_ImplicitContext_h__

#include <IceUtil/PushDisableWarnings.h>
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/ValueF.h>
#include <Ice/Exception.h>
#include <Ice/LocalObject.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Comparable.h>
#include <Ice/Optional.h>
#include <Ice/ExceptionHelpers.h>
#include <Ice/LocalException.h>
#include <Ice/Current.h>
#include <IceUtil/UndefSysMacros.h>

#ifndef ICE_API
#   if defined(ICE_STATIC_LIBS)
#       define ICE_API /**/
#   elif defined(ICE_API_EXPORTS)
#       define ICE_API ICE_DECLSPEC_EXPORT
#   else
#       define ICE_API ICE_DECLSPEC_IMPORT
#   endif
#endif

namespace Ice
{

class ImplicitContext;

}

namespace Ice
{

/**
 * An interface to associate implict contexts with communicators. When you make a remote invocation without an
 * explicit context parameter, Ice uses the per-proxy context (if any) combined with the <code>ImplicitContext</code>
 * associated with the communicator.
 * Ice provides several implementations of <code>ImplicitContext</code>. The implementation used depends on the value
 * of the <code>Ice.ImplicitContext</code> property.
 * <dl>
 * <dt><code>None</code> (default)</dt>
 * <dd>No implicit context at all.</dd>
 * <dt><code>PerThread</code></dt>
 * <dd>The implementation maintains a context per thread.</dd>
 * <dt><code>Shared</code></dt>
 * <dd>The implementation maintains a single context shared by all threads.</dd>
 * </dl>
 * <code>ImplicitContext</code> also provides a number of operations to create, update or retrieve an entry in the
 * underlying context without first retrieving a copy of the entire context. These operations correspond to a subset
 * of the <code>java.util.Map</code> methods, with <code>java.lang.Object</code> replaced by <code>string</code> and
 * null replaced by the empty-string.
 * \headerfile Ice/Ice.h
 */
class ICE_CLASS(ICE_API) ImplicitContext
{
public:

    ICE_MEMBER(ICE_API) virtual ~ImplicitContext();

    /**
     * Get a copy of the underlying context.
     * @return A copy of the underlying context.
     */
    virtual ::Ice::Context getContext() const = 0;

    /**
     * Set the underlying context.
     * @param newContext The new context.
     */
    virtual void setContext(const Context& newContext) = 0;

    /**
     * Check if this key has an associated value in the underlying context.
     * @param key The key.
     * @return True if the key has an associated value, False otherwise.
     */
    virtual bool containsKey(const ::std::string& key) const = 0;

    /**
     * Get the value associated with the given key in the underlying context. Returns an empty string if no value is
     * associated with the key. {@link #containsKey} allows you to distinguish between an empty-string value and no
     * value at all.
     * @param key The key.
     * @return The value associated with the key.
     */
    virtual ::std::string get(const ::std::string& key) const = 0;

    /**
     * Create or update a key/value entry in the underlying context.
     * @param key The key.
     * @param value The value.
     * @return The previous value associated with the key, if any.
     */
    virtual ::std::string put(const ::std::string& key, const ::std::string& value) = 0;

    /**
     * Remove the entry for the given key in the underlying context.
     * @param key The key.
     * @return The value associated with the key, if any.
     */
    virtual ::std::string remove(const ::std::string& key) = 0;
};

}

/// \cond STREAM
namespace Ice
{

}
/// \endcond

/// \cond INTERNAL
namespace Ice
{

using ImplicitContextPtr = ::std::shared_ptr<ImplicitContext>;

}
/// \endcond

#include <IceUtil/PopDisableWarnings.h>
#endif
