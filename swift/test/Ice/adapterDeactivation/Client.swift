// Copyright (c) ZeroC, Inc.

import Ice
import TestCommon

class Client: TestHelperI {
    override func run(args: [String]) async throws {
        let communicator = try initialize(args)
        defer {
            communicator.destroy()
        }

        try await allTests(self)

    }
}
