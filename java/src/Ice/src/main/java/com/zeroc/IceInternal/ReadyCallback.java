//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceInternal;

public interface ReadyCallback {
  /**
   * Sets the transceiver read or write readiness status. This method is used by a transceiver
   * implementation to notify the thread pool that it's ready or not to write or read data.
   *
   * @param op The transceiver read or write operation
   * @param value The status of the operation readiness, true if the transceiver operation is ready,
   *     false otherwise.
   */
  void ready(int op, boolean value);
}
