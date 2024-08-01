//
import Foundation
// Copyright (c) ZeroC, Inc. All rights reserved.
//
import Ice

class TimeoutI: Timeout {
    func op(current _: Current) throws {}

    func sendData(seq _: ByteSeq, current _: Current) throws {}

    func sleep(to: Int32, current _: Current) throws {
        Thread.sleep(forTimeInterval: TimeInterval(to) / 1000)
    }
}

class ControllerI: Controller {
    var _adapter: Ice.ObjectAdapter

    init(_ adapter: Ice.ObjectAdapter) {
        _adapter = adapter
    }

    func holdAdapter(to: Int32, current: Ice.Current) throws {
        _adapter.hold()
        if to >= 0 {
            Task {
                do {
                    self._adapter.waitForHold()
                    try await Task.sleep(for: .milliseconds(Int(to)))
                    try self._adapter.activate()
                } catch {
                    fatalError("unexpected error: \(error)")
                }

            }
        }
    }

    func resumeAdapter(current _: Ice.Current) throws {
        try _adapter.activate()
    }

    func shutdown(current: Ice.Current) {
        current.adapter.getCommunicator().shutdown()
    }
}
