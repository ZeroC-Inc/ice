//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
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
 * This exception is raised when a string conversion to or from UTF-8 fails during marshaling or unmarshaling.
 **/
public class StringConversionException extends MarshalException
{
    public StringConversionException() { super(); }

    public StringConversionException(Throwable cause) { super(cause); }

    public StringConversionException(String reason) { super(reason); }

    public StringConversionException(String reason, Throwable cause) { super(reason, cause); }

    public String ice_id() { return "::Ice::StringConversionException"; }

    /** @hidden */
    public static final long serialVersionUID = -7504009091360618574L;
}
