//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Ice
import TestCommon

class ServerAMD: TestHelperI {
    override public func run(args: [String]) throws {
        let properties = try createTestProperties(args)
        properties.setProperty(key: "Ice.AcceptClassCycles", value: "1")
        var initData = Ice.InitializationData()
        initData.properties = properties
        initData.classResolverPrefix = ["IceOptionalAMD"]
        let communicator = try initialize(initData)
        defer {
            communicator.destroy()
        }

        communicator.getProperties().setProperty(key: "TestAdapter.Endpoints", value: getTestEndpoint(num: 0))
        let adapter = try communicator.createObjectAdapter("TestAdapter")
        try adapter.add(servant: InitialDisp(InitialI()), id: Ice.stringToIdentity("initial"))
        try adapter.activate()
        serverReady()
        communicator.waitForShutdown()
    }
}
