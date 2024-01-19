//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `ConnectionInfo.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

export namespace IceSSL
{
    /**
     * Provides access to the connection details of an SSL connection
     */
    class ConnectionInfo extends Ice.ConnectionInfo
    {
        /**
         * One-shot constructor to initialize all data members.
         * @param underlying The information of the underyling transport or null if there's no underlying transport.
         * @param incoming Whether or not the connection is an incoming or outgoing connection.
         * @param adapterName The name of the adapter associated with the connection.
         * @param connectionId The connection id.
         * @param cipher The negotiated cipher suite.
         * @param certs The certificate chain.
         * @param verified The certificate chain verification status.
         */
        constructor(underlying?:Ice.ConnectionInfo, incoming?:boolean, adapterName?:string, connectionId?:string, cipher?:string, certs?:Ice.StringSeq, verified?:boolean);
        /**
         * The negotiated cipher suite.
         */
        cipher:string;
        /**
         * The certificate chain.
         */
        certs:Ice.StringSeq;
        /**
         * The certificate chain verification status.
         */
        verified:boolean;
    }
}
