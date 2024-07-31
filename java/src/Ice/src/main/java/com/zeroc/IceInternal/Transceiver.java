//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceInternal;

public interface Transceiver {
  /**
   * Returns the selectable channel used by the thread pool's selector to wait for read or write
   * readiness.
   *
   * @return The selectable channel that will be registered with the thread pool's selector or null
   *     if the transceiver doesn't use a selectable channel.
   */
  java.nio.channels.SelectableChannel fd();

  /**
   * Sets the transceiver ready callback. This method is called by the thread pool to provide a
   * callback object that the transceiver can use to notify the thread pool's selector when more
   * data is ready to be read or written by this transceiver. A transceiver implementation typically
   * uses this callback when it buffers data read from the selectable channel if it doesn't use a
   * selectable channel.
   *
   * @param callback The ready callback provided by the thread pool's selector.
   */
  void setReadyCallback(ReadyCallback callback);

  int initialize(Buffer readBuffer, Buffer writeBuffer);

  int closing(boolean initiator, com.zeroc.Ice.LocalException ex);

  void close();

  EndpointI bind();

  int write(Buffer buf);

  int read(Buffer buf);

  /**
   * Checks if this transceiver is waiting to be read, typically because it has bytes readily
   * available for reading.
   *
   * @return true if this transceiver is waiting to be read, false otherwise.
   */
  boolean isWaitingToBeRead();

  String protocol();

  @Override
  String toString();

  String toDetailedString();

  com.zeroc.Ice.ConnectionInfo getInfo();

  void checkSendSize(Buffer buf);

  void setBufferSize(int rcvSize, int sndSize);
}
