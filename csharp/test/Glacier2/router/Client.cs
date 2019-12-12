//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using Test;
using System;
using System.Collections.Generic;
using System.Reflection;
using Ice;

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Client : Test.TestHelper
{
    public override void run(string[] args)
    {
        Ice.Properties properties = createTestProperties(ref args);
        //
        // We must disable connection warnings, because we attempt to
        // ping the router before session establishment, as well as
        // after session destruction. Both will cause a
        // ConnectionLostException.
        //
        properties.setProperty("Ice.Warn.Connections", "0");
        using (var communicator = initialize(properties))
        {
            IObjectPrx routerBase;
            {
                Console.Out.Write("testing stringToProxy for router... ");
                Console.Out.Flush();
                routerBase = IObjectPrx.Parse($"Glacier2/router:{getTestEndpoint(50)}", communicator);
                Console.Out.WriteLine("ok");
            }

            Glacier2.RouterPrx router;
            {
                Console.Out.Write("testing checked cast for router... ");
                Console.Out.Flush();
                router = Glacier2.RouterPrx.CheckedCast(routerBase);
                test(router != null);
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("testing router finder... ");
                Console.Out.Flush();
                RouterFinderPrx finder = RouterFinderPrx.Parse($"Ice/RouterFinder:{getTestEndpoint(50)}", communicator);
                test(finder.getRouter().Identity.Equals(router.Identity));
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("installing router with communicator... ");
                Console.Out.Flush();
                communicator.setDefaultRouter(router);
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("getting the session timeout... ");
                Console.Out.Flush();
                long sessionTimeout = router.getSessionTimeout();
                long acmTimeout = router.getACMTimeout();
                test(sessionTimeout == 30 && acmTimeout == 30);
                Console.Out.WriteLine("ok");
            }

            IObjectPrx @base;
            {
                Console.Out.Write("testing stringToProxy for server object... ");
                Console.Out.Flush();
                @base = IObjectPrx.Parse($"c1/callback:{getTestEndpoint(0)}", communicator);
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("trying to ping server before session creation... ");
                Console.Out.Flush();
                try
                {
                    @base.IcePing();
                    test(false);
                }
                catch (Ice.ConnectionLostException)
                {
                    Console.Out.WriteLine("ok");
                }
                catch (Ice.SocketException)
                {
                    test(false);
                }
            }

            {
                Console.Out.Write("trying to create session with wrong password... ");
                Console.Out.Flush();
                try
                {
                    router.createSession("userid", "xxx");
                    test(false);
                }
                catch (Glacier2.PermissionDeniedException)
                {
                    Console.Out.WriteLine("ok");
                }
                catch (Glacier2.CannotCreateSessionException)
                {
                    test(false);
                }
            }

            {
                Console.Out.Write("trying to destroy non-existing session... ");
                Console.Out.Flush();
                try
                {
                    router.destroySession();
                    test(false);
                }
                catch (Glacier2.SessionNotExistException)
                {
                    Console.Out.WriteLine("ok");
                }
            }

            {
                Console.Out.Write("creating session with correct password... ");
                Console.Out.Flush();
                try
                {
                    router.createSession("userid", "abc123");
                }
                catch (Glacier2.PermissionDeniedException)
                {
                    test(false);
                }
                catch (Glacier2.CannotCreateSessionException)
                {
                    test(false);
                }
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("trying to create a second session... ");
                Console.Out.Flush();
                try
                {
                    router.createSession("userid", "abc123");
                    test(false);
                }
                catch (Glacier2.PermissionDeniedException)
                {
                    test(false);
                }
                catch (Glacier2.CannotCreateSessionException)
                {
                    Console.Out.WriteLine("ok");
                }
            }

            {
                Console.Out.Write("pinging server after session creation... ");
                Console.Out.Flush();
                @base.IcePing();
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("pinging object with client endpoint... ");
                IObjectPrx baseC = IObjectPrx.Parse($"collocated:{getTestEndpoint(50)}", communicator);
                try
                {
                    baseC.IcePing();
                }
                catch (Ice.ObjectNotExistException)
                {
                }
                Console.Out.WriteLine("ok");
            }

            CallbackPrx twoway;

            {
                Console.Out.Write("testing checked cast for server object... ");
                Console.Out.Flush();
                twoway = CallbackPrx.CheckedCast(@base);
                test(twoway != null);
                Console.Out.WriteLine("ok");
            }

            Ice.ObjectAdapter adapter;

            {
                Console.Out.Write("creating and activating callback receiver adapter... ");
                Console.Out.Flush();
                communicator.Properties.setProperty("Ice.PrintAdapterReady", "0");
                adapter = communicator.createObjectAdapterWithRouter("CallbackReceiverAdapter", router);
                adapter.Activate();
                Console.Out.WriteLine("ok");
            }

            string category;

            {
                Console.Out.Write("getting category from router... ");
                Console.Out.Flush();
                category = router.getCategoryForClient();
                Console.Out.WriteLine("ok");
            }

            CallbackReceiverI callbackReceiverImpl;
            CallbackReceiver callbackReceiver;
            CallbackReceiverPrx twowayR;
            CallbackReceiverPrx fakeTwowayR;

            {
                Console.Out.Write("creating and adding callback receiver object... ");
                Console.Out.Flush();
                callbackReceiverImpl = new CallbackReceiverI();
                callbackReceiver = callbackReceiverImpl;
                Ice.Identity callbackReceiverIdent = new Ice.Identity();
                callbackReceiverIdent.name = "callbackReceiver";
                callbackReceiverIdent.category = category;
                twowayR = adapter.Add(callbackReceiver, callbackReceiverIdent);
                Ice.Identity fakeCallbackReceiverIdent = new Ice.Identity();
                fakeCallbackReceiverIdent.name = "callbackReceiver";
                fakeCallbackReceiverIdent.category = "dummy";
                fakeTwowayR = adapter.Add(callbackReceiver, fakeCallbackReceiverIdent);
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("testing oneway callback... ");
                Console.Out.Flush();
                CallbackPrx oneway = twoway.Clone(oneway: true);
                CallbackReceiverPrx onewayR = twowayR.Clone(oneway: true);
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "o";
                oneway.initiateCallback(onewayR, context);
                callbackReceiverImpl.callbackOK();
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("testing twoway callback... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                twoway.initiateCallback(twowayR, context);
                callbackReceiverImpl.callbackOK();
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("ditto, but with user exception... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                try
                {
                    twoway.initiateCallbackEx(twowayR, context);
                    test(false);
                }
                catch (CallbackException ex)
                {
                    test(ex.someValue == 3.14);
                    test(ex.someString.Equals("3.14"));
                }
                callbackReceiverImpl.callbackOK();
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("trying twoway callback with fake category... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                try
                {
                    twoway.initiateCallback(fakeTwowayR, context);
                    test(false);
                }
                catch (Ice.ObjectNotExistException)
                {
                    Console.Out.WriteLine("ok");
                }
            }

            {
                Console.Out.Write("testing whether other allowed category is accepted... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                CallbackPrx otherCategoryTwoway =
                    CallbackPrx.UncheckedCast(twoway.Clone(Identity.Parse("c2/callback")));
                otherCategoryTwoway.initiateCallback(twowayR, context);
                callbackReceiverImpl.callbackOK();
                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("testing whether disallowed category gets rejected... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                try
                {
                    CallbackPrx otherCategoryTwoway =
                        CallbackPrx.UncheckedCast(twoway.Clone(Identity.Parse("c3/callback")));
                    otherCategoryTwoway.initiateCallback(twowayR, context);
                    test(false);
                }
                catch (Ice.ObjectNotExistException)
                {
                    Console.Out.WriteLine("ok");
                }
            }

            {
                Console.Out.Write("testing whether user-id as category is accepted... ");
                Console.Out.Flush();
                Dictionary<string, string> context = new Dictionary<string, string>();
                context["_fwd"] = "t";
                CallbackPrx otherCategoryTwoway =
                    CallbackPrx.UncheckedCast(twoway.Clone(Identity.Parse("_userid/callback")));
                otherCategoryTwoway.initiateCallback(twowayR, context);
                callbackReceiverImpl.callbackOK();
                Console.Out.WriteLine("ok");
            }

            if (args.Length >= 1 && args[0].Equals("--shutdown"))
            {
                Console.Out.Write("testing server shutdown... ");
                Console.Out.Flush();
                twoway.shutdown();
                // No ping, otherwise the router prints a warning message if it's
                // started with --Ice.Warn.Connections.
                Console.Out.WriteLine("ok");
                /*
                  try
                  {
                  base.IcePing();
                  test(false);
                  }
                  // If we use the glacier router, the exact exception reason gets
                  // lost.
                  catch(Ice.UnknownLocalException ex)
                  {
                  Console.Out.WriteLine("ok");
                  }
                */
            }

            {
                Console.Out.Write("destroying session... ");
                Console.Out.Flush();
                try
                {
                    router.destroySession();
                }
                catch (Ice.LocalException)
                {
                    test(false);
                }

                Console.Out.WriteLine("ok");
            }

            {
                Console.Out.Write("trying to ping server after session destruction... ");
                Console.Out.Flush();
                try
                {
                    @base.IcePing();
                    test(false);
                }
                catch (Ice.ConnectionLostException)
                {
                    Console.Out.WriteLine("ok");
                }
                catch (Ice.SocketException)
                {
                    test(false);
                }
            }

            if (args.Length >= 1 && args[0].Equals("--shutdown"))
            {
                {
                    Console.Out.Write("uninstalling router with communicator... ");
                    Console.Out.Flush();
                    communicator.setDefaultRouter(null);
                    Console.Out.WriteLine("ok");
                }

                IObjectPrx processBase;
                {
                    Console.Out.Write("testing stringToProxy for admin object... ");
                    processBase = IObjectPrx.Parse($"Glacier2/admin -f Process:{getTestEndpoint(51)}", communicator);
                    Console.Out.WriteLine("ok");
                }

                /*
                  {
                  Console.Out.Write("uninstalling router with process object... ");
                  processBase.ice_router(null);
                  Console.Out.WriteLine("ok");
                  }
                */

                ProcessPrx process;
                {
                    Console.Out.Write("testing checked cast for process object... ");
                    process = ProcessPrx.CheckedCast(processBase);
                    process.IcePing();
                    Console.Out.WriteLine("ok");
                }

                Console.Out.Write("testing Glacier2 shutdown... ");
                process.shutdown();
                try
                {
                    process.IcePing();
                    test(false);
                }
                catch (LocalException)
                {
                    Console.Out.WriteLine("ok");
                }
            }
        }
    }

    public static int Main(string[] args)
    {
        return Test.TestDriver.runTest<Client>(args);
    }
}
