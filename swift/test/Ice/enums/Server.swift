//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import Dispatch
import Ice
import TestCommon

class Server: TestHelperI {
  override public func run(args: [String]) throws {
    let communicator = try initialize(args)
    defer {
      communicator.destroy()
    }

    communicator.getProperties().setProperty(
      key: "TestAdapter.Endpoints", value: getTestEndpoint(num: 0))
    let adapter = try communicator.createObjectAdapter("TestAdapter")
    try adapter.add(servant: TestIntfDisp(TestI()), id: Ice.stringToIdentity("test"))
    try adapter.activate()
    serverReady()
    communicator.waitForShutdown()
  }
}
