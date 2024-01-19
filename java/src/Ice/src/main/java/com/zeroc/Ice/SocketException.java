//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.8.50
//
// <auto-generated>
//
// Generated from file `LocalException.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * This exception indicates socket errors.
 **/
public class SocketException extends SyscallException
{
    public SocketException()
    {
        super();
    }

    public SocketException(Throwable cause)
    {
        super(cause);
    }

    public SocketException(int error)
    {
        super(error);
    }

    public SocketException(int error, Throwable cause)
    {
        super(error, cause);
    }

    public String ice_id()
    {
        return "::Ice::SocketException";
    }

    /** @hidden */
    public static final long serialVersionUID = -7634050967564791782L;
}
