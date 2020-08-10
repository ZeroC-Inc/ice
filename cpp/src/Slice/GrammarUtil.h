//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef SLICE_GRAMMAR_UTIL_H
#define SLICE_GRAMMAR_UTIL_H

#include "Slice/Parser.h"
#include "Slice/Util.h"

#include <memory>

namespace Slice
{

class StringTok;
class StringListTok;
class BoolTok;
class IntegerTok;
class FloatingTok;
class ExceptionListTok;
class ClassListTok;
class InterfaceListTok;
class EnumeratorListTok;
class ConstDefTok;
class TaggedDefTok;
class ClassIdTok;

typedef ::IceUtil::Handle<StringTok> StringTokPtr;
typedef ::IceUtil::Handle<StringListTok> StringListTokPtr;
typedef ::IceUtil::Handle<BoolTok> BoolTokPtr;
typedef ::IceUtil::Handle<IntegerTok> IntegerTokPtr;
typedef ::IceUtil::Handle<FloatingTok> FloatingTokPtr;
typedef ::IceUtil::Handle<ExceptionListTok> ExceptionListTokPtr;
typedef ::IceUtil::Handle<ClassListTok> ClassListTokPtr;
typedef ::IceUtil::Handle<InterfaceListTok> InterfaceListTokPtr;
typedef ::IceUtil::Handle<EnumeratorListTok> EnumeratorListTokPtr;
typedef ::IceUtil::Handle<ConstDefTok> ConstDefTokPtr;
typedef ::IceUtil::Handle<TaggedDefTok> TaggedDefTokPtr;
typedef ::IceUtil::Handle<ClassIdTok> ClassIdTokPtr;

// ----------------------------------------------------------------------
// StringTok
// ----------------------------------------------------------------------

class StringTok : public GrammarBase
{
public:

    StringTok() { }
    std::string v;
    std::string literal;
};

// ----------------------------------------------------------------------
// StringListTok
// ----------------------------------------------------------------------

class StringListTok : public GrammarBase
{
public:

    StringListTok() { }
    StringList v;
};

// ----------------------------------------------------------------------
// IntegerTok
// ----------------------------------------------------------------------

class IntegerTok : public GrammarBase
{
public:

    IntegerTok() { }
    IceUtil::Int64 v;
    std::string literal;
};

// ----------------------------------------------------------------------
// FloatingTok
// ----------------------------------------------------------------------

class FloatingTok : public GrammarBase
{
public:

    FloatingTok() { }
    double v;
    std::string literal;
};

// ----------------------------------------------------------------------
// BoolTok
// ----------------------------------------------------------------------

class BoolTok : public GrammarBase
{
public:

    BoolTok(bool value) :
        v(value)
    { }
    bool v;
};

// ----------------------------------------------------------------------
// ExceptionListTok
// ----------------------------------------------------------------------

class ExceptionListTok : public GrammarBase
{
public:

    ExceptionListTok() { }
    ExceptionList v;
};

// ----------------------------------------------------------------------
// ClassListTok
// ----------------------------------------------------------------------

class ClassListTok : public GrammarBase
{
public:

    ClassListTok() { }
    ClassList v;
};

// ----------------------------------------------------------------------
// InterfaceListTok
// ----------------------------------------------------------------------

class InterfaceListTok : public GrammarBase
{
public:

    InterfaceListTok() { }
    InterfaceList v;
};

// ----------------------------------------------------------------------
// EnumeratorListTok
// ----------------------------------------------------------------------

class EnumeratorListTok : public GrammarBase
{
public:

    EnumeratorListTok() { }
    EnumeratorList v;
};

// ----------------------------------------------------------------------
// ConstDefTok
// ----------------------------------------------------------------------

class ConstDefTok : public GrammarBase
{
public:

    ConstDefTok() { }
    ConstDefTok(SyntaxTreeBasePtr value, std::string stringValue, std::string literalValue) :
        v(value),
        valueAsString(stringValue),
        valueAsLiteral(literalValue)
    { }

    SyntaxTreeBasePtr v;
    std::string valueAsString;
    std::string valueAsLiteral;
};

// ----------------------------------------------------------------------
// TaggedDefTok
// ----------------------------------------------------------------------

class TaggedDefTok : public GrammarBase
{
public:

    TaggedDefTok() :
        isTagged(false)
    { }
    TaggedDefTok(int t) :
        isTagged(true),
        tag(t)
    { }

    TypePtr type;
    std::string name;
    bool isTagged;
    int tag;
};

// ----------------------------------------------------------------------
// ClassIdTok
// ----------------------------------------------------------------------

class ClassIdTok : public GrammarBase
{
public:

    ClassIdTok() { }
    std::string v;
    int t;
};

// ----------------------------------------------------------------------
// TokenContext: stores the location of tokens.
// ----------------------------------------------------------------------

struct TokenContext
{
    int firstLine;
    int lastLine;
    int firstColumn;
    int lastColumn;
    std::shared_ptr<std::string> filename;
};

}

#endif
