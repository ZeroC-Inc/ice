// Copyright (c) ZeroC, Inc.

import Foundation

/// Base protocol for dynamic asynchronous dispatch servants.
public protocol BlobjectAsync {
    /// Dispatch an incoming request.
    ///
    /// - parameter inEncaps: `Data` - The encoded in-parameters for the operation.
    ///
    /// - parameter current: `Ice.Current` - The Current object to pass to the operation.
    ///
    /// - returns: `(ok: Bool, outParams: Data)` - The result of the operation.
    ///
    ///   - ok: `Bool` - True if the operation completed successfully, false if
    ///     the operation raised a user exception (in this case, outParams
    ///     contains the encoded user exception). If the operation raises an
    ///     Ice run-time exception, it must throw it directly.
    ///
    ///   - outParams: `Data` - The encoded out-parameters and return value
    ///     for the operation. The return value follows any out-parameters.
    func ice_invokeAsync(inEncaps: Data, current: Current) async throws -> (ok: Bool, outParams: Data)
}

/// Request dispatcher for BlobjectAsync servants.
public struct BlobjectAsyncDisp: Dispatcher {
    public let servant: BlobjectAsync

    public init(_ servant: BlobjectAsync) {
        self.servant = servant
    }

    public func dispatch(_ request: IncomingRequest) async throws -> OutgoingResponse {
        let (inEncaps, _) = try request.inputStream.readEncapsulation()
        let result = try await servant.ice_invokeAsync(inEncaps: inEncaps, current: request.current)
        return request.current.makeOutgoingResponse(ok: result.ok, encapsulation: result.outParams)
    }
}
