//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice
import TestCommon

class Server: TestHelperI {
    override public func run(args: [String]) throws {
        let properties = try createTestProperties(args)
        properties.setProperty(key: "Ice.Warn.Dispatch", value: "0")
        var initData = InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceSlicingExceptions", "IceSlicingExceptionsServer"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }
        communicator.getProperties().setProperty(key: "TestAdapter.Endpoints",
                                                 value: "\(getTestEndpoint(num: 0)) -t 2000")
        let adapter = try communicator.createObjectAdapter("TestAdapter")
        try adapter.add(servant: TestIntfDisp(TestI(self)), id: Ice.stringToIdentity("Test"))
        try adapter.activate()
        serverReady()
        communicator.waitForShutdown()
    }
}
