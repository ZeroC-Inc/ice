//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __Ice_Logger_h__
#define __Ice_Logger_h__

#include "ProxyF.h"
#include "ValueF.h"
#include "Exception.h"
#include "Comparable.h"
#include "IceUtil/UndefSysMacros.h"

namespace Ice
{
    class Logger;
}

namespace Ice
{
    /**
     * The Ice message logger. Applications can provide their own logger by implementing this interface and installing
     * it in a communicator. \headerfile Ice/Ice.h
     */
    class ICE_CLASS(ICE_API) Logger
    {
    public:
        ICE_MEMBER(ICE_API) virtual ~Logger();

        /**
         * Print a message. The message is printed literally, without any decorations such as executable name or time
         * stamp.
         * @param message The message to log.
         */
        virtual void print(const ::std::string& message) = 0;

        /**
         * Log a trace message.
         * @param category The trace category.
         * @param message The trace message to log.
         */
        virtual void trace(const ::std::string& category, const ::std::string& message) = 0;

        /**
         * Log a warning message.
         * @param message The warning message to log.
         * @see #error
         */
        virtual void warning(const ::std::string& message) = 0;

        /**
         * Log an error message.
         * @param message The error message to log.
         * @see #warning
         */
        virtual void error(const ::std::string& message) = 0;

        /**
         * Returns this logger's prefix.
         * @return The prefix.
         */
        virtual ::std::string getPrefix() = 0;

        /**
         * Returns a clone of the logger with a new prefix.
         * @param prefix The new prefix for the logger.
         * @return A logger instance.
         */
        virtual ::std::shared_ptr<::Ice::Logger> cloneWithPrefix(const ::std::string& prefix) = 0;
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
    using LoggerPtr = ::std::shared_ptr<Logger>;
}
/// \endcond

#endif
