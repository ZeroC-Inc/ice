// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using Test;

namespace ZeroC.Ice.Test.FaultTolerance
{
    public class AllTests
    {
        private static void ExceptAbortI(Exception ex, TextWriter output)
        {
            try
            {
                throw ex;
            }
            catch (ConnectionLostException)
            {
            }
            catch (ConnectFailedException)
            {
            }
            catch (TransportException)
            {
            }
            catch
            {
                output.WriteLine(ex.ToString());
                TestHelper.Assert(false);
            }
        }

        public static void Run(TestHelper helper, List<int> ports)
        {
            Communicator? communicator = helper.Communicator;
            TestHelper.Assert(communicator != null);
            TextWriter output = helper.Output;
            output.Write("testing stringToProxy... ");
            output.Flush();

            // Build a multi-endpoint proxy by hand.
            // TODO: should the TestHelper help with that?
            string refString = helper.GetTestProxy("test", 0);
            if (ports.Count > 1)
            {
                var sb = new StringBuilder(refString);
                if (helper.Protocol == Protocol.Ice1)
                {
                    string transport = helper.Transport;
                    for (int i = 0; i < ports.Count; ++i)
                    {
                        sb.Append($": {transport} -h ");
                        sb.Append(helper.Host.Contains(":") ? $"\"{helper.Host}\"" : helper.Host);
                        sb.Append(" -p ");
                        sb.Append(helper.BasePort + ports[i]);
                    }
                }
                else
                {
                    sb.Append("?alt-endpoint=");
                    for (int i = 0; i < ports.Count; ++i)
                    {
                        if (i > 0)
                        {
                            sb.Append(',');
                        }
                        sb.Append(helper.Host.Contains(":") ? $"[{helper.Host}]" : helper.Host);
                        sb.Append(':');
                        sb.Append(helper.BasePort + ports[i]);
                    }
                }
                refString = sb.ToString();
            }

            var obj = ITestIntfPrx.Parse(refString, communicator);
            output.WriteLine("ok");

            int oldPid = 0;
            for (int i = 1, j = 0; i <= ports.Count; ++i, ++j)
            {

                output.Write($"testing server #{i}... ");
                output.Flush();
                int pid = obj.Pid();
                TestHelper.Assert(pid != oldPid);
                output.WriteLine("ok");
                oldPid = pid;

                using var cancel = new CancellationTokenSource(100);
                if (i % 2 == 0)
                {
                    output.Write($"shutting down server #{i}... ");
                    output.Flush();
                    obj.Shutdown(cancel: cancel.Token);
                    output.WriteLine("ok");
                }
                else
                {
                    output.Write($"aborting server #{i}... ");
                    output.Flush();
                    try
                    {
                        obj.Abort(cancel: cancel.Token);
                        TestHelper.Assert(false);
                    }
                    catch (ConnectionLostException)
                    {
                        output.WriteLine("ok");
                    }
                    catch (ConnectFailedException)
                    {
                        output.WriteLine("ok");
                    }
                    catch (TransportException)
                    {
                        output.WriteLine("ok");
                    }
                }
            }

            output.Write("testing whether all servers are gone... ");
            output.Flush();
            try
            {
                using var cancel = new CancellationTokenSource(100);
                obj.IcePing(cancel: cancel.Token);
                TestHelper.Assert(false);
            }
            catch
            {
                output.WriteLine("ok");
            }
        }
    }
}
