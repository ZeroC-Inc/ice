//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICESSL_UTIL_H
#define ICESSL_UTIL_H

#include "IceSSL/Plugin.h"

#include <functional>
#include <string>
#include <vector>

#if defined(__APPLE__)
#    include <CoreFoundation/CoreFoundation.h>
#    if TARGET_OS_IPHONE != 0
#        define ICE_USE_SECURE_TRANSPORT_IOS 1
#    else
#        define ICE_USE_SECURE_TRANSPORT_MACOS 1
#    endif
#endif

namespace IceSSL
{
#if defined(__APPLE__)
    //
    // Helper functions to use by Secure Transport.
    //
    std::string fromCFString(CFStringRef);

    inline CFStringRef toCFString(const std::string& s)
    {
        return CFStringCreateWithCString(nullptr, s.c_str(), kCFStringEncodingUTF8);
    }
#endif

    //
    // Adapts the C++11 functions to C++98-like callbacks
    //
    class ICESSL_API CertificateVerifier
    {
    public:
        CertificateVerifier(std::function<bool(const ConnectionInfoPtr&)>);
        bool verify(const ConnectionInfoPtr&);

    private:
        std::function<bool(const ConnectionInfoPtr&)> _verify;
    };
    using CertificateVerifierPtr = std::shared_ptr<CertificateVerifier>;

    class ICESSL_API PasswordPrompt
    {
    public:
        PasswordPrompt(std::function<std::string()>);
        std::string getPassword();

    private:
        std::function<std::string()> _prompt;
    };
    using PasswordPromptPtr = std::shared_ptr<PasswordPrompt>;

    //
    // Constants for X509 certificate alt names (AltNameOther, AltNameORAddress, AltNameEDIPartyName and
    // AltNameObjectIdentifier) are not supported.
    //

    // const int AltNameOther = 0;
    const int AltNameEmail = 1;
    const int AltNameDNS = 2;
    // const int AltNameORAddress = 3;
    const int AltNameDirectory = 4;
    // const int AltNameEDIPartyName = 5;
    const int AltNameURL = 6;
    const int AltNAmeIP = 7;
    // const AltNameObjectIdentifier = 8;

    //
    // Read a file into memory buffer.
    //
    ICESSL_API void readFile(const std::string&, std::vector<char>&);

    //
    // Determine if a file or directory exists, with an optional default
    // directory.
    //
    ICESSL_API bool checkPath(const std::string&, const std::string&, bool, std::string&);

    ICESSL_API bool parseBytes(const std::string&, std::vector<unsigned char>&);
}

#endif
