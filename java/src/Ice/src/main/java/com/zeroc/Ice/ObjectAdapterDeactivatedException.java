//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

/**
 * This exception is raised if an attempt is made to use a deactivated {@link ObjectAdapter}.
 *
 * @see ObjectAdapter#deactivate
 * @see Communicator#shutdown
 */
public class ObjectAdapterDeactivatedException extends LocalException {
  public ObjectAdapterDeactivatedException(String name) {
    this.name = name;
  }

  public String ice_id() {
    return "::Ice::ObjectAdapterDeactivatedException";
  }

  /** Name of the adapter. */
  public String name;

  private static final long serialVersionUID = -1946575462194055987L;
}
