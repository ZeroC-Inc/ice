// Copyright (c) ZeroC, Inc.

package test.Ice.maxConnections;

public class Client extends test.TestHelper {
    @Override
    public void run(String[] args) {
        var properties = createTestProperties(args);
        properties.setProperty("Ice.Package.Test", "test.Ice.maxConnections");

        // We disable retries to make the logs clearer and avoid hiding potential issues.
        properties.setProperty("Ice.RetryIntervals", "-1");

        try (var communicator = initialize(properties)) {
            AllTests.allTests(this);
        }
    }
}
