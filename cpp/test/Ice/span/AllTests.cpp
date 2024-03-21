//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "TestHelper.h"
#include "Test.h"

#include <iterator>

using namespace std;
using namespace Ice;
using namespace Test;

TestIntfPrx
allTests(TestHelper* helper)
{
    CommunicatorPtr communicator = helper->communicator();
    TestIntfPrx prx(communicator, "test:" + helper->getTestEndpoint());

    cout << "testing span... " << flush;
    {
        vector<byte> v{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};

        auto dataIn = span<const byte>{v}.subspan(1, 3);
        vector<byte> dataOut;
        auto r = prx->opByteSpan(dataIn, dataOut);
        vector<byte> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    {
        vector<int16_t> v{0, 1, 2, 3, 4, 5};

        auto dataIn = span<const int16_t>{v}.subspan(1, 3);
        vector<int16_t> dataOut;
        auto r = prx->opShortSpan(dataIn, dataOut);
        vector<int16_t> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    {
        vector<string> v{"a", "bb", "ccc", "dddd", "eeeee"};

        auto dataIn = span<string>{v}.subspan(1, 3);
        vector<string> dataOut;
        auto r = prx->opStringSpan(dataIn, dataOut);
        vector<string> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    cout << "ok" << endl;
    cout << "testing span with optionals... " << flush;
    {
        vector<byte> v{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};

        auto dataIn = span<const byte>{v}.subspan(1, 3);
        optional<vector<byte>> dataOut;
        auto r = prx->opOptionalByteSpan(dataIn, dataOut);
        vector<byte> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    {
        vector<int16_t> v{0, 1, 2, 3, 4, 5};

        auto dataIn = span<const int16_t>{v}.subspan(1, 3);
        optional<vector<int16_t>> dataOut;
        auto r = prx->opOptionalShortSpan(dataIn, dataOut);
        vector<int16_t> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    {
        vector<string> v{"a", "bb", "ccc", "dddd", "eeeee"};

        auto dataIn = span<string>{v}.subspan(1, 3);
        optional<vector<string>> dataOut;
        auto r = prx->opOptionalStringSpan(dataIn, dataOut);
        vector<string> dataInVec{dataIn.begin(), dataIn.end()};
        test(r == dataOut);
        test(r == dataInVec);
    }

    {
        optional<vector<byte>> dataOut;
        auto r = prx->opOptionalByteSpan(nullopt, dataOut);
        test(r == dataOut);
        test(r == nullopt);
    }

    {
        optional<vector<int16_t>> dataOut;
        auto r = prx->opOptionalShortSpan(nullopt, dataOut);
        test(r == dataOut);
        test(r == nullopt);
    }

    {
        optional<vector<string>> dataOut;
        auto r = prx->opOptionalStringSpan(nullopt, dataOut);
        test(r == dataOut);
        test(r == nullopt);
    }
    cout << "ok" << endl;

    return prx;
}
