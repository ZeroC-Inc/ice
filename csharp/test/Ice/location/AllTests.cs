//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Ice.location.Test;

namespace Ice.location
{
    public class AllTests : global::Test.AllTests
    {
        public static void allTests(global::Test.TestHelper helper)
        {
            Communicator communicator = helper.communicator();
            var manager = IServerManagerPrx.Parse($"ServerManager :{helper.getTestEndpoint(0)}", communicator);
            var locator = ITestLocatorPrx.UncheckedCast(communicator.GetDefaultLocator());
            Console.WriteLine("registry checkedcast");
            var registry = ITestLocatorRegistryPrx.CheckedCast(locator.GetRegistry());
            test(registry != null);

            var output = helper.getWriter();
            output.Write("testing stringToProxy... ");
            output.Flush();
            var @base = IObjectPrx.Parse("test @ TestAdapter", communicator);
            var base2 = IObjectPrx.Parse("test @ TestAdapter", communicator);
            var base3 = IObjectPrx.Parse("test", communicator);
            var base4 = IObjectPrx.Parse("ServerManager", communicator);
            var base5 = IObjectPrx.Parse("test2", communicator);
            var base6 = IObjectPrx.Parse("test @ ReplicatedAdapter", communicator);
            output.WriteLine("ok");

            output.Write("testing ice_locator and ice_getLocator... ");
            test(default(ProxyIdentityComparer).Compare(@base.Locator, communicator.GetDefaultLocator()) == 0);
            var anotherLocator = ILocatorPrx.Parse("anotherLocator", communicator);
            @base = @base.Clone(locator: anotherLocator);
            test(default(ProxyIdentityComparer).Compare(@base.Locator, anotherLocator) == 0);
            communicator.SetDefaultLocator(null);
            @base = IObjectPrx.Parse("test @ TestAdapter", communicator);
            test(@base.Locator == null);
            @base = @base.Clone(locator: anotherLocator);
            test(default(ProxyIdentityComparer).Compare(@base.Locator, anotherLocator) == 0);
            communicator.SetDefaultLocator(locator);
            @base = IObjectPrx.Parse("test @ TestAdapter", communicator);
            test(default(ProxyIdentityComparer).Compare(@base.Locator, communicator.GetDefaultLocator()) == 0);

            //
            // We also test ice_router/ice_getRouter(perhaps we should add a
            // test/Ice/router test?)
            //
            test(@base.Router == null);
            var anotherRouter = IRouterPrx.Parse("anotherRouter", communicator);
            @base = @base.Clone(router: anotherRouter);
            test(default(ProxyIdentityComparer).Compare(@base.Router, anotherRouter) == 0);
            var router = IRouterPrx.Parse("dummyrouter", communicator);
            communicator.SetDefaultRouter(router);
            @base = IObjectPrx.Parse("test @ TestAdapter", communicator);
            test(default(ProxyIdentityComparer).Compare(@base.Router, communicator.GetDefaultRouter()) == 0);
            communicator.SetDefaultRouter(null);
            @base = IObjectPrx.Parse("test @ TestAdapter", communicator);
            test(@base.Router == null);
            output.WriteLine("ok");

            output.Write("starting server... ");
            output.Flush();
            manager.startServer();
            output.WriteLine("ok");

            output.Write("testing checked cast... ");
            output.Flush();
            var obj = Test.ITestIntfPrx.CheckedCast(@base);
            test(obj != null);
            var obj2 = Test.ITestIntfPrx.CheckedCast(base2);
            test(obj2 != null);
            var obj3 = Test.ITestIntfPrx.CheckedCast(base3);
            test(obj3 != null);
            var obj4 = Test.IServerManagerPrx.CheckedCast(base4);
            test(obj4 != null);
            var obj5 = Test.ITestIntfPrx.CheckedCast(base5);
            test(obj5 != null);
            var obj6 = Test.ITestIntfPrx.CheckedCast(base6);
            test(obj6 != null);
            output.WriteLine("ok");

            output.Write("testing id@AdapterId indirect proxy... ");
            output.Flush();
            obj.shutdown();
            manager.startServer();
            try
            {
                obj2.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            output.WriteLine("ok");

            output.Write("testing id@ReplicaGroupId indirect proxy... ");
            output.Flush();
            obj.shutdown();
            manager.startServer();
            try
            {
                obj6.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            output.WriteLine("ok");

            output.Write("testing identity indirect proxy... ");
            output.Flush();
            obj.shutdown();
            manager.startServer();
            try
            {
                obj3.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            try
            {
                obj2.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            obj.shutdown();
            manager.startServer();
            try
            {
                obj2.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            try
            {
                obj3.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            obj.shutdown();
            manager.startServer();
            try
            {
                obj2.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            obj.shutdown();
            manager.startServer();
            try
            {
                obj3.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            obj.shutdown();
            manager.startServer();
            try
            {
                obj5 = Test.ITestIntfPrx.CheckedCast(base5);
                obj5.IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            output.WriteLine("ok");

            output.Write("testing proxy with unknown identity... ");
            output.Flush();
            try
            {
                @base = IObjectPrx.Parse("unknown/unknown", communicator);
                @base.IcePing();
                test(false);
            }
            catch (NotRegisteredException ex)
            {
                test(ex.KindOfObject.Equals("object"));
                test(ex.Id.Equals("unknown/unknown"));
            }
            output.WriteLine("ok");

            output.Write("testing proxy with unknown adapter... ");
            output.Flush();
            try
            {
                @base = IObjectPrx.Parse("test @ TestAdapterUnknown", communicator);
                @base.IcePing();
                test(false);
            }
            catch (NotRegisteredException ex)
            {
                test(ex.KindOfObject.Equals("object adapter"));
                test(ex.Id.Equals("TestAdapterUnknown"));
            }
            output.WriteLine("ok");

            output.Write("testing locator cache timeout... ");
            output.Flush();

            var basencc = IObjectPrx.Parse("test@TestAdapter", communicator).Clone(connectionCached: false);
            int count = locator.getRequestCount();
            basencc.Clone(locatorCacheTimeout: 0).IcePing(); // No locator cache.
            test(++count == locator.getRequestCount());
            basencc.Clone(locatorCacheTimeout: 0).IcePing(); // No locator cache.
            test(++count == locator.getRequestCount());
            basencc.Clone(locatorCacheTimeout: 2).IcePing(); // 2s timeout.
            test(count == locator.getRequestCount());
            System.Threading.Thread.Sleep(1300); // 1300ms
            basencc.Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout.
            test(++count == locator.getRequestCount());

            IObjectPrx.Parse("test", communicator).Clone(locatorCacheTimeout: 0).IcePing(); // No locator cache.
            count += 2;
            test(count == locator.getRequestCount());
            IObjectPrx.Parse("test", communicator).Clone(locatorCacheTimeout: 2).IcePing(); // 2s timeout
            test(count == locator.getRequestCount());
            System.Threading.Thread.Sleep(1300); // 1300ms
            IObjectPrx.Parse("test", communicator).Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout
            count += 2;
            test(count == locator.getRequestCount());

            IObjectPrx.Parse("test@TestAdapter", communicator).Clone(locatorCacheTimeout: -1).IcePing();
            test(count == locator.getRequestCount());
            IObjectPrx.Parse("test", communicator).Clone(locatorCacheTimeout: -1).IcePing();
            test(count == locator.getRequestCount());
            IObjectPrx.Parse("test@TestAdapter", communicator).IcePing();
            test(count == locator.getRequestCount());
            IObjectPrx.Parse("test", communicator).IcePing();
            test(count == locator.getRequestCount());

            test(IObjectPrx.Parse("test", communicator).Clone(locatorCacheTimeout: 99).LocatorCacheTimeout == 99);

            output.WriteLine("ok");

            output.Write("testing proxy from server... ");
            output.Flush();
            obj = ITestIntfPrx.Parse("test@TestAdapter", communicator);
            var hello = obj.getHello();
            test(hello.AdapterId.Equals("TestAdapter"));
            hello.sayHello();
            hello = obj.getReplicatedHello();
            test(hello.AdapterId.Equals("ReplicatedAdapter"));
            hello.sayHello();
            output.WriteLine("ok");

            output.Write("testing locator request queuing... ");
            output.Flush();
            hello = obj.getReplicatedHello().Clone(locatorCacheTimeout: 0, connectionCached: false);
            count = locator.getRequestCount();
            hello.IcePing();
            test(++count == locator.getRequestCount());
            List<Task> results = new List<Task>();
            for (int i = 0; i < 1000; i++)
            {
                results.Add(hello.sayHelloAsync());
            }
            Task.WaitAll(results.ToArray());
            results.Clear();
            test(locator.getRequestCount() > count && locator.getRequestCount() < count + 999);
            if (locator.getRequestCount() > count + 800)
            {
                output.Write("queuing = " + (locator.getRequestCount() - count));
            }
            count = locator.getRequestCount();
            hello = hello.Clone(adapterId: "unknown");
            for (int i = 0; i < 1000; i++)
            {
                results.Add(hello.sayHelloAsync().ContinueWith((Task t) =>
                {
                    try
                    {
                        t.Wait();
                    }
                    catch (AggregateException ex) when (ex.InnerException is Ice.NotRegisteredException)
                    {
                    }
                }));
            }
            Task.WaitAll(results.ToArray());
            results.Clear();
            // XXX:
            // Take into account the retries.
            test(locator.getRequestCount() > count && locator.getRequestCount() < count + 1999);
            if (locator.getRequestCount() > count + 800)
            {
                output.Write("queuing = " + (locator.getRequestCount() - count));
            }
            output.WriteLine("ok");

            output.Write("testing adapter locator cache... ");
            output.Flush();
            try
            {
                IObjectPrx.Parse("test@TestAdapter3", communicator).IcePing();
                test(false);
            }
            catch (NotRegisteredException ex)
            {
                test(ex.KindOfObject == "object adapter");
                test(ex.Id.Equals("TestAdapter3"));
            }
            registry.SetAdapterDirectProxy("TestAdapter3", locator.FindAdapterById("TestAdapter"));
            try
            {
                IObjectPrx.Parse("test@TestAdapter3", communicator).IcePing();
                registry.SetAdapterDirectProxy("TestAdapter3",
                                                IObjectPrx.Parse($"dummy:{helper.getTestEndpoint(99)}", communicator));
                IObjectPrx.Parse("test@TestAdapter3", communicator).IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }

            try
            {
                IObjectPrx.Parse("test@TestAdapter3", communicator).Clone(locatorCacheTimeout: 0).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            try
            {
                IObjectPrx.Parse("test@TestAdapter3", communicator).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            registry.SetAdapterDirectProxy("TestAdapter3", locator.FindAdapterById("TestAdapter"));
            try
            {
                IObjectPrx.Parse("test@TestAdapter3", communicator).IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }
            output.WriteLine("ok");

            output.Write("testing well-known object locator cache... ");
            output.Flush();
            registry.addObject(IObjectPrx.Parse("test3@TestUnknown", communicator));
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
                test(false);
            }
            catch (NotRegisteredException ex)
            {
                test(ex.KindOfObject == "object adapter");
                test(ex.Id.Equals("TestUnknown"));
            }
            registry.addObject(IObjectPrx.Parse("test3@TestAdapter4", communicator)); // Update
            registry.SetAdapterDirectProxy("TestAdapter4",
                                            IObjectPrx.Parse($"dummy:{helper.getTestEndpoint(99)}", communicator));
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            registry.SetAdapterDirectProxy("TestAdapter4", locator.FindAdapterById("TestAdapter"));
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }

            registry.SetAdapterDirectProxy("TestAdapter4",
                                            IObjectPrx.Parse($"dummy:{helper.getTestEndpoint(99)}", communicator));
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }

            try
            {
                IObjectPrx.Parse("test@TestAdapter4", communicator).Clone(locatorCacheTimeout: 0).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            try
            {
                IObjectPrx.Parse("test@TestAdapter4", communicator).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            registry.addObject(IObjectPrx.Parse("test3@TestAdapter", communicator));
            try
            {
                IObjectPrx.Parse("test3", communicator).IcePing();
            }
            catch (LocalException)
            {
                test(false);
            }

            registry.addObject(IObjectPrx.Parse("test4", communicator));
            try
            {
                IObjectPrx.Parse("test4", communicator).IcePing();
                test(false);
            }
            catch (NoEndpointException)
            {
            }
            output.WriteLine("ok");

            output.Write("testing locator cache background updates... ");
            output.Flush();
            {
                Dictionary<string, string> properties = communicator.GetProperties();
                properties["Ice.BackgroundLocatorCacheUpdates"] = "1";
                Communicator ic = helper.initialize(properties);

                registry.SetAdapterDirectProxy("TestAdapter5", locator.FindAdapterById("TestAdapter"));
                registry.addObject(IObjectPrx.Parse("test3@TestAdapter", communicator));

                count = locator.getRequestCount();
                IObjectPrx.Parse("test@TestAdapter5", ic).Clone(locatorCacheTimeout: 0).IcePing(); // No locator cache.
                IObjectPrx.Parse("test3", ic).Clone(locatorCacheTimeout: 0).IcePing(); // No locator cache.
                count += 3;
                test(count == locator.getRequestCount());
                registry.SetAdapterDirectProxy("TestAdapter5", null);
                registry.addObject(IObjectPrx.Parse($"test3:{helper.getTestEndpoint(99)}", communicator));
                IObjectPrx.Parse("test@TestAdapter5", ic).Clone(locatorCacheTimeout: 10).IcePing(); // 10s timeout.
                IObjectPrx.Parse("test3", ic).Clone(locatorCacheTimeout: 10).IcePing(); // 10s timeout.
                test(count == locator.getRequestCount());
                System.Threading.Thread.Sleep(1200);

                // The following request should trigger the background
                // updates but still use the cached endpoints and
                // therefore succeed.
                IObjectPrx.Parse("test@TestAdapter5", ic).Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout.
                IObjectPrx.Parse("test3", ic).Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout.

                try
                {
                    while (true)
                    {
                        IObjectPrx.Parse("test@TestAdapter5", ic).Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout.
                        System.Threading.Thread.Sleep(10);
                    }
                }
                catch (LocalException)
                {
                    // Expected to fail once they endpoints have been updated in the background.
                }
                try
                {
                    while (true)
                    {
                        IObjectPrx.Parse("test3", ic).Clone(locatorCacheTimeout: 1).IcePing(); // 1s timeout.
                        System.Threading.Thread.Sleep(10);
                    }
                }
                catch (LocalException)
                {
                    // Expected to fail once they endpoints have been updated in the background.
                }
                ic.Destroy();
            }
            output.WriteLine("ok");

            output.Write("testing proxy from server after shutdown... ");
            output.Flush();
            hello = obj.getReplicatedHello();
            obj.shutdown();
            manager.startServer();
            hello.sayHello();
            output.WriteLine("ok");

            output.Write("testing object migration... ");
            output.Flush();
            hello = IHelloPrx.Parse("hello", communicator);
            obj.migrateHello();
            hello.GetConnection().Close(ConnectionClose.GracefullyWithWait);
            hello.sayHello();
            obj.migrateHello();
            hello.sayHello();
            obj.migrateHello();
            hello.sayHello();
            output.WriteLine("ok");

            output.Write("testing locator encoding resolution... ");
            output.Flush();
            hello = IHelloPrx.Parse("hello", communicator);
            count = locator.getRequestCount();
            IObjectPrx.Parse("test@TestAdapter", communicator).Clone(encodingVersion: Util.Encoding_1_1).IcePing();
            test(count == locator.getRequestCount());
            output.WriteLine("ok");

            output.Write("shutdown server... ");
            output.Flush();
            obj.shutdown();
            output.WriteLine("ok");

            output.Write("testing whether server is gone... ");
            output.Flush();
            try
            {
                obj2.IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            try
            {
                obj3.IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            try
            {
                obj5.IcePing();
                test(false);
            }
            catch (LocalException)
            {
            }
            output.WriteLine("ok");

            output.Write("testing indirect proxies to collocated objects... ");
            output.Flush();

            communicator.SetProperty("Hello.AdapterId", Guid.NewGuid().ToString());
            ObjectAdapter adapter = communicator.CreateObjectAdapterWithEndpoints("Hello", "default");

            var id = new Identity(Guid.NewGuid().ToString(), "");
            adapter.Add(new Hello(), id);
            adapter.Activate();

            // Ensure that calls on the well-known proxy is collocated.
            IHelloPrx? helloPrx = IHelloPrx.Parse("\"" + id.ToString(communicator.ToStringMode) + "\"", communicator);
            test(helloPrx.GetConnection() == null);

            // Ensure that calls on the indirect proxy (with adapter ID) is collocated
            helloPrx = IHelloPrx.CheckedCast(adapter.CreateIndirectProxy(id, IObjectPrx.Factory));
            test(helloPrx != null && helloPrx.GetConnection() == null);

            // Ensure that calls on the direct proxy is collocated
            helloPrx = IHelloPrx.CheckedCast(adapter.CreateDirectProxy(id, IObjectPrx.Factory));
            test(helloPrx != null && helloPrx.GetConnection() == null);

            output.WriteLine("ok");

            output.Write("shutdown server manager... ");
            output.Flush();
            manager.shutdown();
            output.WriteLine("ok");
        }
    }
}
