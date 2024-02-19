//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <IceUtil/Timer.h>
#include <IceUtil/Exception.h>
#include <Ice/ConsoleUtil.h>

using namespace std;
using namespace IceUtil;
using namespace IceInternal;

TimerTask::~TimerTask()
{
}

Timer::Timer() :
    _destroyed(false),
    _wakeUpTime(chrono::steady_clock::time_point()),
    _worker(&Timer::run, this)
{
}

void
Timer::destroy()
{
    {
        std::lock_guard lock(_mutex);
        if(_destroyed)
        {
            return;
        }
        _destroyed = true;
        _tokens.clear();
        _tasks.clear();
        _condition.notify_one();
    }

    if (std::this_thread::get_id() == _worker.get_id())
    {
        _worker.detach();
    }
    else if (_worker.joinable())
    {
        _worker.join();
    }
}

bool
Timer::cancel(const TimerTaskPtr& task)
{
    lock_guard lock(_mutex);
    if(_destroyed)
    {
        return false;
    }

    auto p = _tasks.find(task);
    if(p == _tasks.end())
    {
        return false;
    }

    _tokens.erase(Token { p->second, nullopt, p->first });
    _tasks.erase(p);

    return true;
}

void Timer::run()
{
    Token token { chrono::steady_clock::time_point(), nullopt, nullptr };
    while (true)
    {
        {
            unique_lock lock(_mutex);

            if (!_destroyed)
            {
                // If the task we just ran is a repeated task, schedule it again for execution if it wasn't canceled.
                if (token.delay)
                {
                    auto p = _tasks.find(token.task);
                    if (p != _tasks.end())
                    {
                        token.scheduledTime = chrono::steady_clock::now() + token.delay.value();
                        p->second = token.scheduledTime;
                        _tokens.insert(token);
                    }
                }
                token = { chrono::steady_clock::time_point(), nullopt, nullptr };

                if (_tokens.empty())
                {
                    _wakeUpTime = chrono::steady_clock::time_point();
                    _condition.wait(lock);
                }
            }

            if (_destroyed)
            {
                break;
            }

            while (!_tokens.empty() && !_destroyed)
            {
                const auto now = chrono::steady_clock::now();
                const Token& first = *(_tokens.begin());
                if (first.scheduledTime <= now)
                {
                    token = first;
                    _tokens.erase(_tokens.begin());
                    if (!token.delay)
                    {
                        _tasks.erase(token.task);
                    }
                    break;
                }

                _wakeUpTime = first.scheduledTime;
                _condition.wait_until(lock, first.scheduledTime);
            }

            if (_destroyed)
            {
                break;
            }
        }

        if (token.task)
        {
            try
            {
                runTimerTask(token.task);
            }
            catch(const IceUtil::Exception& e)
            {
                consoleErr << "IceUtil::Timer::run(): uncaught exception:\n" << e.what();
#ifdef __GNUC__
                consoleErr << "\n" << e.ice_stackTrace();
#endif
                consoleErr << endl;
            }
            catch(const std::exception& e)
            {
                consoleErr << "IceUtil::Timer::run(): uncaught exception:\n" << e.what() << endl;
            }
            catch(...)
            {
                consoleErr << "IceUtil::Timer::run(): uncaught exception" << endl;
            }

            if (!token.delay)
            {
                // If the task is not a repeated task, clear the task reference now rather than
                // in the synchronization block above. Clearing the task reference might end up
                // calling user code which could trigger a deadlock. See also issue #352.
                token.task = nullptr;
            }
        }
    }
}

void
Timer::runTimerTask(const TimerTaskPtr& task)
{
    task->runTimerTask();
}
