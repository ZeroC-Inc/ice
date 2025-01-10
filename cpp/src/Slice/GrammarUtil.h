// Copyright (c) ZeroC, Inc.

#ifndef SLICE_GRAMMAR_UTIL_H
#define SLICE_GRAMMAR_UTIL_H

#include "Parser.h"
#include "Util.h"
#include <cassert>
#include <memory>
#include <utility>

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wshadow-field-in-constructor"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wshadow"
#endif

namespace Slice
{
    struct StringTok final : public GrammarBase
    {
        StringTok() {}
        std::string v;
        std::string literal;
    };

    struct MetadataListTok final : public GrammarBase
    {
        MetadataListTok() {}
        MetadataList v;
    };

    struct TypeStringTok final : public GrammarBase
    {
        TypeStringTok(TypePtr type, std::string name) : type(std::move(type)), name(std::move(name)) {}
        TypePtr type;
        std::string name;
    };

    struct IntegerTok final : public GrammarBase
    {
        IntegerTok() : v(0) {}
        std::int64_t v;
        std::string literal;
    };

    struct FloatingTok final : public GrammarBase
    {
        FloatingTok() : v(0) {}
        double v;
        std::string literal;
    };

    struct BoolTok final : public GrammarBase
    {
        BoolTok() : v(false) {}
        bool v;
    };

    struct ExceptionListTok final : public GrammarBase
    {
        ExceptionListTok() {}
        ExceptionList v;
    };

    struct InterfaceListTok final : public GrammarBase
    {
        InterfaceListTok() {}
        InterfaceList v;
    };

    struct EnumeratorListTok final : public GrammarBase
    {
        EnumeratorListTok() {}
        EnumeratorList v;
    };

    struct ConstDefTok final : public GrammarBase
    {
        ConstDefTok() {}
        ConstDefTok(SyntaxTreeBasePtr value, std::string stringValue)
            : v(std::move(value)),
              valueAsString(std::move(stringValue))
        {
        }

        SyntaxTreeBasePtr v;
        std::string valueAsString;
    };

    struct OptionalDefTok final : public GrammarBase
    {
        OptionalDefTok() : isOptional(false), tag(0) {}
        OptionalDefTok(int t) : isOptional(t >= 0), tag(t) {}

        TypePtr type;
        std::string name;
        bool isOptional;
        int tag;
    };

    struct ClassIdTok final : public GrammarBase
    {
        ClassIdTok() : t(0) {}
        std::string v;
        int t;
    };

    struct TokenContext
    {
        int firstLine;
        int lastLine;
        int firstColumn;
        int lastColumn;
        std::string filename;
    };

    using StringTokPtr = std::shared_ptr<StringTok>;
    using IntegerTokPtr = std::shared_ptr<IntegerTok>;
    using FloatingTokPtr = std::shared_ptr<FloatingTok>;
    using ConstDefTokPtr = std::shared_ptr<ConstDefTok>;
}

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#endif
