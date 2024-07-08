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

/** This exception indicates a problem with compressing or uncompressing data. */
public class CompressionException extends ProtocolException {
    public CompressionException() {
        super();
    }

    public CompressionException(Throwable cause) {
        super(cause);
    }

    public CompressionException(String reason) {
        super(reason);
    }

    public CompressionException(String reason, Throwable cause) {
        super(reason, cause);
    }

    public String ice_id() {
        return "::Ice::CompressionException";
    }

    private static final long serialVersionUID = -3980762816174249071L;
}
