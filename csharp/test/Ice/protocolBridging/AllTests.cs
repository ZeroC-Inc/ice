//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Linq;
using Test;

namespace ZeroC.Ice.Test.ProtocolBridging
{
    public class AllTests
    {
        public static ITestIntfPrx Run(TestHelper helper)
        {
            Communicator communicator = helper.Communicator!;

            var forwardSamePrx = ITestIntfPrx.Parse(helper.GetTestProxy("ForwardSame", 0), communicator);
            var forwardOtherPrx = ITestIntfPrx.Parse(helper.GetTestProxy("ForwardOther", 0), communicator);

            // The proxies have the same protocol and encoding; the difference is what they forward to.
            TestHelper.Assert(forwardSamePrx.Protocol == forwardOtherPrx.Protocol);
            TestHelper.Assert(forwardSamePrx.Encoding == forwardOtherPrx.Encoding);

            ITestIntfPrx newPrx;

            System.IO.TextWriter output = helper.Writer;
            output.Write("testing forwarding with same protocol... ");
            output.Flush();
            newPrx = TestProxy(forwardSamePrx);
            TestHelper.Assert(newPrx.Protocol == forwardSamePrx.Protocol);
            TestHelper.Assert(newPrx.Encoding == forwardSamePrx.Encoding);
            _ = TestProxy(newPrx);
            output.WriteLine("ok");

            output.Write("testing forwarding with other protocol... ");
            output.Flush();
            newPrx = TestProxy(forwardOtherPrx);
            TestHelper.Assert(newPrx.Protocol != forwardOtherPrx.Protocol);
            TestHelper.Assert(newPrx.Encoding == forwardOtherPrx.Encoding); // encoding must remain the same
            _ = TestProxy(newPrx);
            output.WriteLine("ok");

            output.Write("testing forwarding with other protocol and other encoding... ");
            output.Flush();
            Encoding encoding = forwardOtherPrx.Encoding == Encoding.V1_1 ? Encoding.V2_0 : Encoding.V1_1;
            newPrx = TestProxy(forwardOtherPrx.Clone(encoding: encoding));
            TestHelper.Assert(newPrx.Protocol != forwardOtherPrx.Protocol);
            TestHelper.Assert(newPrx.Encoding == encoding);
            _ = TestProxy(newPrx);
            output.WriteLine("ok");

            return forwardSamePrx;
        }

        private static ITestIntfPrx TestProxy(ITestIntfPrx prx)
        {
            var ctx = new Dictionary<string, string>(prx.Context);
            ctx.Add("MyCtx", "hello");

            TestHelper.Assert(prx.Op(13, ctx) == 13);
            prx.OpVoid(ctx);

            (int v, string s) = prx.OpReturnOut(34);
            TestHelper.Assert(v == 34 && s == "value=34");

            prx.OpOneway(42);

            try
            {
                prx.OpMyError();
                TestHelper.Assert(false);
            }
            catch (MyError ex)
            {
                 TestHelper.Assert(ex.Number == 42);
            }

            try
            {
                prx.OpObjectNotExistException();
                TestHelper.Assert(false);
            }
            catch (ObjectNotExistException)
            {
            }

            return prx.OpNewProxy().Clone(context: new Dictionary<string, string> { { "Direct", "1" } });
        }
    }
}
