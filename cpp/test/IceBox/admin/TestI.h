//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_I_H
#define TEST_I_H

#include "Test.h"

class TestFacetI : public virtual ::Test::TestFacet
{
public:
    TestFacetI();

    virtual Ice::PropertyDict getChanges(const Ice::Current&);

    virtual void updated(const Ice::PropertyDict&);

private:
    Ice::PropertyDict _changes;
    std::mutex _mutex;
};
using TestFacetIPtr = std::shared_ptr<TestFacetI>;

#endif
