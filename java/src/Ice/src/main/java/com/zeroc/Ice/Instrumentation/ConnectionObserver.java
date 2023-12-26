//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Instrumentation.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice.Instrumentation;

/**
 * The connection observer interface to instrument Ice connections.
 **/
public interface ConnectionObserver extends Observer
{
    /**
     * Notification of sent bytes over the connection.
     * @param num The number of bytes sent.
     **/
    void sentBytes(int num);

    /**
     * Notification of received bytes over the connection.
     * @param num The number of bytes received.
     **/
    void receivedBytes(int num);
}
