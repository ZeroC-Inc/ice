//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Ice.acm
{
    class LoggerI : Ice.ILogger
    {
        public LoggerI(string name, TextWriter output)
        {
            _name = name;
            _output = output;
        }

        public void start()
        {
            lock (this)
            {
                _started = true;
                dump();
            }
        }

        public void print(string msg)
        {
            lock (this)
            {
                _messages.Add(msg);
                if (_started)
                {
                    dump();
                }
            }
        }

        public void trace(string category, string message)
        {
            lock (this)
            {
                System.Text.StringBuilder s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_date, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_time, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append("[");
                s.Append(category);
                s.Append("] ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    dump();
                }
            }
        }

        public void warning(string message)
        {
            lock (this)
            {
                System.Text.StringBuilder s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_date, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_time, CultureInfo.CurrentCulture));
                s.Append(" warning : ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    dump();
                }
            }
        }

        public void error(string message)
        {
            lock (this)
            {
                System.Text.StringBuilder s = new System.Text.StringBuilder(_name);
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_date, CultureInfo.CurrentCulture));
                s.Append(' ');
                s.Append(DateTime.Now.ToString(_time, CultureInfo.CurrentCulture));
                s.Append(" error : ");
                s.Append(message);
                _messages.Add(s.ToString());
                if (_started)
                {
                    dump();
                }
            }
        }

        public string getPrefix() => "";

        public ILogger cloneWithPrefix(string prefix) => this;

        private void dump()
        {
            foreach (string line in _messages)
            {
                _output.WriteLine(line);
            }
            _messages.Clear();
        }

        private string _name;
        private bool _started;
        private readonly static string _date = "d";
        private readonly static string _time = "HH:mm:ss:fff";
        private TextWriter _output;

        private List<string> _messages = new List<string>();
    }

    abstract class TestCase
    {
        public TestCase(string name, Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper)
        {
            _name = name;
            _com = com;
            _output = helper.getWriter();
            _logger = new LoggerI(_name, _output);
            _helper = helper;

            _clientACMTimeout = -1;
            _clientACMClose = -1;
            _clientACMHeartbeat = -1;

            _serverACMTimeout = -1;
            _serverACMClose = -1;
            _serverACMHeartbeat = -1;

            _heartbeat = 0;
            _closed = false;
        }

        public void init()
        {
            _adapter = _com.createObjectAdapter(_serverACMTimeout, _serverACMClose, _serverACMHeartbeat);

            var properties = _com.Communicator.GetProperties();
            properties["Ice.ACM.Timeout"] = "2";
            if (_clientACMTimeout >= 0)
            {
                properties["Ice.ACM.Client.Timeout"] = _clientACMTimeout.ToString();
            }

            if (_clientACMClose >= 0)
            {
                properties["Ice.ACM.Client.Close"] = _clientACMClose.ToString();
            }

            if (_clientACMHeartbeat >= 0)
            {
                properties["Ice.ACM.Client.Heartbeat"] = _clientACMHeartbeat.ToString();
            }
            //initData.SetProperty("Ice.Trace.Protocol", "2");
            //initData.SetProperty("Ice.Trace.Network", "2");
            _communicator = _helper.initialize(properties);
            _thread = new Thread(this.run);
        }

        public void start() => _thread.Start();

        public void destroy()
        {
            _adapter.deactivate();
            _communicator.destroy();
        }

        public void join()
        {
            _output.Write("testing " + _name + "... ");
            _output.Flush();
            _logger.start();
            _thread.Join();
            if (_msg == null)
            {
                _output.WriteLine("ok");
            }
            else
            {
                _output.WriteLine("failed! " + _msg);
                throw new System.Exception();
            }
        }

        public void run()
        {
            var proxy = Test.ITestIntfPrx.Parse(_adapter.getTestIntf().ToString(), _communicator);
            try
            {
                proxy.GetConnection().SetCloseCallback(_ =>
                {
                    lock (this)
                    {
                        _closed = true;
                        Monitor.Pulse(this);
                    }
                });

                proxy.GetConnection().SetHeartbeatCallback(_ =>
                {
                    lock (this)
                    {
                        ++_heartbeat;
                    }
                });

                runTestCase(_adapter, proxy);
            }
            catch (Exception ex)
            {
                _msg = "unexpected exception:\n" + ex.ToString();
            }
        }

        public void waitForClosed()
        {
            lock (this)
            {
                long now = IceInternal.Time.currentMonotonicTimeMillis();
                while (!_closed)
                {
                    Monitor.Wait(this, 30000);
                    if (IceInternal.Time.currentMonotonicTimeMillis() - now > 30000)
                    {
                        System.Diagnostics.Debug.Assert(false); // Waited for more than 30s for close, something's wrong.
                        throw new System.Exception();
                    }
                }
            }
        }

        public abstract void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy);

        public void setClientACM(int timeout, int close, int heartbeat)
        {
            _clientACMTimeout = timeout;
            _clientACMClose = close;
            _clientACMHeartbeat = heartbeat;
        }

        public void setServerACM(int timeout, int close, int heartbeat)
        {
            _serverACMTimeout = timeout;
            _serverACMClose = close;
            _serverACMHeartbeat = heartbeat;
        }

        private string _name;
        private Test.IRemoteCommunicatorPrx _com;
        private string _msg;
        private LoggerI _logger;
        private global::Test.TestHelper _helper;
        private TextWriter _output;
        private Thread _thread;

        private Communicator _communicator;
        private Test.IRemoteObjectAdapterPrx _adapter;

        private int _clientACMTimeout;
        private int _clientACMClose;
        private int _clientACMHeartbeat;
        private int _serverACMTimeout;
        private int _serverACMClose;
        private int _serverACMHeartbeat;

        protected int _heartbeat;
        protected bool _closed;
    }

    public class AllTests : global::Test.AllTests
    {
        class InvocationHeartbeatTest : TestCase
        {
            public InvocationHeartbeatTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("invocation heartbeat", com, helper)
            {
                setServerACM(1, -1, -1); // Faster ACM to make sure we receive enough ACM heartbeats
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                proxy.sleep(4);

                lock (this)
                {
                    test(_heartbeat >= 4);
                }
            }
        }

        class InvocationHeartbeatOnHoldTest : TestCase
        {
            public InvocationHeartbeatOnHoldTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("invocation with heartbeat on hold", com, helper)
            {
                // Use default ACM configuration.
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                try
                {
                    // When the OA is put on hold, connections shouldn't
                    // send heartbeats, the invocation should therefore
                    // fail.
                    proxy.sleepAndHold(10);
                    test(false);
                }
                catch (Ice.ConnectionTimeoutException)
                {
                    adapter.activate();
                    proxy.interruptSleep();
                    waitForClosed();
                }
            }
        }

        class InvocationNoHeartbeatTest : TestCase
        {
            public InvocationNoHeartbeatTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("invocation with no heartbeat", com, helper)
            {
                setServerACM(2, 2, 0); // Disable heartbeat on invocations
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                try
                {
                    // Heartbeats are disabled on the server, the
                    // invocation should fail since heartbeats are
                    // expected.
                    proxy.sleep(10);
                    test(false);
                }
                catch (Ice.ConnectionTimeoutException)
                {
                    proxy.interruptSleep();

                    waitForClosed();
                    lock (this)
                    {
                        test(_heartbeat == 0);
                    }
                }
            }
        }

        class InvocationHeartbeatCloseOnIdleTest : TestCase
        {
            public InvocationHeartbeatCloseOnIdleTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("invocation with no heartbeat and close on idle", com, helper)
            {
                setClientACM(1, 1, 0); // Only close on idle.
                setServerACM(1, 2, 0); // Disable heartbeat on invocations
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                // No close on invocation, the call should succeed this
                // time.
                proxy.sleep(3);

                lock (this)
                {
                    test(_heartbeat == 0);
                    test(!_closed);
                }
            }
        }

        class CloseOnIdleTest : TestCase
        {
            public CloseOnIdleTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("close on idle", com, helper)
            {
                setClientACM(1, 1, 0); // Only close on idle
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                waitForClosed();
                lock (this)
                {
                    test(_heartbeat == 0);
                }
            }
        }

        class CloseOnInvocationTest : TestCase
        {
            public CloseOnInvocationTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("close on invocation", com, helper)
            {
                setClientACM(1, 2, 0); // Only close on invocation
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                Thread.Sleep(3000); // Idle for 3 seconds

                lock (this)
                {
                    test(_heartbeat == 0);
                    test(!_closed);
                }
            }
        }

        class CloseOnIdleAndInvocationTest : TestCase
        {
            public CloseOnIdleAndInvocationTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("close on idle and invocation", com, helper)
            {
                setClientACM(3, 3, 0); // Only close on idle and invocation
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                //
                // Put the adapter on hold. The server will not respond to
                // the graceful close. This allows to test whether or not
                // the close is graceful or forceful.
                //
                adapter.hold();
                Thread.Sleep(5000); // Idle for 5 seconds

                lock (this)
                {
                    test(_heartbeat == 0);
                    test(!_closed); // Not closed yet because of graceful close.
                }

                adapter.activate();
                waitForClosed();
            }
        }

        class ForcefulCloseOnIdleAndInvocationTest : TestCase
        {
            public ForcefulCloseOnIdleAndInvocationTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("forceful close on idle and invocation", com, helper)
            {
                setClientACM(1, 4, 0); // Only close on idle and invocation
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                adapter.hold();
                waitForClosed();
                lock (this)
                {
                    test(_heartbeat == 0);
                }
            }
        }

        class HeartbeatOnIdleTest : TestCase
        {
            public HeartbeatOnIdleTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("heartbeat on idle", com, helper)
            {
                setServerACM(1, -1, 2); // Enable server heartbeats.
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                Thread.Sleep(3000);

                lock (this)
                {
                    test(_heartbeat >= 3);
                }
            }
        }

        class HeartbeatAlwaysTest : TestCase
        {
            public HeartbeatAlwaysTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("heartbeat always", com, helper)
            {
                setServerACM(1, -1, 3); // Enable server heartbeats.
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                for (int i = 0; i < 10; i++)
                {
                    proxy.IcePing();
                    Thread.Sleep(300);
                }

                lock (this)
                {
                    test(_heartbeat >= 3);
                }
            }
        }

        class HeartbeatManualTest : TestCase
        {
            public HeartbeatManualTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("manual heartbeats", com, helper)
            {
                //
                // Disable heartbeats.
                //
                setClientACM(10, -1, 0);
                setServerACM(10, -1, 0);
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                proxy.startHeartbeatCount();
                Connection con = proxy.GetConnection();
                con.Heartbeat();
                con.Heartbeat();
                con.Heartbeat();
                con.Heartbeat();
                con.Heartbeat();
                proxy.waitForHeartbeatCount(5);
            }
        }

        class SetACMTest : TestCase
        {
            public SetACMTest(Test.IRemoteCommunicatorPrx com, global::Test.TestHelper helper) :
                base("setACM/getACM", com, helper)
            {
                setClientACM(15, 4, 0);
            }

            public override void runTestCase(Test.IRemoteObjectAdapterPrx adapter, Test.ITestIntfPrx proxy)
            {
                Ice.Connection con = proxy.GetCachedConnection();

                try
                {
                    con.SetACM(-19, null, null);
                    test(false);
                }
                catch (ArgumentException)
                {
                }

                Ice.ACM acm = con.GetACM();
                test(acm.Timeout == 15);
                test(acm.Close == Ice.ACMClose.CloseOnIdleForceful);
                test(acm.Heartbeat == Ice.ACMHeartbeat.HeartbeatOff);

                con.SetACM(null, null, null);
                acm = con.GetACM();
                test(acm.Timeout == 15);
                test(acm.Close == Ice.ACMClose.CloseOnIdleForceful);
                test(acm.Heartbeat == Ice.ACMHeartbeat.HeartbeatOff);

                con.SetACM(1, Ice.ACMClose.CloseOnInvocationAndIdle, Ice.ACMHeartbeat.HeartbeatAlways);
                acm = con.GetACM();
                test(acm.Timeout == 1);
                test(acm.Close == Ice.ACMClose.CloseOnInvocationAndIdle);
                test(acm.Heartbeat == Ice.ACMHeartbeat.HeartbeatAlways);

                proxy.startHeartbeatCount();
                proxy.waitForHeartbeatCount(2);

                var t1 = new TaskCompletionSource<object>();
                con.SetCloseCallback(_ => { t1.SetResult(null); });

                con.Close(ConnectionClose.Gracefully);
                test(t1.Task.Result == null);

                try
                {
                    con.ThrowException();
                    test(false);
                }
                catch (Ice.ConnectionManuallyClosedException)
                {
                }

                var t2 = new TaskCompletionSource<object>();
                con.SetCloseCallback(_ => { t2.SetResult(null); });
                test(t2.Task.Result == null);

                con.SetHeartbeatCallback(_ => { test(false); });
            }
        }

        public static void allTests(global::Test.TestHelper helper)
        {
            Ice.Communicator communicator = helper.communicator();
            var com = Test.IRemoteCommunicatorPrx.Parse($"communicator:{helper.getTestEndpoint(0)}", communicator);

            var output = helper.getWriter();

            List<TestCase> tests = new List<TestCase>();

            tests.Add(new InvocationHeartbeatTest(com, helper));
            tests.Add(new InvocationHeartbeatOnHoldTest(com, helper));
            tests.Add(new InvocationNoHeartbeatTest(com, helper));
            tests.Add(new InvocationHeartbeatCloseOnIdleTest(com, helper));

            tests.Add(new CloseOnIdleTest(com, helper));
            tests.Add(new CloseOnInvocationTest(com, helper));
            tests.Add(new CloseOnIdleAndInvocationTest(com, helper));
            tests.Add(new ForcefulCloseOnIdleAndInvocationTest(com, helper));

            tests.Add(new HeartbeatOnIdleTest(com, helper));
            tests.Add(new HeartbeatAlwaysTest(com, helper));
            tests.Add(new HeartbeatManualTest(com, helper));
            tests.Add(new SetACMTest(com, helper));

            foreach (TestCase test in tests)
            {
                test.init();
            }
            foreach (TestCase test in tests)
            {
                test.start();
            }
            foreach (TestCase test in tests)
            {
                test.join();
            }
            foreach (TestCase test in tests)
            {
                test.destroy();
            }

            output.Write("shutting down... ");
            output.Flush();
            com.shutdown();
            output.WriteLine("ok");
        }
    }
}
