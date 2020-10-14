// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    internal static class Network
    {
        // Which versions of the Internet Protocol are enabled?
        internal const int EnableIPv4 = 0;
        internal const int EnableIPv6 = 1;
        internal const int EnableBoth = 2;

        internal static Socket CreateServerSocket(IPEndpoint endpoint, AddressFamily family)
        {
            Socket socket = CreateSocket(endpoint.IsDatagram, family);
            if (family == AddressFamily.InterNetworkV6)
            {
                try
                {
                    socket.SetSocketOption(SocketOptionLevel.IPv6,
                                           SocketOptionName.IPv6Only,
                                           endpoint.IsIPv6Only ? 1 : 0);
                }
                catch (SocketException ex)
                {
                    socket.CloseNoThrow();
                    throw new TransportException(ex);
                }
            }
            return socket;
        }

        internal static void SetMulticastInterface(Socket socket, string iface, AddressFamily family)
        {
            if (family == AddressFamily.InterNetwork)
            {
                socket.SetSocketOption(SocketOptionLevel.IP,
                                       SocketOptionName.MulticastInterface,
                                       GetInterfaceAddress(iface, family)!.GetAddressBytes());
            }
            else
            {
                socket.SetSocketOption(SocketOptionLevel.IPv6,
                                       SocketOptionName.MulticastInterface,
                                       GetInterfaceIndex(iface, family));
            }
        }

        internal static Socket CreateSocket(bool udp, AddressFamily family)
        {
            Socket socket;

            try
            {
                if (udp)
                {
                    socket = new Socket(family, SocketType.Dgram, ProtocolType.Udp);
                }
                else
                {
                    socket = new Socket(family, SocketType.Stream, ProtocolType.Tcp);
                }
            }
            catch (SocketException ex)
            {
                throw new TransportException(ex);
            }

            if (!udp)
            {
                try
                {
                    socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);
                    socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.KeepAlive, 1);
                }
                catch (SocketException ex)
                {
                    socket.CloseNoThrow();
                    throw new TransportException(ex);
                }
            }
            return socket;
        }

        internal static IEnumerable<IPEndPoint> GetAddresses(
            string host,
            int port,
            int ipVersion,
            EndpointSelectionType selType)
        {
            try
            {
                ValueTask<IEnumerable<IPEndPoint>> task = GetAddressesAsync(host,
                                                                            port,
                                                                            ipVersion,
                                                                            selType);
                return task.IsCompleted ? task.Result : task.AsTask().Result;
            }
            catch (AggregateException ex)
            {
                Debug.Assert(ex.InnerException != null);
                throw ExceptionUtil.Throw(ex.InnerException);
            }
        }

        internal static async ValueTask<IEnumerable<IPEndPoint>> GetAddressesAsync(
            string host,
            int port,
            int ipVersion,
            EndpointSelectionType selType,
            CancellationToken cancel = default)
        {
            Debug.Assert(host.Length > 0);

            int retry = 5;
            while (true)
            {
                var addresses = new List<IPEndPoint>();
                try
                {
                    // Trying to parse the IP address is necessary to handle wildcard addresses such as 0.0.0.0 or ::0
                    // since GetHostAddressesAsync fails to resolve them.
                    var a = IPAddress.Parse(host);
                    if ((a.AddressFamily == AddressFamily.InterNetwork && ipVersion != EnableIPv6) ||
                        (a.AddressFamily == AddressFamily.InterNetworkV6 && ipVersion != EnableIPv4))
                    {
                        addresses.Add(new IPEndPoint(a, port));
                        return addresses;
                    }
                    else
                    {
                        throw new DNSException(host);
                    }
                }
                catch (FormatException)
                {
                }

                try
                {
                    foreach (IPAddress a in await Dns.GetHostAddressesAsync(host).WaitAsync(cancel).ConfigureAwait(false))
                    {
                        if ((a.AddressFamily == AddressFamily.InterNetwork && ipVersion != EnableIPv6) ||
                            (a.AddressFamily == AddressFamily.InterNetworkV6 && ipVersion != EnableIPv4))
                        {
                            addresses.Add(new IPEndPoint(a, port));
                        }
                    }

                    // No InterNetwork/InterNetworkV6 available.
                    if (addresses.Count == 0)
                    {
                        throw new DNSException(host);
                    }

                    IEnumerable<IPEndPoint> addrs = addresses;
                    if (selType == EndpointSelectionType.Random)
                    {
                        addrs = addrs.Shuffle();
                    }

                    return addrs;
                }
                catch (DNSException)
                {
                    throw;
                }
                catch (SocketException ex)
                {
                    if (ex.SocketErrorCode == SocketError.TryAgain && --retry >= 0)
                    {
                        continue;
                    }
                    throw new DNSException(host, ex);
                }
                catch (Exception ex)
                {
                    throw new DNSException(host, ex);
                }
            }
        }

        internal static async ValueTask<IEnumerable<IPEndPoint>> GetAddressesForClientEndpointAsync(
            string host,
            int port,
            int ipVersion,
            EndpointSelectionType selType,
            CancellationToken cancel)
        {
            Debug.Assert(host.Length > 0);

            return await GetAddressesAsync(host, port, ipVersion, selType, cancel).ConfigureAwait(false);
        }

        internal static IPEndPoint GetAddressForServerEndpoint(string host, int port, int ipVersion)
        {
            // TODO: Fix this method to be asynchronous.

            Debug.Assert(host.Length > 0);

            try
            {
                // Get the addresses for the given host and return the first one
                ValueTask<IEnumerable<IPEndPoint>> task = GetAddressesAsync(host,
                                                                            port,
                                                                            ipVersion,
                                                                            EndpointSelectionType.Ordered);
                return (task.IsCompleted ? task.Result : task.AsTask().Result).First();
            }
            catch (AggregateException ex)
            {
                Debug.Assert(ex.InnerException != null);
                throw ExceptionUtil.Throw(ex.InnerException);
            }
        }

        internal static List<string> GetHostsForEndpointExpand(string host, int ipVersion, bool includeLoopback)
        {
            var hosts = new List<string>();
            if (IsWildcard(host, ipVersion))
            {
                foreach (IPAddress a in GetLocalAddresses(ipVersion, includeLoopback, false))
                {
                    if (!IsLinklocal(a))
                    {
                        hosts.Add(a.ToString());
                    }
                }
                if (hosts.Count == 0)
                {
                    // Return loopback if only loopback is available no other local addresses are available.
                    foreach (IPAddress a in GetLoopbackAddresses(ipVersion))
                    {
                        hosts.Add(a.ToString());
                    }
                }
            }
            return hosts;
        }

        internal static List<string> GetInterfacesForMulticast(string? intf, int ipVersion)
        {
            var interfaces = new List<string>();

            if (intf == null || IsWildcard(intf, ipVersion))
            {
                interfaces.AddRange(GetLocalAddresses(ipVersion, true, true).Select(i => i.ToString()));
            }

            if (intf != null && interfaces.Count == 0)
            {
                interfaces.Add(intf);
            }
            return interfaces;
        }

        internal static int GetIPVersion(IPAddress addr) =>
            addr.AddressFamily == AddressFamily.InterNetwork ? EnableIPv4 : EnableIPv6;

        internal static EndPoint? GetLocalAddress(Socket socket)
        {
            try
            {
                return socket.LocalEndPoint;
            }
            catch (SocketException ex)
            {
                throw new TransportException(ex);
            }
        }

        internal static IPAddress[] GetLocalAddresses(int ipVersion, bool includeLoopback, bool singleAddressPerInterface)
        {
            List<IPAddress> addresses;
            int retry = 5;

        repeatGetHostByName:
            try
            {
                addresses = new List<IPAddress>();
                NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
                foreach (NetworkInterface ni in nics)
                {
                    IPInterfaceProperties ipProps = ni.GetIPProperties();
                    UnicastIPAddressInformationCollection uniColl = ipProps.UnicastAddresses;
                    foreach (UnicastIPAddressInformation uni in uniColl)
                    {
                        if ((uni.Address.AddressFamily == AddressFamily.InterNetwork && ipVersion != EnableIPv6) ||
                           (uni.Address.AddressFamily == AddressFamily.InterNetworkV6 && ipVersion != EnableIPv4))
                        {
                            if (!addresses.Contains(uni.Address) &&
                               (includeLoopback || !IPAddress.IsLoopback(uni.Address)))
                            {
                                addresses.Add(uni.Address);
                                if (singleAddressPerInterface)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            catch (SocketException ex)
            {
                if (ex.SocketErrorCode == SocketError.TryAgain && --retry >= 0)
                {
                    goto repeatGetHostByName;
                }
                throw new TransportException("error retrieving local network interface IP addresses", ex);
            }
            catch (Exception ex)
            {
                throw new TransportException("error retrieving local network interface IP addresses", ex);
            }

            return addresses.ToArray();
        }

        internal static EndPoint? GetRemoteAddress(Socket socket)
        {
            try
            {
                return socket.RemoteEndPoint;
            }
            catch (SocketException)
            {
            }
            return null;
        }

        internal static List<IPAddress> GetLoopbackAddresses(int ipVersion)
        {
            var addresses = new List<IPAddress>();
            if (ipVersion != EnableIPv4)
            {
                addresses.Add(IPAddress.IPv6Loopback);
            }
            if (ipVersion != EnableIPv6)
            {
                addresses.Add(IPAddress.Loopback);
            }
            return addresses;
        }

        internal static bool IsLinklocal(IPAddress addr)
        {
            if (addr.IsIPv6LinkLocal)
            {
                return true;
            }
            else if (addr.AddressFamily == AddressFamily.InterNetwork)
            {
                byte[] bytes = addr.GetAddressBytes();
                return bytes[0] == 169 && bytes[1] == 254;
            }
            return false;
        }

        internal static bool IsMulticast(IPEndPoint addr) =>
            addr.AddressFamily == AddressFamily.InterNetwork ?
                (addr.Address.GetAddressBytes()[0] & 0xF0) == 0xE0 : addr.Address.IsIPv6Multicast;

        internal static string LocalAddrToString(EndPoint? endpoint) => endpoint?.ToString() ?? "<not bound>";

        internal static string RemoteAddrToString(EndPoint? endpoint) => endpoint?.ToString() ?? "<not connected>";

        internal static void SetBufSize(Socket socket, Communicator communicator, Transport transport)
        {
            int rcvSize = communicator.GetPropertyAsByteSize($"Ice.{transport}.RcvSize") ?? 0;
            if (rcvSize > 0)
            {
                // Try to set the buffer size. The kernel will silently adjust the size to an acceptable value. Then
                // read the size back to get the size that was actually set.
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveBuffer, rcvSize);
                int size = (int)socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveBuffer)!;
                if (size < rcvSize)
                {
                    // Warn if the size that was set is less than the requested size and we have not already warned.
                    BufSizeWarnInfo warningInfo = communicator.GetBufSizeWarn(Transport.TCP);
                    if (!warningInfo.RcvWarn || rcvSize != warningInfo.RcvSize)
                    {
                        communicator.Logger.Warning(
                            $"{transport} receive buffer size: requested size of {rcvSize} adjusted to {size}");
                        communicator.SetRcvBufSizeWarn(Transport.TCP, rcvSize);
                    }
                }
            }

            int sndSize = communicator.GetPropertyAsByteSize($"Ice.{transport}.SndSize") ?? 0;
            if (sndSize > 0)
            {
                // Try to set the buffer size. The kernel will silently adjust the size to an acceptable value. Then
                // read the size back to get the size that was actually set.
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendBuffer, sndSize);
                int size = (int)socket.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendBuffer)!;
                if (size < sndSize) // Warn if the size that was set is less than the requested size.
                {
                    // Warn if the size that was set is less than the requested size and we have not already warned.
                    BufSizeWarnInfo warningInfo = communicator.GetBufSizeWarn(Transport.TCP);
                    if (!warningInfo.SndWarn || sndSize != warningInfo.SndSize)
                    {
                        communicator.Logger.Warning(
                            $"{transport} send buffer size: requested size of {sndSize} adjusted to {size}");
                        communicator.SetSndBufSizeWarn(Transport.TCP, sndSize);
                    }
                }
            }
        }

        internal static void SetMulticastGroup(Socket s, IPAddress group, string? iface)
        {
            var indexes = new HashSet<int>();
            foreach (string intf in GetInterfacesForMulticast(iface, GetIPVersion(group)))
            {
                if (group.AddressFamily == AddressFamily.InterNetwork)
                {
                    MulticastOption option;
                    IPAddress? addr = GetInterfaceAddress(intf, group.AddressFamily);
                    if (addr == null)
                    {
                        option = new MulticastOption(group);
                    }
                    else
                    {
                        option = new MulticastOption(group, addr);
                    }
                    s.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.AddMembership, option);
                }
                else
                {
                    int index = GetInterfaceIndex(intf, group.AddressFamily);
                    if (!indexes.Contains(index))
                    {
                        indexes.Add(index);
                        IPv6MulticastOption option;
                        if (index == -1)
                        {
                            option = new IPv6MulticastOption(group);
                        }
                        else
                        {
                            option = new IPv6MulticastOption(group, index);
                        }
                        s.SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.AddMembership, option);
                    }
                }
            }
        }

        internal static string SocketToString(Socket socket, INetworkProxy? proxy, EndPoint? target)
        {
            try
            {
                if (socket == null)
                {
                    return "<closed>";
                }

                EndPoint? remote = GetRemoteAddress(socket);

                var s = new System.Text.StringBuilder();
                s.Append("local address = " + LocalAddrToString(GetLocalAddress(socket)));
                if (proxy != null)
                {
                    if (remote == null)
                    {
                        remote = proxy.Address;
                    }
                    s.Append("\n" + proxy.Name + " proxy address = " + RemoteAddrToString(remote));
                    s.Append("\nremote address = " + RemoteAddrToString(target));
                }
                else
                {
                    if (remote == null)
                    {
                        remote = target;
                    }
                    s.Append("\nremote address = " + RemoteAddrToString(remote));
                }
                return s.ToString();
            }
            catch (ObjectDisposedException)
            {
                return "<closed>";
            }
        }

        internal static void SetMulticastTtl(Socket socket, int ttl, AddressFamily family)
        {
            if (family == AddressFamily.InterNetwork)
            {
                socket.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.MulticastTimeToLive, ttl);
            }
            else
            {
                socket.SetSocketOption(SocketOptionLevel.IPv6, SocketOptionName.MulticastTimeToLive, ttl);
            }
        }

        internal static string SocketToString(Socket? socket)
        {
            try
            {
                if (socket == null)
                {
                    return "<closed>";
                }
                var s = new System.Text.StringBuilder();
                s.Append("local address = " + LocalAddrToString(GetLocalAddress(socket)));
                s.Append("\nremote address = " + RemoteAddrToString(GetRemoteAddress(socket)));
                return s.ToString();
            }
            catch (ObjectDisposedException)
            {
                return "<closed>";
            }
        }

        private static IPAddress? GetInterfaceAddress(string iface, AddressFamily family)
        {
            Debug.Assert(iface.Length > 0);

            // The iface parameter must either be an IP address, an index or the name of an interface. If it's an index
            // we just return it. If it's an IP addess we search for an interface which has this IP address. If it's a
            // name we search an interface with this name.
            try
            {
                return IPAddress.Parse(iface);
            }
            catch (FormatException)
            {
            }

            NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
            try
            {
                int index = int.Parse(iface, CultureInfo.InvariantCulture);
                foreach (NetworkInterface ni in nics)
                {
                    IPInterfaceProperties ipProps = ni.GetIPProperties();
                    int interfaceIndex = -1;
                    if (family == AddressFamily.InterNetwork)
                    {
                        IPv4InterfaceProperties ipv4Props = ipProps.GetIPv4Properties();
                        if (ipv4Props != null && ipv4Props.Index == index)
                        {
                            interfaceIndex = ipv4Props.Index;
                        }
                    }
                    else
                    {
                        IPv6InterfaceProperties ipv6Props = ipProps.GetIPv6Properties();
                        if (ipv6Props != null && ipv6Props.Index == index)
                        {
                            interfaceIndex = ipv6Props.Index;
                        }
                    }
                    if (interfaceIndex >= 0)
                    {
                        foreach (UnicastIPAddressInformation a in ipProps.UnicastAddresses)
                        {
                            if (a.Address.AddressFamily == family)
                            {
                                return a.Address;
                            }
                        }
                    }
                }
            }
            catch (FormatException)
            {
            }

            foreach (NetworkInterface ni in nics)
            {
                if (ni.Name == iface)
                {
                    IPInterfaceProperties ipProps = ni.GetIPProperties();
                    foreach (UnicastIPAddressInformation a in ipProps.UnicastAddresses)
                    {
                        if (a.Address.AddressFamily == family)
                        {
                            return a.Address;
                        }
                    }
                }
            }

            throw new ArgumentException("couldn't find interface `" + iface + "'");
        }

        private static int GetInterfaceIndex(string iface, AddressFamily family)
        {
            if (iface.Length == 0)
            {
                return -1;
            }

            // The iface parameter must either be an IP address, an index or the name of an interface. If it's an index
            // we just return it. If it's an IP addess we search for an interface which has this IP address. If it's a
            // name we search an interface with this name.
            try
            {
                return int.Parse(iface, CultureInfo.InvariantCulture);
            }
            catch (FormatException)
            {
            }

            NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
            try
            {
                var addr = IPAddress.Parse(iface);
                foreach (NetworkInterface ni in nics)
                {
                    IPInterfaceProperties ipProps = ni.GetIPProperties();
                    foreach (UnicastIPAddressInformation uni in ipProps.UnicastAddresses)
                    {
                        if (uni.Address.Equals(addr))
                        {
                            if (addr.AddressFamily == AddressFamily.InterNetwork)
                            {
                                IPv4InterfaceProperties ipv4Props = ipProps.GetIPv4Properties();
                                if (ipv4Props != null)
                                {
                                    return ipv4Props.Index;
                                }
                            }
                            else
                            {
                                IPv6InterfaceProperties ipv6Props = ipProps.GetIPv6Properties();
                                if (ipv6Props != null)
                                {
                                    return ipv6Props.Index;
                                }
                            }
                        }
                    }
                }
            }
            catch (FormatException)
            {
            }

            foreach (NetworkInterface ni in nics)
            {
                if (ni.Name == iface)
                {
                    IPInterfaceProperties ipProps = ni.GetIPProperties();
                    if (family == AddressFamily.InterNetwork)
                    {
                        IPv4InterfaceProperties ipv4Props = ipProps.GetIPv4Properties();
                        if (ipv4Props != null)
                        {
                            return ipv4Props.Index;
                        }
                    }
                    else
                    {
                        IPv6InterfaceProperties ipv6Props = ipProps.GetIPv6Properties();
                        if (ipv6Props != null)
                        {
                            return ipv6Props.Index;
                        }
                    }
                }
            }

            throw new ArgumentException("couldn't find interface `" + iface + "'");
        }

        private static bool IsWildcard(string address, int ipVersion)
        {
            Debug.Assert(address.Length > 0);

            try
            {
                var addr = IPAddress.Parse(address);
                return ipVersion != EnableIPv4 ? addr.Equals(IPAddress.IPv6Any) : addr.Equals(IPAddress.Any);
            }
            catch (Exception)
            {
            }

            return false;
        }
    }
}
