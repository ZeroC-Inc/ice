//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
using Ice.defaultServant.Test;

namespace Ice.defaultServant
{
    public class AllTests : global::Test.AllTests
    {
        public static void
        allTests(global::Test.TestHelper helper)
        {
            var output = helper.getWriter();
            Communicator communicator = helper.communicator();
            ObjectAdapter oa = communicator.createObjectAdapterWithEndpoints("MyOA", "tcp -h localhost");
            oa.Activate();

            var servantI = new MyObject();
            var servantT = default(MyObjectTraits);
            Disp servantD = (incoming, current) => servantT.Dispatch(servantI, incoming, current);

            //
            // Register default servant with category "foo"
            //
            oa.AddDefaultServant(servantD, "foo");

            //
            // Start test
            //
            output.Write("testing single category... ");
            output.Flush();

            Disp r = oa.FindDefaultServant("foo");
            test(r == servantD);

            r = oa.FindDefaultServant("bar");
            test(r == null);

            Ice.Identity identity = new Ice.Identity();
            identity.Category = "foo";

            string[] names = new string[] { "foo", "bar", "x", "y", "abcdefg" };

            IMyObjectPrx prx = null;
            for (int idx = 0; idx < 5; ++idx)
            {
                identity.Name = names[idx];
                prx = IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));
                prx.IcePing();
                test(prx.getName() == names[idx]);
            }

            identity.Name = "ObjectNotExist";
            prx = IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));
            try
            {
                prx.IcePing();
                test(false);
            }
            catch (ObjectNotExistException)
            {
                // Expected
            }

            try
            {
                prx.getName();
                test(false);
            }
            catch (ObjectNotExistException)
            {
                // Expected
            }

            identity.Name = "FacetNotExist";
            prx = IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));
            try
            {
                prx.IcePing();
                test(false);
            }
            catch (FacetNotExistException)
            {
                // Expected
            }

            try
            {
                prx.getName();
                test(false);
            }
            catch (FacetNotExistException)
            {
                // Expected
            }

            identity.Category = "bar";
            for (int idx = 0; idx < 5; idx++)
            {
                identity.Name = names[idx];
                prx = Test.IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));

                try
                {
                    prx.IcePing();
                    test(false);
                }
                catch (Ice.ObjectNotExistException)
                {
                    // Expected
                }

                try
                {
                    prx.getName();
                    test(false);
                }
                catch (Ice.ObjectNotExistException)
                {
                    // Expected
                }
            }

            oa.RemoveDefaultServant("foo");
            identity.Category = "foo";
            prx = IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));
            try
            {
                prx.IcePing();
            }
            catch (ObjectNotExistException)
            {
                // Expected
            }

            output.WriteLine("ok");

            output.Write("testing default category... ");
            output.Flush();

            oa.AddDefaultServant(servantD, "");

            r = oa.FindDefaultServant("bar");
            test(r == null);

            r = oa.FindDefaultServant("");
            test(r == servantD);

            for (int idx = 0; idx < 5; ++idx)
            {
                identity.Name = names[idx];
                prx = IMyObjectPrx.UncheckedCast(oa.CreateProxy(identity));
                prx.IcePing();
                test(prx.getName() == names[idx]);
            }

            output.WriteLine("ok");
        }
    }
}
