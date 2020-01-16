//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

namespace IceInternal
{

    internal interface ILoggerAdminLogger : Ice.ILogger
    {
        Ice.ILoggerAdmin getFacet();
        void destroy();
    }

    internal sealed class LoggerAdminLogger : ILoggerAdminLogger
    {
        public void print(string message)
        {
            Ice.LogMessage logMessage = new Ice.LogMessage(Ice.LogMessageType.PrintMessage, now(), "", message);
            _localLogger.print(message);
            log(logMessage);
        }

        public void trace(string category, string message)
        {
            Ice.LogMessage logMessage = new Ice.LogMessage(Ice.LogMessageType.TraceMessage, now(), category, message);
            _localLogger.trace(category, message);
            log(logMessage);
        }

        public void warning(string message)
        {
            Ice.LogMessage logMessage = new Ice.LogMessage(Ice.LogMessageType.WarningMessage, now(), "", message);
            _localLogger.warning(message);
            log(logMessage);
        }

        public void error(string message)
        {
            Ice.LogMessage logMessage = new Ice.LogMessage(Ice.LogMessageType.ErrorMessage, now(), "", message);
            _localLogger.error(message);
            log(logMessage);
        }

        public string getPrefix()
        {
            return _localLogger.getPrefix();
        }

        public Ice.ILogger cloneWithPrefix(string prefix)
        {
            return _localLogger.cloneWithPrefix(prefix);
        }

        public Ice.ILoggerAdmin getFacet()
        {
            return _loggerAdmin;
        }

        public void destroy()
        {
            Thread thread = null;
            lock (this)
            {
                if (_sendLogThread != null)
                {
                    thread = _sendLogThread;
                    _sendLogThread = null;
                    _destroyed = true;
                    Monitor.PulseAll(this);
                }
            }

            if (thread != null)
            {
                thread.Join();
            }

            _loggerAdmin.destroy();
        }

        internal LoggerAdminLogger(Ice.Communicator communicator, Ice.ILogger localLogger)
        {
            if (localLogger is LoggerAdminLogger)
            {
                _localLogger = ((LoggerAdminLogger)localLogger).getLocalLogger();
            }
            else
            {
                _localLogger = localLogger;
            }
            _loggerAdmin = new LoggerAdmin(communicator, this);
        }

        internal Ice.ILogger getLocalLogger()
        {
            return _localLogger;
        }

        internal void log(Ice.LogMessage logMessage)
        {
            List<Ice.IRemoteLoggerPrx> remoteLoggers = _loggerAdmin.log(logMessage);

            if (remoteLoggers != null)
            {
                Debug.Assert(remoteLoggers.Count > 0);

                lock (this)
                {
                    if (_sendLogThread == null)
                    {
                        _sendLogThread = new Thread(new ThreadStart(run));
                        _sendLogThread.Name = "Ice.SendLogThread";
                        _sendLogThread.IsBackground = true;
                        _sendLogThread.Start();
                    }

                    _jobQueue.Enqueue(new Job(remoteLoggers, logMessage));
                    Monitor.PulseAll(this);
                }
            }
        }

        private void run()
        {
            if (_loggerAdmin.getTraceLevel() > 1)
            {
                _localLogger.trace(_traceCategory, "send log thread started");
            }

            for (; ; )
            {
                Job job = null;
                lock (this)
                {
                    while (!_destroyed && _jobQueue.Count == 0)
                    {
                        Monitor.Wait(this);
                    }

                    if (_destroyed)
                    {
                        break; // for(;;)
                    }

                    Debug.Assert(_jobQueue.Count > 0);
                    job = _jobQueue.Dequeue();
                }

                foreach (var p in job.remoteLoggers)
                {
                    if (_loggerAdmin.getTraceLevel() > 1)
                    {
                        _localLogger.trace(_traceCategory, "sending log message to `" + p.ToString() + "'");
                    }

                    try
                    {
                        //
                        // p is a proxy associated with the _sendLogCommunicator
                        //
                        p.LogAsync(job.logMessage).ContinueWith(
                            (t) =>
                            {
                                try
                                {
                                    t.Wait();
                                    if (_loggerAdmin.getTraceLevel() > 1)
                                    {
                                        _localLogger.trace(_traceCategory, "log on `" + p.ToString()
                                                           + "' completed successfully");
                                    }
                                }
                                catch (AggregateException ae)
                                {
                                    if (ae.InnerException is Ice.CommunicatorDestroyedException)
                                    {
                                        // expected if there are outstanding calls during communicator destruction
                                    }
                                    if (ae.InnerException is Ice.LocalException)
                                    {
                                        _loggerAdmin.deadRemoteLogger(p, _localLogger,
                                                                      (Ice.LocalException)ae.InnerException, "log");
                                    }
                                }
                            },
                            System.Threading.Tasks.TaskScheduler.Current);
                    }
                    catch (Ice.LocalException ex)
                    {
                        _loggerAdmin.deadRemoteLogger(p, _localLogger, ex, "log");
                    }
                }
            }

            if (_loggerAdmin.getTraceLevel() > 1)
            {
                _localLogger.trace(_traceCategory, "send log thread completed");
            }
        }

        private static long now()
        {
            return DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() * 1000;
        }

        private class Job
        {
            internal Job(List<Ice.IRemoteLoggerPrx> r, Ice.LogMessage l)
            {
                remoteLoggers = r;
                logMessage = l;
            }

            internal readonly List<Ice.IRemoteLoggerPrx> remoteLoggers;
            internal readonly Ice.LogMessage logMessage;
        }

        private readonly Ice.ILogger _localLogger;
        private readonly LoggerAdmin _loggerAdmin;
        private bool _destroyed = false;
        private Thread _sendLogThread;
        private readonly Queue<Job> _jobQueue = new Queue<Job>();

        private const string _traceCategory = "Admin.Logger";
    }

}
