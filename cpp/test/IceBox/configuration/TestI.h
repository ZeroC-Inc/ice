//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_I_H
#define TEST_I_H

#include "Test.h"

class TestI : public ::Test::TestIntf
{
public:
    TestI(const Ice::StringSeq&);

    std::string getProperty(std::string, const Ice::Current&) override;
    Ice::StringSeq getArgs(const Ice::Current&) override;

private:
    const Ice::StringSeq _args;
};

#endif
