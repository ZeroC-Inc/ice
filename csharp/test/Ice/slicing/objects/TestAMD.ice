//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[3.7]]

// 'Test.ice' and 'TestAMD.ice' need to generate code into separate namespaces, but with identical type-ids. So we use
// the 'cs:namespace' metadata here to place the AMD code into a separate AMD namespace, instead of another module;
// Which would result in differing type-ids.
[cs:namespace:ZeroC.Ice.Test.AMD]
module Slicing::Objects
{

class SBase
{
    string sb = "";
}

class SBSKnownDerived : SBase
{
    string sbskd = "";
}

class B
{
    string sb = "";
    B pb;
}

class D1 : B
{
    string sd1 = "";
    B pd1;
}

sequence<B> BSeq;

class SS1
{
    BSeq s;
}

class SS2
{
    BSeq s;
}

struct SS3
{
    SS1 c1;
    SS2 c2;
}

dictionary<int, B> BDict;

exception BaseException
{
    string sbe;
    B pb;
}

exception DerivedException : BaseException
{
    string sde;
    D1 pd1;
}

class Forward;

class PBase
{
    int pi;
}

sequence<PBase> PBaseSeq;

[preserve-slice]
class Preserved : PBase
{
    string ps;
}

class PDerived : Preserved
{
    PBase pb;
}

[preserve-slice]
class PNode
{
    PNode next;
}

exception PreservedException
{
}

[amd] [format:sliced]
interface TestIntf
{
    Object SBaseAsObject();
    SBase SBaseAsSBase();
    SBase SBSKnownDerivedAsSBase();
    SBSKnownDerived SBSKnownDerivedAsSBSKnownDerived();

    SBase SBSUnknownDerivedAsSBase();

    [format:compact] SBase SBSUnknownDerivedAsSBaseCompact();

    Object SUnknownAsObject();
    void checkSUnknown(Object o);

    B oneElementCycle();
    B twoElementCycle();
    B D1AsB();
    D1 D1AsD1();
    B D2AsB();

    void paramTest1(out B p1, out B p2);
    void paramTest2(out B p2, out B p1);
    B paramTest3(out B p1, out B p2);
    B paramTest4(out B p);

    B returnTest1(out B p1, out B p2);
    B returnTest2(out B p2, out B p1);
    B returnTest3(B p1, B p2);

    SS3 sequenceTest(SS1 p1, SS2 p2);

    BDict dictionaryTest(BDict bin, out BDict bout);

    PBase exchangePBase(PBase pb);

    Preserved PBSUnknownAsPreserved();
    void checkPBSUnknown(Preserved p);

    Preserved PBSUnknownAsPreservedWithGraph();
    void checkPBSUnknownWithGraph(Preserved p);

    Preserved PBSUnknown2AsPreservedWithGraph();
    void checkPBSUnknown2WithGraph(Preserved p);

    PNode exchangePNode(PNode pn);

    void throwBaseAsBase() throws BaseException;
    void throwDerivedAsBase() throws BaseException;
    void throwDerivedAsDerived() throws DerivedException;
    void throwUnknownDerivedAsBase() throws BaseException;
    void throwPreservedException() throws PreservedException;

    void useForward(out Forward f); /* Use of forward-declared class to verify that code is generated correctly. */

    void shutdown();
}

// Used to test Ice.Default.SlicedFormat property
[amd]
interface TestIntf2
{
    SBase SBSUnknownDerivedAsSBase();
    void CUnknownAsSBase(SBase cUnknown);
}

class Hidden
{
    Forward f;
}

class Forward
{
    Hidden h;
}

}
