//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

#include <Test.ice>

module Test
{

class D3 : B
{
    string sd3;
    B pd3;
}

[preserve-slice]
class PCUnknown : PBase
{
    string pu;
}

class PCDerived : PDerived
{
    PBaseSeq pbs;
}

class PCDerived2 : PCDerived
{
    int pcd2;
}

class PCDerived3 : PCDerived2
{
    Object pcd3;
}

class CompactPCDerived(57) : CompactPDerived
{
    PBaseSeq pbs;
}

}
