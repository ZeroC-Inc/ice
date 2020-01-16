//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;

namespace Ice.info
{
    public class AllTests : global::Test.AllTests
    {
        private static TCPEndpointInfo getTCPEndpointInfo(EndpointInfo info)
        {
            for (; info != null; info = info.underlying)
            {
                if (info is TCPEndpointInfo)
                {
                    return info as TCPEndpointInfo;
                }
            }
            return null;
        }

        private static TCPConnectionInfo getTCPConnectionInfo(ConnectionInfo info)
        {
            for (; info != null; info = info.Underlying)
            {
                if (info is TCPConnectionInfo)
                {
                    return info as TCPConnectionInfo;
                }
            }
            return null;
        }

        public static void allTests(global::Test.TestHelper helper)
        {
            Communicator communicator = helper.communicator();
            var output = helper.getWriter();
            output.Write("testing proxy endpoint information... ");
            output.Flush();
            {
                var p1 = IObjectPrx.Parse(
                                "test -t:default -h tcphost -p 10000 -t 1200 -z --sourceAddress 10.10.10.10:" +
                                "udp -h udphost -p 10001 --interface eth0 --ttl 5 --sourceAddress 10.10.10.10:" +
                                "opaque -e 1.8 -t 100 -v ABCD", communicator);

                IEndpoint[] endps = p1.Endpoints;

                EndpointInfo info = endps[0].getInfo();
                TCPEndpointInfo tcpEndpoint = getTCPEndpointInfo(info);
                test(tcpEndpoint.host.Equals("tcphost"));
                test(tcpEndpoint.port == 10000);
                test(tcpEndpoint.sourceAddress.Equals("10.10.10.10"));
                test(tcpEndpoint.timeout == 1200);
                test(tcpEndpoint.compress);
                test(!tcpEndpoint.datagram());

                test(tcpEndpoint.type() == TCPEndpointType.value && !tcpEndpoint.secure() ||
                        tcpEndpoint.type() == SSLEndpointType.value && tcpEndpoint.secure() ||
                        tcpEndpoint.type() == WSEndpointType.value && !tcpEndpoint.secure() ||
                        tcpEndpoint.type() == WSSEndpointType.value && tcpEndpoint.secure());
                test(tcpEndpoint.type() == TCPEndpointType.value && info is TCPEndpointInfo ||
                        tcpEndpoint.type() == SSLEndpointType.value && info is IceSSL.EndpointInfo ||
                        tcpEndpoint.type() == WSEndpointType.value && info is WSEndpointInfo ||
                        tcpEndpoint.type() == WSSEndpointType.value && info is WSEndpointInfo);

                UDPEndpointInfo udpEndpoint = (UDPEndpointInfo)endps[1].getInfo();
                test(udpEndpoint.host.Equals("udphost"));
                test(udpEndpoint.port == 10001);
                test(udpEndpoint.mcastInterface.Equals("eth0"));
                test(udpEndpoint.mcastTtl == 5);
                test(udpEndpoint.sourceAddress.Equals("10.10.10.10"));
                test(udpEndpoint.timeout == -1);
                test(!udpEndpoint.compress);
                test(!udpEndpoint.secure());
                test(udpEndpoint.datagram());
                test(udpEndpoint.type() == 3);

                OpaqueEndpointInfo opaqueEndpoint = (OpaqueEndpointInfo)endps[2].getInfo();
                test(opaqueEndpoint.rawBytes.Length > 0);
                test(opaqueEndpoint.rawEncoding.Equals(new EncodingVersion(1, 8)));
            }
            output.WriteLine("ok");

            ObjectAdapter adapter;
            output.Write("test object adapter endpoint information... ");
            output.Flush();
            {
                string host = communicator.GetPropertyAsInt("Ice.IPv6") != 0 ? "::1" : "127.0.0.1";
                communicator.SetProperty("TestAdapter.Endpoints", "tcp -h \"" + host +
                    "\" -t 15000:udp -h \"" + host + "\"");
                System.Console.WriteLine(communicator.GetProperty("TestAdapter.Endpoints"));
                adapter = communicator.createObjectAdapter("TestAdapter");

                IEndpoint[] endpoints = adapter.GetEndpoints();
                test(endpoints.Length == 2);
                IEndpoint[] publishedEndpoints = adapter.GetPublishedEndpoints();
                test(Collections.Equals(endpoints, publishedEndpoints));

                TCPEndpointInfo tcpEndpoint = getTCPEndpointInfo(endpoints[0].getInfo());
                test(tcpEndpoint.type() == TCPEndpointType.value ||
                        tcpEndpoint.type() == SSLEndpointType.value ||
                        tcpEndpoint.type() == WSEndpointType.value ||
                        tcpEndpoint.type() == WSSEndpointType.value);

                test(tcpEndpoint.host.Equals(host));
                test(tcpEndpoint.port > 0);
                test(tcpEndpoint.timeout == 15000);

                UDPEndpointInfo udpEndpoint = (UDPEndpointInfo)endpoints[1].getInfo();
                test(udpEndpoint.host.Equals(host));
                test(udpEndpoint.datagram());
                test(udpEndpoint.port > 0);

                endpoints = new IEndpoint[] { endpoints[0] };
                test(endpoints.Length == 1);
                adapter.SetPublishedEndpoints(endpoints);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                test(Collections.Equals(endpoints, publishedEndpoints));

                adapter.Destroy();

                int port = helper.getTestPort(1);
                communicator.SetProperty("TestAdapter.Endpoints", $"default -h * -p {port}");
                communicator.SetProperty("TestAdapter.PublishedEndpoints", helper.getTestEndpoint(1));
                adapter = communicator.createObjectAdapter("TestAdapter");

                endpoints = adapter.GetEndpoints();
                test(endpoints.Length >= 1);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                test(publishedEndpoints.Length == 1);

                foreach (var endpoint in endpoints)
                {
                    tcpEndpoint = getTCPEndpointInfo(endpoint.getInfo());
                    test(tcpEndpoint.port == port);
                }

                tcpEndpoint = getTCPEndpointInfo(publishedEndpoints[0].getInfo());
                test(tcpEndpoint.host == "127.0.0.1");
                test(tcpEndpoint.port == port);

                adapter.Destroy();
            }
            output.WriteLine("ok");

            int endpointPort = helper.getTestPort(0);

            var @base = IObjectPrx.Parse("test:" +
                                            helper.getTestEndpoint(0) + ":" +
                                            helper.getTestEndpoint(0, "udp"), communicator);
            var testIntf = Test.ITestIntfPrx.CheckedCast(@base);

            string defaultHost = communicator.GetProperty("Ice.Default.Host") ?? "";

            output.Write("test connection endpoint information... ");
            output.Flush();
            {
                EndpointInfo info = @base.GetConnection().Endpoint.getInfo();
                TCPEndpointInfo tcpinfo = getTCPEndpointInfo(info);
                test(tcpinfo.port == endpointPort);
                test(!tcpinfo.compress);
                test(tcpinfo.host.Equals(defaultHost));

                Dictionary<string, string> ctx = testIntf.getEndpointInfoAsContext();
                test(ctx["host"].Equals(tcpinfo.host));
                test(ctx["compress"].Equals("false"));
                int port = int.Parse(ctx["port"]);
                test(port > 0);

                info = @base.Clone(invocationMode: InvocationMode.Datagram).GetConnection().Endpoint.getInfo();
                UDPEndpointInfo udp = (UDPEndpointInfo)info;
                test(udp.port == endpointPort);
                test(udp.host.Equals(defaultHost));
            }
            output.WriteLine("ok");

            output.Write("testing connection information... ");
            output.Flush();
            {
                Connection connection = @base.GetConnection();
                connection.SetBufferSize(1024, 2048);

                ConnectionInfo info = connection.ConnectionInfo;
                TCPConnectionInfo ipInfo = getTCPConnectionInfo(info);
                test(!info.Incoming);
                test(info.AdapterName.Length == 0);
                test(ipInfo.RemotePort == endpointPort);
                test(ipInfo.LocalPort > 0);
                if (defaultHost.Equals("127.0.0.1"))
                {
                    test(ipInfo.LocalAddress.Equals(defaultHost));
                    test(ipInfo.RemoteAddress.Equals(defaultHost));
                }
                test(ipInfo.RcvSize >= 1024);
                test(ipInfo.SndSize >= 2048);

                Dictionary<string, string> ctx = testIntf.getConnectionInfoAsContext();
                test(ctx["incoming"].Equals("true"));
                test(ctx["adapterName"].Equals("TestAdapter"));
                test(ctx["remoteAddress"].Equals(ipInfo.LocalAddress));
                test(ctx["localAddress"].Equals(ipInfo.RemoteAddress));
                test(ctx["remotePort"].Equals(ipInfo.LocalPort.ToString()));
                test(ctx["localPort"].Equals(ipInfo.RemotePort.ToString()));

                if (@base.GetConnection().type().Equals("ws") || @base.GetConnection().type().Equals("wss"))
                {
                    Dictionary<string, string> headers = ((WSConnectionInfo)info).Headers;
                    test(headers["Upgrade"].Equals("websocket"));
                    test(headers["Connection"].Equals("Upgrade"));
                    test(headers["Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    test(headers["Sec-WebSocket-Accept"] != null);

                    test(ctx["ws.Upgrade"].Equals("websocket"));
                    test(ctx["ws.Connection"].Equals("Upgrade"));
                    test(ctx["ws.Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    test(ctx["ws.Sec-WebSocket-Version"].Equals("13"));
                    test(ctx["ws.Sec-WebSocket-Key"] != null);
                }

                connection = @base.Clone(invocationMode: InvocationMode.Datagram).GetConnection();
                connection.SetBufferSize(2048, 1024);

                UDPConnectionInfo udpInfo = (UDPConnectionInfo)connection.ConnectionInfo;
                test(!udpInfo.Incoming);
                test(udpInfo.AdapterName.Length == 0);
                test(udpInfo.LocalPort > 0);
                test(udpInfo.RemotePort == endpointPort);

                if (defaultHost.Equals("127.0.0.1"))
                {
                    test(udpInfo.RemoteAddress.Equals(defaultHost));
                    test(udpInfo.LocalAddress.Equals(defaultHost));
                }
                test(udpInfo.RcvSize >= 2048);
                test(udpInfo.SndSize >= 1024);
            }
            output.WriteLine("ok");

            testIntf.shutdown();

            communicator.shutdown();
            communicator.waitForShutdown();
        }
    }
}
