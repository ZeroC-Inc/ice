//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "StringConverterI.h"

using namespace std;
using namespace Ice;

byte*
Test::StringConverterI::toUTF8(const char* sourceStart, const char* sourceEnd, UTF8Buffer& buffer) const
{
    size_t size = static_cast<size_t>(sourceEnd - sourceStart);
    byte* targetStart = buffer.getMoreBytes(size, 0);
    byte* targetEnd = targetStart + size;

    for (size_t i = 0; i < size; ++i)
    {
        targetStart[i] = static_cast<byte>(tolower(sourceStart[i]));
    }

    return targetEnd;
}

void
Test::StringConverterI::fromUTF8(const byte* sourceStart, const byte* sourceEnd, string& target) const
{
    size_t size = static_cast<size_t>(sourceEnd - sourceStart);
    target.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        target[i] = static_cast<char>(toupper(static_cast<uint8_t>(sourceStart[i])));
    }
}

byte*
Test::WstringConverterI::toUTF8(const wchar_t* sourceStart, const wchar_t* sourceEnd, UTF8Buffer& buffer) const
{
    wstring ws(sourceStart, sourceEnd);
    string s = wstringToString(ws);

    size_t size = s.size();
    byte* targetStart = buffer.getMoreBytes(size, 0);
    byte* targetEnd = targetStart + size;

    for (size_t i = 0; i < size; ++i)
    {
        targetStart[i] = static_cast<byte>(tolower(s[i]));
    }
    return targetEnd;
}

void
Test::WstringConverterI::fromUTF8(const byte* sourceStart, const byte* sourceEnd, wstring& target) const
{
    string s(reinterpret_cast<const uint8_t*>(sourceStart), reinterpret_cast<const uint8_t*>(sourceEnd));
    for (size_t i = 0; i < s.size(); ++i)
    {
        s[i] = static_cast<char>(toupper(s[i]));
    }
    target = stringToWstring(s);
}
