//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_EXCEPTION_H
#define ICE_EXCEPTION_H

#include "Config.h"
#include "ValueF.h"

#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace Ice
{
    /**
     * Abstract base class for all Ice exceptions. It has only two derives classed: LocalException and UserException.
     * \headerfile Ice/Ice.h
     */
    class ICE_API Exception : public std::exception
    {
    public:
        /**
         * Constructs an exception with a default message.
         * @param file The file where this exception is constructed. This C string is not copied.
         * @param line The line where this exception is constructed.
         */
        Exception(const char* file, int line) noexcept;

        /**
         * Constructs an exception.
         * @param file The file where this exception is constructed. This C string is not copied.
         * @param line The line where this exception is constructed.
         * @param message The error message adopted by this exception and returned by what().
         */
        Exception(const char* file, int line, std::string message);

        /**
         * Copy constructor.
         * @param other The exception to copy.
         */
        Exception(const Exception& other) noexcept = default;

        /**
         * Assignment operator.
         * @param rhs The exception to assign.
         */
        Exception& operator=(const Exception& rhs) noexcept = default;

        /**
         * Returns the error message of this exception.
         * @return The error message.
         */
        const char* what() const noexcept override;

        /**
         * Returns the type ID of this exception. This corresponds to the Slice
         * type ID for Slice-defined exceptions, and to a similar fully scoped name
         * for other exceptions. For example "::Ice::CommunicatorDestroyedException".
         * @return The type ID of this exception
         */
        virtual const char* ice_id() const noexcept = 0;

        /**
         * Outputs a description of this exception to a stream.
         * @param os The output stream.
         */
        virtual void ice_print(std::ostream& os) const;

        /**
         * Returns the name of the file where this exception was constructed.
         * @return The file name.
         */
        const char* ice_file() const noexcept;

        /**
         * Returns the line number where this exception was constructed.
         * @return The line number.
         */
        int ice_line() const noexcept;

        /**
         * Returns the stack trace at the point this exception was constructed
         * @return The stack trace as a string.
         */
        std::string ice_stackTrace() const;

    private:
        const char* _file;                                // can be nullptr
        int _line;                                        // not used when _file is nullptr
        std::shared_ptr<std::string> _whatString;         // shared storage for custom _what message.
        const char* _what;                                // can be nullptr
        std::shared_ptr<std::vector<void*>> _stackFrames; // shared storage for stack frames.
    };

    ICE_API std::ostream& operator<<(std::ostream&, const Exception&);
}

#endif
