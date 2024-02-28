//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __Ice_Properties_h__
#define __Ice_Properties_h__

#include <IceUtil/PushDisableWarnings.h>
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/ValueF.h>
#include <Ice/Exception.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Comparable.h>
#include <Ice/Proxy.h>
#include <Ice/Object.h>
#include <Ice/Value.h>
#include <Ice/Incoming.h>
#include <Ice/FactoryTableInit.h>
#include <optional>
#include <Ice/PropertyDict.h>
#include <IceUtil/UndefSysMacros.h>

#ifndef ICE_API
#    if defined(ICE_STATIC_LIBS)
#        define ICE_API /**/
#    elif defined(ICE_API_EXPORTS)
#        define ICE_API ICE_DECLSPEC_EXPORT
#    else
#        define ICE_API ICE_DECLSPEC_IMPORT
#    endif
#endif

namespace Ice
{

    class Properties;

}

namespace Ice
{

    /**
     * A property set used to configure Ice and Ice applications. Properties are key/value pairs, with both keys and
     * values being strings. By convention, property keys should have the form
     * <em>application-name</em>[.<em>category</em>[.<em>sub-category</em>]].<em>name</em>.
     * \headerfile Ice/Ice.h
     */
    class ICE_CLASS(ICE_API) Properties
    {
    public:
        ICE_MEMBER(ICE_API) virtual ~Properties();

        /**
         * Get a property by key. If the property is not set, an empty string is returned.
         * @param key The property key.
         * @return The property value.
         * @see #setProperty
         */
        virtual ::std::string getProperty(const ::std::string& key) noexcept = 0;

        /**
         * Get a property by key. If the property is not set, the given default value is returned.
         * @param key The property key.
         * @param value The default value to use if the property does not exist.
         * @return The property value or the default value.
         * @see #setProperty
         */
        virtual ::std::string getPropertyWithDefault(const ::std::string& key, const ::std::string& value) noexcept = 0;

        /**
         * Get a property as an integer. If the property is not set, 0 is returned.
         * @param key The property key.
         * @return The property value interpreted as an integer.
         * @see #setProperty
         */
        virtual int getPropertyAsInt(const ::std::string& key) noexcept = 0;

        /**
         * Get a property as an integer. If the property is not set, the given default value is returned.
         * @param key The property key.
         * @param value The default value to use if the property does not exist.
         * @return The property value interpreted as an integer, or the default value.
         * @see #setProperty
         */
        virtual int getPropertyAsIntWithDefault(const ::std::string& key, int value) noexcept = 0;

        /**
         * Get a property as a list of strings. The strings must be separated by whitespace or comma. If the property is
         * not set, an empty list is returned. The strings in the list can contain whitespace and commas if they are
         * enclosed in single or double quotes. If quotes are mismatched, an empty list is returned. Within single
         * quotes or double quotes, you can escape the quote in question with a backslash, e.g. O'Reilly can be written
         * as O'Reilly, "O'Reilly" or 'O\'Reilly'.
         * @param key The property key.
         * @return The property value interpreted as a list of strings.
         * @see #setProperty
         */
        virtual ::Ice::StringSeq getPropertyAsList(const ::std::string& key) noexcept = 0;

        /**
         * Get a property as a list of strings.  The strings must be separated by whitespace or comma. If the property
         * is not set, the default list is returned. The strings in the list can contain whitespace and commas if they
         * are enclosed in single or double quotes. If quotes are mismatched, the default list is returned. Within
         * single quotes or double quotes, you can escape the quote in question with a backslash, e.g. O'Reilly can be
         * written as O'Reilly, "O'Reilly" or 'O\'Reilly'.
         * @param key The property key.
         * @param value The default value to use if the property is not set.
         * @return The property value interpreted as list of strings, or the default value.
         * @see #setProperty
         */
        virtual ::Ice::StringSeq
        getPropertyAsListWithDefault(const ::std::string& key, const StringSeq& value) noexcept = 0;

        /**
         * Get all properties whose keys begins with <em>prefix</em>. If <em>prefix</em> is an empty string, then all
         * properties are returned.
         * @param prefix The prefix to search for (empty string if none).
         * @return The matching property set.
         */
        virtual ::Ice::PropertyDict getPropertiesForPrefix(const ::std::string& prefix) noexcept = 0;

        /**
         * Set a property. To unset a property, set it to the empty string.
         * @param key The property key.
         * @param value The property value.
         * @see #getProperty
         */
        virtual void setProperty(const ::std::string& key, const ::std::string& value) = 0;

        /**
         * Get a sequence of command-line options that is equivalent to this property set. Each element of the returned
         * sequence is a command-line option of the form <code>--<em>key</em>=<em>value</em></code>.
         * @return The command line options for this property set.
         */
        virtual ::Ice::StringSeq getCommandLineOptions() noexcept = 0;

        /**
         * Convert a sequence of command-line options into properties. All options that begin with
         * <code>--<em>prefix</em>.</code> are converted into properties. If the prefix is empty, all options that begin
         * with <code>--</code> are converted to properties.
         * @param prefix The property prefix, or an empty string to convert all options starting with <code>--</code>.
         * @param options The command-line options.
         * @return The command-line options that do not start with the specified prefix, in their original order.
         */
        virtual ::Ice::StringSeq parseCommandLineOptions(const ::std::string& prefix, const StringSeq& options) = 0;

        /**
         * Convert a sequence of command-line options into properties. All options that begin with one of the following
         * prefixes are converted into properties: <code>--Ice</code>, <code>--IceBox</code>, <code>--IceGrid</code>,
         * <code>--IcePatch2</code>, <code>--IceSSL</code>, <code>--IceStorm</code>, <code>--Freeze</code>, and
         * <code>--Glacier2</code>.
         * @param options The command-line options.
         * @return The command-line options that do not start with one of the listed prefixes, in their original order.
         */
        virtual ::Ice::StringSeq parseIceCommandLineOptions(const StringSeq& options) = 0;

        /**
         * Load properties from a file.
         * @param file The property file.
         */
        virtual void load(const ::std::string& file) = 0;

        /**
         * Create a copy of this property set.
         * @return A copy of this property set.
         */
        virtual ::std::shared_ptr<::Ice::Properties> clone() noexcept = 0;
    };

}
/// \endcond

/// \cond INTERNAL
namespace Ice
{

    using PropertiesPtr = ::std::shared_ptr<Properties>;

}
/// \endcond

#include <IceUtil/PopDisableWarnings.h>
#endif
