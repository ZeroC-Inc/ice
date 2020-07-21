//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Invoke
{
    public class AllTests
    {
        private const string _testString = "This is a test string";

        public static IMyClassPrx allTests(TestHelper helper)
        {
            Communicator? communicator = helper.Communicator();
            TestHelper.Assert(communicator != null);
            var cl = IMyClassPrx.Parse($"test:{helper.GetTestEndpoint(0)}", communicator);
            IMyClassPrx oneway = cl.Clone(oneway: true);

            System.IO.TextWriter output = helper.GetWriter();
            output.Write("testing Invoke... ");
            output.Flush();

            {
                var request = OutgoingRequestFrame.WithEmptyParamList(oneway, "opOneway", idempotent: false);

                // Whether the proxy is oneway or not does not matter for Invoke's oneway parameter.

                IncomingResponseFrame response = cl.Invoke(request, oneway: true);
                TestHelper.Assert(response.ReplyStatus == ReplyStatus.OK);

                response = cl.Invoke(request, oneway: false);
                TestHelper.Assert(response.ReplyStatus == ReplyStatus.UserException);

                response = oneway.Invoke(request, oneway: true);
                TestHelper.Assert(response.ReplyStatus == ReplyStatus.OK);

                response = oneway.Invoke(request, oneway: false);
                TestHelper.Assert(response.ReplyStatus == ReplyStatus.UserException);

                request = OutgoingRequestFrame.WithParamList(cl, "opString", idempotent: false,
                    format: null, context: null, _testString, OutputStream.IceWriterFromString);
                response = cl.Invoke(request);
                (string s1, string s2) = response.ReadReturnValue(communicator, istr =>
                    {
                        string s1 = istr.ReadString();
                        string s2 = istr.ReadString();
                        return (s1, s2);
                    });
                TestHelper.Assert(s1.Equals(_testString) && s2.Equals(_testString));
            }

            for (int i = 0; i < 2; ++i)
            {
                Dictionary<string, string>? ctx = null;
                if (i == 1)
                {
                    ctx = new Dictionary<string, string>
                    {
                        ["raise"] = ""
                    };
                }

                var request = OutgoingRequestFrame.WithEmptyParamList(cl, "opException", idempotent: false, context: ctx);
                IncomingResponseFrame response = cl.Invoke(request);
                try
                {
                    response.ReadVoidReturnValue(communicator);
                }
                catch (MyException)
                {
                    // expected
                }
                catch (System.Exception)
                {
                    TestHelper.Assert(false);
                }
            }

            output.WriteLine("ok");

            output.Write("testing InvokeAsync... ");
            output.Flush();

            {
                var request = OutgoingRequestFrame.WithEmptyParamList(oneway, "opOneway", idempotent: false);
                IncomingResponseFrame response;
                try
                {
                    response = oneway.InvokeAsync(request, oneway: true).AsTask().Result;
                }
                catch (System.Exception)
                {
                    TestHelper.Assert(false);
                }

                request = OutgoingRequestFrame.WithParamList(cl, "opString", idempotent: false,
                    format: null, context: null, _testString, OutputStream.IceWriterFromString);

                response = cl.InvokeAsync(request).AsTask().Result;
                (string s1, string s2) = response.ReadReturnValue(communicator, istr =>
                    {
                        string s1 = istr.ReadString();
                        string s2 = istr.ReadString();
                        return (s1, s2);
                    });
                TestHelper.Assert(s1.Equals(_testString));
                TestHelper.Assert(s2.Equals(_testString));
            }

            {
                var request = OutgoingRequestFrame.WithEmptyParamList(cl, "opException", idempotent: false);
                IncomingResponseFrame response = cl.InvokeAsync(request).AsTask().Result;

                try
                {
                    response.ReadVoidReturnValue(communicator);
                    TestHelper.Assert(false);
                }
                catch (MyException)
                {
                }
                catch (System.Exception)
                {
                    TestHelper.Assert(false);
                }
            }

            output.WriteLine("ok");
            return cl;
        }
    }
}
