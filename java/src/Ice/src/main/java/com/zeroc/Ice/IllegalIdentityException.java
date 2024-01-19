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
 * This exception is raised if an illegal identity is encountered.
 **/
public class IllegalIdentityException extends LocalException
{
    public IllegalIdentityException()
    {
        this.id = new Identity();
    }

    public IllegalIdentityException(Throwable cause)
    {
        super(cause);
        this.id = new Identity();
    }

    public IllegalIdentityException(Identity id)
    {
        this.id = id;
    }

    public IllegalIdentityException(Identity id, Throwable cause)
    {
        super(cause);
        this.id = id;
    }

    public String ice_id()
    {
        return "::Ice::IllegalIdentityException";
    }

    /**
     * The illegal identity.
     **/
    public Identity id;

    /** @hidden */
    public static final long serialVersionUID = -2349418144702375351L;
}
