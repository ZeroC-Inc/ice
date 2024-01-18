#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

import Ice, Test, Twoways, TwowaysFuture, Oneways, OnewaysFuture, BatchOneways, sys
import BatchOnewaysFuture

def test(b):
    if not b:
        raise RuntimeError('test assertion failed')

def allTests(helper, communicator):
    ref = "test:{0}".format(helper.getTestEndpoint())
    base = communicator.stringToProxy(ref)
    cl = Test.MyClassPrx.checkedCast(base)
    derived = Test.MyDerivedClassPrx.checkedCast(cl)

    sys.stdout.write("testing twoway operations... ")
    sys.stdout.flush()
    Twoways.twoways(helper, cl)
    Twoways.twoways(helper, derived)
    derived.opDerived()
    print("ok")

    sys.stdout.write("testing oneway operations... ")
    sys.stdout.flush()
    Oneways.oneways(helper, cl)
    print("ok")

    sys.stdout.write("testing twoway operations with futures... ")
    sys.stdout.flush()
    TwowaysFuture.twowaysFuture(helper, cl)
    print("ok")

    sys.stdout.write("testing oneway operations with futures... ")
    sys.stdout.flush()
    OnewaysFuture.onewaysFuture(helper, cl)
    print("ok")

    sys.stdout.write("testing batch oneway operations...  ")
    sys.stdout.flush()
    BatchOneways.batchOneways(cl)
    BatchOneways.batchOneways(derived)
    print("ok")

    sys.stdout.write("testing batch oneway operations with futures...  ")
    sys.stdout.flush()
    BatchOnewaysFuture.batchOneways(cl)
    BatchOnewaysFuture.batchOneways(derived)
    print("ok")

    sys.stdout.write("testing server shutdown... ")
    sys.stdout.flush()
    cl.shutdown()
    try:
        cl.ice_timeout(100).ice_ping()  # Use timeout to speed up testing on Windows
        test(False)
    except Ice.LocalException:
        print("ok")
