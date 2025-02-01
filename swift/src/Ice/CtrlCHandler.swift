// Copyright (c) ZeroC, Inc.

import IceImpl

#if os(macOS)
    /// Helps applications handle Ctrl+C (SIGINT) and similar signals (SIGHUP and SIGTERM). Only available on macOS.
    public final class CtrlCHandler {
        private let handle = ICECtrlCHandler()

        /// Creates a CtrlCHandler. Only one instance of this class can exist in a program at any point in time.
        /// This instance must be created before starting any thread.
        public init() {}

        /// Waits until this handler catches a Ctrl+C or similar signal.
        /// - Returns: The signal number.
        public func catchSignal() async -> Int32 {
            return await withCheckedContinuation { continuation in
                self.handle.catchSignal { signal in
                    continuation.resume(returning: signal)
                }
            }
        }
    }
#endif
