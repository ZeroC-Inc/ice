// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ZeroC.Ice
{
    internal sealed class SslTransceiver : ITransceiver
    {
        public Socket? Socket => _underlying.Socket;
        public SslStream? SslStream { get; private set; }

        private readonly string? _adapterName;
        private readonly Communicator _communicator;
        private readonly Endpoint _endpoint;
        private readonly SslEngine _engine;
        private readonly string? _host;
        private readonly bool _incoming;
        private BufferedStream? _writeStream;
        private readonly ITransceiver _underlying;

        public async ValueTask InitializeAsync(CancellationToken cancel)
        {
            await _underlying.InitializeAsync(cancel).ConfigureAwait(false);

            SslStream = new SslStream(new NetworkStream(_underlying.Socket!, false), false);

            try
            {
                if (_incoming)
                {
                    var options = new SslServerAuthenticationOptions();
                    options.ServerCertificate = _engine.TlsServerOptions.ServerCertificate;
                    options.ClientCertificateRequired = _engine.TlsServerOptions.RequireClientCertificate;
                    options.EnabledSslProtocols = _engine.TlsServerOptions.EnabledSslProtocols!.Value;
                    options.RemoteCertificateValidationCallback =
                        _engine.TlsServerOptions.ClientCertificateValidationCallback ??
                        RemoteCertificateValidationCallback;
                    options.CertificateRevocationCheckMode = X509RevocationMode.NoCheck;
                    await SslStream.AuthenticateAsServerAsync(options, cancel).ConfigureAwait(false);
                }
                else
                {
                    var options = new SslClientAuthenticationOptions();
                    options.TargetHost = _host;
                    options.ClientCertificates = _engine.TlsClientOptions.ClientCertificates;
                    options.EnabledSslProtocols = _engine.TlsClientOptions.EnabledSslProtocols!.Value;
                    options.RemoteCertificateValidationCallback =
                        _engine.TlsClientOptions.ServerCertificateValidationCallback ??
                        RemoteCertificateValidationCallback;
                    options.LocalCertificateSelectionCallback =
                        _engine.TlsClientOptions.ClientCertificateSelectionCallback ??
                        (options.ClientCertificates?.Count > 0 ?
                            CertificateSelectionCallback : (LocalCertificateSelectionCallback?)null);
                    options.CertificateRevocationCheckMode = X509RevocationMode.NoCheck;
                    await SslStream.AuthenticateAsClientAsync(options, cancel).ConfigureAwait(false);
                }
            }
            catch (IOException ex) when (ex.IsConnectionLost())
            {
                throw new ConnectionLostException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            catch (IOException ex)
            {
                throw new TransportException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            catch (AuthenticationException ex)
            {
                throw new TransportException(ex, RetryPolicy.OtherReplica, _endpoint);
            }

            if (_engine.SecurityTraceLevel >= 1)
            {
                _engine.TraceStream(SslStream, ToString());
            }

            // Use a buffered stream for writes. This ensures that small requests which are composed of multiple
            // small buffers will be sent within a single SSL frame.
            _writeStream = new BufferedStream(SslStream);
        }

        public ValueTask CloseAsync(Exception exception, CancellationToken cancel) =>
            // TODO: implement TLS close_notify and call ShutdownAsync? This might be required for implementation
            // session resumption if we want to allow connection migration.
            _underlying.CloseAsync(exception, cancel);

        public void CheckSendSize(int size) => _underlying.CheckSendSize(size);

        public void Dispose()
        {
            _underlying.Dispose();

            if (SslStream != null)
            {
                SslStream.Dispose();
                try
                {
                    _writeStream!.Dispose();
                }
                catch (Exception)
                {
                    // Ignore: the buffer flush which will fail since the underlying transport is closed.
                }
            }
        }

        public ValueTask<ArraySegment<byte>> ReceiveDatagramAsync(CancellationToken cancel) =>
            throw new InvalidOperationException("only supported by datagram transports");

        public async ValueTask<int> ReceiveAsync(ArraySegment<byte> buffer, CancellationToken cancel)
        {
            if (buffer.Count == 0)
            {
                throw new ArgumentException($"empty {nameof(buffer)}");
            }

            int received;
            try
            {
                received = await SslStream!.ReadAsync(buffer, cancel).ConfigureAwait(false);
            }
            catch (IOException ex) when (ex.IsConnectionLost())
            {
                throw new ConnectionLostException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            catch (IOException ex)
            {
                throw new TransportException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            if (received == 0)
            {
                throw new ConnectionLostException(RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            return received;
        }

        public async ValueTask<int> SendAsync(IList<ArraySegment<byte>> buffer, CancellationToken cancel)
        {
            try
            {
                int sent = 0;
                foreach (ArraySegment<byte> segment in buffer)
                {
                    await _writeStream!.WriteAsync(segment, cancel).ConfigureAwait(false);
                    sent += segment.Count;
                }
                await _writeStream!.FlushAsync(cancel).ConfigureAwait(false);
                return sent;
            }
            catch (ObjectDisposedException ex)
            {
                // The stream might have been disposed if the connection is closed.
                throw new TransportException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            catch (IOException ex) when (ex.IsConnectionLost())
            {
                throw new ConnectionLostException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
            catch (IOException ex)
            {
                throw new TransportException(ex, RetryPolicy.AfterDelay(TimeSpan.Zero), _endpoint);
            }
        }

        public override string ToString() => _underlying.ToString()!;

        // Only for use by TcpEndpoint.
        internal SslTransceiver(
            Communicator communicator,
            Endpoint endpoint,
            ITransceiver underlying,
            string hostOrAdapterName,
            bool incoming)
        {
            Debug.Assert(incoming || endpoint != null);
            _communicator = communicator;
            _endpoint = endpoint;
            _engine = communicator.SslEngine;
            _underlying = underlying;
            _incoming = incoming;
            if (_incoming)
            {
                _adapterName = hostOrAdapterName;
            }
            else
            {
                _host = hostOrAdapterName;
            }
        }

        private X509Certificate CertificateSelectionCallback(
            object sender,
            string targetHost,
            X509CertificateCollection? certs,
            X509Certificate? remoteCertificate,
            string[]? acceptableIssuers)
        {
            Debug.Assert(certs != null && certs.Count > 0);
            // Use the first certificate that match the acceptable issuers.
            if (acceptableIssuers != null && acceptableIssuers.Length > 0)
            {
                foreach (X509Certificate certificate in certs)
                {
                    if (Array.IndexOf(acceptableIssuers, certificate.Issuer) != -1)
                    {
                        return certificate;
                    }
                }
            }
            return certs[0];
        }

        private bool RemoteCertificateValidationCallback(
            object sender,
            X509Certificate? certificate,
            X509Chain? chain,
            SslPolicyErrors errors)
        {
            var message = new StringBuilder();

            if ((errors & SslPolicyErrors.RemoteCertificateNotAvailable) > 0)
            {
                // For an outgoing connection the peer must always provide a certificate, for an incoming connection
                // the certificate is only required if the RequireClientCertificate option was set.
                if (!_incoming || _engine.TlsServerOptions.RequireClientCertificate)
                {
                    if (_engine.SecurityTraceLevel >= 1)
                    {
                        _communicator.Logger.Trace(
                            SslEngine.SecurityTraceCategory,
                            "SSL certificate validation failed - remote certificate not provided");
                    }
                    return false;
                }
                else
                {
                    errors ^= SslPolicyErrors.RemoteCertificateNotAvailable;
                    message.Append("\nremote certificate not provided (ignored)");
                }
            }

            if ((errors & SslPolicyErrors.RemoteCertificateNameMismatch) > 0)
            {
                if (_engine.SecurityTraceLevel >= 1)
                {
                    _communicator.Logger.Trace(
                        SslEngine.SecurityTraceCategory,
                        "SSL certificate validation failed - Hostname mismatch");
                }
                return false;
            }

            X509Certificate2Collection? trustedCertificateAuthorities = _incoming ?
                _engine.TlsServerOptions.ClientCertificateCertificateAuthorities :
                _engine.TlsClientOptions.ServerCertificateCertificateAuthorities;

            bool useMachineContext = _incoming ?
                _engine.TlsServerOptions.UseMachineContex : _engine.TlsClientOptions.UseMachineContex;

            bool buildCustomChain =
                (trustedCertificateAuthorities != null || useMachineContext) && certificate != null;
            try
            {
                // If using custom certificate authorities or the machine context and the peer provides a certificate,
                // we rebuild the certificate chain with our custom chain policy.
                if (buildCustomChain)
                {
                    chain = new X509Chain(useMachineContext);
                    chain.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;

                    if (trustedCertificateAuthorities != null)
                    {
                        // We need to set this flag to be able to use a certificate authority from the extra store.
                        chain.ChainPolicy.VerificationFlags = X509VerificationFlags.AllowUnknownCertificateAuthority;
                        foreach (X509Certificate2 cert in trustedCertificateAuthorities)
                        {
                            chain.ChainPolicy.ExtraStore.Add(cert);
                        }
                    }
                    chain.Build((X509Certificate2)certificate!);
                }

                if (chain != null && chain.ChainStatus != null)
                {
                    var chainStatus = new List<X509ChainStatus>(chain.ChainStatus);

                    if (trustedCertificateAuthorities != null)
                    {
                        // Untrusted root is OK when using our custom chain engine if the CA certificate is present in
                        // the chain policy extra store.
                        X509ChainElement root = chain.ChainElements[^1];
                        if (chain.ChainPolicy.ExtraStore.Contains(root.Certificate) &&
                            chainStatus.Exists(status => status.Status == X509ChainStatusFlags.UntrustedRoot))
                        {
                            chainStatus.Remove(
                                chainStatus.Find(status => status.Status == X509ChainStatusFlags.UntrustedRoot));
                            errors ^= SslPolicyErrors.RemoteCertificateChainErrors;
                        }
                        else if (!chain.ChainPolicy.ExtraStore.Contains(root.Certificate) &&
                                 !chainStatus.Exists(status => status.Status == X509ChainStatusFlags.UntrustedRoot))
                        {
                            chainStatus.Add(new X509ChainStatus() { Status = X509ChainStatusFlags.UntrustedRoot });
                            errors |= SslPolicyErrors.RemoteCertificateChainErrors;
                        }
                    }

                    foreach (X509ChainStatus status in chainStatus)
                    {
                        if (status.Status != X509ChainStatusFlags.NoError)
                        {
                            message.Append("\ncertificate chain error: ");
                            message.Append(status.Status);
                            errors |= SslPolicyErrors.RemoteCertificateChainErrors;
                        }
                    }
                }
            }
            finally
            {
                if (buildCustomChain)
                {
                    chain!.Dispose();
                }
            }

            if (errors > 0)
            {
                if (_engine.SecurityTraceLevel >= 1)
                {
                    _communicator.Logger.Trace(
                        SslEngine.SecurityTraceCategory,
                        message.Length > 0 ?
                            $"SSL certificate validation failed: {message}" : "SSL certificate validation failed");
                }
                return false;
            }

            if (message.Length > 0 && _engine.SecurityTraceLevel >= 1)
            {
                _communicator.Logger.Trace(SslEngine.SecurityTraceCategory,
                    $"SSL certificate validation status: {message}");
            }
            return true;
        }
    }
}
