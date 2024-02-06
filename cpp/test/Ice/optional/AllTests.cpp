//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifdef _MSC_VER
#   pragma warning(disable:4244) // '=': conversion from x to y, possible loss of data
#endif

#include <Ice/Ice.h>
#include <TestHelper.h>
#include <Test.h>

using namespace std;
using namespace Test;

namespace
{

//
// Converts a vector to an "array range"
//
template<typename T> pair<const T*, const T*>
toArrayRange(const vector<T>& v)
{
    return make_pair(&v[0], &v[0] + v.size());
}

template<typename T> pair<const T*, const T*>
toArrayRange(const T* v, size_t sz)
{
    return make_pair(v, v + sz);
}

}

class TestObjectReader : public Ice::Value
{
public:
    virtual void _iceWrite(Ice::OutputStream*) const { }

    virtual void _iceRead(Ice::InputStream* in)
    {
        in->startValue();
        in->startSlice();
        in->endSlice();
        in->endValue(false);
    }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }
};

class BObjectReader : public Ice::Value
{
public:
    virtual void _iceWrite(Ice::OutputStream*) const { }

    virtual void _iceRead(Ice::InputStream* in)
    {
        in->startValue();
        // ::Test::B
        in->startSlice();
        Ice::Int v;
        in->read(v);
        in->endSlice();
        // ::Test::A
        in->startSlice();
        in->read(v);
        in->endSlice();
        in->endValue(false);
    }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }
};

class CObjectReader : public Ice::Value
{
public:
    virtual void _iceWrite(Ice::OutputStream*) const { }

    virtual void _iceRead(Ice::InputStream* in)
    {
        in->startValue();
        // ::Test::C
        in->startSlice();
        in->skipSlice();
        // ::Test::B
        in->startSlice();
        Ice::Int v;
        in->read(v);
        in->endSlice();
        // ::Test::A
        in->startSlice();
        in->read(v);
        in->endSlice();
        in->endValue(false);
    }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }
};

class DObjectWriter : public Ice::Value
{
public:

    virtual void _iceWrite(Ice::OutputStream* out) const
    {
        out->startValue(0);
        // ::Test::D
        out->startSlice("::Test::D", -1, false);
        string s = "test";
        out->write(s);
        optional<vector<string> > o;
        o = vector<string>();
        o->push_back("test1");
        o->push_back("test2");
        o->push_back("test3");
        o->push_back("test4");
        out->write(1, o);
        APtr a = make_shared<A>();
        a->mc = 18;
        out->write(1000, optional<APtr>(a));
        out->endSlice();
        // ::Test::B
        out->startSlice(B::ice_staticId(), -1, false);
        Ice::Int v = 14;
        out->write(v);
        out->endSlice();
        // ::Test::A
        out->startSlice(A::ice_staticId(), -1, true);
        out->write(v);
        out->endSlice();
        out->endValue();
    }

    virtual void _iceRead(Ice::InputStream*) { }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }
};

class DObjectReader : public Ice::Value
{
public:
    virtual void _iceWrite(Ice::OutputStream*) const { }

    virtual void _iceRead(Ice::InputStream* in)
    {
        in->startValue();
        // ::Test::D
        in->startSlice();
        string s;
        in->read(s);
        test(s == "test");
        optional<vector<string> > o;
        in->read(1, o);
        test(o && o->size() == 4 &&
             (*o)[0] == "test1" && (*o)[1] == "test2" && (*o)[2] == "test3" && (*o)[3] == "test4");
        in->read(1000, a);
        in->endSlice();
        // ::Test::B
        in->startSlice();
        Ice::Int v;
        in->read(v);
        in->endSlice();
        // ::Test::A
        in->startSlice();
        in->read(v);
        in->endSlice();
        in->endValue(false);
    }

    void check()
    {
        test((*a)->mc == 18);
    }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }

private:

    optional<APtr> a;
};

class FObjectReader : public Ice::Value
{
public:
    virtual void _iceWrite(Ice::OutputStream*) const { }

    virtual void _iceRead(Ice::InputStream* in)
    {
        _f = make_shared<F>();
        in->startValue();
        in->startSlice();
        // Don't read af on purpose
        //in.read(1, _f->af);
        in->endSlice();
        in->startSlice();
        in->read(_f->ae);
        in->endSlice();
        in->endValue(false);
    }

    FPtr
    getF()
    {
        return _f;
    }

protected:

    virtual std::shared_ptr<Value> _iceCloneImpl() const
    {
        assert(0); // not used
        return nullptr;
    }

private:

    FPtr _f;
};

class FactoryI
{
    bool _enabled;

public:

    FactoryI() : _enabled(false)
    {
    }

    shared_ptr<Ice::Value>
    create(const string& typeId)
    {
        if(!_enabled)
        {
            return 0;
        }

        if(typeId == "::Test::OneOptional")
        {
           return make_shared<TestObjectReader>();
        }
        else if(typeId == "::Test::MultiOptional")
        {
           return make_shared<TestObjectReader>();
        }
        else if(typeId == "::Test::B")
        {
           return make_shared<BObjectReader>();
        }
        else if(typeId == "::Test::C")
        {
           return make_shared<CObjectReader>();
        }
        else if(typeId == "::Test::D")
        {
           return make_shared<DObjectReader>();
        }
        else if(typeId == "::Test::F")
        {
           return make_shared<FObjectReader>();
        }

        return 0;
    }

    void
    setEnabled(bool enabled)
    {
        _enabled = enabled;
    }
};

InitialPrxPtr
allTests(Test::TestHelper* helper, bool)
{
    Ice::CommunicatorPtr communicator = helper->communicator();
    auto factory = make_shared<FactoryI>();

    communicator->getValueFactoryManager()->add([factory](const string& typeId)
                                                {
                                                    return factory->create(typeId);
                                                },
                                                "");

    cout << "testing stringToProxy... " << flush;
    string ref = "initial:" + helper->getTestEndpoint();
    Ice::ObjectPrxPtr base = communicator->stringToProxy(ref);
    test(base);
    cout << "ok" << endl;

    cout << "testing checked cast... " << flush;
    InitialPrxPtr initial = Ice::checkedCast<InitialPrx>(base);
    test(initial);

    test(targetEqualTo(initial, base));

    bool supportsCppStringView = initial->supportsCppStringView();
    cout << "ok" << endl;

    cout << "testing constructor, copy constructor, and assignment operator... " << flush;

    OneOptionalPtr oo1 = make_shared<OneOptional>();
    test(!oo1->a);
    oo1->a = 15;
    test(oo1->a && *oo1->a == 15);

    OneOptionalPtr oo2 = make_shared<OneOptional>(16);
    test(oo2->a && *oo2->a == 16);

    OneOptionalPtr oo3 = make_shared<OneOptional>(*oo2);
    test(oo3->a && *oo3->a == 16);

    *oo3 = *oo1;
    test(oo3->a && *oo3->a == 15);

    OneOptionalPtr oon = make_shared<OneOptional>(nullopt);
    test(!oon->a);

    MultiOptionalPtr mo1 = make_shared<MultiOptional>();
    mo1->a = static_cast<Ice::Byte>(15);
    mo1->b = true;
    mo1->c = static_cast<Ice::Short>(19);
    mo1->d = 78;
    mo1->e = 99;
    mo1->f = 5.5f;
    mo1->g = 1.0;
    mo1->h = string("test");
    mo1->i = MyEnum::MyEnumMember;
    mo1->j = Ice::uncheckedCast<MyInterfacePrx>(communicator->stringToProxy("test"));
    mo1->k = mo1;
    mo1->bs = ByteSeq();
    (*mo1->bs).push_back(5);
    mo1->ss = StringSeq();
    mo1->ss->push_back("test");
    mo1->ss->push_back("test2");
    mo1->iid = IntIntDict();
    (*mo1->iid)[4] = 3;
    mo1->sid = StringIntDict();
    (*mo1->sid)["test"] = 10;
    FixedStruct fs;
    fs.m = 78;
    mo1->fs = fs;
    VarStruct vs;
    vs.m = "hello";
    mo1->vs = vs;

    mo1->shs = ShortSeq();
    mo1->shs->push_back(1);
    mo1->es = MyEnumSeq();
    mo1->es->push_back(MyEnum::MyEnumMember);
    mo1->es->push_back(MyEnum::MyEnumMember);
    mo1->fss = FixedStructSeq();
    mo1->fss->push_back(fs);
    mo1->vss = VarStructSeq();
    mo1->vss->push_back(vs);
    mo1->oos = OneOptionalSeq();
    mo1->oos->push_back(oo1);
    mo1->mips = MyInterfacePrxSeq();
    mo1->mips->push_back(Ice::uncheckedCast<MyInterfacePrx>(communicator->stringToProxy("test")));

    mo1->ied = IntEnumDict();
    mo1->ied.value()[4] = MyEnum::MyEnumMember;
    mo1->ifsd = IntFixedStructDict();
    mo1->ifsd.value()[4] = fs;
    mo1->ivsd = IntVarStructDict();
    mo1->ivsd.value()[5] = vs;
    mo1->iood = IntOneOptionalDict();
    mo1->iood.value()[5] = make_shared<OneOptional>();
    mo1->iood.value()[5]->a = 15;
    mo1->imipd = IntMyInterfacePrxDict();
    mo1->imipd.value()[5] = Ice::uncheckedCast<MyInterfacePrx>(communicator->stringToProxy("test"));

    mo1->bos = BoolSeq();
    mo1->bos->push_back(false);
    mo1->bos->push_back(true);
    mo1->bos->push_back(false);

    MultiOptionalPtr mo2 = make_shared<MultiOptional>(*mo1);

    MultiOptionalPtr mo3 = make_shared<MultiOptional>();
    *mo3 = *mo2;

    test(mo3->a == static_cast<Ice::Byte>(15));
    test(mo3->b == true);
    test(mo3->c == static_cast<short>(19));
    test(mo3->d == 78);
    test(mo3->e == static_cast<Ice::Long>(99));
    test(mo3->f == 5.5f);
    test(mo3->g == 1.0);
    test(mo3->h == string("test"));
    test(mo3->i = MyEnum::MyEnumMember);
    test(mo3->j = Ice::uncheckedCast<MyInterfacePrx>(communicator->stringToProxy("test")));
    test(mo3->k == mo1);
    test(mo3->bs == mo1->bs);
    test(mo3->ss == mo1->ss);
    test(mo3->iid == mo1->iid);
    test(mo3->sid == mo1->sid);
    test(mo3->fs == mo1->fs);
    test(mo3->vs == mo1->vs);

    test(mo3->shs == mo1->shs);
    test(mo3->es == mo1->es);
    test(mo3->fss == mo1->fss);
    test(mo3->vss == mo1->vss);
    test(mo3->oos == mo1->oos);
    test(mo3->mips == mo1->mips);

    test(mo3->ied == mo1->ied);
    test(mo3->ifsd == mo1->ifsd);
    test(mo3->ivsd == mo1->ivsd);
    test(mo3->iood == mo1->iood);
    test(mo3->imipd == mo1->imipd);

    test(mo3->bos == mo1->bos);

    cout << "ok" << endl;

    cout << "testing comparison operators... " << flush;

    test(mo1->a == static_cast<Ice::Byte>(15) && static_cast<Ice::Byte>(15) == mo1->a &&
        mo1->a != static_cast<Ice::Byte>(16) && static_cast<Ice::Byte>(16) != mo1->a);
    test(mo1->a < static_cast<Ice::Byte>(16) && mo1->a > static_cast<Ice::Byte>(14) &&
        mo1->a <= static_cast<Ice::Byte>(15) && mo1->a >= static_cast<Ice::Byte>(15) &&
        mo1->a <= static_cast<Ice::Byte>(16) && mo1->a >= static_cast<Ice::Byte>(14));
    test(mo1->a > optional<Ice::Byte>() && optional<Ice::Byte>() < mo1->a);
    test(14 > optional<int>() && optional<int>() < 14);

    test(mo1->h == string("test") && string("test") == mo1->h && mo1->h != string("testa") && string("testa") != mo1->h);
    test(mo1->h < string("test1") && mo1->h > string("tesa") && mo1->h <= string("test"));
    test(mo1->h >= string("test") && mo1->h <= string("test1") && mo1->h >= string("tesa"));
    test(mo1->h > optional<string>() && optional<string>() < mo1->h);
    test(string("test1") > optional<string>() && optional<string>() < string("test1"));

    cout << "ok" << endl;

    cout << "testing marshalling... " << flush;
    OneOptionalPtr oo4 = ICE_DYNAMIC_CAST(OneOptional, initial->pingPong(make_shared<OneOptional>()));
    test(!oo4->a);

    OneOptionalPtr oo5 = ICE_DYNAMIC_CAST(OneOptional, initial->pingPong(oo1));
    test(oo1->a == oo5->a);

    MultiOptionalPtr mo4 = ICE_DYNAMIC_CAST(MultiOptional, initial->pingPong(make_shared<MultiOptional>()));
    test(!mo4->a);
    test(!mo4->b);
    test(!mo4->c);
    test(!mo4->d);
    test(!mo4->e);
    test(!mo4->f);
    test(!mo4->g);
    test(!mo4->h);
    test(!mo4->i);
    test(!mo4->j);
    test(!mo4->k);
    test(!mo4->bs);
    test(!mo4->ss);
    test(!mo4->iid);
    test(!mo4->sid);
    test(!mo4->fs);
    test(!mo4->vs);

    test(!mo4->shs);
    test(!mo4->es);
    test(!mo4->fss);
    test(!mo4->vss);
    test(!mo4->oos);
    test(!mo4->mips);

    test(!mo4->ied);
    test(!mo4->ifsd);
    test(!mo4->ivsd);
    test(!mo4->iood);
    test(!mo4->imipd);

    test(!mo4->bos);

    mo1->k = mo1;
    MultiOptionalPtr mo5 = ICE_DYNAMIC_CAST(MultiOptional, initial->pingPong(mo1));

    test(mo5->a == mo1->a);
    test(mo5->b == mo1->b);
    test(mo5->c == mo1->c);
    test(mo5->d == mo1->d);
    test(mo5->e == mo1->e);
    test(mo5->f == mo1->f);
    test(mo5->g == mo1->g);
    test(mo5->h == mo1->h);
    test(mo5->i == mo1->i);
    test(targetEqualTo(mo5->j.value(), mo1->j.value()));
    test(mo5->k == mo5->k);
    test(mo5->bs == mo1->bs);
    test(mo5->ss == mo1->ss);
    test(mo5->iid == mo1->iid);
    test(mo5->sid == mo1->sid);
    test(mo5->fs == mo1->fs);
    test(mo5->vs == mo1->vs);

    test(mo5->shs == mo1->shs);
    test(mo5->es == mo1->es);
    test(mo5->fss == mo1->fss);
    test(mo5->vss == mo1->vss);
    test(!mo5->oos->empty() && (*mo5->oos)[0]->a == oo1->a);

    test(mo5->mips.value().size() == mo1->mips.value().size());
    for(size_t i = 0; i< mo5->mips.value().size(); ++i)
    {
        test(targetEqualTo(mo5->mips.value()[i], mo1->mips.value()[i]));
    }

    test(mo5->ied == mo1->ied);
    test(mo5->ifsd == mo1->ifsd);
    test(mo5->ivsd == mo1->ivsd);
    test(!mo5->iood->empty() && (*mo5->iood)[5]->a == 15);

    test(mo5->imipd.value().size() == mo1->imipd.value().size());
    for(auto& v : mo5->imipd.value())
    {
        test(targetEqualTo(mo1->imipd.value()[v.first], v.second));
    }

    test(mo5->bos == mo1->bos);

    // Clear the first half of the optional parameters
    MultiOptionalPtr mo6 = make_shared<MultiOptional>(*mo5);
    mo6->a = nullopt;
    mo6->c = nullopt;
    mo6->e = nullopt;
    mo6->g = nullopt;
    mo6->i = nullopt;
    mo6->k = nullopt;
    mo6->ss = nullopt;
    mo6->sid = nullopt;
    mo6->vs = nullopt;

    mo6->es = nullopt;
    mo6->vss = nullopt;
    mo6->mips = nullopt;

    mo6->ied = nullopt;
    mo6->ivsd = nullopt;
    mo6->imipd = nullopt;

    MultiOptionalPtr mo7 = ICE_DYNAMIC_CAST(MultiOptional, initial->pingPong(mo6));
    test(!mo7->a);
    test(mo7->b == mo1->b);
    test(!mo7->c);
    test(mo7->d == mo1->d);
    test(!mo7->e);
    test(mo7->f == mo1->f);
    test(!mo7->g);
    test(mo7->h == mo1->h);
    test(!mo7->i);
    test(targetEqualTo(mo7->j.value(), mo1->j.value()));
    test(!mo7->k);
    test(mo7->bs == mo1->bs);
    test(!mo7->ss);
    test(mo7->iid == mo1->iid);
    test(!mo7->sid);
    test(mo7->fs == mo1->fs);
    test(!mo7->vs);

    test(mo7->shs == mo1->shs);
    test(!mo7->es);
    test(mo7->fss == mo1->fss);
    test(!mo7->vss);
    test(!mo7->oos->empty() && (*mo7->oos)[0]->a == oo1->a);
    test(!mo7->mips);

    test(!mo7->ied);
    test(mo7->ifsd == mo1->ifsd);
    test(!mo7->ivsd);
    test(!mo7->iood->empty() && (*mo7->iood)[5]->a == 15);
    test(!mo7->imipd);

    // Clear the second half of the optional parameters
    MultiOptionalPtr mo8 = make_shared<MultiOptional>(*mo5);
    mo8->b = nullopt;
    mo8->d = nullopt;
    mo8->f = nullopt;
    mo8->h = nullopt;
    mo8->j = nullopt;
    mo8->bs = nullopt;
    mo8->iid = nullopt;
    mo8->fs = nullopt;

    mo8->shs = nullopt;
    mo8->fss = nullopt;
    mo8->oos = nullopt;

    mo8->ifsd = nullopt;
    mo8->iood = nullopt;

    mo8->k = mo8;
    MultiOptionalPtr mo9 = ICE_DYNAMIC_CAST(MultiOptional, initial->pingPong(mo8));
    test(mo9->a == mo1->a);
    test(!mo9->b);
    test(mo9->c == mo1->c);
    test(!mo9->d);
    test(mo9->e == mo1->e);
    test(!mo9->f);
    test(mo9->g == mo1->g);
    test(!mo9->h);
    test(mo9->i == mo1->i);
    test(!mo9->j);
    test(mo9->k == mo9);
    test(!mo9->bs);
    test(mo9->ss == mo1->ss);
    test(!mo9->iid);
    test(mo9->sid == mo1->sid);
    test(!mo9->fs);
    test(mo9->vs == mo1->vs);

    test(!mo8->shs);
    test(mo8->es == mo1->es);
    test(!mo8->fss);
    test(mo8->vss == mo1->vss);
    test(!mo8->oos);

    test(mo8->mips.value().size() == mo1->mips.value().size());
    for(size_t i = 0; i< mo8->mips.value().size(); ++i)
    {
        test(targetEqualTo(mo8->mips.value()[i], mo1->mips.value()[i]));
    }

    test(mo8->ied == mo1->ied);
    test(!mo8->ifsd);
    test(mo8->ivsd == mo1->ivsd);
    test(!mo8->iood);

    Ice::ByteSeq inEncaps;
    Ice::ByteSeq outEncaps;

    //
    // Send a request using blobjects. Upon receival, we don't read
    // any of the optional members. This ensures the optional members
    // are skipped even if the receiver knows nothing about them.
    //
    {
        factory->setEnabled(true);
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(oo1);
        out.endEncapsulation();
        out.finished(inEncaps);
        test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        Ice::ValuePtr obj;
        in.read(obj);
        in.endEncapsulation();
        test(obj && dynamic_cast<TestObjectReader*>(obj.get()));
    }

    {
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(mo1);
        out.endEncapsulation();
        out.finished(inEncaps);
        test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        Ice::ValuePtr obj;
        in.read(obj);
        in.endEncapsulation();
        test(obj && dynamic_cast<TestObjectReader*>(obj.get()));
        factory->setEnabled(false);
    }

    mo1->k = nullptr;
    mo2->k = nullptr;
    mo3->k = nullptr;
    mo4->k = nullptr;
    mo5->k = nullptr;
    mo6->k = nullptr;
    mo7->k = nullptr;
    mo8->k = nullptr;
    mo9->k = nullptr;

    //
    // Use the 1.0 encoding with operations whose only class parameters are optional.
    //
    optional<OneOptionalPtr> oo(make_shared<OneOptional>(53));
    initial->sendOptionalClass(true, oo);
    initial->ice_encodingVersion(Ice::Encoding_1_0)->sendOptionalClass(true, oo);

    initial->returnOptionalClass(true, oo);
    test(oo);
    initial->ice_encodingVersion(Ice::Encoding_1_0)->returnOptionalClass(true, oo);
    test(!oo);

    RecursiveSeq recursive1;
    recursive1.push_back(make_shared<Recursive>());
    RecursiveSeq recursive2;
    recursive2.push_back(make_shared<Recursive>());
    recursive1[0]->value = recursive2;
    RecursivePtr outer = make_shared<Recursive>();
    outer->value = recursive1;
    initial->pingPong(outer);

    GPtr g = make_shared<G>();
    g->gg1Opt = make_shared<G1>("gg1Opt");
    g->gg2 = make_shared<G2>(10);
    g->gg2Opt = make_shared<G2>(20);
    g->gg1 = make_shared<G1>("gg1");
    GPtr r = initial->opG(g);
    test("gg1Opt" == r->gg1Opt.value()->a);
    test(10 == r->gg2->a);
    test(20 == r->gg2Opt.value()->a);
    test("gg1" == r->gg1->a);

    initial->opVoid();

    {
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(1, optional<int>(15));
        out.write(2, optional<string>("test"));
        out.endEncapsulation();
        out.finished(inEncaps);
        test(initial->ice_invoke("opVoid", Ice::OperationMode::Normal, inEncaps, outEncaps));
    }

    cout << "ok" << endl;

    cout << "testing marshalling of large containers with fixed size elements..." << flush;
    MultiOptionalPtr mc = make_shared<MultiOptional>();

    ByteSeq byteSeq;
    byteSeq.resize(1000);
    mc->bs = byteSeq;

    ShortSeq shortSeq;
    shortSeq.resize(300);
    mc->shs = shortSeq;

    FixedStructSeq fsSeq;
    fsSeq.resize(300);
    mc->fss = fsSeq;

    IntFixedStructDict ifsd;
    for(int i = 0; i < 300; ++i)
    {
        ifsd.insert(make_pair(i, FixedStruct()));
    }
    mc->ifsd = ifsd;

    mc = ICE_DYNAMIC_CAST(MultiOptional, initial->pingPong(mc));
    test(mc->bs->size() == 1000);
    test(mc->shs->size() == 300);
    test(mc->fss->size() == 300);
    test(mc->ifsd->size() == 300);

    {
        factory->setEnabled(true);
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(mc);
        out.endEncapsulation();
        out.finished(inEncaps);
        test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        Ice::ValuePtr obj;
        in.read(obj);
        in.endEncapsulation();
        test(obj && dynamic_cast<TestObjectReader*>(obj.get()));
        factory->setEnabled(false);
    }

    cout << "ok" << endl;

    cout << "testing tag marshalling... " << flush;
    {
        BPtr b = make_shared<B>();
        BPtr b2 = ICE_DYNAMIC_CAST(B, initial->pingPong(b));
        test(!b2->ma);
        test(!b2->mb);
        test(!b2->mc);

        b->ma = 10;
        b->mb = 11;
        b->mc = 12;
        b->md = 13;

        b2 = ICE_DYNAMIC_CAST(B, initial->pingPong(b));
        test(b2->ma == 10);
        test(b2->mb == 11);
        test(b2->mc == 12);
        test(b2->md == 13);

        factory->setEnabled(true);
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(b);
        out.endEncapsulation();
        out.finished(inEncaps);
        test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        Ice::ValuePtr obj;
        in.read(obj);
        in.endEncapsulation();
        test(obj);
        factory->setEnabled(false);
    }

    cout << "ok" << endl;

    cout << "testing marshalling of objects with optional objects..." << flush;
    {
        FPtr f = make_shared<F>();

        f->af = make_shared<A>();
        f->ae = *f->af;

        FPtr rf = ICE_DYNAMIC_CAST(F, initial->pingPong(f));
        test(rf->ae == *rf->af);

        factory->setEnabled(true);
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(f);
        out.endEncapsulation();
        out.finished(inEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), inEncaps);
        in.startEncapsulation();
        Ice::ValuePtr obj;
        in.read(obj);
        in.endEncapsulation();
        factory->setEnabled(false);

        rf = dynamic_cast<FObjectReader*>(obj.get())->getF();
        test(rf->ae && !rf->af);
    }
    cout << "ok" << endl;

    cout << "testing optional with default values... " << flush;
    WDPtr wd = ICE_DYNAMIC_CAST(WD, initial->pingPong(make_shared<WD>()));
    test(*wd->a == 5);
    test(*wd->s == "test");
    wd->a = nullopt;
    wd->s = nullopt;
    wd = ICE_DYNAMIC_CAST(WD, initial->pingPong(wd));
    test(!wd->a);
    test(!wd->s);
    cout << "ok" << endl;

    if(communicator->getProperties()->getPropertyAsInt("Ice.Default.SlicedFormat") > 0)
    {
        cout << "testing marshalling with unknown class slices... " << flush;
        {
            CPtr c = make_shared<C>();
            c->ss = "test";
            c->ms = string("testms");

            {
                Ice::OutputStream out(communicator);
                out.startEncapsulation();
                out.write(c);
                out.endEncapsulation();
                out.finished(inEncaps);
                factory->setEnabled(true);
                test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
                Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
                in.startEncapsulation();
                Ice::ValuePtr obj;
                in.read(obj);
                in.endEncapsulation();
                test(dynamic_cast<CObjectReader*>(obj.get()));
                factory->setEnabled(false);
            }

            {
                factory->setEnabled(true);
                Ice::OutputStream out(communicator);
                out.startEncapsulation();
                Ice::ValuePtr d = make_shared<DObjectWriter>();
                out.write(d);
                out.endEncapsulation();
                out.finished(inEncaps);
                test(initial->ice_invoke("pingPong", Ice::OperationMode::Normal, inEncaps, outEncaps));
                Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
                in.startEncapsulation();
                Ice::ValuePtr obj;
                in.read(obj);
                in.endEncapsulation();
                test(obj && dynamic_cast<DObjectReader*>(obj.get()));
                dynamic_cast<DObjectReader*>(obj.get())->check();
                factory->setEnabled(false);
            }
        }
        cout << "ok" << endl;

        cout << "testing optionals with unknown classes..." << flush;
        {
            APtr a = make_shared<A>();

            Ice::OutputStream out(communicator);
            out.startEncapsulation();
            out.write(a);
            out.write(1, make_optional(make_shared<DObjectWriter>()));
            out.endEncapsulation();
            out.finished(inEncaps);
            test(initial->ice_invoke("opClassAndUnknownOptional", Ice::OperationMode::Normal, inEncaps, outEncaps));

            Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
            in.startEncapsulation();
            in.endEncapsulation();
        }
        cout << "ok" << endl;
    }

    cout << "testing optional parameters... " << flush;
    {
        optional<Ice::Byte> p1;
        optional<Ice::Byte> p3;
        optional<Ice::Byte> p2 = initial->opByte(p1, p3);
        test(!p2 && !p3);

        const Ice::Byte bval = 56;

        p1 = bval;
        p2 = initial->opByte(p1, p3);
        test(p2 == bval && p3 == bval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opByte", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);

        optional<Ice::Byte> p4 = static_cast<Ice::Byte>(0x08);
        in.read(89, p4);

        in.endEncapsulation();
        test(p2 == bval && p3 == bval && !p4);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<bool> p1;
        optional<bool> p3;
        optional<bool> p2 = initial->opBool(p1, p3);
        test(!p2 && !p3);

        p1 = true;
        p2 = initial->opBool(p1, p3);
        test(*p2 == true && *p3 == true);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opBool", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(*p2 == true && *p3 == true);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Ice::Short> p1;
        optional<Ice::Short> p3;
        optional<Ice::Short> p2 = initial->opShort(p1, p3);
        test(!p2 && !p3);

        const Ice::Short sval = 56;

        p1 = sval;
        p2 = initial->opShort(p1, p3);
        test(p2 == sval && p3 == sval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opShort", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == sval && p3 == sval);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Ice::Int> p1;
        optional<Ice::Int> p3;
        optional<Ice::Int> p2 = initial->opInt(p1, p3);
        test(!p2 && !p3);

        const Ice::Int ival = 56;

        p1 = ival;
        p2 = initial->opInt(p1, p3);
        test(p2 == 56 && p3 == 56);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opInt", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == ival && p3 == ival);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Ice::Long> p1;
        optional<Ice::Long> p3;
        optional<Ice::Long> p2 = initial->opLong(p1, p3);
        test(!p2 && !p3);

        const Ice::Long lval = 56;

        p1 = lval;
        p2 = initial->opLong(p1, p3);
        test(p2 == lval && p3 == lval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(1, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opLong", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(2, p3);
        in.read(3, p2);
        in.endEncapsulation();
        test(p2 == lval && p3 == lval);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Ice::Float> p1;
        optional<Ice::Float> p3;
        optional<Ice::Float> p2 = initial->opFloat(p1, p3);
        test(!p2 && !p3);

        const Ice::Float fval = 1.0f;

        p1 = fval;
        p2 = initial->opFloat(p1, p3);
        test(p2 == fval && p3 == fval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opFloat", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == fval && p3 == fval);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Ice::Double> p1;
        optional<Ice::Double> p3;
        optional<Ice::Double> p2 = initial->opDouble(p1, p3);
        test(!p2 && !p3);

        const Ice::Double dval = 1.0;

        p1 = dval;
        p2 = initial->opDouble(p1, p3);
        test(p2 == dval && p3 == dval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opDouble", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == dval && p3 == dval);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<string> p1;
        optional<string> p3;
        optional<string> p2 = initial->opString(p1, p3);
        test(!p2 && !p3);

        const string sval = "test";

        p1 = sval;
        p2 = initial->opString(sval, p3);
        test(p2 == sval && p3 == sval);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opString", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == sval && p3 == sval);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        if(supportsCppStringView)
        {
            optional<Util::string_view> p1;
            optional<string> p3;
            optional<string> p2 = initial->opCustomString(p1, p3);
            test(!p2 && !p3);

            const string sval = "test";

            p1 = Util::string_view(sval);
            p2 = initial->opString(sval, p3);
            test(p2 == sval && p3 == sval);

            Ice::OutputStream out(communicator);
            out.startEncapsulation();
            out.write(2, p1);
            out.endEncapsulation();
            out.finished(inEncaps);
            initial->ice_invoke("opCustomString", Ice::OperationMode::Normal, inEncaps, outEncaps);
            Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
            in.startEncapsulation();
            in.read(1, p2);
            in.read(3, p3);
            in.endEncapsulation();
            test(p2 == sval && p3 == sval);

            Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
            in2.startEncapsulation();
            in2.endEncapsulation();
        }
    }

    {
        optional<Test::MyEnum> p1;
        optional<Test::MyEnum> p3;
        optional<Test::MyEnum> p2 = initial->opMyEnum(p1, p3);
        test(!p2 && !p3);

        p1 = MyEnum::MyEnumMember;
        p2 = initial->opMyEnum(p1, p3);
        test(p2 == MyEnum::MyEnumMember && p3 == MyEnum::MyEnumMember);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opMyEnum", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == MyEnum::MyEnumMember && p3 == MyEnum::MyEnumMember);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Test::SmallStruct> p1;
        optional<Test::SmallStruct> p3;
        optional<Test::SmallStruct> p2 = initial->opSmallStruct(p1, p3);
        test(!p2 && !p3);

        p1 = Test::SmallStruct();
        p1->m = 56;
        p2 = initial->opSmallStruct(p1, p3);
        test(p2->m == 56 && p3->m == 56);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opSmallStruct", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2->m == 56 && p3->m == 56);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Test::FixedStruct> p1;
        optional<Test::FixedStruct> p3;
        optional<Test::FixedStruct> p2 = initial->opFixedStruct(p1, p3);
        test(!p2 && !p3);

        p1 = Test::FixedStruct();
        p1->m = 56;
        p2 = initial->opFixedStruct(p1, p3);
        test(p2->m == 56 && p3->m == 56);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opFixedStruct", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2->m == 56 && p3->m == 56);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<Test::VarStruct> p1;
        optional<Test::VarStruct> p3;
        optional<Test::VarStruct> p2 = initial->opVarStruct(p1, p3);
        test(!p2 && !p3);

        p1 = Test::VarStruct();
        p1->m = "test";
        p2 = initial->opVarStruct(p1, p3);
        test(p2->m == "test" && p3->m == "test");

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opVarStruct", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2->m == "test" && p3->m == "test");

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<OneOptionalPtr> p1;
        optional<OneOptionalPtr> p3;
        optional<OneOptionalPtr> p2 = initial->opOneOptional(p1, p3);
        test(!p2 && !p3);

        if(initial->supportsNullOptional())
        {
            p2 = initial->opOneOptional(OneOptionalPtr(), p3);
            test(*p2 == nullptr && *p3 == nullptr);
        }

        p1 = make_shared<OneOptional>(58);
        p2 = initial->opOneOptional(p1, p3);
        test((*p2)->a == 58 && (*p3)->a == 58);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opOneOptional", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test((*p2)->a == 58 && (*p3)->a == 58);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<MyInterfacePrxPtr> p1;
        optional<MyInterfacePrxPtr> p3;
        optional<MyInterfacePrxPtr> p2 = initial->opMyInterfaceProxy(p1, p3);
        test(!p2 && !p3);

        p1 = Ice::uncheckedCast<MyInterfacePrx>(communicator->stringToProxy("test"));
        p2 = initial->opMyInterfaceProxy(p1, p3);

        test(targetEqualTo(p2.value(), p1.value()) && targetEqualTo(p3.value(), p1.value()));

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opMyInterfaceProxy", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();

        test(targetEqualTo(p2.value(), p1.value()) && targetEqualTo(p3.value(), p1.value()));

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        FPtr f = make_shared<F>();
        f->af = make_shared<A>();
        (*f->af)->requiredA = 56;
        f->ae = *f->af;

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(1, make_optional(f));
        out.write(2, make_optional(f->ae));
        out.endEncapsulation();
        out.finished(inEncaps);

        Ice::InputStream in(communicator, out.getEncoding(), inEncaps);
        in.startEncapsulation();
        optional<APtr> a;
        in.read(2, a);
        in.endEncapsulation();
        test(a && *a && (*a)->requiredA == 56);
    }
    cout << "ok" << endl;

    cout << "testing optional parameters and custom sequences... " << flush;
    {
        optional<std::pair<const Ice::Byte*, const Ice::Byte*> > p1;
        optional<ByteSeq> p3;
        optional<ByteSeq> p2 = initial->opByteSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Byte> bs(100);
        fill(bs.begin(), bs.end(), 56);
        p1 = toArrayRange(bs);
        p2 = initial->opByteSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opByteSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const bool*, const bool*> > p1;
        optional<BoolSeq> p3;
        optional<BoolSeq> p2 = initial->opBoolSeq(p1, p3);
        test(!p2 && !p3);

        bool bs[100];
        fill(&bs[0], &bs[0] + 100, true);
        vector<bool> bsv(&bs[0], &bs[0] + 100);
        p1 = toArrayRange(bs, 100);
        p2 = initial->opBoolSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bsv && p3 == bsv);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opBoolSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bsv && p3 == bsv);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const Ice::Short*, const Ice::Short*> > p1;
        optional<ShortSeq> p3;
        optional<ShortSeq> p2 = initial->opShortSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Short> bs(100);
        fill(bs.begin(), bs.end(), 56);
        p1 = toArrayRange(bs);
        p2 = initial->opShortSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opShortSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const Ice::Int*, const Ice::Int*> > p1;
        optional<IntSeq> p3;
        optional<IntSeq> p2 = initial->opIntSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Int> bs(100);
        fill(bs.begin(), bs.end(), 56);
        p1 = toArrayRange(bs);
        p2 = initial->opIntSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opIntSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const Ice::Long*, const Ice::Long*> > p1;
        optional<LongSeq> p3;
        optional<LongSeq> p2 = initial->opLongSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Long> bs(100);
        fill(bs.begin(), bs.end(), 56);
        p1 = toArrayRange(bs);
        p2 = initial->opLongSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opLongSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const Ice::Float*, const Ice::Float*> > p1;
        optional<FloatSeq> p3;
        optional<FloatSeq> p2 = initial->opFloatSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Float> bs(100);
        fill(bs.begin(), bs.end(), 1.0f);
        p1 = toArrayRange(bs);
        p2 = initial->opFloatSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opFloatSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const Ice::Double*, const Ice::Double*> > p1;
        optional<DoubleSeq> p3;
        optional<DoubleSeq> p2 = initial->opDoubleSeq(p1, p3);
        test(!p2 && !p3);

        vector<Ice::Double> bs(100);
        fill(bs.begin(), bs.end(), 1.0);
        p1 = toArrayRange(bs);
        p2 = initial->opDoubleSeq(p1, p3);
        test(p2 && p3);
        test(p2 == bs && p3 == bs);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opDoubleSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == bs && p3 == bs);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<std::pair<const FixedStruct*, const FixedStruct*> > p1;
        optional<FixedStructSeq> p3;
        optional<FixedStructSeq> p2 = initial->opFixedStructSeq(p1, p3);
        test(!p2 && !p3);

        std::pair<const FixedStruct*, const FixedStruct*> p;
        p.first = p.second = 0;
        p2 = initial->opFixedStructSeq(p, p3);
        test(p2 && p3 && p2.value().empty() && p3.value().empty());

        FixedStruct fss[10];
        fss[0].m = 1;
        fss[1].m = 2;
        fss[2].m = 3;
        fss[3].m = 4;
        fss[4].m = 5;
        fss[5].m = 6;
        fss[6].m = 7;
        fss[7].m = 8;
        fss[8].m = 9;
        fss[9].m = 10;
        vector<FixedStruct> fssv(&fss[0], &fss[0] + 10);
        p1 = toArrayRange(fss, 10);
        p2 = initial->opFixedStructSeq(p1, p3);
        test(p2 && p3);
        test(p2 == fssv && p3 == fssv);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opFixedStructSeq", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == fssv && p3 == fssv);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }
    cout << "ok" << endl;

    cout << "testing optional parameters and dictionaries... " << flush;
    {
        optional<IntIntDict> p1;
        optional<IntIntDict> p3;
        optional<IntIntDict> p2 = initial->opIntIntDict(p1, p3);
        test(!p2 && !p3);

        IntIntDict ss;
        ss.insert(make_pair(1, 1));
        p1 = ss;
        p2 = initial->opIntIntDict(p1, p3);
        test(p2 && p3);
        test(p2 == ss && p3 == ss);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opIntIntDict", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == ss && p3 == ss);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<StringIntDict> p1;
        optional<StringIntDict> p3;
        optional<StringIntDict> p2 = initial->opStringIntDict(p1, p3);
        test(!p2 && !p3);

        StringIntDict ss;
        ss.insert(make_pair<string, int>("test", 1));
        p1 = ss;
        p2 = initial->opStringIntDict(p1, p3);
        test(p2 && p3);
        test(p2 == ss && p3 == ss);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opStringIntDict", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test(p2 == ss && p3 == ss);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        optional<IntOneOptionalDict> p1;
        optional<IntOneOptionalDict> p3;
        optional<IntOneOptionalDict> p2 = initial->opIntOneOptionalDict(p1, p3);
        test(!p2 && !p3);

        IntOneOptionalDict ss;
        ss.insert(make_pair<int, OneOptionalPtr>(1, make_shared<OneOptional>(58)));
        p1 = ss;
        p2 = initial->opIntOneOptionalDict(p1, p3);
        test(p2 && p3);
        test((*p2)[1]->a == 58 && (*p3)[1]->a == 58);

        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(2, p1);
        out.endEncapsulation();
        out.finished(inEncaps);
        initial->ice_invoke("opIntOneOptionalDict", Ice::OperationMode::Normal, inEncaps, outEncaps);
        Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
        in.startEncapsulation();
        in.read(1, p2);
        in.read(3, p3);
        in.endEncapsulation();
        test((*p2)[1]->a == 58 && (*p3)[1]->a == 58);

        Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
        in2.startEncapsulation();
        in2.endEncapsulation();
    }

    {
        if(supportsCppStringView)
        {
            optional<std::map<int, Util::string_view> > p1;
            optional<IntStringDict> p3;
            optional<IntStringDict> p2 = initial->opCustomIntStringDict(p1, p3);
            test(!p2 && !p3);

            map<int, Util::string_view> ss;
            ss.insert(make_pair<int, Util::string_view>(5, "testing"));
            p1 = ss;
            p2 = initial->opCustomIntStringDict(p1, p3);
            test(p2 && p3);
            test(p2 == p3);
            test(p2->size() == p1->size());
            test((*p2)[5] == ss[5].to_string());

            Ice::OutputStream out(communicator);
            out.startEncapsulation();
            out.write(2, p1);
            out.endEncapsulation();
            out.finished(inEncaps);
            initial->ice_invoke("opCustomIntStringDict", Ice::OperationMode::Normal, inEncaps, outEncaps);
            Ice::InputStream in(communicator, out.getEncoding(), outEncaps);
            in.startEncapsulation();
            in.read(1, p2);
            in.read(3, p3);
            in.endEncapsulation();
            test(p2 && p3);
            test(p2 == p3);
            test(p2->size() == p1->size());
            test((*p2)[5] == ss[5].to_string());

            Ice::InputStream in2(communicator, out.getEncoding(), outEncaps);
            in2.startEncapsulation();
            in2.endEncapsulation();
        }
    }

    cout << "ok" << endl;

    cout << "testing exception optionals... " << flush;
    {
        try
        {
            initial->opOptionalException(nullopt, nullopt, nullopt);
            test(false);
        }
        catch(const OptionalException& ex)
        {
            test(!ex.a);
            test(!ex.b);
            test(!ex.o);
        }

        try
        {
            initial->opOptionalException(30, string("test"), make_shared<OneOptional>(53));
            test(false);
        }
        catch(const OptionalException& ex)
        {
            test(ex.a == 30);
            test(ex.b == string("test"));
            test((*ex.o)->a = 53);
        }

        try
        {
            //
            // Use the 1.0 encoding with an exception whose only class members are optional.
            //
            initial->ice_encodingVersion(Ice::Encoding_1_0)->
                opOptionalException(30, string("test"), make_shared<OneOptional>(53));
            test(false);
        }
        catch(const OptionalException& ex)
        {
            test(!ex.a);
            test(!ex.b);
            test(!ex.o);
        }

        try
        {
            optional<Ice::Int> a;
            optional<string> b;
            optional<OneOptionalPtr> o;
            initial->opDerivedException(a, b, o);
            test(false);
        }
        catch(const DerivedException& ex)
        {
            test(!ex.a);
            test(!ex.b);
            test(!ex.o);
            test(!ex.ss);
            test(!ex.o2);
            test(ex.d1 == "d1");
            test(ex.d2 == "d2");
        }
        catch(const OptionalException&)
        {
            test(false);
        }

        try
        {
            optional<Ice::Int> a = 30;
            optional<string> b = string("test2");
            optional<OneOptionalPtr> o = make_shared<OneOptional>(53);
            initial->opDerivedException(a, b, o);
            test(false);
        }
        catch(const DerivedException& ex)
        {
            test(ex.a == 30);
            test(ex.b == string("test2"));
            test((*ex.o)->a == 53);
            test(ex.ss == string("test2"));
            test((*ex.o2)->a == 53);
            test(ex.d1 == "d1");
            test(ex.d2 == "d2");
        }
        catch(const OptionalException&)
        {
            test(false);
        }

        try
        {
            optional<Ice::Int> a;
            optional<string> b;
            optional<OneOptionalPtr> o;
            initial->opRequiredException(a, b, o);
            test(false);
        }
        catch(const RequiredException& ex)
        {
            test(!ex.a);
            test(!ex.b);
            test(!ex.o);
            test(ex.ss == string("test"));
            test(!ex.o2);
        }
        catch(const OptionalException&)
        {
            test(false);
        }

        try
        {
            optional<Ice::Int> a = 30;
            optional<string> b = string("test2");
            optional<OneOptionalPtr> o = make_shared<OneOptional>(53);
            initial->opRequiredException(a, b, o);
            test(false);
        }
        catch(const RequiredException& ex)
        {
            test(ex.a == 30);
            test(ex.b == string("test2"));
            test((*ex.o)->a == 53);
            test(ex.ss == string("test2"));
            test(ex.o2->a == 53);
        }
        catch(const OptionalException&)
        {
            test(false);
        }
    }
    cout << "ok" << endl;

    cout << "testing optionals with marshaled results... " << flush;
    {
        test(initial->opMStruct1());
        test(initial->opMDict1());
        test(initial->opMSeq1());
        test(initial->opMG1());

        {
            optional<Test::SmallStruct> p1, p2, p3;
            p3 = initial->opMStruct2(nullopt, p2);
            test(!p2 && !p3);

            p1 = Test::SmallStruct();
            p3 = initial->opMStruct2(p1, p2);
            test(p2 == p1 && p3 == p1);
        }
        {
            optional<Test::StringSeq> p1, p2, p3;
            p3 = initial->opMSeq2(nullopt, p2);
            test(!p2 && !p3);

            Test::StringSeq seq;
            seq.push_back("hello");
            p1 = seq;
            p3 = initial->opMSeq2(p1, p2);
            test(p2 == p1 && p3 == p1);
        }
        {
            optional<Test::StringIntDict> p1, p2, p3;
            p3 = initial->opMDict2(nullopt, p2);
            test(!p2 && !p3);

            Test::StringIntDict dict;
            dict["test"] = 54;
            p1 = dict;
            p3 = initial->opMDict2(p1, p2);
            test(p2 == p1 && p3 == p1);
        }
        {
            optional<Test::GPtr> p1, p2, p3;
            p3 = initial->opMG2(nullopt, p2);
            test(!p2 && !p3);

            p1 = make_shared<Test::G>();
            p3 = initial->opMG2(p1, p2);
            test(p2 && p3 && *p3 == *p2);
        }
    }
    cout << "ok" << endl;
    return initial;
}
