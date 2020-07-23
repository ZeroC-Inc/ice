//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Diagnostics;
using System.Linq;

namespace ZeroC.Ice
{

    /// <summary>Provides helper methods to parse and print URI strings that comply with the ice and ice+transport
    /// URI schemes.</summary>
    internal static class UriParser
    {
        /// <summary>Provides the proxy options parsed by the UriParser.</summary>
        internal struct ProxyOptions
        {
            // TODO: add more proxy options
            internal Encoding? Encoding;
            internal Protocol? Protocol;
        }

        // Common options for the ice and ice[+transport] parsers we register for each transport.
        private const GenericUriParserOptions ParserOptions =
            GenericUriParserOptions.DontConvertPathBackslashes |
            GenericUriParserOptions.DontUnescapePathDotsAndSlashes |
            GenericUriParserOptions.Idn |
            GenericUriParserOptions.IriParsing |
            GenericUriParserOptions.NoUserInfo;

        /// <summary>Checks if a string is an ice+transport URI, and not an endpoint string using the ice1 string
        /// format.</summary>
        /// <param name="s">The string to check.</param>
        /// <returns>True when the string is most likely an ice+transport URI; otherwise, false.</returns>
        internal static bool IsEndpointUri(string s) => s.StartsWith("ice+") && s.Contains("://");

        /// <summary>Checks if a string is an ice or ice+transport URI, and not a proxy string using the ice1 string
        /// format.</summary>
        /// <param name="s">The string to check.</param>
        /// <returns>True when the string is most likely an ice or ice+transport URI; otherwise, false.</returns>
        internal static bool IsProxyUri(string s) => s.StartsWith("ice:") || IsEndpointUri(s);

        /// <summary>Parses an ice+transport URI string that represents one or more object adapter endpoints.</summary>
        /// <param name="uriString">The URI string to parse.</param>
        /// <param name="communicator">The communicator.</param>
        /// <returns>The list of endpoints.</returns>
        internal static IReadOnlyList<Endpoint> ParseEndpoints(string uriString, Communicator communicator) =>
            Parse(uriString, oaEndpoints: true, communicator).Endpoints;

        /// <summary>Parses a relative URI [category/]name[#facet] into an identity and facet.</summary>
        internal static (Identity Identity, string Facet) ParseIdentityAndFacet(string uriString)
        {
            // First extract the facet, if any
            string facet = "";
            string path;
            int hashPos = uriString.IndexOf('#');
            if (hashPos != -1 && hashPos != uriString.Length - 1)
            {
                facet = Uri.UnescapeDataString(uriString.Substring(hashPos + 1));
                path = uriString[0..hashPos];
            }
            else
            {
                path = uriString;
            }
            return (Identity.Parse(path), facet);
        }

        /// <summary>Parses an ice or ice+transport URI string that represents a proxy.</summary>
        /// <param name="uriString">The URI string to parse.</param>
        /// <param name="communicator">The communicator.</param>
        /// <returns>The components of the proxy.</returns>
        internal static (IReadOnlyList<Endpoint> Endpoints,
                        List<string> Path,
                        ProxyOptions ProxyOptions,
                        string Facet) ParseProxy(string uriString, Communicator communicator)
        {
            (Uri uri, IReadOnlyList<Endpoint> endpoints, ProxyOptions proxyOptions) =
                Parse(uriString, oaEndpoints: false, communicator);

            string facet = uri.Fragment.Length >= 2 ? Uri.UnescapeDataString(uri.Fragment.TrimStart('#')) : "";
            var path = uri.AbsolutePath.TrimStart('/').Split('/').Select(s => Uri.UnescapeDataString(s)).ToList();
            return (endpoints, path, proxyOptions, facet);
        }

        /// <summary>Registers the ice and ice+universal schemes.</summary>
        internal static void RegisterCommon()
        {
            RegisterTransport("universal", UniversalEndpoint.DefaultUniversalPort);

            // There is actually no authority at all with the ice scheme, but we emulate it with an empty authority
            // during parsing by the Uri class and the GenericUriParser.
            GenericUriParserOptions options =
                ParserOptions |
                GenericUriParserOptions.AllowEmptyAuthority |
                GenericUriParserOptions.NoPort;

            System.UriParser.Register(new GenericUriParser(options), "ice", -1);
        }

        /// <summary>Registers an ice+transport scheme.</summary>
        /// <param name="transportName">The name of the transport (cannot be empty).</param>
        /// <param name="defaultPort">The default port for this transport.</param>
        internal static void RegisterTransport(string transportName, ushort defaultPort) =>
            System.UriParser.Register(new GenericUriParser(ParserOptions), $"ice+{transportName}", defaultPort);

        private static Endpoint CreateEndpoint(
            Communicator communicator,
            bool oaEndpoint,
            Dictionary<string, string> options,
            Protocol protocol,
            Uri uri,
            string uriString)
        {
            Debug.Assert(uri.Scheme.StartsWith("ice+"));
            string transportName = uri.Scheme.Substring(4); // i.e. chop-off "ice+"

            ushort port;
            checked
            {
                port = (ushort)uri.Port;
            }

            IEndpointFactory? factory = null;
            Transport transport;

            if (transportName == "universal")
            {
                if (oaEndpoint)
                {
                    throw new FormatException("ice+universal cannot specify an object adapter endpoint");
                }

                // Enumerator names can only be used for "well-known" transports.
                transport = Enum.Parse<Transport>(options["transport"], ignoreCase: true);
                options.Remove("transport");

                if (protocol == Protocol.Ice2)
                {
                    // It's possible we have a factory for this transport, and we check it only when the protocol is
                    // ice2 (otherwise, we want to create a UniversalEndpoint).
                    factory = communicator.IceFindEndpointFactory(transport);
                }
            }
            else if (communicator.FindEndpointFactory(transportName) is (EndpointFactory f, Transport t))
            {
                if (protocol != Protocol.Ice2)
                {
                    throw new FormatException(
                        $"cannot create an `{uri.Scheme}' endpoint for protocol `{protocol.GetName()}'");
                }
                factory = f;
                transport = t;
            }
            else
            {
                throw new FormatException($"unknown transport `{transportName}'");
            }

            Endpoint endpoint = factory?.Create(transport,
                                                protocol,
                                                uri.DnsSafeHost,
                                                port,
                                                options,
                                                oaEndpoint,
                                                uriString) ??
                new UniversalEndpoint(communicator, transport, protocol, uri.DnsSafeHost, port, options);

            if (options.Count > 0)
            {
                throw new FormatException($"unknown option `{options.First().Key}' for transport `{transportName}'");
            }
            return endpoint;
        }

        /// <summary>Creates a Uri and parses its query.</summary>
        /// <param name="uriString">The string to parse.</param>
        /// <param name="pureEndpoints">When true, the string represents one or more endpoints, and proxy options are
        /// not allowed in the query.</param>
        /// <param name="endpointOptions">A dictionary that accepts the parsed endpoint options. Set to null when
        /// parsing an ice URI (and in this case pureEndpoints must be false).</param>
        /// <returns>The parsed URI, the alt-endpoint option (if set) and the ProxyOptions struct.</returns>
        private static (Uri Uri, string? AltEndpoint, ProxyOptions ProxyOptions) InitialParse(
            string uriString,
            bool pureEndpoints,
            Dictionary<string, string>? endpointOptions)
        {
            if (endpointOptions == null) // i.e. ice scheme
            {
                Debug.Assert(uriString.StartsWith("ice:"));
                Debug.Assert(!pureEndpoints);

                string body = uriString.Substring(4);
                if (body.StartsWith("//"))
                {
                    throw new FormatException("the ice URI scheme cannot define a host or port");
                }
                // Add empty authority for Uri's constructor.
                if (body.StartsWith('/'))
                {
                    uriString = $"ice://{body}";
                }
                else
                {
                    uriString = $"ice:///{body}";
                }
            }

            var uri = new Uri(uriString);

            if (pureEndpoints)
            {
                Debug.Assert(uri.AbsolutePath[0] == '/'); // there is always a first segment
                if (uri.AbsolutePath.Length > 1 || uri.Fragment.Length > 0)
                {
                    throw new FormatException($"endpoint `{uriString}' must not specify a path or fragment");
                }
            }

            string[] nvPairs = uri.Query.Length >= 2 ? uri.Query.TrimStart('?').Split('&') : Array.Empty<string>();

            string? altEndpoint = null;
            ProxyOptions proxyOptions = default;

            foreach (string p in nvPairs)
            {
                int equalPos = p.IndexOf('=');
                if (equalPos <= 0 || equalPos == p.Length - 1)
                {
                    throw new FormatException($"invalid option `{p}'");
                }
                string name = p.Substring(0, equalPos);
                string value = p.Substring(equalPos + 1);

                if (name == "encoding")
                {
                    if (pureEndpoints)
                    {
                        throw new FormatException($"encoding is not a valid option for endpoint `{uriString}'");
                    }
                    if (proxyOptions.Encoding != null)
                    {
                        throw new FormatException($"multiple encoding options in `{uriString}'");
                    }
                    proxyOptions.Encoding = Encoding.Parse(value);
                }
                else if (name == "protocol")
                {
                    if (pureEndpoints)
                    {
                        throw new FormatException($"protocol is not a valid option for endpoint `{uriString}'");
                    }
                    if (proxyOptions.Protocol != null)
                    {
                        throw new FormatException($"multiple protocol options in `{uriString}'");
                    }
                    proxyOptions.Protocol = ProtocolExtensions.Parse(value);
                    if (proxyOptions.Protocol == Protocol.Ice1)
                    {
                        throw new FormatException("the URI format does not support protocol ice1");
                    }
                }
                else if (endpointOptions == null)
                {
                    throw new FormatException($"the ice URI scheme does not support option `{name}'");
                }
                else if (name == "alt-endpoint")
                {
                    altEndpoint = altEndpoint == null ? value : $"{altEndpoint},{value}";
                }
                else
                {
                    if (endpointOptions.TryGetValue(name, out string? existingValue))
                    {
                        endpointOptions[name] = $"{existingValue},{value}";
                    }
                    else
                    {
                        endpointOptions.Add(name, value);
                    }
                }
            }
            return (uri, altEndpoint, proxyOptions);
        }

        /// <summary>Parses an ice or ice+transport URI string.</summary>
        /// <param name="uriString">The URI string to parse.</param>
        /// <param name="oaEndpoints">True when parsing the endpoints of an object adapter; false when parsing a proxy.
        /// </param>
        /// <param name="communicator">The communicator.</param>
        /// <returns>The Uri and endpoints of the ice or ice+transport URI.</returns>
        private static (Uri Uri, IReadOnlyList<Endpoint> Endpoints, ProxyOptions ProxyOptions) Parse(
            string uriString,
            bool oaEndpoints,
            Communicator communicator)
        {
            Debug.Assert(IsProxyUri(uriString));

            try
            {
                bool iceScheme = uriString.StartsWith("ice:");
                if (iceScheme && oaEndpoints)
                {
                    throw new FormatException("an object adapter endpoint supports only ice+transport URIs");
                }

                Dictionary<string, string>? endpointOptions = iceScheme ? null : new Dictionary<string, string>();

                (Uri uri, string? altEndpoint, ProxyOptions proxyOptions) =
                    InitialParse(uriString, pureEndpoints: oaEndpoints, endpointOptions);

                Protocol protocol = proxyOptions.Protocol ?? Protocol.Ice2;

                List<Endpoint>? endpoints = null;

                if (endpointOptions != null) // i.e. not ice scheme
                {
                    endpoints = new List<Endpoint>
                    {
                        CreateEndpoint(communicator, oaEndpoints, endpointOptions, protocol, uri, uriString)
                    };

                    if (altEndpoint != null)
                    {
                        foreach (string endpointStr in altEndpoint.Split(','))
                        {
                            if (endpointStr.StartsWith("ice:"))
                            {
                                throw new FormatException(
                                    $"invalid URI scheme for endpoint `{endpointStr}': must be empty or ice+transport");
                            }

                            string altUriString = endpointStr;
                            if (!altUriString.StartsWith("ice+"))
                            {
                                altUriString = $"{uri.Scheme}://{altUriString}";
                            }

                            // The separator for endpoint options in alt-endpoint is $, and we replace these $ by &
                            // before sending the string the main parser (InitialParse), which uses & as separator.
                            altUriString = altUriString.Replace('$', '&');

                            // No need to clear endpointOptions before reusing it since CreateEndpoint consumes all the
                            // endpoint options
                            Debug.Assert(endpointOptions.Count == 0);

                            (Uri endpointUri, string? endpointAltEndpoint, _) =
                                InitialParse(altUriString, pureEndpoints: true, endpointOptions);

                            if (endpointAltEndpoint != null)
                            {
                                throw new FormatException(
                                    $"invalid option `alt-endpoint' in endpoint `{endpointStr}'");
                            }

                            endpoints.Add(CreateEndpoint(communicator,
                                                         oaEndpoints,
                                                         endpointOptions,
                                                         protocol,
                                                         endpointUri,
                                                         endpointStr));
                        }
                    }
                }
                return (uri, (IReadOnlyList<Endpoint>?)endpoints ?? ImmutableArray<Endpoint>.Empty, proxyOptions);
            }
            catch (Exception ex)
            {
                // Give context to the exception.
                throw new FormatException($"failed to parse URI `{uriString}'", ex);
            }
        }
    }
}
