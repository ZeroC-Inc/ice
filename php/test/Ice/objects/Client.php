<?php
//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

require_once('Test.php');
require_once('Forward.php');

class BI extends Test\B
{
    function ice_pretUnmarshal()
    {
        $this->preUnmarshalInvoked = true;
    }

    function ice_postUnmarshal()
    {
        $this->postUnmarshalInvoked = true;
    }
}

class CI extends Test\C
{
    function ice_preUnmarshal()
    {
        $this->preUnmarshalInvoked = true;
    }

    function ice_postUnmarshal()
    {
        $this->postUnmarshalInvoked = true;
    }
}

class DI extends Test\D
{
    function ice_preUnmarshal()
    {
        $this->preUnmarshalInvoked = true;
    }

    function ice_postUnmarshal()
    {
        $this->postUnmarshalInvoked = true;
    }
}

class EI extends Test\E
{
    function __construct()
    {
        $this->i = 1;
        $this->s = "hello";
    }

    function checkValues()
    {
        return $this->i == 1 && $this->s == "hello";
    }
}

class FI extends Test\F
{
    function __construct($e=null)
    {
        $this->e1 = $e;
        $this->e2 = $e;
    }

    function checkValues()
    {
        return $this->e1 != null && $this->e1 === $this->e2;
    }
}

class MyValueFactory implements Ice\ValueFactory
{
    function create($id)
    {
        if($id == "::Test::B")
        {
            return new BI();
        }
        else if($id == "::Test::C")
        {
            return new CI();
        }
        else if($id == "::Test::D")
        {
            return new DI();
        }
        else if($id == "::Test::E")
        {
            return new EI();
        }
        else if($id == "::Test::F")
        {
            return new FI();
        }
        return null;
    }
}

function allTests($helper)
{
    echo "testing stringToProxy... ";
    flush();
    $ref = sprintf("initial:%s", $helper->getTestEndpoint());
    $communicator = $helper->communicator();
    $base = $communicator->stringToProxy($ref);
    test($base != null);
    echo "ok\n";

    echo "testing checked cast... ";
    flush();
    $initial = $base->ice_checkedCast("::Test::Initial");
    test($initial != null);
    test($initial == $base);
    echo "ok\n";

    echo "getting B1... ";
    flush();
    $b1 = $initial->getB1();
    test($b1 != null);
    echo "ok\n";

    echo "getting B2... ";
    flush();
    $b2 = $initial->getB2();
    test($b2 != null);
    echo "ok\n";

    echo "getting C... ";
    flush();
    $c = $initial->getC();
    test($c != null);
    echo "ok\n";

    echo "getting D... ";
    flush();
    $d = $initial->getD();
    test($d != null);
    echo "ok\n";

    echo "checking consistency... ";
    flush();
    test($b1 !== $b2);
    test($b1 !== $c);
    test($b1 !== $d);
    test($b2 !== $c);
    test($b2 !== $d);
    test($c !== $d);
    test($b1->theB != null);
    test($b1->theB === $b1);
    test($b1->theC == null);
    test($b1->theA != null);
    test($b1->theA->theA === $b1->theA);
    test($b1->theA->theB === $b1);
    test($b1->theA->theC != null);
    test($b1->theA->theC->theB === $b1->theA);
    test($b1->preMarshalInvoked);
    test($b1->postUnmarshalInvoked);
    test($b1->theA->preMarshalInvoked);
    test($b1->theA->postUnmarshalInvoked);
    test($b1->theA->theC->preMarshalInvoked);
    test($b1->theA->theC->postUnmarshalInvoked);
    // More tests possible for b2 and d, but I think this is already sufficient.
    test($b2->theA === $b2);
    test($d->theC == null);
    echo "ok\n";

    //
    // Break cyclic dependencies
    //
    $b1->theA->theA = null;
    $b1->theA->theB = null;
    $b1->theA->theC = null;
    $b1->theA = null;
    $b1->theB = null;

    $b2->theA = null;
    $b2->theB->theA = null;
    $b2->theB->theB = null;
    $b2->theC = null;

    $c->theB->theA = null;
    $c->theB->theB->theA = null;
    $c->theB->theB->theB = null;
    $c->theB = null;

    $d->theA->theA->theA = null;
    $d->theA->theA->theB = null;
    $d->theA->theB->theA = null;
    $d->theA->theB->theB = null;
    $d->theB->theA = null;
    $d->theB->theB = null;
    $d->theB->theC = null;

    echo "getting B1, B2, C, and D all at once... ";
    flush();
    $initial->getAll($b1, $b2, $c, $d);
    test($b1 != null);
    test($b2 != null);
    test($c != null);
    test($d != null);
    echo "ok\n";

    echo "checking consistency... ";
    flush();
    test($b1 !== $b2);
    test($b1 !== $c);
    test($b1 !== $d);
    test($b2 !== $c);
    test($b2 !== $d);
    test($c !== $d);
    test($b1->theA === $b2);
    test($b1->theB === $b1);
    test($b1->theC == null);
    test($b2->theA === $b2);
    test($b2->theB === $b1);
    test($b2->theC === $c);
    test($c->theB === $b2);
    test($d->theA === $b1);
    test($d->theB === $b2);
    test($d->theC == null);
    test($d->preMarshalInvoked);
    test($d->postUnmarshalInvoked);
    test($d->theA->preMarshalInvoked);
    test($d->theA->postUnmarshalInvoked);
    test($d->theB->preMarshalInvoked);
    test($d->theB->postUnmarshalInvoked);
    test($d->theB->theC->preMarshalInvoked);
    test($d->theB->theC->postUnmarshalInvoked);
    echo "ok\n";

    //
    // Break cyclic dependencies
    //
    $b1->theA = null;
    $b1->theB = null;
    $b2->theA = null;
    $b2->theB = null;
    $b2->theC = null;
    $c->theB = null;
    $d->theA = null;
    $d->theB = null;

    echo "testing protected members... ";
    flush();
    $e = $initial->getE();
    test($e->checkValues());
    $prop = new ReflectionProperty("Test\E", "i");
    test($prop->isProtected());
    $prop = new ReflectionProperty("Test\E", "s");
    test($prop->isProtected());
    $f = $initial->getF();
    test($f->checkValues());
    test($f->e2->checkValues());
    $prop = new ReflectionProperty("Test\F", "e1");
    test($prop->isProtected());
    $prop = new ReflectionProperty("Test\F", "e2");
    test($prop->isPublic());
    echo "ok\n";

    echo "getting K... ";
    flush();
    $k = $initial->getK();
    test($k->value->data == "l");
    echo "ok\n";

    echo "testing Value as parameter... ";
    flush();
    $v1 = new Test\L();
    $v1->data = "l";
    $v2 = null;
    $v3 = $initial->opValue($v1, $v2);
    test($v2->data == "l");
    test($v3->data == "l");

    $v1 = array(new Test\L());
    $v1[0]->data = "l";
    $v2 = null;
    $v3 = $initial->opValueSeq($v1, $v2);
    test($v2[0]->data == "l");
    test($v3[0]->data == "l");

    $v1 = array("l" => new Test\L());
    $v1["l"]->data = "l";
    $v2 = null;
    $v3 = $initial->opValueMap($v1, $v2);
    test($v2["l"]->data == "l");
    test($v3["l"]->data == "l");

    echo "ok\n";

    echo "getting D1... ";
    flush();
    $d1 = $initial->getD1(new Test\D1(new Test\A1("a1"), new Test\A1("a2"), new Test\A1("a3"), new Test\A1("a4")));
    test($d1->a1->name == "a1");
    test($d1->a2->name == "a2");
    test($d1->a3->name == "a3");
    test($d1->a4->name == "a4");
    echo "ok\n";

    echo "throw EDerived... ";
    flush();
    try
    {
        $initial->throwEDerived();
        test(false);
    }
    catch(Test\EDerived $ex)
    {
        test($ex->a1->name == "a1");
        test($ex->a2->name == "a2");
        test($ex->a3->name == "a3");
        test($ex->a4->name == "a4");
    }
    echo "ok\n";

    echo "setting G... ";
    flush();
    try
    {
        $initial->setG(new Test\G(new Test\S("hello"), "g"));
    }
    catch(Ice\OperationNotExistException $ex)
    {
    }
    echo "ok\n";

    echo "testing sequences... ";
    flush();
    $outS = null;
    $initial->opBaseSeq(array(), $outS);

    $seq = array();
    for($i = 0; $i < 120; $i++)
    {
        $b = new Test\Base();
        $b->str = "b" . $i;
        $b->theS = new Test\S();
        $b->theS->str = "b" . $i;
        $seq[$i] = $b;
    }

    $retS = $initial->opBaseSeq($seq, $outS);
    test($seq == $retS);
    test($seq == $outS);
    $i = 0;
    foreach($retS as $obj)
    {
        test($obj == $seq[$i++]);
    }
    $i = 0;
    foreach($outS as $obj)
    {
        test($obj == $seq[$i++]);
    }
    echo "ok\n";

    echo "testing recursive type... ";
    flush();
    $top = new Test\Recursive();
    $p = $top;
    $depth = 0;
    try
    {
        while($depth <= 700)
        {
            $p->v = new Test\Recursive();
            $p = $p->v;
            if(($depth < 10 && ($depth % 10) == 0) ||
               ($depth < 1000 && ($depth % 100) == 0) ||
               ($depth < 10000 && ($depth % 1000) == 0) ||
               ($depth % 10000) == 0)
            {
                $initial->setRecursive($top);
            }
            $depth += 1;
        }
        test(!$initial->supportsClassGraphDepthMax());
    }
    catch(Exception $ex)
    {
        if($ex instanceof Ice\UnknownLocalException)
        {
            // Expected marshal exception from the server (max class graph depth reached)
        }
        else if($ex instanceof Ice\UnknownException)
        {
            // Expected stack overflow from the server (Java only)
        }
        else
        {
            throw $ex;
        }
    }
    $initial->setRecursive(new Test\Recursive());
    echo "ok\n";

    echo "testing compact ID... ";
    flush();
    try
    {
        $r = $initial->getCompact();
        test($r != null);
    }
    catch(Ice\OperationNotExistException $ex)
    {
    }
    echo "ok\n";

    echo "testing marshaled results... ";
    flush();
    $b1 = $initial->getMB();
    test($b1 != null && $b1->theB == $b1);
    $b1 = $initial->getAMDMB();
    test($b1 != null && $b1->theB == $b1);
    echo "ok\n";

    echo "testing UnexpectedObjectException... ";
    flush();
    $ref = sprintf("uoet:%s", $helper->getTestEndpoint());
    $base = $communicator->stringToProxy($ref);
    test($base != null);
    $uoet = $base->ice_uncheckedCast("::Test::UnexpectedObjectExceptionTest");
    test($uoet != null);
    try
    {
        $uoet->op();
        test(false);
    }
    catch(Exception $ex)
    {
        if($ex instanceof Ice\UnexpectedObjectException)
        {
            test($ex->type == "::Test::AlsoEmpty");
            test($ex->expectedType == "::Test::Empty");
        }
        else if($ex instanceof Ice\UnmarshalOutOfBoundsException)
        {
            //
            // We get UnmarshalOutOfBoundsException on Windows with VC6.
            //
        }
        else
        {
            throw $ex;
        }
    }
    echo "ok\n";

    echo "testing forward declarations... ";
    $f12 = null;
    $f11 = $initial->opF1(new Test\F1("F11"), $f12);
    test($f11->name == "F11");
    test($f12->name == "F12");

    $f22 = null;
    $ref = sprintf("F21:%s", $helper->getTestEndpoint());
    $f21 = $initial->opF2($communicator->stringToProxy($ref)->ice_uncheckedCast("::Test::F2"), $f22);
    test($f21->ice_getIdentity()->name == "F21");
    $f21->op();
    test($f22->ice_getIdentity()->name == "F22");

    if($initial->hasF3())
    {
        $f32 = null;
        $f31 = $initial->opF3(new Test\F3($f11, $f22), $f32);
        test($f31->f1->name == "F11");
        test($f31->f2->ice_getIdentity()->name = "F21");

        test($f32->f1->name == "F12");
        test($f32->f2->ice_getIdentity()->name = "F22");
    }
    echo "ok\n";

    echo "testing sending class cycle...";
    $rec = new Test\Recursive();
    $rec->v = $rec;
    $acceptsCycles = $initial->acceptsClassCycles();
    try
    {
        $initial->setCycle($rec);
        test($acceptsCycles);
    }
    catch(Ice\UnknownLocalException $ex)
    {
        test(!$acceptsCycles);
    }
    echo "ok\n";

    return $initial;
}

class Client extends TestHelper
{
    function run($args)
    {
        try
        {
            $communicator = $this->initialize($args);
            $factory = new MyValueFactory();
            $communicator->getValueFactoryManager()->add($factory, "::Test::B");
            $communicator->getValueFactoryManager()->add($factory, "::Test::C");
            $communicator->getValueFactoryManager()->add($factory, "::Test::D");
            $communicator->getValueFactoryManager()->add($factory, "::Test::E");
            $communicator->getValueFactoryManager()->add($factory, "::Test::F");
            $communicator->getValueFactoryManager()->add($factory, "::Test::I");
            $communicator->getValueFactoryManager()->add($factory, "::Test::J");
            $initial = allTests($this);
            $initial->shutdown();
            $communicator->destroy();
        }
        catch(Exception $ex)
        {
            $communicator->destroy();
            throw $ex;
        }
    }
}
?>
