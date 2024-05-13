// Copyright (c) ZeroC, Inc.

namespace Ice
{
    namespace interceptor
    {
        internal class MyRetryException : Ice.LocalException
        {
            override public string ice_id()
            {
                return "::MyRetryException";
            }
        }

        internal class MyObjectI : Test.MyObjectDisp_
        {
            protected static void
            test(bool b)
            {
                if (!b)
                {
                    throw new System.Exception();
                }
            }

            public override int
            add(int x, int y, Ice.Current current)
            {
                return x + y;
            }

            public override int
            addWithRetry(int x, int y, Ice.Current current)
            {
                test(current != null);
                test(current.ctx != null);

                if (current.ctx.ContainsKey("retry") && current.ctx["retry"] == "no")
                {
                    return x + y;
                }
                throw new MyRetryException();
            }

            public override int
            badAdd(int x, int y, Ice.Current current)
            {
                throw new Test.InvalidInputException();
            }

            public override int
            notExistAdd(int x, int y, Ice.Current current)
            {
                throw new Ice.ObjectNotExistException();
            }

            //
            // AMD
            //
            public override async Task<int>
            amdAddAsync(int x, int y, Ice.Current current)
            {
                await Task.Delay(10);
                return x + y;
            }

            public override async Task<int>
            amdAddWithRetryAsync(int x, int y, Ice.Current current)
            {
                if (current.ctx.ContainsKey("retry") && current.ctx["retry"] == "no")
                {
                    await Task.Delay(10);
                    return x + y;
                }
                else
                {
                    throw new MyRetryException();
                }
            }

            public override async Task<int>
            amdBadAddAsync(int x, int y, Ice.Current current)
            {
                await Task.Delay(10);
                throw new Test.InvalidInputException();
            }

            public override async Task<int>
            amdNotExistAddAsync(int x, int y, Ice.Current current)
            {
                await Task.Delay(10);
                throw new Ice.ObjectNotExistException();
            }
        }
    }
}
