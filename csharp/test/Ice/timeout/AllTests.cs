// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Timeout
{
    public class AllTests
    {
        public static void Run(TestHelper helper)
        {
            Communicator? communicator = helper.Communicator;
            TestHelper.Assert(communicator != null);
            var controller = IControllerPrx.Parse(helper.GetTestProxy("controller", 1), communicator);
            try
            {
                RunWithController(helper, controller);
            }
            catch
            {
                // Ensure the adapter is not in the holding state when an unexpected exception occurs to prevent
                // the test from hanging on exit in case a connection which disables timeouts is still opened.
                controller.ResumeAdapter();
                throw;
            }
        }

        public static void RunWithController(TestHelper helper, IControllerPrx controller)
        {
            Communicator? communicator = helper.Communicator;
            TestHelper.Assert(communicator != null);
            bool ice1 = TestHelper.GetTestProtocol(communicator.GetProperties()) == Protocol.Ice1;

            var timeout = ITimeoutPrx.Parse(helper.GetTestProxy("timeout", 0), communicator);

            System.IO.TextWriter output = helper.Output;
            output.Write("testing connect timeout... ");
            output.Flush();
            {
                Dictionary<string, string>? properties = communicator.GetProperties();
                properties["Ice.ConnectTimeout"] = "100ms";
                using var comm = new Communicator(properties);

                var to = ITimeoutPrx.Parse(helper.GetTestProxy("timeout", 0), comm);

                // Expect ConnectTimeoutException.
                controller.HoldAdapter(-1);
                try
                {
                    to.Op();
                    TestHelper.Assert(false);
                }
                catch (ConnectTimeoutException)
                {
                    // Expected.
                }
                controller.ResumeAdapter();
                timeout.Op(); // Ensure adapter is active.
            }
            {
                // Expect success.
                controller.HoldAdapter(100);
                timeout.Op();
            }
            output.WriteLine("ok");

            // The sequence needs to be large enough to fill the write/recv buffers
            byte[] seq = new byte[2000000];

            output.Write("testing connection timeout... ");
            output.Flush();
            {
                // Expect TimeoutException.
                controller.HoldAdapter(-1);
                timeout.GetConnection().Acm = new Acm(TimeSpan.FromMilliseconds(50),
                                                      AcmClose.OnInvocationAndIdle,
                                                      AcmHeartbeat.Off);
                try
                {
                    timeout.SendData(seq);
                    TestHelper.Assert(false);
                }
                catch (ConnectionClosedException)
                {
                    // Expected.
                }
                controller.ResumeAdapter();
                timeout.Op(); // Ensure adapter is active.
            }
            {
                // Expect success.
                controller.HoldAdapter(100);
                try
                {
                    timeout.SendData(new byte[1000000]);
                }
                catch (ConnectionClosedException)
                {
                    TestHelper.Assert(false);
                }
            }
            output.WriteLine("ok");

            output.Write("testing invocation timeout... ");
            output.Flush();
            {
                timeout.IcePing(); // Makes sure a working connection is associated with the proxy
                Connection? connection = timeout.GetConnection();
                try
                {
                    timeout.Clone(invocationTimeout: TimeSpan.FromMilliseconds(100)).SleepAsync(1000).Wait();
                    TestHelper.Assert(false);
                }
                catch (AggregateException ex) when (ex.InnerException is OperationCanceledException)
                {
                }
                timeout.IcePing();

                TestHelper.Assert(connection == timeout.GetConnection());
                try
                {
                    timeout.Clone(invocationTimeout: TimeSpan.FromMilliseconds(1000)).SleepAsync(100).Wait();
                }
                catch (AggregateException ex) when (ex.InnerException is OperationCanceledException)
                {
                    TestHelper.Assert(false);
                }
                TestHelper.Assert(connection == timeout.GetConnection());
            }
            output.WriteLine("ok");

            output.Write("testing close timeout... ");
            output.Flush();
            {
                Dictionary<string, string> properties = communicator.GetProperties();
                properties["Ice.CloseTimeout"] = "100ms";
                using var comm = new Communicator(properties);

                var to = ITimeoutPrx.Parse(helper.GetTestProxy("timeout", 0), comm);

                Connection? connection = to.GetConnection();
                Connection? connection2 = timeout.GetConnection(); // No close timeout

                TestHelper.Assert(connection != null && connection2 != null);

                controller.HoldAdapter(-1);

                // Make sure there's no ReadAsync pending
                _ = to.IcePingAsync();
                _ = timeout.IcePingAsync();

                var semaphore = new System.Threading.SemaphoreSlim(0);
                connection.Closed += (sender, args) => semaphore.Release();
                connection.GoAwayAsync();
                TestHelper.Assert(semaphore.Wait(500));

                connection2.Closed += (sender, args) => semaphore.Release();
                connection2.GoAwayAsync();
                TestHelper.Assert(!semaphore.Wait(500));

                controller.ResumeAdapter();
                timeout.Op(); // Ensure adapter is active.
            }
            output.WriteLine("ok");

            output.Write("testing invocation timeouts with collocated calls... ");
            output.Flush();
            {
                communicator.SetProperty("TimeoutCollocated.AdapterId", "timeoutAdapter");

                ObjectAdapter adapter = communicator.CreateObjectAdapter("TimeoutCollocated");
                adapter.Activate();

                ITimeoutPrx proxy = adapter.AddWithUUID(new Timeout(), ITimeoutPrx.Factory);
                try
                {
                    proxy.Clone(invocationTimeout: TimeSpan.FromMilliseconds(100)).SleepAsync(500).Wait();
                    TestHelper.Assert(false);
                }
                catch (AggregateException ex) when (ex.InnerException is OperationCanceledException)
                {
                }

                adapter.Dispose();
            }
            output.WriteLine("ok");

            if (!ice1)
            {
                output.Write("testing deadlines... ");
                output.Flush();
                {
                    var comm1 = new Communicator(
                        communicator.GetProperties(),
                        invocationInterceptors: new InvocationInterceptor[]
                        {
                            (target, request, next, cancel) =>
                            {
                                request.AddBinaryContextEntry(10, request.Deadline, (ostr, value) =>
                                {
                                    var deadline = (value - DateTime.UnixEpoch).TotalMilliseconds;
                                    ostr.WriteVarLong((long)deadline);
                                });
                                return next(target, request, cancel);
                            }
                        });

                    for (int i = 1000; i < 5000;)
                    {
                        i += 33;
                        ITimeoutPrx to = ITimeoutPrx.Parse(helper.GetTestProxy("timeout", 0), comm1).Clone(
                            invocationTimeout: TimeSpan.FromMilliseconds(i));
                        TestHelper.Assert(to.CheckDeadline());
                    }
                }
                output.WriteLine("ok");
            }
            controller.Shutdown();
        }
    }
}
