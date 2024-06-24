//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "IceBT/Types.h"

using namespace std;

const char*
IceBT::BluetoothException::ice_id() const
{
    return ice_staticId();
}

const char*
IceBT::BluetoothException::ice_staticId() noexcept
{
    return "::IceBT::BluetoothException";
}
