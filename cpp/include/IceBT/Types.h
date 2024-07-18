//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_BT_TYPES_H
#define ICE_BT_TYPES_H

#include "Ice/Config.h"
#include "Ice/LocalExceptions.h"

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
    class ICEBT_API BluetoothException final : public Ice::LocalException
    {
    public:
        using Ice::LocalException::LocalException;

        const char* ice_id() const noexcept final;
    };
}

#endif
