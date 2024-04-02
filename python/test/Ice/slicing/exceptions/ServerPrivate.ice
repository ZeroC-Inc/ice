//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

#include "Test.ice"

module Test
{

exception UnknownDerived extends Base
{
    string ud;
}

exception UnknownIntermediate extends Base
{
   string ui;
}

exception UnknownMostDerived1 extends KnownIntermediate
{
   string umd1;
}

exception UnknownMostDerived2 extends UnknownIntermediate
{
   string umd2;
}

}
