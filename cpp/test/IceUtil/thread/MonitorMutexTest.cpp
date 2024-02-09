//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <IceUtil/IceUtil.h>

#include <MonitorMutexTest.h>
#include <TestHelper.h>

using namespace std;
using namespace IceUtil;

class MonitorMutexTestThread final : public Thread
{
public:

    MonitorMutexTestThread(Monitor<Mutex>& m) :
        _monitor(m),
        _tryLock(false)
    {
    }

    void run() final
    {
        Monitor<Mutex>::TryLock tlock(_monitor);
        test(!tlock.acquired());

        {
            Mutex::Lock lock(_tryLockMutex);
            _tryLock = true;
        }
        _tryLockCond.signal();

        Monitor<Mutex>::Lock lock(_monitor);
    }

    void
    waitTryLock()
    {
        Mutex::Lock lock(_tryLockMutex);
        while(!_tryLock)
        {
            _tryLockCond.wait(lock);
        }
    }

private:

    Monitor<Mutex>& _monitor;
    bool _tryLock;
    //
    // Use native Condition variable here, not Monitor.
    //
    Cond _tryLockCond;
    Mutex _tryLockMutex;
};
using MonitorMutexTestThreadPtr = shared_ptr<MonitorMutexTestThread>;

class MonitorMutexTestThread2 : public Thread
{
public:

    MonitorMutexTestThread2(Monitor<Mutex>& monitor) :
        finished(false),
        _monitor(monitor)
    {
    }

    virtual void run()
    {
        Monitor<Mutex>::Lock lock(_monitor);
        _monitor.wait();
        finished = true;
    }

    bool finished;

private:

    Monitor<Mutex>& _monitor;
};
using MonitorMutexTestThread2Ptr = shared_ptr<MonitorMutexTestThread2>;

static const string monitorMutexTestName("monitor<mutex>");

MonitorMutexTest::MonitorMutexTest() :
    TestBase(monitorMutexTestName)
{
}

void
MonitorMutexTest::run()
{
    Monitor<Mutex> monitor;
    MonitorMutexTestThreadPtr t;
    MonitorMutexTestThread2Ptr t2;
    MonitorMutexTestThread2Ptr t3;
    ThreadControl control;
    ThreadControl control2;

    {
        Monitor<Mutex>::Lock lock(monitor);

        try
        {
            Monitor<Mutex>::TryLock tlock(monitor);
            test(!tlock.acquired());
        }
        catch(const ThreadLockedException&)
        {
            //
            // pthread_mutex_trylock returns EDEADLK in FreeBSD's new threading implementation
            // as well as in Fedora Core 5.
            //
        }

        // TEST: Start thread, try to acquire the mutex.
        t = make_shared<MonitorMutexTestThread>(monitor);
        control = t->start();

        // TEST: Wait until the tryLock has been tested.
        t->waitTryLock();
    }

    //
    // TEST: Once the mutex has been released, the thread should
    // acquire the mutex and then terminate.
    //
    control.join();

    // TEST: notify() wakes one consumer.
    t2 = make_shared<MonitorMutexTestThread2>(monitor);
    control = t2->start();
    t3 = make_shared<MonitorMutexTestThread2>(monitor);
    control2 = t3->start();

    // Give the thread time to start waiting.
    ThreadControl::sleep(Time::seconds(1));

    {
        Monitor<Mutex>::Lock lock(monitor);
        monitor.notify();
    }

    // Give one thread time to terminate
    ThreadControl::sleep(Time::seconds(1));

    test((t2->finished && !t3->finished) || (t3->finished && !t2->finished));

    {
        Monitor<Mutex>::Lock lock(monitor);
        monitor.notify();
    }
    control.join();
    control2.join();

    // TEST: notifyAll() wakes one consumer.
    t2 = make_shared<MonitorMutexTestThread2>(monitor);
    control = t2->start();
    t3 = make_shared<MonitorMutexTestThread2>(monitor);
    control2 = t3->start();

    // Give the threads time to start waiting.
    ThreadControl::sleep(Time::seconds(1));

    {
        Monitor<Mutex>::Lock lock(monitor);
        monitor.notifyAll();
    }

    control.join();
    control2.join();

    // TEST: timedWait
    {
        Monitor<Mutex>::Lock lock(monitor);

        try
        {
            monitor.timedWait(Time::milliSeconds(-1));
            test(false);
        }
        catch(const IceUtil::InvalidTimeoutException&)
        {
        }

        test(!monitor.timedWait(Time::milliSeconds(500)));
    }
}
