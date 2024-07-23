// Copyright (c) ZeroC, Inc.

import Ice
import PromiseKit
import TestCommon

public class Client: TestHelperI {
    override public func run(args: [String]) async throws {
        var initData = Ice.InitializationData()
        initData.properties = try createTestProperties(args)
        initData.classResolverPrefix = ["IceSlicingExceptions", "IceSlicingExceptionsClient"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }
        let testIntf = try await allTests(self)
        try testIntf.shutdown()
    }
}
