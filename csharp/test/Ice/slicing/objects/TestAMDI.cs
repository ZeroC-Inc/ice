//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Test;

public sealed class TestIntf : ITestIntf
{
    private static void test(bool b)
    {
        if (!b)
        {
            throw new Exception();
        }
    }

    public Task shutdownAsync(Ice.Current current)
    {
        current.Adapter.Communicator.Shutdown();
        return null;
    }

    public Task<Ice.AnyClass>
    SBaseAsObjectAsync(Ice.Current current) => Task.FromResult<Ice.AnyClass>(new SBase("SBase.sb"));

    public Task<SBase> SBaseAsSBaseAsync(Ice.Current current) => Task.FromResult(new SBase("SBase.sb"));

    public Task<SBase>
    SBSKnownDerivedAsSBaseAsync(Ice.Current current) =>
        Task.FromResult<SBase>(new SBSKnownDerived("SBSKnownDerived.sb", "SBSKnownDerived.sbskd"));

    public Task<SBSKnownDerived>
    SBSKnownDerivedAsSBSKnownDerivedAsync(Ice.Current current) =>
        Task.FromResult(new SBSKnownDerived("SBSKnownDerived.sb", "SBSKnownDerived.sbskd"));

    public Task<SBase>
    SBSUnknownDerivedAsSBaseAsync(Ice.Current current) =>
        Task.FromResult<SBase>(new SBSUnknownDerived("SBSUnknownDerived.sb", "SBSUnknownDerived.sbsud"));

    public Task<SBase>
    SBSUnknownDerivedAsSBaseCompactAsync(Ice.Current current) =>
        Task.FromResult<SBase>(new SBSUnknownDerived("SBSUnknownDerived.sb", "SBSUnknownDerived.sbsud"));

    public Task<Ice.AnyClass> SUnknownAsObjectAsync(Ice.Current current)
    {
        var su = new SUnknown("SUnknown.su", null);
        su.cycle = su;
        return Task.FromResult<Ice.AnyClass>(su);
    }

    public Task checkSUnknownAsync(Ice.AnyClass obj, Ice.Current current)
    {
        test(obj is SUnknown);
        SUnknown su = (SUnknown)obj;
        test(su.su.Equals("SUnknown.su"));
        return null;
    }

    public Task<B> oneElementCycleAsync(Ice.Current current)
    {
        B b = new B();
        b.sb = "B1.sb";
        b.pb = b;
        return Task.FromResult(b);
    }

    public Task<B> twoElementCycleAsync(Ice.Current current)
    {
        B b1 = new B();
        b1.sb = "B1.sb";
        B b2 = new B();
        b2.sb = "B2.sb";
        b2.pb = b1;
        b1.pb = b2;
        return Task.FromResult(b1);
    }

    public Task<B> D1AsBAsync(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult<B>(d1);
    }

    public Task<D1> D1AsD1Async(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult(d1);
    }

    public Task<B> D2AsBAsync(Ice.Current current)
    {
        D2 d2 = new D2();
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        D1 d1 = new D1();
        d1.pb = d2;
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        d1.pd1 = d2;
        d2.pb = d1;
        d2.pd2 = d1;
        return Task.FromResult<B>(d2);
    }

    public Task<ITestIntf.ParamTest1ReturnValue>
    paramTest1Async(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult(new ITestIntf.ParamTest1ReturnValue(d1, d2));
    }

    public Task<ITestIntf.ParamTest2ReturnValue>
    paramTest2Async(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult(new ITestIntf.ParamTest2ReturnValue(d2, d1));
    }

    public Task<ITestIntf.ParamTest3ReturnValue>
    paramTest3Async(Ice.Current current)
    {
        D2 d2 = new D2();
        d2.sb = "D2.sb (p1 1)";
        d2.pb = null;
        d2.sd2 = "D2.sd2 (p1 1)";

        D1 d1 = new D1();
        d1.sb = "D1.sb (p1 2)";
        d1.pb = null;
        d1.sd1 = "D1.sd2 (p1 2)";
        d1.pd1 = null;
        d2.pd2 = d1;

        D2 d4 = new D2();
        d4.sb = "D2.sb (p2 1)";
        d4.pb = null;
        d4.sd2 = "D2.sd2 (p2 1)";

        D1 d3 = new D1();
        d3.sb = "D1.sb (p2 2)";
        d3.pb = null;
        d3.sd1 = "D1.sd2 (p2 2)";
        d3.pd1 = null;
        d4.pd2 = d3;

        return Task.FromResult(new ITestIntf.ParamTest3ReturnValue(d3, d2, d4));
    }

    public Task<ITestIntf.ParamTest4ReturnValue>
    paramTest4Async(Ice.Current current)
    {
        D4 d4 = new D4();
        d4.sb = "D4.sb (1)";
        d4.pb = null;
        d4.p1 = new B();
        d4.p1.sb = "B.sb (1)";
        d4.p2 = new B();
        d4.p2.sb = "B.sb (2)";
        return Task.FromResult(new ITestIntf.ParamTest4ReturnValue(d4.p2, d4));
    }

    public Task<ITestIntf.ReturnTest1ReturnValue>
    returnTest1Async(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult(new ITestIntf.ReturnTest1ReturnValue(d2, d2, d1));
    }

    public Task<ITestIntf.ReturnTest2ReturnValue>
    returnTest2Async(Ice.Current current)
    {
        D1 d1 = new D1();
        d1.sb = "D1.sb";
        d1.sd1 = "D1.sd1";
        D2 d2 = new D2();
        d2.pb = d1;
        d2.sb = "D2.sb";
        d2.sd2 = "D2.sd2";
        d2.pd2 = d1;
        d1.pb = d2;
        d1.pd1 = d2;
        return Task.FromResult(new ITestIntf.ReturnTest2ReturnValue(d1, d1, d2));
    }

    public Task<B>
    returnTest3Async(B p1, B p2, Ice.Current current) => Task.FromResult(p1);

    public Task<SS3>
    sequenceTestAsync(SS1 p1, SS2 p2, Ice.Current current) => Task.FromResult(new SS3(p1, p2));

    public Task<ITestIntf.DictionaryTestReturnValue>
    dictionaryTestAsync(Dictionary<int, B> bin, Ice.Current current)
    {
        var bout = new Dictionary<int, B>();
        int i;
        for (i = 0; i < 10; ++i)
        {
            B b = bin[i];
            var d2 = new D2();
            d2.sb = b.sb;
            d2.pb = b.pb;
            d2.sd2 = "D2";
            d2.pd2 = d2;
            bout[i * 10] = d2;
        }
        var r = new Dictionary<int, B>();
        for (i = 0; i < 10; ++i)
        {
            string s = "D1." + (i * 20).ToString();
            var d1 = new D1();
            d1.sb = s;
            d1.pb = (i == 0 ? null : r[(i - 1) * 20]);
            d1.sd1 = s;
            d1.pd1 = d1;
            r[i * 20] = d1;
        }
        return Task.FromResult(new ITestIntf.DictionaryTestReturnValue(r, bout));
    }

    public Task<PBase>
    exchangePBaseAsync(PBase pb, Ice.Current current) => Task.FromResult(pb);

    public Task<Preserved>
    PBSUnknownAsPreservedAsync(Ice.Current current)
    {
        PSUnknown r = new PSUnknown();
        r.pi = 5;
        r.ps = "preserved";
        r.psu = "unknown";
        r.graph = null;
        r.cl = new MyClass(15);
        return Task.FromResult<Preserved>(r);
    }

    public Task
    checkPBSUnknownAsync(Preserved p, Ice.Current current)
    {
        var pu = p as PSUnknown;
        test(pu.pi == 5);
        test(pu.ps.Equals("preserved"));
        test(pu.psu.Equals("unknown"));
        test(pu.graph == null);
        test(pu.cl != null && pu.cl.i == 15);
        return Task.CompletedTask;
    }

    public Task<Preserved>
    PBSUnknownAsPreservedWithGraphAsync(Ice.Current current)
    {
        var r = new PSUnknown();
        r.pi = 5;
        r.ps = "preserved";
        r.psu = "unknown";
        r.graph = new PNode();
        r.graph.next = new PNode();
        r.graph.next.next = new PNode();
        r.graph.next.next.next = r.graph;
        return Task.FromResult<Preserved>(r);
    }

    public Task
    checkPBSUnknownWithGraphAsync(Preserved p, Ice.Current current)
    {
        test(p is PSUnknown);
        var pu = (PSUnknown)p;
        test(pu.pi == 5);
        test(pu.ps.Equals("preserved"));
        test(pu.psu.Equals("unknown"));
        test(pu.graph != pu.graph.next);
        test(pu.graph.next != pu.graph.next.next);
        test(pu.graph.next.next.next == pu.graph);
        return Task.CompletedTask;
    }

    public Task<Preserved>
    PBSUnknown2AsPreservedWithGraphAsync(Ice.Current current)
    {
        var r = new PSUnknown2();
        r.pi = 5;
        r.ps = "preserved";
        r.pb = r;
        return Task.FromResult<Preserved>(r);
    }

    public Task
    checkPBSUnknown2WithGraphAsync(Preserved p, Ice.Current current)
    {
        var pu = (PSUnknown2)p;
        test(pu.pi == 5);
        test(pu.ps.Equals("preserved"));
        test(pu.pb == pu);
        return Task.CompletedTask;
    }

    public Task<PNode>
    exchangePNodeAsync(PNode pn, Ice.Current current) => Task.FromResult(pn);

    public Task throwBaseAsBaseAsync(Ice.Current current)
    {
        var be = new BaseException();
        be.sbe = "sbe";
        be.pb = new B();
        be.pb.sb = "sb";
        be.pb.pb = be.pb;
        throw be;
    }

    public Task throwDerivedAsBaseAsync(Ice.Current current)
    {
        DerivedException de = new DerivedException();
        de.sbe = "sbe";
        de.pb = new B();
        de.pb.sb = "sb1";
        de.pb.pb = de.pb;
        de.sde = "sde1";
        de.pd1 = new D1();
        de.pd1.sb = "sb2";
        de.pd1.pb = de.pd1;
        de.pd1.sd1 = "sd2";
        de.pd1.pd1 = de.pd1;
        throw de;
    }

    public Task
    throwDerivedAsDerivedAsync(Ice.Current current)
    {
        var de = new DerivedException();
        de.sbe = "sbe";
        de.pb = new B();
        de.pb.sb = "sb1";
        de.pb.pb = de.pb;
        de.sde = "sde1";
        de.pd1 = new D1();
        de.pd1.sb = "sb2";
        de.pd1.pb = de.pd1;
        de.pd1.sd1 = "sd2";
        de.pd1.pd1 = de.pd1;
        throw de;
    }

    public Task throwUnknownDerivedAsBaseAsync(Ice.Current current)
    {
        var d2 = new D2();
        d2.sb = "sb d2";
        d2.pb = d2;
        d2.sd2 = "sd2 d2";
        d2.pd2 = d2;

        var ude = new UnknownDerivedException();
        ude.sbe = "sbe";
        ude.pb = d2;
        ude.sude = "sude";
        ude.pd2 = d2;

        throw ude;
    }

    public Task throwPreservedExceptionAsync(Ice.Current current)
    {
        var ue = new PSUnknownException();
        ue.p = new PSUnknown2();
        ue.p.pi = 5;
        ue.p.ps = "preserved";
        ue.p.pb = ue.p;

        throw ue;
    }

    public Task<Forward>
    useForwardAsync(Ice.Current current)
    {
        var f = new Forward();
        f.h = new Hidden();
        f.h.f = f;
        return Task.FromResult(f);
    }
}
