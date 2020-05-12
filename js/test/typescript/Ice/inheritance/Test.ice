//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[suppress-warning:deprecated]] // For classes with operations
[[js:es6-module]]

module Test
{

module MA
{

interface IA
{
    IA* iaop(IA* p);
}

class CA
{
    CA* caop(CA* p);
}

}

module MB
{

interface IB1 : MA::IA
{
    IB1* ib1op(IB1* p);
}

interface IB2 : MA::IA
{
    IB2* ib2op(IB2* p);
}

class CB : MA::CA
{
    CB* cbop(CB* p);
}

}

module MA
{

interface IC : MB::IB1, MB::IB2
{
    IC* icop(IC* p);
}

class CC : MB::CB
{
    CC* ccop(CC* p);
}

class CD : CC implements MB::IB1, MB::IB2
{
    CD* cdop(CD* p);
}

}

interface Initial
{
    void shutdown();
    MA::CA* caop();
    MB::CB* cbop();
    MA::CC* ccop();
    MA::CD* cdop();
    MA::IA* iaop();
    MB::IB1* ib1op();
    MB::IB2* ib2op();
    MA::IC* icop();
}

module MC
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

module MD
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

module ME
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

module MF
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

module MG
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

module MH
{

class A
{
    int aA;
}

class B : A
{
    int bB;
}

class C : B
{
    int cC;
}

class D : C
{
    int dD;
}

}

interface Echo
{
    void setConnection();
    void startBatch();
    void flushBatch();
    void shutdown();
}

}
