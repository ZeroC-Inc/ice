//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[cs:namespace:Ice.retry]
module Test
{

exception SystemFailure
{
}

interface Retry
{
    void op(bool kill);

    idempotent int opIdempotent(int c);
    void opNotIdempotent();
    void opSystemException();

    idempotent void sleep(int delay);

    idempotent void shutdown();
}

}
