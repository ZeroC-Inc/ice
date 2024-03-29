//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <TestI.h>
#include "Ice/Ice.h"

#include <thread>
#include <chrono>

using namespace std;
using namespace Ice;

void
TimeoutI::op(const Ice::Current&)
{
}

void
TimeoutI::sendData(Test::ByteSeq, const Ice::Current&)
{
}

void
TimeoutI::sleep(int32_t to, const Ice::Current&)
{
    this_thread::sleep_for(chrono::milliseconds(to));
}

ControllerI::ControllerI(const Ice::ObjectAdapterPtr& adapter) : _adapter(adapter) {}

void
ControllerI::holdAdapter(int32_t to, const Ice::Current&)
{
    _adapter->hold();

    if (to >= 0)
    {
        std::thread activateThread(
            [this, to]
            {
                this_thread::sleep_for(chrono::milliseconds(to));
                _adapter->activate();
            });
        activateThread.detach();
    }
}

void
ControllerI::resumeAdapter(const Ice::Current&)
{
    _adapter->activate();
}

void
ControllerI::shutdown(const Ice::Current&)
{
    _adapter->getCommunicator()->shutdown();
}
