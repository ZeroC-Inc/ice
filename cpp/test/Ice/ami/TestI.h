//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_I_H
#define TEST_I_H

#include <Test.h>
#include <TestHelper.h>

class TestIntfControllerI;
using TestIntfControllerIPtr = std::shared_ptr<TestIntfControllerI>;

class TestIntfI final : public Test::TestIntf
{
public:

    TestIntfI();

    void op(const Ice::Current&) final;
    int opWithResult(const Ice::Current&) final;
    void opWithUE(const Ice::Current&) final;
    int opWithResultAndUE(const Ice::Current&) final;
    void opWithPayload(Ice::ByteSeq, const Ice::Current&) final;
    void opBatch(const Ice::Current&) final;
    std::int32_t opBatchCount(const Ice::Current&) final;
    void opWithArgs(std::int32_t&, std::int32_t&, std::int32_t&, std::int32_t&, std::int32_t&, std::int32_t&, std::int32_t&,
                            std::int32_t&, std::int32_t&, std::int32_t&, std::int32_t&, const Ice::Current&) final;
    bool waitForBatch(std::int32_t, const Ice::Current&) final;
    void close(Test::CloseMode, const Ice::Current&) final;
    void sleep(std::int32_t, const Ice::Current&) final;
    void startDispatchAsync(std::function<void()>, std::function<void(std::exception_ptr)>,
                                    const Ice::Current&) final;
    void finishDispatch(const Ice::Current&) final;
    void shutdown(const Ice::Current&) final;

    bool supportsAMD(const Ice::Current&) final;
    bool supportsFunctionalTests(const Ice::Current&) final;

    void pingBiDir(std::optional<Test::PingReplyPrx>, const Ice::Current&) final;

private:

    int _batchCount;
    bool _shutdown;
    std::function<void()> _pending;
    std::mutex _mutex;
    std::condition_variable _condition;
};

class TestIntfControllerI : public Test::TestIntfController
{
public:

    void holdAdapter(const Ice::Current&) final;
    void resumeAdapter(const Ice::Current&) final;

    TestIntfControllerI(const Ice::ObjectAdapterPtr&);

private:

    Ice::ObjectAdapterPtr _adapter;
    std::mutex _mutex;
};

class TestIntfII : public Test::Outer::Inner::TestIntf
{
public:

    std::int32_t op(std::int32_t, std::int32_t&, const Ice::Current&) final;
};

#endif
