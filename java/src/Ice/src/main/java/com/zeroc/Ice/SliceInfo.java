//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

/** SliceInfo encapsulates the details of a slice for an unknown class or exception type. */
public class SliceInfo {
  /** The Slice type ID for this slice. */
  public final String typeId;

  /** The Slice compact type ID for this slice. */
  public final int compactId;

  /** The encoded bytes for this slice, including the leading size integer. */
  public final byte[] bytes;

  /** The class instances referenced by this slice. */
  public com.zeroc.Ice.Value[] instances;

  /** Whether or not the slice contains optional members. */
  public final boolean hasOptionalMembers;

  /** Whether or not this is the last slice. */
  public final boolean isLastSlice;

  /** The SliceInfo constructor. */
  public SliceInfo(
      String typeId, int compactId, byte[] bytes, boolean hasOptionalMembers, boolean isLastSlice) {
    this.typeId = typeId;
    this.compactId = compactId;
    this.bytes = bytes;
    this.hasOptionalMembers = hasOptionalMembers;
    this.isLastSlice = isLastSlice;
  }
}
