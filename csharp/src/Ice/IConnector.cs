// Copyright (c) ZeroC, Inc. All rights reserved.

namespace ZeroC.Ice
{
    /// <summary>A connector holds all the information needed to establish a connection to a remote peer. It creates
    /// a connection that can be used to exchange data with the remote peer once the connection is established.
    /// </summary>
    public interface IConnector
    {
        /// <summary>Creates a connection. The connection may not be fully connected until its
        /// <see cref="Connection.InitializeAsync"/> method is called.</summary>
        /// <return>The connection.</return>
        Connection Connect(string connectionId);
    }
}
