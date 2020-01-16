//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using Test;
using Ice;
public class AllTests : Test.AllTests
{
    private class Relay : IRelay
    {
        public void knownPreservedAsBase(Ice.Current current)
        {
            KnownPreservedDerived ex = new KnownPreservedDerived();
            ex.b = "base";
            ex.kp = "preserved";
            ex.kpd = "derived";
            throw ex;
        }

        public void knownPreservedAsKnownPreserved(Ice.Current current)
        {
            KnownPreservedDerived ex = new KnownPreservedDerived();
            ex.b = "base";
            ex.kp = "preserved";
            ex.kpd = "derived";
            throw ex;
        }

        public void unknownPreservedAsBase(Ice.Current current)
        {
            Preserved2 ex = new Preserved2();
            ex.b = "base";
            ex.kp = "preserved";
            ex.kpd = "derived";
            ex.p1 = new PreservedClass("bc", "pc");
            ex.p2 = ex.p1;
            throw ex;
        }

        public void unknownPreservedAsKnownPreserved(Ice.Current current)
        {
            Preserved2 ex = new Preserved2();
            ex.b = "base";
            ex.kp = "preserved";
            ex.kpd = "derived";
            ex.p1 = new PreservedClass("bc", "pc");
            ex.p2 = ex.p1;
            throw ex;
        }
    }

    public static ITestIntfPrx allTests(Test.TestHelper helper, bool collocated)
    {
        Communicator communicator = helper.communicator();
        var output = helper.getWriter();
        output.Write("testing stringToProxy... ");
        output.Flush();
        string @ref = "Test:" + helper.getTestEndpoint(0) + " -t 2000";
        var @base = IObjectPrx.Parse(@ref, communicator);
        test(@base != null);
        output.WriteLine("ok");

        output.Write("testing checked cast... ");
        output.Flush();
        ITestIntfPrx testPrx = ITestIntfPrx.CheckedCast(@base);
        test(testPrx != null);
        test(testPrx.Equals(@base));
        output.WriteLine("ok");

        output.Write("base... ");
        output.Flush();
        {
            try
            {
                testPrx.baseAsBase();
                test(false);
            }
            catch (Base b)
            {
                test(b.b.Equals("Base.b"));
                test(b.GetType().FullName.Equals("Test.Base"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.baseAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (Base b)
                {
                    test(b.b.Equals("Base.b"));
                    test(b.GetType().Name.Equals("Base"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown derived... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownDerivedAsBase();
                test(false);
            }
            catch (Base b)
            {
                test(b.b.Equals("UnknownDerived.b"));
                test(b.GetType().FullName.Equals("Test.Base"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown derived (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownDerivedAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (Base b)
                {
                    test(b.b.Equals("UnknownDerived.b"));
                    test(b.GetType().Name.Equals("Base"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known derived as base... ");
        output.Flush();
        {
            try
            {
                testPrx.knownDerivedAsBase();
                test(false);
            }
            catch (KnownDerived k)
            {
                test(k.b.Equals("KnownDerived.b"));
                test(k.kd.Equals("KnownDerived.kd"));
                test(k.GetType().FullName.Equals("Test.KnownDerived"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known derived as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownDerivedAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownDerived k)
                {
                    test(k.b.Equals("KnownDerived.b"));
                    test(k.kd.Equals("KnownDerived.kd"));
                    test(k.GetType().Name.Equals("KnownDerived"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known derived as derived... ");
        output.Flush();
        {
            try
            {
                testPrx.knownDerivedAsKnownDerived();
                test(false);
            }
            catch (KnownDerived k)
            {
                test(k.b.Equals("KnownDerived.b"));
                test(k.kd.Equals("KnownDerived.kd"));
                test(k.GetType().FullName.Equals("Test.KnownDerived"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known derived as derived (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownDerivedAsKnownDerivedAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownDerived k)
                {
                    test(k.b.Equals("KnownDerived.b"));
                    test(k.kd.Equals("KnownDerived.kd"));
                    test(k.GetType().Name.Equals("KnownDerived"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown intermediate as base... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownIntermediateAsBase();
                test(false);
            }
            catch (Base b)
            {
                test(b.b.Equals("UnknownIntermediate.b"));
                test(b.GetType().FullName.Equals("Test.Base"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown intermediate as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownIntermediateAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (Base b)
                {
                    test(b.b.Equals("UnknownIntermediate.b"));
                    test(b.GetType().Name.Equals("Base"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of known intermediate as base... ");
        output.Flush();
        {
            try
            {
                testPrx.knownIntermediateAsBase();
                test(false);
            }
            catch (KnownIntermediate ki)
            {
                test(ki.b.Equals("KnownIntermediate.b"));
                test(ki.ki.Equals("KnownIntermediate.ki"));
                test(ki.GetType().FullName.Equals("Test.KnownIntermediate"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of known intermediate as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownIntermediateAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownIntermediate ki)
                {
                    test(ki.b.Equals("KnownIntermediate.b"));
                    test(ki.ki.Equals("KnownIntermediate.ki"));
                    test(ki.GetType().Name.Equals("KnownIntermediate"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of known most derived as base... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsBase();
                test(false);
            }
            catch (KnownMostDerived kmd)
            {
                test(kmd.b.Equals("KnownMostDerived.b"));
                test(kmd.ki.Equals("KnownMostDerived.ki"));
                test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                test(kmd.GetType().FullName.Equals("Test.KnownMostDerived"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of known most derived as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownMostDerived kmd)
                {
                    test(kmd.b.Equals("KnownMostDerived.b"));
                    test(kmd.ki.Equals("KnownMostDerived.ki"));
                    test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                    test(kmd.GetType().Name.Equals("KnownMostDerived"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known intermediate as intermediate... ");
        output.Flush();
        {
            try
            {
                testPrx.knownIntermediateAsKnownIntermediate();
                test(false);
            }
            catch (KnownIntermediate ki)
            {
                test(ki.b.Equals("KnownIntermediate.b"));
                test(ki.ki.Equals("KnownIntermediate.ki"));
                test(ki.GetType().FullName.Equals("Test.KnownIntermediate"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known intermediate as intermediate (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownIntermediateAsKnownIntermediateAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownIntermediate ki)
                {
                    test(ki.b.Equals("KnownIntermediate.b"));
                    test(ki.ki.Equals("KnownIntermediate.ki"));
                    test(ki.GetType().Name.Equals("KnownIntermediate"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known most derived as intermediate... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsKnownIntermediate();
                test(false);
            }
            catch (KnownMostDerived kmd)
            {
                test(kmd.b.Equals("KnownMostDerived.b"));
                test(kmd.ki.Equals("KnownMostDerived.ki"));
                test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                test(kmd.GetType().FullName.Equals("Test.KnownMostDerived"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known most derived as intermediate (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsKnownIntermediateAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownMostDerived kmd)
                {
                    test(kmd.b.Equals("KnownMostDerived.b"));
                    test(kmd.ki.Equals("KnownMostDerived.ki"));
                    test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                    test(kmd.GetType().Name.Equals("KnownMostDerived"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known most derived as most derived... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsKnownMostDerived();
                test(false);
            }
            catch (KnownMostDerived kmd)
            {
                test(kmd.b.Equals("KnownMostDerived.b"));
                test(kmd.ki.Equals("KnownMostDerived.ki"));
                test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                test(kmd.GetType().FullName.Equals("Test.KnownMostDerived"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("non-slicing of known most derived as most derived (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.knownMostDerivedAsKnownMostDerivedAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownMostDerived kmd)
                {
                    test(kmd.b.Equals("KnownMostDerived.b"));
                    test(kmd.ki.Equals("KnownMostDerived.ki"));
                    test(kmd.kmd.Equals("KnownMostDerived.kmd"));
                    test(kmd.GetType().Name.Equals("KnownMostDerived"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, known intermediate as base... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived1AsBase();
                test(false);
            }
            catch (KnownIntermediate ki)
            {
                test(ki.b.Equals("UnknownMostDerived1.b"));
                test(ki.ki.Equals("UnknownMostDerived1.ki"));
                test(ki.GetType().FullName.Equals("Test.KnownIntermediate"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, known intermediate as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived1AsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownIntermediate ki)
                {
                    test(ki.b.Equals("UnknownMostDerived1.b"));
                    test(ki.ki.Equals("UnknownMostDerived1.ki"));
                    test(ki.GetType().Name.Equals("KnownIntermediate"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, known intermediate as intermediate... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived1AsKnownIntermediate();
                test(false);
            }
            catch (KnownIntermediate ki)
            {
                test(ki.b.Equals("UnknownMostDerived1.b"));
                test(ki.ki.Equals("UnknownMostDerived1.ki"));
                test(ki.GetType().FullName.Equals("Test.KnownIntermediate"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, known intermediate as intermediate (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived1AsKnownIntermediateAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (KnownIntermediate ki)
                {
                    test(ki.b.Equals("UnknownMostDerived1.b"));
                    test(ki.ki.Equals("UnknownMostDerived1.ki"));
                    test(ki.GetType().Name.Equals("KnownIntermediate"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, unknown intermediate thrown as base... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived2AsBase();
                test(false);
            }
            catch (Base b)
            {
                test(b.b.Equals("UnknownMostDerived2.b"));
                test(b.GetType().FullName.Equals("Test.Base"));
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("slicing of unknown most derived, unknown intermediate thrown as base (AMI)... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived2AsBaseAsync().Wait();
                test(false);
            }
            catch (AggregateException ae)
            {
                try
                {
                    throw ae.InnerException;
                }
                catch (Base b)
                {
                    test(b.b.Equals("UnknownMostDerived2.b"));
                    test(b.GetType().Name.Equals("Base"));
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }
        }
        output.WriteLine("ok");

        output.Write("unknown most derived in compact format... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownMostDerived2AsBaseCompact();
                test(false);
            }
            catch (Base)
            {
                test(false);
            }
            catch (Ice.UnknownUserException)
            {
                //
                // A MarshalException is raised for the compact format because the
                // most-derived type is unknown and the exception cannot be sliced.
                //
            }
            catch (Ice.OperationNotExistException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }
        }
        output.WriteLine("ok");

        output.Write("preserved exceptions... ");
        output.Flush();
        {
            try
            {
                testPrx.unknownPreservedAsBase();
                test(false);
            }
            catch (Base ex)
            {
                Ice.SlicedData slicedData = ex.ice_getSlicedData();
                test(slicedData != null);
                test(slicedData.slices.Length == 2);
                test(slicedData.slices[1].typeId.Equals("::Test::SPreserved1"));
                test(slicedData.slices[0].typeId.Equals("::Test::SPreserved2"));
            }

            try
            {
                testPrx.unknownPreservedAsKnownPreserved();
                test(false);
            }
            catch (KnownPreserved ex)
            {
                test(ex.kp.Equals("preserved"));
                Ice.SlicedData slicedData = ex.ice_getSlicedData();
                test(slicedData != null);
                test(slicedData.slices.Length == 2);
                test(slicedData.slices[1].typeId.Equals("::Test::SPreserved1"));
                test(slicedData.slices[0].typeId.Equals("::Test::SPreserved2"));
            }

            ObjectAdapter adapter = communicator.createObjectAdapter("");
            IRelayPrx relay = adapter.Add(new Relay());
            adapter.Activate();
            testPrx.GetConnection().SetAdapter(adapter);

            try
            {
                testPrx.relayKnownPreservedAsBase(relay);
                test(false);
            }
            catch (KnownPreservedDerived ex)
            {
                test(ex.b.Equals("base"));
                test(ex.kp.Equals("preserved"));
                test(ex.kpd.Equals("derived"));
            }
            catch (Ice.OperationNotExistException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                testPrx.relayKnownPreservedAsKnownPreserved(relay);
                test(false);
            }
            catch (KnownPreservedDerived ex)
            {
                test(ex.b.Equals("base"));
                test(ex.kp.Equals("preserved"));
                test(ex.kpd.Equals("derived"));
            }
            catch (Ice.OperationNotExistException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                testPrx.relayUnknownPreservedAsBase(relay);
                test(false);
            }
            catch (Preserved2 ex)
            {
                test(ex.b.Equals("base"));
                test(ex.kp.Equals("preserved"));
                test(ex.kpd.Equals("derived"));
                test(ex.p1.ice_id().Equals(PreservedClass.ice_staticId()));
                PreservedClass pc = ex.p1 as PreservedClass;
                test(pc.bc.Equals("bc"));
                test(pc.pc.Equals("pc"));
                test(ex.p2 == ex.p1);
            }
            catch (Ice.OperationNotExistException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                testPrx.relayUnknownPreservedAsKnownPreserved(relay);
                test(false);
            }
            catch (Preserved2 ex)
            {
                test(ex.b.Equals("base"));
                test(ex.kp.Equals("preserved"));
                test(ex.kpd.Equals("derived"));
                test(ex.p1.ice_id().Equals(PreservedClass.ice_staticId()));
                PreservedClass pc = ex.p1 as PreservedClass;
                test(pc.bc.Equals("bc"));
                test(pc.pc.Equals("pc"));
                test(ex.p2 == ex.p1);
            }
            catch (Ice.OperationNotExistException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            adapter.Destroy();
        }
        output.WriteLine("ok");

        return testPrx;
    }
}
