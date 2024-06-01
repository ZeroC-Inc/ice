//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#ifndef ICEIMPL_API
#    define ICEIMPL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus

#    include "Ice/Ice.h"

#    if TARGET_OS_IPHONE
#        include "IceIAP/IceIAP.h"
#    endif

#endif
