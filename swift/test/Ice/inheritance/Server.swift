//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice
import TestCommon
import Dispatch

public class TestFactoryI: TestFactory {
    public class func create() -> TestHelper {
        return Server()
    }
}

class Server: TestHelperI {
    public override func run(args: [String]) throws {

        let (communicator, _) = try self.initialize(args: args)
        defer {
            communicator.destroy()
        }

        communicator.getProperties().setProperty(
            key: "TestAdapter.Endpoints",
            value: "\(getTestEndpoint(num: 0)):\(getTestEndpoint(num: 0, prot: "udp"))")
        let adapter = try communicator.createObjectAdapter("TestAdapter")
        _ = try adapter.add(servant: InitialI(adapter), id: Ice.stringToIdentity("initial"))
        try adapter.activate()
        serverReady()
        communicator.waitForShutdown()
    }
}
