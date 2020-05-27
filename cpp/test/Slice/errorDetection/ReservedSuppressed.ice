//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

// All warnings should be suppressed and this file shouldn't emit any warnings.
[[suppress-warnings:reserved-identifiers]]

#include <include/IcePrefix.ice>

module OK
{
const long PrxA = 0;
const long APrxA = 0;
const long prxB = 0;
const long Bprx = 0;
const long prx = 0;
const long PtrA = 0;
const long HelperA = 0;
const long HolderA = 0;
const long aIce = 0;
}

module errors
{
const long Prx = 0;
const long abcPrx = 0;
const long Ptr = 0;
const long abcPtr = 0;
const long Helper = 0;
const long abcHelper = 0;
const long Holder = 0;
const long abcHolder = 0;
const long Ice = 0;
const long ice = 0;
const long icea = 0;
const long Iceblah = 0;
const long IceFoo = 0;
const long icecream = 0;
const long ICEpick = 0;
const long iCEaxe = 0;
}

module Ice {}
module IceFoo {}

module all::good::here {}
module an::iceberg::ahead {}
module aPtr::okay::bPrx::fine::cHelper {}

interface _a;           // Illegal leading underscore
interface _true;        // Illegal leading underscore
interface \_true;       // Illegal leading underscore

interface b_;           // Illegal trailing underscore

interface b__c;         // Illegal double underscores
interface b___c;        // Illegal double underscores

interface _a_;          // Illegal underscores
interface a_b;          // Illegal underscore
interface a_b_c;        // Illegal underscores
interface _a__b__;      // Illegal underscores
