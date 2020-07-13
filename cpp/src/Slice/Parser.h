//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef SLICE_PARSER_H
#define SLICE_PARSER_H

#include <IceUtil/Shared.h>
#include <IceUtil/Handle.h>
#include <IceUtil/Exception.h>
#include <array>
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <stdio.h>

namespace Slice
{

class CompilerException : public ::IceUtil::Exception
{
public:

    CompilerException(const char*, int, const std::string&);
    std::string ice_id() const override;
    void ice_print(std::ostream&) const override;
    CompilerException* ice_cloneImpl() const override;
    void ice_throw() const override;

    std::string reason() const;

private:

    static const char* _name;
    const std::string _reason;
};

#if defined(_WIN32)

const IceUtil::Int64 Int32Max =  0x7fffffffi64;
const IceUtil::Int64 Int32Min = -Int32Max - 1i64;

#else

#   if defined(INT32_MIN) && defined(INT32_MAX)

const IceUtil::Int64 Int32Max =  INT32_MAX;
const IceUtil::Int64 Int32Min =  INT32_MIN;

#   else

const IceUtil::Int64 Int32Max =  0x7fffffffLL;
const IceUtil::Int64 Int32Min = -Int32Max - 1LL;

#   endif

#endif

const IceUtil::Int64 Int16Max =  0x7fff;
const IceUtil::Int64 Int16Min = -Int16Max - 1;
const IceUtil::Int64 ByteMax = 0xff;
const IceUtil::Int64 ByteMin = 0x00;

enum NodeType
{
    Dummy,
    Real
};

//
// Format preference for classes and exceptions.
//
enum FormatType
{
    DefaultFormat,    // No preference was specified.
    CompactFormat,    // Minimal format.
    SlicedFormat      // Full format.
};

enum WarningCategory
{
    All,
    Deprecated,
    InvalidMetaData,
    ReservedIdentifier
};

class GrammarBase;
class SyntaxTreeBase;
class Type;
class Builtin;
class Contained;
class Container;
class Module;
class Constructed;
class ClassDecl;
class ClassDef;
class InterfaceDecl;
class InterfaceDef;
class Exception;
class Optional;
class Struct;
class Operation;
class ParamDecl;
class DataMember;
class Sequence;
class Dictionary;
class Enum;
class Enumerator;
class Const;
class Unit;
class CICompare;
class DerivedToBaseCompare;
class ModulePartialCompare;

typedef ::IceUtil::Handle<GrammarBase> GrammarBasePtr;
typedef ::IceUtil::Handle<SyntaxTreeBase> SyntaxTreeBasePtr;
typedef ::IceUtil::Handle<Type> TypePtr;
typedef ::IceUtil::Handle<Builtin> BuiltinPtr;
typedef ::IceUtil::Handle<Contained> ContainedPtr;
typedef ::IceUtil::Handle<Container> ContainerPtr;
typedef ::IceUtil::Handle<Module> ModulePtr;
typedef ::IceUtil::Handle<Constructed> ConstructedPtr;
typedef ::IceUtil::Handle<ClassDecl> ClassDeclPtr;
typedef ::IceUtil::Handle<ClassDef> ClassDefPtr;
typedef ::IceUtil::Handle<InterfaceDecl> InterfaceDeclPtr;
typedef ::IceUtil::Handle<InterfaceDef> InterfaceDefPtr;
typedef ::IceUtil::Handle<Optional> OptionalPtr;
typedef ::IceUtil::Handle<Exception> ExceptionPtr;
typedef ::IceUtil::Handle<Struct> StructPtr;
typedef ::IceUtil::Handle<Operation> OperationPtr;
typedef ::IceUtil::Handle<ParamDecl> ParamDeclPtr;
typedef ::IceUtil::Handle<DataMember> DataMemberPtr;
typedef ::IceUtil::Handle<Sequence> SequencePtr;
typedef ::IceUtil::Handle<Dictionary> DictionaryPtr;
typedef ::IceUtil::Handle<Enum> EnumPtr;
typedef ::IceUtil::Handle<Enumerator> EnumeratorPtr;
typedef ::IceUtil::Handle<Const> ConstPtr;
typedef ::IceUtil::Handle<Unit> UnitPtr;

typedef std::list<TypePtr> TypeList;
typedef std::set<std::string> StringSet;
typedef std::list<std::string> StringList;
typedef std::pair<TypePtr, std::string> TypeString;
typedef std::list<TypeString> TypeStringList;
typedef std::list<ContainedPtr> ContainedList;
typedef std::list<ModulePtr> ModuleList;
typedef std::list<ConstructedPtr> ConstructedList;
typedef std::list<ClassDefPtr> ClassList;
typedef std::list<InterfaceDefPtr> InterfaceList;
typedef std::list<ExceptionPtr> ExceptionList;
typedef std::list<StructPtr> StructList;
typedef std::list<SequencePtr> SequenceList;
typedef std::list<DictionaryPtr> DictionaryList;
typedef std::list<EnumPtr> EnumList;
typedef std::list<ConstPtr> ConstList;
typedef std::list<OperationPtr> OperationList;
typedef std::list<DataMemberPtr> DataMemberList;
typedef std::list<ParamDeclPtr> ParamDeclList;
typedef std::list<EnumeratorPtr> EnumeratorList;

// ----------------------------------------------------------------------
// CICompare -- function object to do case-insensitive string comparison.
// ----------------------------------------------------------------------

class CICompare
{
public:

    bool operator()(const std::string&, const std::string&) const;
};

// ----------------------------------------------------------------------
// DerivedToBaseCompare -- function object to do sort exceptions into
// most-derived to least-derived order.
// ----------------------------------------------------------------------

class DerivedToBaseCompare
{
public:

    bool operator()(const ExceptionPtr&, const ExceptionPtr&) const;
};

// ----------------------------------------------------------------------
// ParserVisitor
// ----------------------------------------------------------------------

class ParserVisitor
{
public:

    virtual ~ParserVisitor() { }
    virtual bool visitUnitStart(const UnitPtr&) { return true; }
    virtual void visitUnitEnd(const UnitPtr&) { }
    virtual bool visitModuleStart(const ModulePtr&) { return true; }
    virtual void visitModuleEnd(const ModulePtr&) { }
    virtual void visitClassDecl(const ClassDeclPtr&) { }
    virtual bool visitClassDefStart(const ClassDefPtr&) { return true; }
    virtual void visitClassDefEnd(const ClassDefPtr&) { }
    virtual void visitInterfaceDecl(const InterfaceDeclPtr&) { }
    virtual bool visitInterfaceDefStart(const InterfaceDefPtr&) { return true; }
    virtual void visitInterfaceDefEnd(const InterfaceDefPtr&) { }
    virtual bool visitExceptionStart(const ExceptionPtr&) { return true; }
    virtual void visitExceptionEnd(const ExceptionPtr&) { }
    virtual bool visitStructStart(const StructPtr&) { return true; }
    virtual void visitStructEnd(const StructPtr&) { }
    virtual void visitOperation(const OperationPtr&) { }
    virtual void visitParamDecl(const ParamDeclPtr&) { }
    virtual void visitDataMember(const DataMemberPtr&) { }
    virtual void visitSequence(const SequencePtr&) { }
    virtual void visitDictionary(const DictionaryPtr&) { }
    virtual void visitEnum(const EnumPtr&) { }
    virtual void visitConst(const ConstPtr&) { }
};

// ----------------------------------------------------------------------
// DefinitionContext
// ----------------------------------------------------------------------

class DefinitionContext : public ::IceUtil::SimpleShared
{
public:

    DefinitionContext(int, const StringList&);

    std::string filename() const;
    int includeLevel() const;

    void setFilename(const std::string&);

    void setMetaData(const StringList&);
    std::string findMetaData(const std::string&) const;
    StringList getMetaData() const;

    // When parsing Slice definitions, apply 3.7 or 4.0 semantics for class parameters, Object etc.
    bool compatMode() const;

    //
    // Emit warning unless filtered out by [["suppress-warning"]]
    //
    void warning(WarningCategory, const std::string&, int, const std::string&) const;
    void warning(WarningCategory, const std::string&, const std::string&, const std::string&) const;

    void error(const std::string&, int, const std::string&) const;
    void error(const std::string&, const std::string&, const std::string&) const;

private:

    bool suppressWarning(WarningCategory) const;
    void initSuppressedWarnings();

    int _includeLevel;
    StringList _metaData;
    std::string _filename;
    std::set<WarningCategory> _suppressedWarnings;
};
typedef ::IceUtil::Handle<DefinitionContext> DefinitionContextPtr;

// ----------------------------------------------------------------------
// Comment
// ----------------------------------------------------------------------

class Comment : public ::IceUtil::SimpleShared
{
public:

    bool isDeprecated() const;
    StringList deprecated() const;

    StringList overview() const;  // Contains all introductory lines up to the first tag.
    StringList misc() const;      // Contains unrecognized tags.
    StringList seeAlso() const;   // Targets of @see tags.

    StringList returns() const;                           // Description of an operation's return value.
    std::map<std::string, StringList> parameters() const; // Parameter descriptions for an op. Key is parameter name.
    std::map<std::string, StringList> exceptions() const; // Exception descriptions for an op. Key is exception name.

private:

    Comment();

    bool _isDeprecated;
    StringList _deprecated;
    StringList _overview;
    StringList _misc;
    StringList _seeAlso;

    StringList _returns;
    std::map<std::string, StringList> _parameters;
    std::map<std::string, StringList> _exceptions;

    friend class Contained;
};
typedef ::IceUtil::Handle<Comment> CommentPtr;

// ----------------------------------------------------------------------
// GrammarBase
// ----------------------------------------------------------------------

class GrammarBase : public ::IceUtil::SimpleShared
{
};

// ----------------------------------------------------------------------
// SyntaxTreeBase
// ----------------------------------------------------------------------

class SyntaxTreeBase : public GrammarBase
{
public:

    virtual void destroy();
    UnitPtr unit() const;
    DefinitionContextPtr definitionContext() const; // May be nil
    virtual void visit(ParserVisitor*, bool);

protected:

    SyntaxTreeBase(const UnitPtr&, const DefinitionContextPtr& = nullptr);

    UnitPtr _unit;
    DefinitionContextPtr _definitionContext;
};

// ----------------------------------------------------------------------
// Type
// ----------------------------------------------------------------------

class Type : public virtual SyntaxTreeBase
{
public:

    virtual std::string typeId() const = 0;
    virtual bool usesClasses() const = 0; // TODO: can we remove this method?
    virtual bool isClassType() const { return false; }
    virtual bool isInterfaceType() const { return false; }
    virtual size_t minWireSize() const = 0;
    virtual std::string getTagFormat() const = 0;
    virtual bool isVariableLength() const = 0;

protected:

    Type(const UnitPtr&);
};

// ----------------------------------------------------------------------
// Builtin
// ----------------------------------------------------------------------

class Builtin : public virtual Type
{
public:

    enum Kind
    {
        KindBool,
        KindByte,
        KindShort,
        KindUShort,
        KindInt,
        KindUInt,
        KindVarInt,
        KindVarUInt,
        KindLong,
        KindULong,
        KindVarLong,
        KindVarULong,
        KindFloat,
        KindDouble,
        KindString,
        KindObject, // the implicit base for all proxies
        KindAnyClass
    };

    std::string typeId() const override;
    bool usesClasses() const override;
    bool isClassType() const override { return _kind == KindAnyClass; }
    bool isInterfaceType() const override { return _kind == KindObject; }
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;

    bool isNumericType() const;
    bool isNumericTypeOrBool() const;
    bool isIntegralType() const;
    bool isUnsignedType() const;
    std::pair<std::int64_t, std::uint64_t> integralRange() const;

    Kind kind() const;
    std::string kindAsString() const;
    static std::optional<Kind> kindFromString(std::string_view);

    inline static const std::array<std::string, 17> builtinTable =
    {
        "bool",
        "byte",
        "short",
        "ushort",
        "int",
        "uint",
        "varint",
        "varuint",
        "long",
        "ulong",
        "varlong",
        "varulong",
        "float",
        "double",
        "string",
        "Object",
        "AnyClass"
    };

protected:

    Builtin(const UnitPtr&, Kind);

    friend class Unit;

    const Kind _kind;
};

// ----------------------------------------------------------------------
// Contained
// ----------------------------------------------------------------------

class Contained : public virtual SyntaxTreeBase
{
public:

    ContainerPtr container() const;
    std::string name() const;
    std::string scoped() const;
    std::string scope() const;
    std::string flattenedScope() const;
    std::string file() const;
    std::string line() const;
    std::string comment() const;
    CommentPtr parseComment(bool) const;

    int includeLevel() const;
    void updateIncludeLevel();

    bool hasMetaData(const std::string&) const;
    bool hasMetaDataWithPrefix(const std::string&) const;
    bool findMetaData(const std::string&, std::string&) const;
    std::string findMetaDataWithPrefix(const std::string&) const;
    std::list<std::string> getMetaData() const;
    void setMetaData(const std::list<std::string>&);
    void addMetaData(const std::string&); // TODO: remove this method once "cs:" and "vb:" are hard errors.

    static FormatType parseFormatMetaData(const std::list<std::string>&);

    virtual bool uses(const ContainedPtr&) const = 0;
    virtual std::string kindOf() const = 0;

    bool operator<(const Contained&) const;
    bool operator==(const Contained&) const;

protected:

    Contained(const ContainerPtr&, const std::string&);

    ContainerPtr _container;
    std::string _name;
    std::string _scoped;
    std::string _file;
    std::string _line;
    std::string _comment;
    int _includeLevel;
    std::list<std::string> _metaData;
};

// ----------------------------------------------------------------------
// Container
// ----------------------------------------------------------------------

class Container : public virtual SyntaxTreeBase
{
public:

    void destroy() override;
    TypeList lookupType(const std::string&, bool = true);
    TypeList lookupTypeNoBuiltin(const std::string&, bool = true, bool = false);
    ContainedList lookupContained(const std::string&, bool = true);
    ExceptionPtr lookupException(const std::string&, bool = true);
    // Finds enumerators using the deprecated unscoped enumerators lookup
    EnumeratorList enumerators(const std::string&) const;
    virtual ContainedList contents() const = 0;
    bool hasContentsWithMetaData(const std::string&) const;
    std::string thisScope() const;
    void containerRecDependencies(std::set<ConstructedPtr>&); // Internal operation, don't use directly.

    bool checkIntroduced(const std::string&, ContainedPtr = nullptr);

protected:

    Container(const UnitPtr&);

    bool checkFileMetaData(const StringList&, const StringList&);
    bool validateConstant(const std::string&, const TypePtr&, SyntaxTreeBasePtr&, const std::string&, bool);

    std::map<std::string, ContainedPtr, CICompare> _introducedMap;
};

// ----------------------------------------------------------------------
// Module
// ----------------------------------------------------------------------

class Module : public virtual Container, public virtual Contained
{
public:

    void destroy() override;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    ModulePtr createModule(const std::string&);
    ClassDefPtr createClassDef(const std::string&, int, const ClassDefPtr&);
    ClassDeclPtr createClassDecl(const std::string&);
    InterfaceDefPtr createInterfaceDef(const std::string&, const InterfaceList&);
    InterfaceDeclPtr createInterfaceDecl(const std::string&);
    ExceptionPtr createException(const std::string&, const ExceptionPtr&, NodeType = Real);
    StructPtr createStruct(const std::string&, NodeType = Real);
    SequencePtr createSequence(const std::string&, const TypePtr&, const StringList&, NodeType = Real);
    DictionaryPtr createDictionary(const std::string&, const TypePtr&, const StringList&, const TypePtr&,
                                   const StringList&, NodeType = Real);
    EnumPtr createEnum(const std::string&, bool, NodeType = Real);
    ConstPtr createConst(const std::string, const TypePtr&, const StringList&, const SyntaxTreeBasePtr&,
                         const std::string&, const std::string&, NodeType = Real);
    EnumList enums() const;
    ConstList consts() const;
    bool hasSequences() const;
    bool hasStructs() const;
    bool hasExceptions() const;
    bool hasDictionaries() const;
    bool hasEnums() const;
    bool hasClassDecls() const;
    bool hasClassDefs() const;
    bool hasInterfaceDecls() const;
    bool hasInterfaceDefs() const;
    bool hasOnlyClassDecls() const;
    bool hasOnlyInterfaces() const;
    bool hasOperations() const; // interfaces or classes with operations
    bool hasOtherConstructedOrExceptions() const; // Exceptions or constructed types other than classes.
    bool hasAsyncOps() const;
    bool hasOnlySubModules() const;

protected:

    Module(const ContainerPtr&, const std::string&);

    friend class Unit;

    ContainedList _contents;
};

// ----------------------------------------------------------------------
// Constructed
// ----------------------------------------------------------------------

class Constructed : public virtual Type, public virtual Contained
{
public:

    std::string typeId() const override;
    bool isVariableLength() const override = 0;
    ConstructedList dependencies();
    virtual void recDependencies(std::set<ConstructedPtr>&) = 0; // Internal operation, don't use directly.

protected:

    Constructed(const ContainerPtr&, const std::string&);
};

// ----------------------------------------------------------------------
// ClassDecl
// ----------------------------------------------------------------------

class ClassDecl : public virtual Constructed
{
public:

    void destroy() override;
    ClassDefPtr definition() const;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    bool isClassType() const override { return true; }
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    void visit(ParserVisitor*, bool) override;
    std::string kindOf() const override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.

protected:

    ClassDecl(const ContainerPtr&, const std::string&);

    friend class Module;

    ClassDefPtr _definition;
};

// ----------------------------------------------------------------------
// ClassDef
// ----------------------------------------------------------------------

//
// Note: For the purpose of this parser, a class definition is not
// considered to be a type, but a class declaration is. And each class
// definition has at least one class declaration (but not vice versa),
// so if you need the class as a "constructed type", use the
// declaration() operation to navigate to the class declaration.
//
class ClassDef : public virtual Container, public virtual Contained
{
public:

    void destroy() override;
    DataMemberPtr createDataMember(const std::string&, const TypePtr&, bool, int, const SyntaxTreeBasePtr& = nullptr,
                                   const std::string& = "", const std::string& = "");
    ClassDeclPtr declaration() const;
    ClassDefPtr base() const;
    ClassList allBases() const;
    DataMemberList dataMembers() const;
    DataMemberList sortedTaggedDataMembers() const;
    DataMemberList allDataMembers() const;
    DataMemberList classDataMembers() const;
    DataMemberList allClassDataMembers() const;
    bool canBeCyclic() const;
    bool isA(const std::string&) const;
    bool hasDataMembers() const;
    bool hasDefaultValues() const;
    bool inheritsMetaData(const std::string&) const;
    bool hasBaseDataMembers() const;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    int compactId() const;
    StringList ids() const;

protected:

    ClassDef(const ContainerPtr&, const std::string&, int, const ClassDefPtr&);

    friend class Module;

    ClassDeclPtr _declaration;
    ClassDefPtr _base;
    std::list<DataMemberPtr> _dataMembers;
    int _compactId;
};

// ----------------------------------------------------------------------
// InterfaceDecl
// ----------------------------------------------------------------------

class InterfaceDecl : public virtual Constructed
{
public:

    void destroy() override;
    InterfaceDefPtr definition() const;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    bool isInterfaceType() const override { return true; }
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    void visit(ParserVisitor*, bool) override;
    std::string kindOf() const override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.

    static void checkBasesAreLegal(const std::string&, const InterfaceList&, const UnitPtr&);

protected:

    InterfaceDecl(const ContainerPtr&, const std::string&);

    friend class Module;

    InterfaceDefPtr _definition;

private:

    typedef std::list<InterfaceList> GraphPartitionList;
    typedef std::list<StringList> StringPartitionList;

    static bool isInList(const GraphPartitionList&, const InterfaceDefPtr&);
    static void addPartition(GraphPartitionList&, GraphPartitionList::reverse_iterator, const InterfaceDefPtr&);
    static StringPartitionList toStringPartitionList(const GraphPartitionList&);
    static void checkPairIntersections(const StringPartitionList&, const std::string&, const UnitPtr&);
};

// ----------------------------------------------------------------------
// Operation
// ----------------------------------------------------------------------

class Operation : public virtual Contained, public virtual Container
{
public:

    //
    // Note: The order of definitions here *must* match the order of
    // definitions of ::Ice::OperationMode in Ice/Current.h
    //
    enum Mode
    {
        Normal,
        Nonmutating,
        Idempotent
    };

    InterfaceDefPtr interface() const;

    // The "in" bit sequence length. It corresponds to the number of in-parameters with optional types that are not
    // class/proxy and that are not tagged.
    size_t inBitSequenceSize() const;

    // The "return" bit sequence length. It corresponds to the number of return parameters with optional types that are
    // not class/proxy and that are not tagged.
    size_t returnBitSequenceSize() const;

    void destroy() override;
    TypePtr returnType() const;
    bool returnIsTagged() const;
    int returnTag() const;
    Mode mode() const;
    Mode sendMode() const;
    bool hasMarshaledResult() const;
    ParamDeclPtr createParamDecl(const std::string&, const TypePtr&, bool, bool, int);
    ParamDeclList parameters() const;
    ParamDeclList inParameters() const;
    void inParameters(ParamDeclList&, ParamDeclList&) const;
    ParamDeclList outParameters() const;
    void outParameters(ParamDeclList&, ParamDeclList&) const;
    ExceptionList throws() const;
    void setExceptionList(const ExceptionList&);
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    bool sendsClasses(bool) const;
    bool returnsClasses(bool) const;
    bool returnsData() const;
    bool returnsMultipleValues() const;
    int attributes() const;
    FormatType format() const;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;

protected:

    Operation(const ContainerPtr&, const std::string&, const TypePtr&, bool, int, Mode);

    friend class InterfaceDef;

    std::list<ParamDeclPtr> _inParameters;
    std::list<ParamDeclPtr> _outParameters;
    TypePtr _returnType;
    bool _returnIsTagged;
    int _returnTag;
    std::list<ExceptionPtr> _throws;
    Mode _mode;
};

// ----------------------------------------------------------------------
// InterfaceDef
// ----------------------------------------------------------------------

//
// Note: For the purpose of this parser, an interface definition is not
// considered to be a type, but an interface declaration is. And each interface
// definition has at least one interface declaration (but not vice versa),
// so if you need the interface as a "constructed type", use the
// declaration() function to navigate to the interface declaration.
//
class InterfaceDef : public virtual Container, public virtual Contained
{
public:

    void destroy() override;
    OperationPtr createOperation(const std::string&, const TypePtr&, bool, int, Operation::Mode = Operation::Normal);

    InterfaceDeclPtr declaration() const;
    InterfaceList bases() const;
    InterfaceList allBases() const;
    OperationList operations() const;
    OperationList allOperations() const;
    bool isA(const std::string&) const;
    bool hasOperations() const;
    bool inheritsMetaData(const std::string&) const;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    StringList ids() const;

protected:

    InterfaceDef(const ContainerPtr&, const std::string&, const InterfaceList&);

    friend class Module;

    InterfaceDeclPtr _declaration;
    InterfaceList _bases;
    std::list<OperationPtr> _operations;
};

// ----------------------------------------------------------------------
// Optional (for T? types)
// ----------------------------------------------------------------------

class Optional : public Type
{
public:

    Optional(const TypePtr& underlying);

    std::string typeId() const override;
    bool usesClasses() const override;
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    TypePtr underlying() const { return _underlying; }
    bool encodedUsingBitSequence() const { return minWireSize() == 0; }

private:

    const TypePtr _underlying;
};

// ----------------------------------------------------------------------
// Exception
// ----------------------------------------------------------------------

// No inheritance from Constructed, as this is not a Type
class Exception : public virtual Container, public virtual Contained
{
public:

    void destroy() override;
    DataMemberPtr createDataMember(const std::string&, const TypePtr&, bool, int, const SyntaxTreeBasePtr& = nullptr,
                                   const std::string& = "", const std::string& = "");
    DataMemberList dataMembers() const;
    DataMemberList sortedTaggedDataMembers() const;
    DataMemberList allDataMembers() const;
    DataMemberList classDataMembers() const;
    DataMemberList allClassDataMembers() const;
    ExceptionPtr base() const;
    ExceptionList allBases() const;
    bool isBaseOf(const ExceptionPtr&) const;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses(bool) const;
    bool hasDefaultValues() const;
    bool inheritsMetaData(const std::string&) const;
    bool hasBaseDataMembers() const;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;

protected:

    Exception(const ContainerPtr&, const std::string&, const ExceptionPtr&);

    friend class Container;
    friend class Module;

    ExceptionPtr _base;
    std::list<DataMemberPtr> _dataMembers;
};

// ----------------------------------------------------------------------
// Struct
// ----------------------------------------------------------------------

class Struct : public virtual Container, public virtual Constructed
{
public:

    void destroy() override;
    DataMemberPtr createDataMember(const std::string&, const TypePtr&, bool, const SyntaxTreeBasePtr& = nullptr,
                                   const std::string& = "", const std::string& = "");
    DataMemberList dataMembers() const;
    DataMemberList classDataMembers() const;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    bool hasDefaultValues() const;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.

protected:

    Struct(const ContainerPtr&, const std::string&);

    friend class Container;
    friend class Module;

    std::list<DataMemberPtr> _dataMembers;
};

// ----------------------------------------------------------------------
// Sequence
// ----------------------------------------------------------------------

class Sequence : public virtual Constructed
{
public:

    TypePtr type() const;
    StringList typeMetaData() const;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.

protected:

    Sequence(const ContainerPtr&, const std::string&, const TypePtr&, const StringList&);

    friend class Container;
    friend class Module;

    TypePtr _type;
    StringList _typeMetaData;
};

// ----------------------------------------------------------------------
// Dictionary
// ----------------------------------------------------------------------

class Dictionary : public virtual Constructed
{
public:

    TypePtr keyType() const;
    TypePtr valueType() const;
    StringList keyMetaData() const;
    StringList valueMetaData() const;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.

    static bool legalKeyType(const TypePtr&, bool&);

protected:

    Dictionary(const ContainerPtr&, const std::string&, const TypePtr&, const StringList&, const TypePtr&,
               const StringList&);

    friend class Container;
    friend class Module;

    TypePtr _keyType;
    TypePtr _valueType;
    StringList _keyMetaData;
    StringList _valueMetaData;
};

// ----------------------------------------------------------------------
// Enum
// ----------------------------------------------------------------------

class Enum : public virtual Container, public virtual Constructed
{
public:

    void destroy() override;
    EnumeratorPtr createEnumerator(const std::string&);
    EnumeratorPtr createEnumerator(const std::string&, std::int64_t);
    EnumeratorList enumerators() const;

    // The underlying type. The default is nullptr, which means a range of 0..INT32_MAX encoded as a variable-length
    // size. The only permissible underlying types are byte, short, ushort, int, and uint.
    BuiltinPtr underlying() const;

    // A Slice enum is checked by default: the generated unmarshaling code verifies the value matches one of the enum's
    // enumerators.
    bool isUnchecked() const { return _unchecked; }

    bool explicitValue() const;

    std::int64_t minValue() const;
    std::int64_t maxValue() const;
    ContainedList contents() const override;
    bool uses(const ContainedPtr&) const override;
    bool usesClasses() const override;
    size_t minWireSize() const override;
    std::string getTagFormat() const override;
    bool isVariableLength() const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;
    void recDependencies(std::set<ConstructedPtr>&) override; // Internal operation, don't use directly.
    EnumeratorPtr validateEnumerator(const std::string&);

    // Sets the underlying type shortly after construction and before any enumerator is added.
    void initUnderlying(const TypePtr&);

protected:

    Enum(const ContainerPtr&, const std::string&, bool);
    std::int64_t newEnumerator(const EnumeratorPtr&);

    friend class Container;
    friend class Module;
    friend class Enumerator;

    std::list<EnumeratorPtr> _enumerators;
    const bool _unchecked;
    BuiltinPtr _underlying;
    bool _explicitValue;
    std::int64_t _minValue;
    std::int64_t _maxValue;
    std::int64_t _lastValue;
};

// ----------------------------------------------------------------------
// Enumerator
// ----------------------------------------------------------------------

class Enumerator : public virtual Contained
{
public:

    EnumPtr type() const;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;

    bool explicitValue() const;
    std::int64_t value() const;

protected:

    Enumerator(const EnumPtr&, const std::string&);
    Enumerator(const EnumPtr&, const std::string&, std::int64_t);

    friend class Enum;

    bool _explicitValue;
    std::int64_t _value;
};

// ----------------------------------------------------------------------
// Const
// ----------------------------------------------------------------------

class Const : public virtual Contained
{
public:

    TypePtr type() const;
    StringList typeMetaData() const;
    SyntaxTreeBasePtr valueType() const;
    std::string value() const;
    std::string literal() const;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;

protected:

    Const(const ContainerPtr&, const std::string&, const TypePtr&, const StringList&, const SyntaxTreeBasePtr&,
          const std::string&, const std::string&);

    friend class Container;
    friend class Module;

    TypePtr _type;
    StringList _typeMetaData;
    SyntaxTreeBasePtr _valueType;
    std::string _value;
    std::string _literal;
};

// ----------------------------------------------------------------------
// ParamDecl
// ----------------------------------------------------------------------

class ParamDecl : public virtual Contained
{
public:

    TypePtr type() const;
    bool isOutParam() const;
    bool tagged() const;
    int tag() const;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;

protected:

    ParamDecl(const ContainerPtr&, const std::string&, const TypePtr&, bool, bool, int);

    friend class Operation;

    TypePtr _type;
    bool _isOutParam;
    bool _tagged;
    int _tag;
};

// ----------------------------------------------------------------------
// DataMember
// ----------------------------------------------------------------------

class DataMember : public virtual Contained
{
public:

    TypePtr type() const;
    bool tagged() const;
    int tag() const;
    std::string defaultValue() const;
    std::string defaultLiteral() const;
    SyntaxTreeBasePtr defaultValueType() const;
    bool uses(const ContainedPtr&) const override;
    std::string kindOf() const override;
    void visit(ParserVisitor*, bool) override;

protected:

    DataMember(const ContainerPtr&, const std::string&, const TypePtr&, bool, int, const SyntaxTreeBasePtr&,
               const std::string&, const std::string&);

    friend class ClassDef;
    friend class Struct;
    friend class Exception;

    TypePtr _type;
    bool _tagged;
    int _tag;
    SyntaxTreeBasePtr _defaultValueType;
    std::string _defaultValue;
    std::string _defaultLiteral;
};

// ----------------------------------------------------------------------
// Unit
// ----------------------------------------------------------------------

class Unit : public virtual Container
{
public:

    static UnitPtr createUnit(bool, bool, const StringList& = StringList());
    ModulePtr createModule(const std::string& name);

    bool ignRedefs() const;
    bool compatMode() const;
    void checkType(const TypePtr&);

    void setComment(const std::string&);
    void addToComment(const std::string&);
    std::string currentComment(); // Not const, as this function removes the current comment.
    std::string currentFile() const;
    std::string topLevelFile() const;
    int currentLine() const;

    int setCurrentFile(const std::string&, int);
    int currentIncludeLevel() const;

    void addFileMetaData(const StringList&);

    void error(const std::string&); // Not const because error count is increased
    void warning(WarningCategory, const std::string&) const;

    ContainerPtr currentContainer() const;
    ModulePtr currentModule() const;
    void pushContainer(const ContainerPtr&);
    void popContainer();

    DefinitionContextPtr currentDefinitionContext() const;
    void pushDefinitionContext();
    void popDefinitionContext();
    DefinitionContextPtr findDefinitionContext(const std::string&) const;

    void addContent(const ContainedPtr&);
    void removeContent(const ContainedPtr&);
    ContainedList findContents(const std::string&) const;
    ClassList findDerivedClasses(const ClassDefPtr&) const;
    ExceptionList findDerivedExceptions(const ExceptionPtr&) const;
    ContainedList findUsedBy(const ContainedPtr&) const;

    void addTypeId(int, const std::string&);
    std::string getTypeId(int) const;
    bool hasCompactTypeId() const;

    bool usesConsts() const;

    //
    // Returns the path names of the files included directly by the top-level file.
    //
    StringList includeFiles() const;

    //
    // Returns the path names of all files parsed by this unit.
    //
    StringList allFiles() const;

    int parse(const std::string&, FILE*, bool);

    void destroy() override;
    ContainedList contents() const override;
    void visit(ParserVisitor*, bool) override;
    bool hasSequences() const;
    bool hasStructs() const;
    bool hasExceptions() const;
    bool hasDictionaries() const;
    bool hasEnums() const;
    bool hasClassDecls() const;
    bool hasClassDefs() const;
    bool hasInterfaceDecls() const;
    bool hasInterfaceDefs() const;
    bool hasOnlyClassDecls() const;
    bool hasOnlyInterfaces() const;
    bool hasOperations() const; // interfaces or classes with operations
    bool hasOtherConstructedOrExceptions() const; // Exceptions or constructed types other than classes.
    bool hasAsyncOps() const;

    // Not const, as builtins are created on the fly. (Lazy initialization.)
    BuiltinPtr builtin(Builtin::Kind);
    OptionalPtr optionalBuiltin(Builtin::Kind);

    void addTopLevelModule(const std::string&, const std::string&);
    std::set<std::string> getTopLevelModules(const std::string&) const;

private:

    Unit(bool, bool, const StringList&);

    bool _ignRedefs;
    bool _all;
    StringList _defaultFileMetaData;
    int _errors;
    std::string _currentComment;
    int _currentIncludeLevel;
    std::string _topLevelFile;
    std::stack<DefinitionContextPtr> _definitionContextStack;
    StringList _includeFiles;
    ModulePtr _globalModule;
    std::list<ModulePtr> _modules;
    std::stack<ContainerPtr> _containerStack;
    std::map<Builtin::Kind, BuiltinPtr> _builtins;
    std::map<Builtin::Kind, OptionalPtr> _optionalBuiltins;
    std::map<std::string, ContainedList> _contentMap;
    std::map<std::string, DefinitionContextPtr> _definitionContextMap;
    std::map<int, std::string> _typeIds;
    std::map< std::string, std::set<std::string> > _fileTopLevelModules;
};

extern Unit* unit; // The current parser for bison/flex

}

#endif
