//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Linq;
using Test;

namespace ZeroC.Ice.Test.Info
{
    public class AllTests
    {
        public static void allTests(TestHelper helper)
        {
            Communicator? communicator = helper.Communicator();
            TestHelper.Assert(communicator != null);
            var output = helper.GetWriter();
            output.Write("testing proxy endpoint information... ");
            output.Flush();
            {
                var p1 = IObjectPrx.Parse(
                                "test -t:default -h tcphost -p 10000 -t 1200 -z --sourceAddress 10.10.10.10:" +
                                "udp -h udphost -p 10001 --interface eth0 --ttl 5 --sourceAddress 10.10.10.10:" +
                                "opaque -e 1.8 -t 100 -v ABCD", communicator);

                var endps = p1.Endpoints;

                Endpoint endpoint = endps[0];
                var tcpEndpoint = endpoint as TcpEndpoint;
                TestHelper.Assert(tcpEndpoint != null);
                TestHelper.Assert(tcpEndpoint.Host.Equals("tcphost"));
                TestHelper.Assert(tcpEndpoint.Port == 10000);
                TestHelper.Assert(tcpEndpoint.SourceAddress!.ToString().Equals("10.10.10.10"));
                TestHelper.Assert(tcpEndpoint.Timeout == 1200);
                TestHelper.Assert(tcpEndpoint.HasCompressionFlag);
                TestHelper.Assert(!tcpEndpoint.IsDatagram);

                TestHelper.Assert(tcpEndpoint.Transport == Transport.TCP && !tcpEndpoint.IsSecure ||
                        tcpEndpoint.Transport == Transport.SSL && tcpEndpoint.IsSecure ||
                        tcpEndpoint.Transport == Transport.WS && !tcpEndpoint.IsSecure ||
                        tcpEndpoint.Transport == Transport.WSS && tcpEndpoint.IsSecure);
                TestHelper.Assert(tcpEndpoint.Transport == Transport.TCP && endpoint is TcpEndpoint ||
                        tcpEndpoint.Transport == Transport.SSL && endpoint is SslEndpoint ||
                        tcpEndpoint.Transport == Transport.WS && endpoint is WSEndpoint ||
                        tcpEndpoint.Transport == Transport.WSS && endpoint is WSEndpoint);

                UdpEndpoint udpEndpoint = (UdpEndpoint)endps[1];
                TestHelper.Assert(udpEndpoint.Host.Equals("udphost"));
                TestHelper.Assert(udpEndpoint.Port == 10001);
                TestHelper.Assert(udpEndpoint.McastInterface.Equals("eth0"));
                TestHelper.Assert(udpEndpoint.McastTtl == 5);
                TestHelper.Assert(udpEndpoint.SourceAddress!.ToString().Equals("10.10.10.10"));
                TestHelper.Assert(udpEndpoint.Timeout == -1);
                TestHelper.Assert(!udpEndpoint.HasCompressionFlag);
                TestHelper.Assert(!udpEndpoint.IsSecure);
                TestHelper.Assert(udpEndpoint.IsDatagram);
                TestHelper.Assert(udpEndpoint.Transport == Transport.UDP);

                OpaqueEndpoint opaqueEndpoint = (OpaqueEndpoint)endps[2];
                TestHelper.Assert(opaqueEndpoint.Bytes.Length > 0);
                TestHelper.Assert(opaqueEndpoint.Encoding.Equals(new Encoding(1, 8)));
            }
            output.WriteLine("ok");

            ObjectAdapter adapter;
            output.Write("test object adapter endpoint information... ");
            output.Flush();
            {
                string host = (communicator.GetPropertyAsBool("Ice.IPv6") ?? false) ? "::1" : "127.0.0.1";
                communicator.SetProperty("TestAdapter.Endpoints", "tcp -h \"" + host +
                    "\" -t 15000:udp -h \"" + host + "\"");
                adapter = communicator.CreateObjectAdapter("TestAdapter");

                var endpoints = adapter.GetEndpoints();
                TestHelper.Assert(endpoints.Count == 2);
                var publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(endpoints.SequenceEqual(publishedEndpoints));

                var tcpEndpoint = endpoints[0] as TcpEndpoint;
                TestHelper.Assert(tcpEndpoint != null);
                TestHelper.Assert(
                        tcpEndpoint.Transport == Transport.TCP ||
                        tcpEndpoint.Transport == Transport.SSL ||
                        tcpEndpoint.Transport == Transport.WS ||
                        tcpEndpoint.Transport == Transport.WSS);

                TestHelper.Assert(tcpEndpoint.Host.Equals(host));
                TestHelper.Assert(tcpEndpoint.Port > 0);
                TestHelper.Assert(tcpEndpoint.Timeout == 15000);

                UdpEndpoint udpEndpoint = (UdpEndpoint)endpoints[1];
                TestHelper.Assert(udpEndpoint.Host.Equals(host));
                TestHelper.Assert(udpEndpoint.IsDatagram);
                TestHelper.Assert(udpEndpoint.Port > 0);

                endpoints = new List<Endpoint> { endpoints[0] };
                TestHelper.Assert(endpoints.Count == 1);

                adapter.SetPublishedEndpoints(endpoints);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(endpoints.SequenceEqual(publishedEndpoints));

                adapter.Destroy();

                int port = helper.GetTestPort(1);
                communicator.SetProperty("TestAdapter.Endpoints", $"default -h * -p {port}");
                communicator.SetProperty("TestAdapter.PublishedEndpoints", helper.GetTestEndpoint(1));
                adapter = communicator.CreateObjectAdapter("TestAdapter");

                endpoints = adapter.GetEndpoints();
                TestHelper.Assert(endpoints.Count >= 1);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(publishedEndpoints.Count == 1);

                foreach (var endpoint in endpoints)
                {
                    tcpEndpoint = endpoint as TcpEndpoint;
                    TestHelper.Assert(tcpEndpoint!.Port == port);
                }

                tcpEndpoint = publishedEndpoints[0] as TcpEndpoint;
                TestHelper.Assert(tcpEndpoint!.Host == "127.0.0.1");
                TestHelper.Assert(tcpEndpoint!.Port == port);

                adapter.Destroy();
            }
            output.WriteLine("ok");

            int endpointPort = helper.GetTestPort(0);

            var testIntf = ITestIntfPrx.Parse("test:" +
                                              helper.GetTestEndpoint(0) + ":" +
                                              helper.GetTestEndpoint(0, "udp"), communicator);

            string defaultHost = communicator.GetProperty("Ice.Default.Host") ?? "";

            output.Write("test connection endpoint information... ");
            output.Flush();
            {
                Endpoint endpoint = testIntf.GetConnection()!.Endpoint;
                var tcpEndpoint = endpoint as TcpEndpoint;
                TestHelper.Assert(tcpEndpoint != null);
                TestHelper.Assert(tcpEndpoint.Port == endpointPort);
                TestHelper.Assert(!tcpEndpoint.HasCompressionFlag);
                TestHelper.Assert(tcpEndpoint.Host.Equals(defaultHost));

                Dictionary<string, string> ctx = testIntf.getEndpointInfoAsContext();
                TestHelper.Assert(ctx["host"].Equals(tcpEndpoint.Host));
                TestHelper.Assert(ctx["compress"].Equals("false"));
                int port = int.Parse(ctx["port"]);
                TestHelper.Assert(port > 0);

                endpoint = testIntf.Clone(invocationMode: InvocationMode.Datagram).GetConnection()!.Endpoint;
                UdpEndpoint udp = (UdpEndpoint)endpoint;
                TestHelper.Assert(udp.Port == endpointPort);
                TestHelper.Assert(udp.Host.Equals(defaultHost));
            }
            output.WriteLine("ok");

            output.Write("testing connection information... ");
            output.Flush();
            {
                IPConnection connection = (IPConnection)testIntf.GetConnection()!;

                TestHelper.Assert(!connection.IsIncoming);
                TestHelper.Assert(connection.Adapter == null);
                TestHelper.Assert(connection.RemoteEndpoint!.Port == endpointPort);
                TestHelper.Assert(connection.LocalEndpoint!.Port > 0);
                if (defaultHost.Equals("127.0.0.1"))
                {
                    TestHelper.Assert(connection.LocalEndpoint!.Address.ToString().Equals(defaultHost));
                    TestHelper.Assert(connection.RemoteEndpoint!.Address.ToString().Equals(defaultHost));
                }

                Dictionary<string, string> ctx = testIntf.getConnectionInfoAsContext();
                TestHelper.Assert(ctx["incoming"].Equals("true"));
                TestHelper.Assert(ctx["adapterName"].Equals("TestAdapter"));
                TestHelper.Assert(ctx["remoteAddress"].Equals(connection.LocalEndpoint!.Address.ToString()));
                TestHelper.Assert(ctx["localAddress"].Equals(connection.RemoteEndpoint!.Address.ToString()));
                TestHelper.Assert(ctx["remotePort"].Equals(connection.LocalEndpoint!.Port.ToString()));
                TestHelper.Assert(ctx["localPort"].Equals(connection.RemoteEndpoint!.Port.ToString()));

                if ((connection as WSConnection)?.Headers is IReadOnlyDictionary<string, string> headers)
                {
                    TestHelper.Assert(headers["Upgrade"].Equals("websocket"));
                    TestHelper.Assert(headers["Connection"].Equals("Upgrade"));
                    TestHelper.Assert(headers["Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    TestHelper.Assert(headers["Sec-WebSocket-Accept"] != null);

                    TestHelper.Assert(ctx["ws.Upgrade"].Equals("websocket"));
                    TestHelper.Assert(ctx["ws.Connection"].Equals("Upgrade"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Version"].Equals("13"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Key"] != null);
                }

                connection = (IPConnection)testIntf.Clone(invocationMode: InvocationMode.Datagram).GetConnection()!;

                var udpConnection = connection as UdpConnection;
                TestHelper.Assert(udpConnection != null);
                TestHelper.Assert(!udpConnection.IsIncoming);
                TestHelper.Assert(udpConnection.Adapter == null);
                TestHelper.Assert(udpConnection.LocalEndpoint?.Port > 0);
                TestHelper.Assert(udpConnection.RemoteEndpoint?.Port == endpointPort);

                if (defaultHost.Equals("127.0.0.1"))
                {
                    TestHelper.Assert(udpConnection.RemoteEndpoint.Address.ToString().Equals(defaultHost));
                    TestHelper.Assert(udpConnection.LocalEndpoint.Address.ToString().Equals(defaultHost));
                }
            }
            output.WriteLine("ok");

            testIntf.shutdown();

            communicator.Shutdown();
            communicator.WaitForShutdown();
        }
    }
}
