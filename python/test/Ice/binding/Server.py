#!/usr/bin/env python3

# Copyright (c) ZeroC, Inc.

import Ice
from TestHelper import TestHelper

TestHelper.loadSlice("Test.ice")
import TestI


class Server(TestHelper):
    def run(self, args):
        with self.initialize(args=args) as communicator:
            communicator.getProperties().setProperty(
                "TestAdapter.Endpoints", self.getTestEndpoint()
            )
            adapter = communicator.createObjectAdapter("TestAdapter")
            adapter.add(
                TestI.RemoteCommunicatorI(), Ice.stringToIdentity("communicator")
            )
            adapter.activate()

            communicator.waitForShutdown()
