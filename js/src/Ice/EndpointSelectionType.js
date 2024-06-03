//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { defineEnum } from './EnumBase';

/**
 *  Determines the order in which the Ice run time uses the endpoints in a proxy when establishing a connection.
 **/
export const EndpointSelectionType = defineEnum([['Random', 0], ['Ordered', 1]]);
