//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef __IceBT_Types_h__
#define __IceBT_Types_h__

#include <IceUtil/PushDisableWarnings.h>
#include <Ice/ProxyF.h>
#include <Ice/ObjectF.h>
#include <Ice/ValueF.h>
#include <Ice/Exception.h>
#include <Ice/StreamHelpers.h>
#include <Ice/Comparable.h>
#include <optional>
#include <Ice/ExceptionHelpers.h>
#include <IceUtil/UndefSysMacros.h>

#ifndef ICEBT_API
#    if defined(ICE_STATIC_LIBS)
#        define ICEBT_API /**/
#    elif defined(ICEBT_API_EXPORTS)
#        define ICEBT_API ICE_DECLSPEC_EXPORT
#    else
#        define ICEBT_API ICE_DECLSPEC_IMPORT
#    endif
#endif

namespace IceBT
{
    /**
     * Indicates a failure in the Bluetooth plug-in.
     * \headerfile IceBT/IceBT.h
     */
    class ICEBT_API BluetoothException : public Ice::LocalException
    {
    public:
        using LocalException::LocalException;

        /**
         * One-shot constructor to initialize all data members.
         * The file and line number are required for all local exceptions.
         * @param file The file name in which the exception was raised, typically __FILE__.
         * @param line The line number at which the exception was raised, typically __LINE__.
         * @param reason Provides more information about the failure.
         */
        BluetoothException(const char* file, int line, std::string reason) noexcept
            : LocalException(file, line),
              reason(std::move(reason))
        {
        }

        /**
         * Obtains a tuple containing all of the exception's data members.
         * @return The data members in a tuple.
         */
        std::tuple<const ::std::string&> ice_tuple() const noexcept { return std::tie(reason); }

        /**
         * Obtains the Slice type ID of this exception.
         * @return The fully-scoped type ID.
         */
        static ::std::string_view ice_staticId() noexcept;
        /**
         * Prints this exception to the given stream.
         * @param stream The target stream.
         */
        void ice_print(std::ostream& stream) const override;

        std::string ice_id() const override;

        void ice_throw() const override;

        /**
         * Provides more information about the failure.
         */
        ::std::string reason;
    };
}

#include <IceUtil/PopDisableWarnings.h>
#endif
