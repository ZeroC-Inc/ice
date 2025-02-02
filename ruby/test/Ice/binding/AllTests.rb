# Copyright (c) ZeroC, Inc.

def test(b)
    if !b
        raise RuntimeError, 'test assertion failed'
    end
end

def createTestIntfPrx(adapters)
    endpoints = []
    test = nil
    for p in adapters
        test = p.getTestIntf()
        edpts = test.ice_getEndpoints()
        endpoints.concat(edpts)
    end
    return test.ice_endpoints(endpoints)
end

def deactivate(com, adapters)
    for p in adapters
        com.deactivateObjectAdapter(p)
    end
end

def allTests(helper, communicator)
    ref = "communicator:#{helper.getTestEndpoint()}"
    com = Test::RemoteCommunicatorPrx.new(communicator, ref)

    print "testing binding with single endpoint... "
    STDOUT.flush

    adapter = com.createObjectAdapter("Adapter", "default")

    test1 = adapter.getTestIntf()
    test2 = adapter.getTestIntf()
    test(test1.ice_getConnection() == test2.ice_getConnection())

    test1.ice_ping()
    test2.ice_ping()

    com.deactivateObjectAdapter(adapter)

    test3 = test1

    begin
        test3.ice_ping()
        test(false)
    rescue Ice::ConnectionRefusedException
        # Expected
    rescue Ice::ConnectTimeoutException
        # Expected
    end

    puts "ok"

    print "testing binding with multiple endpoints... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter11", "default"))
    adapters.push(com.createObjectAdapter("Adapter12", "default"))
    adapters.push(com.createObjectAdapter("Adapter13", "default"))

    #
    # Ensure that when a connection is opened it's reused for new
    # proxies and that all endpoints are eventually tried.
    #
    names = ["Adapter11", "Adapter12", "Adapter13"]
    while names.length > 0
        adpts = adapters.clone

        test1 = createTestIntfPrx(adpts)
        adpts = adpts.sort_by { rand }
        test2 = createTestIntfPrx(adpts)
        adpts = adpts.sort_by { rand }
        test3 = createTestIntfPrx(adpts)

        test(test1.ice_getConnection() == test2.ice_getConnection())
        test(test2.ice_getConnection() == test3.ice_getConnection())

        name = test1.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
        test1.ice_getConnection().close()
    end

    #
    # Ensure that the proxy correctly caches the connection (we
    # always send the request over the same connection.)
    #
    for a in adapters
        a.getTestIntf().ice_ping()
    end

    t = createTestIntfPrx(adapters)
    name = t.getAdapterName()
    i = 0
    nRetry = 5
    while i < nRetry and t.getAdapterName() == name
        i = i + 1
    end
    test(i == nRetry)

    for a in adapters
        a.getTestIntf().ice_getConnection().close()
    end

    #
    # Deactivate an adapter and ensure that we can still
    # establish the connection to the remaining adapters.
    #
    com.deactivateObjectAdapter(adapters[0])
    names.push("Adapter12")
    names.push("Adapter13")
    while names.length > 0
        adpts = adapters.clone

        test1 = createTestIntfPrx(adpts)
        adpts = adpts.sort_by { rand }
        test2 = createTestIntfPrx(adpts)
        adpts = adpts.sort_by { rand }
        test3 = createTestIntfPrx(adpts)

        test(test1.ice_getConnection() == test2.ice_getConnection())
        test(test2.ice_getConnection() == test3.ice_getConnection())

        name = test1.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
        test1.ice_getConnection().close()
    end

    #
    # Deactivate an adapter and ensure that we can still
    # establish the connection to the remaining adapters.
    #
    com.deactivateObjectAdapter(adapters[2])
    t = createTestIntfPrx(adapters)
    test(t.getAdapterName() == "Adapter12")

    deactivate(com, adapters)

    puts "ok"

    print "testing random endpoint selection... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter21", "default"))
    adapters.push(com.createObjectAdapter("Adapter22", "default"))
    adapters.push(com.createObjectAdapter("Adapter23", "default"))

    t = createTestIntfPrx(adapters)
    test(t.ice_getEndpointSelection() == Ice::EndpointSelectionType::Random)

    names = ["Adapter21", "Adapter22", "Adapter23"]
    while names.length > 0
        name = t.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
        t.ice_getConnection().close()
    end

    t = t.ice_endpointSelection(Ice::EndpointSelectionType::Random)
    test(t.ice_getEndpointSelection() == Ice::EndpointSelectionType::Random)

    names.push("Adapter21")
    names.push("Adapter22")
    names.push("Adapter23")
    while names.length > 0
        name = t.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
        t.ice_getConnection().close()
    end

    deactivate(com, adapters)

    puts "ok"

    print "testing ordered endpoint selection... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter31", "default"))
    adapters.push(com.createObjectAdapter("Adapter32", "default"))
    adapters.push(com.createObjectAdapter("Adapter33", "default"))

    t = createTestIntfPrx(adapters)
    t = t.ice_endpointSelection(Ice::EndpointSelectionType::Ordered)
    test(t.ice_getEndpointSelection() == Ice::EndpointSelectionType::Ordered)
    nRetry = 5

    #
    # Ensure that endpoints are tried in order by deactivating the adapters
    # one after the other.
    #
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter31"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[0])
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter32"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[1])
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter33"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[2])

    begin
        t.getAdapterName()
    rescue Ice::ConnectionRefusedException
        # Expected
    rescue Ice::ConnectTimeoutException
        # Expected
    end

    endpoints = t.ice_getEndpoints()

    adapters = []

    #
    # Now, re-activate the adapters with the same endpoints in the opposite
    # order.
    #
    adapters.push(com.createObjectAdapter("Adapter36", endpoints[2].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter36"
        i = i + 1
    end
    test(i == nRetry)
    t.ice_getConnection().close()
    adapters.push(com.createObjectAdapter("Adapter35", endpoints[1].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter35"
        i = i + 1
    end
    test(i == nRetry)
    t.ice_getConnection().close()
    adapters.push(com.createObjectAdapter("Adapter34", endpoints[0].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter34"
        i = i + 1
    end
    test(i == nRetry)

    deactivate(com, adapters)

    puts "ok"

    print "testing per request binding with single endpoint... "
    STDOUT.flush

    adapter = com.createObjectAdapter("Adapter41", "default")

    test1 = adapter.getTestIntf().ice_connectionCached(false)

    test1.ice_ping()

    com.deactivateObjectAdapter(adapter)

    test3 = test1
    begin
        test(test3.ice_getConnection() == test1.ice_getConnection())
        test(false)
    rescue Ice::ConnectionRefusedException
        # Expected
    rescue Ice::ConnectTimeoutException
        # Expected
    end

    puts "ok"

    print "testing per request binding with multiple endpoints... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter51", "default"))
    adapters.push(com.createObjectAdapter("Adapter52", "default"))
    adapters.push(com.createObjectAdapter("Adapter53", "default"))

    t = createTestIntfPrx(adapters).ice_connectionCached(false)
    test(!t.ice_isConnectionCached())

    names = ["Adapter51", "Adapter52", "Adapter53"]
    while names.length > 0
        name = t.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
    end

    com.deactivateObjectAdapter(adapters[0])

    names.push("Adapter52")
    names.push("Adapter53")
    while names.length > 0
        name = t.getAdapterName()
        if names.include?(name)
            names.delete(name)
        end
    end

    com.deactivateObjectAdapter(adapters[2])

    test(t.getAdapterName() == "Adapter52")

    deactivate(com, adapters)

    puts "ok"

    print "testing per request binding and ordered endpoint selection... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter61", "default"))
    adapters.push(com.createObjectAdapter("Adapter62", "default"))
    adapters.push(com.createObjectAdapter("Adapter63", "default"))

    t = createTestIntfPrx(adapters)
    t = t.ice_endpointSelection(Ice::EndpointSelectionType::Ordered)
    test(t.ice_getEndpointSelection() == Ice::EndpointSelectionType::Ordered)
    t = t.ice_connectionCached(false)
    test(!t.ice_isConnectionCached())
    nRetry = 5

    #
    # Ensure that endpoints are tried in order by deactivating the adapters
    # one after the other.
    #
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter61"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[0])
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter62"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[1])
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter63"
        i = i + 1
    end
    test(i == nRetry)
    com.deactivateObjectAdapter(adapters[2])

    begin
        t.getAdapterName()
    rescue Ice::ConnectionRefusedException
        # Expected
    rescue Ice::ConnectTimeoutException
        # Expected
    end

    endpoints = t.ice_getEndpoints()

    adapters = []

    #
    # Now, re-activate the adapters with the same endpoints in the opposite
    # order.
    #
    adapters.push(com.createObjectAdapter("Adapter66", endpoints[2].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter66"
        i = i + 1
    end
    test(i == nRetry)
    adapters.push(com.createObjectAdapter("Adapter65", endpoints[1].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter65"
        i = i + 1
    end
    test(i == nRetry)
    adapters.push(com.createObjectAdapter("Adapter64", endpoints[0].toString()))
    i = 0
    while i < nRetry and t.getAdapterName() == "Adapter64"
        i = i + 1
    end
    test(i == nRetry)

    deactivate(com, adapters)

    puts "ok"

    print "testing endpoint mode filtering... "
    STDOUT.flush

    adapters = []
    adapters.push(com.createObjectAdapter("Adapter71", "default"))
    adapters.push(com.createObjectAdapter("Adapter72", "udp"))

    t = createTestIntfPrx(adapters)
    test(t.getAdapterName() == "Adapter71")

    testUDP = t.ice_datagram()
    test(t.ice_getConnection() != testUDP.ice_getConnection())
    begin
        testUDP.getAdapterName()
    rescue Ice::TwowayOnlyException
        # Expected
    end

    puts "ok"

    if communicator.getProperties().getIceProperty("Ice.Default.Protocol") == "ssl"
        print "testing unsecure vs. secure endpoints... "
        STDOUT.flush

        adapters = []
        adapters.push(com.createObjectAdapter("Adapter81", "ssl"))
        adapters.push(com.createObjectAdapter("Adapter82", "tcp"))

        t = createTestIntfPrx(adapters)
        for i in 0...5
            test(t.getAdapterName() == "Adapter82")
            t.ice_getConnection().close()
        end

        testSecure = t.ice_secure(true)
        test(testSecure.ice_isSecure())
        testSecure = t.ice_secure(false)
        test(!testSecure.ice_isSecure())
        testSecure = t.ice_secure(true)
        test(testSecure.ice_isSecure())
        test(t.ice_getConnection() != testSecure.ice_getConnection())

        com.deactivateObjectAdapter(adapters[1])

        for i in 0...5
            test(t.getAdapterName() == "Adapter81")
            t.ice_getConnection().close()
        end

        com.createObjectAdapter("Adapter83", (t.ice_getEndpoints()[1]).toString()) # Reactive tcp OA.

        for i in 0...5
            test(t.getAdapterName() == "Adapter83")
            t.ice_getConnection().close()
        end

        com.deactivateObjectAdapter(adapters[0])
        begin
            testSecure.ice_ping()
            test(false)
        rescue Ice::ConnectionRefusedException
            # Expected
        rescue Ice::ConnectTimeoutException
            # Expected
        end

        deactivate(com, adapters)

        puts "ok"
    end

    com.shutdown()
end
