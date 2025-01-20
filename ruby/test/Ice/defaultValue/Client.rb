#!/usr/bin/env ruby
# Copyright (c) ZeroC, Inc.

require "Ice"
Ice::loadSlice("Test.ice")
require './AllTests'

class Client < ::TestHelper
    def run(args)
        allTests()
    end
end
