// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_IMPLICIT_CONTEXT_ICE
#define ICE_IMPLICIT_CONTEXT_ICE

#include <Ice/LocalException.ice>
#include <Ice/Current.ice>

module Ice
{

/**
 *
 * An interface to associate implict contexts with communicators.
 *
 * When you make a remote invocation without an explicit [Context] parameter,
 * Ice uses the per-proxy [Context] (if any) combined with the <tt>ImplicitContext</tt>
 * associated with the communicator.</p>
 * <p>Ice provides several implementations of <tt>ImplicitContext</tt>. The implementation
 * used depends on the value of the <tt>Ice.ImplicitContext</tt> property.
 * <dl>
 * <dt><tt>None</tt> (default)</dt>
 * <dd>No implicit context at all.</dd>
 * <dt><tt>PerThread</tt></dt>
 * <dd>The implementation maintains a [Context] per thread.</dd>
 * <dt><tt>Shared</tt></dt>
 * <dd>The implementation maintains a single [Context] shared 
 * by all threads, and serializes access to this [Context].</dd>
 * <dt><tt>SharedWithoutLocking</tt></dt>
 * <dd>
 * The implementation maintains a single 
 * [Context] shared by all threads, and does not serialize access to this [Context].
 * </dd>
 * </dl><p>
 *
 **/

local interface ImplicitContext
{
    /**
     * Get the underlying context. The operation returns a null proxy if no implicit
     * context is established on the communicator (that is, if <tt>Ice.ImplicitContext</tt>
     * is set to <tt>None</tt>).
     *
     * @return The underlying context.
     *
     **/
    ["cpp:const"] Context getContext();
    
    /**
     * Set the underlying context.
     *
     * @param newContext The new context.
     * 
     **/
    void setContext(Context newContext);

    /**
     * Get the value associated with the given key in the underlying context.
     * Throws [NotSetException] when no value is associated with the given key.
     *
     * @param key The key.
     *
     * @return The value associated with the key.
     *
     **/
    ["cpp:const"] string get(string key);

    /**
     * Get the value associated with the given key in the underlying context.
     *
     * @param key The key.
     *
     * @param defaultValue The default value
     *
     * @return The value associated with the key, or defaultValue when no
     * value is associated with the given key.
     *
     **/
    ["cpp:const"] string getWithDefault(string key, string defaultValue);

    /**
     * Set the value associated with the given key in the underlying context.
     *
     * @param key The key.
     *
     * @param value The value.
     *
     **/
    void set(string key, string value);

    /**
     * Remove the value associated with the given key in the underlying context.
     * Throws [NotSetException] when no value is associated with the given key.
     *
     * @param key The key.
     *
     **/
    void remove(string key);
};
};

#endif
