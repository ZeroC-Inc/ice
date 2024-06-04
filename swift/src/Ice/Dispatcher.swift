// Copyright (c) ZeroC, Inc.

import PromiseKit

/// A dispatcher accepts incoming requests and returns outgoing responses.
public protocol Dispatcher {
    /// Dispatches an incoming request and returns the corresponding outgoing response.
    /// - Parameter request: The incoming request.
    /// - Returns: The outgoing response, wrapped in a Promise.
    func dispatch(_ request: IncomingRequest) -> Promise<OutgoingResponse>
}
