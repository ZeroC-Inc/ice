//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/CtrlCHandler.h"
#include "Ice/LocalException.h"

#ifdef _WIN32
#    include <windows.h>
#endif

#include <cassert>
#include <mutex>

#ifndef _WIN32
#    include <signal.h>
#endif

using namespace Ice;
using namespace std;

namespace
{
    CtrlCHandlerCallback _callback = nullptr;

    const CtrlCHandler* _handler = nullptr;

    mutex globalMutex;
}

CtrlCHandlerCallback
CtrlCHandler::setCallback(CtrlCHandlerCallback callback)
{
    lock_guard lock(globalMutex);
    CtrlCHandlerCallback oldCallback = _callback;
    _callback = callback;
    return oldCallback;
}

CtrlCHandlerCallback
CtrlCHandler::getCallback() const
{
    lock_guard lock(globalMutex);
    return _callback;
}

#ifdef _WIN32

static BOOL WINAPI
handlerRoutine(DWORD dwCtrlType)
{
    CtrlCHandlerCallback callback;
    {
        lock_guard lock(globalMutex);
        if (!_handler) // The handler is destroyed.
        {
            return FALSE;
        }
        callback = _callback;
    }
    if (callback)
    {
        callback(dwCtrlType);
    }
    return TRUE;
}

CtrlCHandler::CtrlCHandler(CtrlCHandlerCallback callback)
{
    unique_lock lock(globalMutex);
    bool handler = _handler != 0;

    if (handler)
    {
        throw Ice::LocalException{__FILE__, __LINE__, "another CtrlCHandler was already created"};
    }
    else
    {
        _callback = callback;
        _handler = this;
        lock.unlock();

        SetConsoleCtrlHandler(handlerRoutine, TRUE);
    }
}

CtrlCHandler::~CtrlCHandler()
{
    SetConsoleCtrlHandler(handlerRoutine, FALSE);
    {
        lock_guard lock(globalMutex);
        _handler = 0;
        _callback = nullptr;
    }
}

#else

extern "C"
{
    static void* sigwaitThread(void*)
    {
        sigset_t ctrlCLikeSignals;
        sigemptyset(&ctrlCLikeSignals);
        sigaddset(&ctrlCLikeSignals, SIGHUP);
        sigaddset(&ctrlCLikeSignals, SIGINT);
        sigaddset(&ctrlCLikeSignals, SIGTERM);

        //
        // Run until the handler is destroyed (_handler == 0)
        //
        for (;;)
        {
            int signal = 0;
            int rc = sigwait(&ctrlCLikeSignals, &signal);
            if (rc == EINTR)
            {
                //
                // Some sigwait() implementations incorrectly return EINTR
                // when interrupted by an unblocked caught signal
                //
                continue;
            }
            assert(rc == 0);

            CtrlCHandlerCallback callback;
            {
                lock_guard lock(globalMutex);
                if (!_handler) // The handler is destroyed.
                {
                    return nullptr;
                }
                callback = _callback;
            }

            if (callback)
            {
                callback(signal);
            }
        }
        return nullptr;
    }
}

namespace
{
    pthread_t _tid;
}

CtrlCHandler::CtrlCHandler(CtrlCHandlerCallback callback)
{
    unique_lock lock(globalMutex);
    bool handler = _handler != nullptr;

    if (handler)
    {
        throw Ice::LocalException{__FILE__, __LINE__, "another CtrlCHandler was already created"};
    }
    else
    {
        _callback = callback;
        _handler = this;

        lock.unlock();

        // We block these CTRL+C like signals in the main thread,
        // and by default all other threads will inherit this signal
        // mask.

        sigset_t ctrlCLikeSignals;
        sigemptyset(&ctrlCLikeSignals);
        sigaddset(&ctrlCLikeSignals, SIGHUP);
        sigaddset(&ctrlCLikeSignals, SIGINT);
        sigaddset(&ctrlCLikeSignals, SIGTERM);

#    ifndef NDEBUG
        int rc = pthread_sigmask(SIG_BLOCK, &ctrlCLikeSignals, nullptr);
        assert(rc == 0);

        // Joinable thread
        rc = pthread_create(&_tid, nullptr, sigwaitThread, nullptr);
        assert(rc == 0);
#    else
        pthread_sigmask(SIG_BLOCK, &ctrlCLikeSignals, 0);

        // Joinable thread
        pthread_create(&_tid, 0, sigwaitThread, 0);
#    endif
    }
}

CtrlCHandler::~CtrlCHandler()
{
    //
    // Clear the handler, the sigwaitThread will exit if _handler is null
    //
    {
        lock_guard lock(globalMutex);
        _handler = nullptr;
        _callback = nullptr;
    }

    //
    // Signal the sigwaitThread and join it.
    //
    void* status = nullptr;
#    ifndef NDEBUG
    int rc = pthread_kill(_tid, SIGTERM);
    assert(rc == 0);
    rc = pthread_join(_tid, &status);
    assert(rc == 0);
#    else
    pthread_kill(_tid, SIGTERM);
    pthread_join(_tid, &status);
#    endif
}

#endif
