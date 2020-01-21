//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Diagnostics;
using System.Collections.Generic;
using Ice.admin.Test;

namespace Ice.admin
{
    public class TestFacet : ITestFacet
    {
        public void op(Ice.Current current)
        {
        }
    }

    public class RemoteCommunicator : IRemoteCommunicator
    {
        public RemoteCommunicator(Communicator communicator) => _communicator = communicator;

        public IObjectPrx getAdmin(Current current) => _communicator.GetAdmin();

        public Dictionary<string, string> getChanges(Ice.Current current)
        {
            lock (this)
            {
                return _changes;
            }
        }

        public void print(string message, Current current) => _communicator.Logger.print(message);

        public void trace(string category, string message, Current current) => _communicator.Logger.trace(category, message);

        public void warning(string message, Current current) => _communicator.Logger.warning(message);

        public void error(string message, Current current) => _communicator.Logger.error(message);

        public void shutdown(Current current) => _communicator.Shutdown();

        // Note that we are executing in a thread of the *main* communicator,
        // not the one that is being shut down.
        public void waitForShutdown(Current current) => _communicator.WaitForShutdown();

        public void destroy(Current current) => _communicator.Destroy();

        public void updated(Dictionary<string, string> changes)
        {
            lock (this)
            {
                _changes = changes;
            }
        }

        private Communicator _communicator;
        private Dictionary<string, string> _changes;
    }

    public class RemoteCommunicatorFactoryI : IRemoteCommunicatorFactory
    {
        public IRemoteCommunicatorPrx createCommunicator(Dictionary<string, string> props, Current current)
        {
            //
            // Prepare the property set using the given properties.
            //
            ILogger? logger = null;
            string? value;
            int nullLogger;
            if (props.TryGetValue("NullLogger", out value) && int.TryParse(value, out nullLogger) && nullLogger > 0)
            {
                logger = new NullLogger();
            }

            //
            // Initialize a new communicator.
            //
            var communicator = new Communicator(props, logger: logger);

            //
            // Install a custom admin facet.
            //
            try
            {
                communicator.AddAdminFacet<ITestFacet, TestFacetTraits>(new TestFacet(), "TestFacet");
            }
            catch (System.ArgumentException)
            {
            }

            //
            // The RemoteCommunicator servant also implements PropertiesAdminUpdateCallback.
            // Set the callback on the admin facet.
            //
            var servant = new RemoteCommunicator(communicator);
            var propFacet = communicator.FindAdminFacet("Properties").servant;

            if (propFacet != null)
            {
                var admin = (INativePropertiesAdmin)propFacet;
                Debug.Assert(admin != null);
                admin.addUpdateCallback(servant.updated);
            }

            return current.Adapter.Add(servant);
        }

        public void shutdown(Current current) => current.Adapter.Communicator.Shutdown();

        private class NullLogger : ILogger
        {
            public void print(string message)
            {
            }

            public void trace(string category, string message)
            {
            }

            public void warning(string message)
            {
            }

            public void error(string message)
            {
            }

            public string getPrefix() => "NullLogger";

            public ILogger cloneWithPrefix(string prefix) => this;
        }
    }
}
