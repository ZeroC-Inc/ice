// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using ZeroC.Test;

namespace ZeroC.Ice.Test.ACM
{
    public class Logger : ILogger
    {
        public string Prefix => "";

        private const string DateFormat = "d";
        private const string TimeFormat = "HH:mm:ss:fff";

        private readonly string _name;
        private readonly List<string> _messages = new();
        private readonly object _mutex = new();
        private readonly TextWriter _output;
        private bool _started;

        public Logger(string name, TextWriter output)
        {
            _name = name;
            _output = output;
        }

        public ILogger CloneWithPrefix(string prefix) => this;

        public void Error(string message)
        {
            lock (_mutex)
            {
                var s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(DateFormat, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(TimeFormat, CultureInfo.CurrentCulture));
                s.Append(" error : ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    Dump();
                }
            }
        }

        public void Print(string msg)
        {
            lock (_mutex)
            {
                _messages.Add(msg);
                if (_started)
                {
                    Dump();
                }
            }
        }

        public void Start()
        {
            lock (_mutex)
            {
                _started = true;
                Dump();
            }
        }

        public void Trace(string category, string message)
        {
            lock (_mutex)
            {
                var s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(DateFormat, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(TimeFormat, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append('[');
                s.Append(category);
                s.Append("] ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    Dump();
                }
            }
        }

        public void Warning(string message)
        {
            lock (_mutex)
            {
                var s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(DateFormat, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(TimeFormat, CultureInfo.CurrentCulture));
                s.Append(" warning : ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    Dump();
                }
            }
        }

        private void Dump()
        {
            foreach (string line in _messages)
            {
                _output.WriteLine(line);
            }
            _messages.Clear();
        }
    }

    public abstract class TestCase
    {
        protected bool Closed;
        protected int Heartbeat;
        protected readonly object Mutex = new();

        private IRemoteObjectAdapterPrx? _adapter;
        private readonly IRemoteCommunicatorPrx _com;
        private int? _clientIdleTimeout;
        private bool? _clientKeepAlive;
        private Communicator? _communicator;
        private readonly TestHelper _helper;
        private readonly Logger _logger;
        private string? _msg;
        private readonly string _name;
        private readonly TextWriter _output;
        private int? _serverIdleTimeout;
        private bool? _serverKeepAlive;
        private readonly Stopwatch _stopwatch = new();
        private Thread? _thread;

        public TestCase(string name, IRemoteCommunicatorPrx com, TestHelper helper)
        {
            _name = name;
            _com = com;
            _output = helper.Output;
            _logger = new Logger(_name, _output);
            _helper = helper;
            _stopwatch.Start();
        }

        public void Init()
        {
            _adapter = _com.CreateObjectAdapter(_serverIdleTimeout ?? 2, _serverKeepAlive ?? false);

            Dictionary<string, string> properties = _com.Communicator.GetProperties();
            properties["Ice.IdleTimeout"] = $"{_clientIdleTimeout ?? 2}s";
            properties["Ice.KeepAlive"] = _clientKeepAlive?.ToString() ?? "0";
            _communicator = TestHelper.CreateCommunicator(properties);
            _thread = new Thread(Run);
        }

        public void Start() => _thread!.Start();

        public void Destroy()
        {
            _adapter!.Deactivate();
            _communicator!.Dispose();
        }

        public void Join()
        {
            _output.Write($"testing {_name}... ");
            _output.Flush();
            _logger.Start();
            _thread!.Join();
            if (_msg == null)
            {
                _output.WriteLine("ok");
            }
            else
            {
                _output.WriteLine("failed! " + _msg);
                throw new Exception();
            }
        }

        public void Run()
        {
            var proxy = ITestIntfPrx.Parse(_adapter!.GetTestIntf()!.ToString() ?? "", _communicator!);
            try
            {
                proxy.GetConnection().Closed += (sender, args) =>
                    {
                        lock (Mutex)
                        {
                            Closed = true;
                            Monitor.Pulse(Mutex);
                        }
                    };

                proxy.GetConnection().PingReceived += (sender, args) =>
                    {
                        lock (Mutex)
                        {
                            ++Heartbeat;
                        }
                    };

                RunTestCase(_adapter, proxy);
            }
            catch (Exception ex)
            {
                _msg = $"unexpected exception:\n{ex}";
            }
        }

        public void WaitForClosed()
        {
            lock (Mutex)
            {
                TimeSpan now = _stopwatch.Elapsed;
                while (!Closed)
                {
                    Monitor.Wait(Mutex, TimeSpan.FromSeconds(30));
                    if (_stopwatch.Elapsed - now > TimeSpan.FromSeconds(30))
                    {
                        TestHelper.Assert(false); // Waited for more than 30s for close, something's wrong.
                        throw new Exception();
                    }
                }
            }
        }

        public abstract void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy);

        public void SetClientParams(int idleTimeout, bool keepAlive)
        {
            _clientIdleTimeout = idleTimeout;
            _clientKeepAlive = keepAlive;
        }

        public void SetServerParams(int idleTimeout, bool keepAlive)
        {
            _serverIdleTimeout = idleTimeout;
            _serverKeepAlive = keepAlive;
        }
    }

    public static class AllTests
    {
        private class InvocationHeartbeatTest : TestCase
        {
            // Faster ACM to make sure we receive enough ACM heartbeats
            public InvocationHeartbeatTest(IRemoteCommunicatorPrx com, TestHelper helper)
                : base("invocation heartbeat", com, helper) => SetServerParams(2, false);
            public override void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy)
            {
                proxy.Sleep(4);

                lock (Mutex)
                {
                    TestHelper.Assert(Heartbeat >= 2);
                    TestHelper.Assert(!Closed);
                }
            }
        }

        private class CloseOnIdleTest : TestCase
        {
            // Only close on idle
            public CloseOnIdleTest(IRemoteCommunicatorPrx com, TestHelper helper)
                : base("close on idle", com, helper) => SetClientParams(1, false);

            public override void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy)
            {
                Connection connection = proxy.GetConnection()!;
                WaitForClosed();
                lock (Mutex)
                {
                    TestHelper.Assert(Heartbeat == 0);
                    TestHelper.Assert(!connection.IsActive);
                }
            }
        }

        private class HeartbeatOnIdleTest : TestCase
        {
            // Enable server heartbeats.
            public HeartbeatOnIdleTest(IRemoteCommunicatorPrx com, TestHelper helper)
                : base("heartbeat on idle", com, helper) => SetServerParams(1, true);

            public override void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy)
            {
                Thread.Sleep(3000);

                lock (Mutex)
                {
                    TestHelper.Assert(Heartbeat >= 3);
                }
            }
        }

        private class HeartbeatManualTest : TestCase
        {
            public HeartbeatManualTest(IRemoteCommunicatorPrx com, TestHelper helper)
                : base("manual heartbeats", com, helper)
            {
                // Disable heartbeats.
                SetClientParams(10, false);
                SetServerParams(10, false);
            }

            public override void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy)
            {
                proxy.StartHeartbeatCount();
                Connection con = proxy.GetConnection()!;
                con.Ping();
                con.Ping();
                con.Ping();
                con.Ping();
                con.Ping();
                proxy.WaitForHeartbeatCount(5);
            }
        }

        private class SetAcmTest : TestCase
        {
            public SetAcmTest(IRemoteCommunicatorPrx com, TestHelper helper)
                : base("setACM/getACM", com, helper)
            {
                SetClientParams(1, false);
                SetServerParams(60, false);
            }

            public override void RunTestCase(IRemoteObjectAdapterPrx adapter, ITestIntfPrx proxy)
            {
                Connection? con = proxy.GetCachedConnection();
                TestHelper.Assert(con != null);

                TestHelper.Assert(con.IdleTimeout == TimeSpan.FromSeconds(1));
                TestHelper.Assert(con.KeepAlive == false);

                con.KeepAlive = true;
                TestHelper.Assert(con.KeepAlive == true);

                try
                {
                    con.IdleTimeout = TimeSpan.FromSeconds(1);
                }
                catch
                {
                }

                proxy.StartHeartbeatCount();
                proxy.WaitForHeartbeatCount(2);

                var t1 = new TaskCompletionSource<object?>();
                var t2 = new TaskCompletionSource<object?>();
                con.Closed += (sender, args) => t1.SetResult(null);
                con.Closed += (sender, args) => t2.SetResult(null);

                con.GoAwayAsync();
                TestHelper.Assert(t1.Task.Result == null);
                TestHelper.Assert(t2.Task.Result == null);

                TestHelper.Assert(!con.IsActive);

                var t3 = new TaskCompletionSource<object?>();
                con.Closed += (sender, args) => t3.SetResult(null);
                TestHelper.Assert(t3.Task.Result == null);

                con.PingReceived += (sender, args) => TestHelper.Assert(false);

                foreach ((int idleTimeout, string keepAlive) in new (int, string)[]
                                                                {
                                                                    (10,  "true"),
                                                                    (50, "false"),
                                                                })
                {
                    using var communicator = new Communicator(
                        new Dictionary<string, string>(proxy.Communicator.GetProperties())
                        {
                            ["Ice.IdleTimeout"] = $"{idleTimeout}s",
                            ["Ice.KeepAlive"] = keepAlive
                        });

                    proxy = ITestIntfPrx.Parse(proxy.ToString()!, communicator);
                    Connection? connection = proxy.GetConnection()!;
                    TestHelper.Assert(connection.IdleTimeout == TimeSpan.FromSeconds(idleTimeout));
                    TestHelper.Assert(connection.KeepAlive == bool.Parse(keepAlive));
                }
            }
        }

        public static Task RunAsync(TestHelper helper)
        {
            Communicator communicator = helper.Communicator;
            var com = IRemoteCommunicatorPrx.Parse(helper.GetTestProxy("communicator", 0), communicator);

            TextWriter output = helper.Output;

            // TODO: remove tests which are no longer supported when we refactor ACM.
            var tests = new List<TestCase>
            {
                new InvocationHeartbeatTest(com, helper),
                new CloseOnIdleTest(com, helper),
                new HeartbeatOnIdleTest(com, helper),
                new HeartbeatManualTest(com, helper),
                new SetAcmTest(com, helper)
            };

            foreach (TestCase test in tests)
            {
                test.Init();
            }
            foreach (TestCase test in tests)
            {
                test.Start();
            }
            foreach (TestCase test in tests)
            {
                test.Join();
            }
            foreach (TestCase test in tests)
            {
                test.Destroy();
            }

            output.Write("shutting down... ");
            output.Flush();
            com.Shutdown();
            output.WriteLine("ok");
            return Task.CompletedTask;
        }
    }
}
