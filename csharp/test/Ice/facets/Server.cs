// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

using Test;

namespace Ice
{
    namespace facets
    {
        public class Server : TestHelper
        {
            public override void run(string[] args)
            {
                var properties = createTestProperties(ref args);
                properties.setProperty("Ice.Package.Test", "Ice.facets");
                using(var communicator = initialize(properties))
                {
                    communicator.getProperties().setProperty("TestAdapter.Endpoints", getTestEndpoint(0));
                    Ice.ObjectAdapter adapter = communicator.createObjectAdapter("TestAdapter");
                    Ice.Object d = new DI();
                    adapter.add(d, Ice.Util.stringToIdentity("d"));
                    adapter.addFacet(d, Ice.Util.stringToIdentity("d"), "facetABCD");
                    Ice.Object f = new FI();
                    adapter.addFacet(f, Ice.Util.stringToIdentity("d"), "facetEF");
                    Ice.Object h = new HI(communicator);
                    adapter.addFacet(h, Ice.Util.stringToIdentity("d"), "facetGH");
                    adapter.activate();
                    serverReady();
                    communicator.waitForShutdown();
                }
            }

            public static int Main(string[] args)
            {
                return TestDriver.runTest<Server>(args);
            }
        }
    }
}
