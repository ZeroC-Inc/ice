//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Diagnostics;
using System.Threading;

namespace Ice
{
    namespace operations
    {
        public class OnewaysAMI
        {
            private static void test(bool b)
            {
                if (!b)
                {
                    throw new System.Exception();
                }
            }

            private class CallbackBase
            {
                internal CallbackBase()
                {
                    _called = false;
                }

                public virtual void check()
                {
                    lock (this)
                    {
                        while (!_called)
                        {
                            Monitor.Wait(this);
                        }
                        _called = false;
                    }
                }

                public virtual void called()
                {
                    lock (this)
                    {
                        Debug.Assert(!_called);
                        _called = true;
                        Monitor.Pulse(this);
                    }
                }

                private bool _called;
            }

            private class Callback : CallbackBase
            {
                public Callback()
                {
                }

                public void
                sent(bool sentSynchronously)
                {
                    called();
                }

                public void noException(Ice.Exception ex)
                {
                    test(false);
                }
            }

            internal static void onewaysAMI(global::Test.TestHelper helper, Test.MyClassPrx proxy)
            {
                Ice.Communicator communicator = helper.communicator();
                Test.MyClassPrx p = Test.MyClassPrxHelper.uncheckedCast(proxy.ice_oneway());

                {
                    Callback cb = new Callback();
                    _ = p.ice_pingAsync(progress: new Progress<bool>(
                        sentSynchronously =>
                        {
                            cb.sent(sentSynchronously);
                        }));
                    cb.check();
                }

                {
                    try
                    {
                        _ = p.ice_isAAsync("::Test::MyClass");
                        test(false);
                    }
                    catch (Ice.TwowayOnlyException)
                    {
                    }
                }

                {
                    try
                    {
                        _ = p.ice_idAsync();
                        test(false);
                    }
                    catch (Ice.TwowayOnlyException)
                    {
                    }
                }

                {
                    try
                    {
                        _ = p.ice_idsAsync();
                        test(false);
                    }
                    catch (Ice.TwowayOnlyException)
                    {
                    }
                }

                {
                    Callback cb = new Callback();
                    _ = p.opVoidAsync(progress: new Progress<bool>(
                        sentSynchronously =>
                        {
                            cb.sent(sentSynchronously);
                        }));
                    cb.check();
                }

                {
                    Callback cb = new Callback();
                    _ = p.opIdempotentAsync(progress: new Progress<bool>(
                        sentSynchronously =>
                        {
                            cb.sent(sentSynchronously);
                        }));
                    cb.check();
                }

                {
                    Callback cb = new Callback();
                    _ = p.opNonmutatingAsync(progress: new Progress<bool>(
                        sentSynchronously =>
                        {
                            cb.sent(sentSynchronously);
                        }));
                    cb.check();
                }

                {
                    try
                    {
                        _ = p.opByteAsync(0xff, 0x0f);
                        test(false);
                    }
                    catch (Ice.TwowayOnlyException)
                    {
                    }
                }
            }
        }
    }
}
