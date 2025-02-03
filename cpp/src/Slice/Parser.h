// Copyright (c) ZeroC, Inc.

#ifndef SLICE_PARSER_H
#define SLICE_PARSER_H

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <stack>
#include <stdio.h>
#include <string>
#include <string_view>

namespace Slice
{
    enum NodeType
    {
        Dummy,
        Real
    };

    //
    // Format to use when marshaling a class instance.
    //
    enum FormatType
    {
        CompactFormat, // Minimal format.
        SlicedFormat   // Full format.
    };

    enum WarningCategory
    {
        All,
        Deprecated,
        InvalidMetadata,
        InvalidComment
    };

    class GrammarBase;
    class SyntaxTreeBase;
    class Metadata;
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
    class Struct;
    class Operation;
    class Parameter;
    class DataMember;
    class Sequence;
    class Dictionary;
    class Enum;
    class Enumerator;
    class Const;
    class Unit;

    using GrammarBasePtr = std::shared_ptr<GrammarBase>;
    using SyntaxTreeBasePtr = std::shared_ptr<SyntaxTreeBase>;
    using MetadataPtr = std::shared_ptr<Metadata>;
    using TypePtr = std::shared_ptr<Type>;
    using BuiltinPtr = std::shared_ptr<Builtin>;
    using ContainedPtr = std::shared_ptr<Contained>;
    using ContainerPtr = std::shared_ptr<Container>;
    using ModulePtr = std::shared_ptr<Module>;
    using ConstructedPtr = std::shared_ptr<Constructed>;
    using ClassDeclPtr = std::shared_ptr<ClassDecl>;
    using ClassDefPtr = std::shared_ptr<ClassDef>;
    using InterfaceDeclPtr = std::shared_ptr<InterfaceDecl>;
    using InterfaceDefPtr = std::shared_ptr<InterfaceDef>;
    using ExceptionPtr = std::shared_ptr<Exception>;
    using StructPtr = std::shared_ptr<Struct>;
    using OperationPtr = std::shared_ptr<Operation>;
    using ParameterPtr = std::shared_ptr<Parameter>;
    using DataMemberPtr = std::shared_ptr<DataMember>;
    using SequencePtr = std::shared_ptr<Sequence>;
    using DictionaryPtr = std::shared_ptr<Dictionary>;
    using EnumPtr = std::shared_ptr<Enum>;
    using EnumeratorPtr = std::shared_ptr<Enumerator>;
    using ConstPtr = std::shared_ptr<Const>;
    using UnitPtr = std::shared_ptr<Unit>;

    bool containedCompare(const ContainedPtr& lhs, const ContainedPtr& rhs);
    bool containedEqual(const ContainedPtr& lhs, const ContainedPtr& rhs);

    using StringList = std::list<std::string>;
    using MetadataList = std::list<MetadataPtr>;
    using TypeList = std::list<TypePtr>;
    using ContainedList = std::list<ContainedPtr>;
    using ModuleList = std::list<ModulePtr>;
    using ClassList = std::list<ClassDefPtr>;
    using InterfaceList = std::list<InterfaceDefPtr>;
    using ExceptionList = std::list<ExceptionPtr>;
    using EnumList = std::list<EnumPtr>;
    using OperationList = std::list<OperationPtr>;
    using DataMemberList = std::list<DataMemberPtr>;
    using ParameterList = std::list<ParameterPtr>;
    using EnumeratorList = std::list<EnumeratorPtr>;

    // ----------------------------------------------------------------------
    // CICompare -- function object to do case-insensitive string comparison.
    // ----------------------------------------------------------------------

    class CICompare
    {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const;
    };

    // ----------------------------------------------------------------------
    // DerivedToBaseCompare -- function object to do sort exceptions into
    // most-derived to least-derived order.
    // ----------------------------------------------------------------------

    class DerivedToBaseCompare
    {
    public:
        bool operator()(const ExceptionPtr& lhs, const ExceptionPtr& rhs) const;
    };

    // ----------------------------------------------------------------------
    // ParserVisitor
    // ----------------------------------------------------------------------

    class ParserVisitor
    {
    public:
        virtual ~ParserVisitor() = default;
        virtual bool visitUnitStart(const UnitPtr& /*unit*/) { return true; }
        virtual void visitUnitEnd(const UnitPtr& /*unit*/) {}
        virtual bool visitModuleStart(const ModulePtr& /*module*/) { return true; }
        virtual void visitModuleEnd(const ModulePtr& /*module*/) {}
        virtual void visitClassDecl(const ClassDeclPtr& /*classDecl*/) {}
        virtual bool visitClassDefStart(const ClassDefPtr& /*classDef*/) { return true; }
        virtual void visitClassDefEnd(const ClassDefPtr& /*classDef*/) {}
        virtual void visitInterfaceDecl(const InterfaceDeclPtr& /*interfaceDecl*/) {}
        virtual bool visitInterfaceDefStart(const InterfaceDefPtr& /*interfaceDef*/) { return true; }
        virtual void visitInterfaceDefEnd(const InterfaceDefPtr& /*interfaceDef*/) {}
        virtual bool visitExceptionStart(const ExceptionPtr& /*exceptionDef*/) { return true; }
        virtual void visitExceptionEnd(const ExceptionPtr& /*exceptionDef*/) {}
        virtual bool visitStructStart(const StructPtr& /*structDef*/) { return true; }
        virtual void visitStructEnd(const StructPtr& /*structDef*/) {}
        virtual void visitOperation(const OperationPtr& /*operation*/) {}
        virtual void visitParameter(const ParameterPtr& /*parameter*/) {}
        virtual void visitDataMember(const DataMemberPtr& /*member*/) {}
        virtual void visitSequence(const SequencePtr& /*sequence*/) {}
        virtual void visitDictionary(const DictionaryPtr& /*dictionary*/) {}
        virtual void visitEnum(const EnumPtr& /*enumDef*/) {}
        virtual void visitConst(const ConstPtr& /*constDef*/) {}

        [[nodiscard]] virtual bool shouldVisitIncludedDefinitions() const { return false; }
    };

    // ----------------------------------------------------------------------
    // GrammarBase
    // ----------------------------------------------------------------------

    class GrammarBase
    {
    public:
        virtual ~GrammarBase() = default;
    };

    // ----------------------------------------------------------------------
    // Metadata
    // ----------------------------------------------------------------------

    class Metadata final : public GrammarBase
    {
    public:
        Metadata(std::string rawMetadata, std::string file, int line);
        [[nodiscard]] const std::string& directive() const;
        [[nodiscard]] const std::string& arguments() const;

        [[nodiscard]] std::string file() const;
        [[nodiscard]] int line() const;

        friend std::ostream& operator<<(std::ostream& out, const Metadata& metadata);

    private:
        std::string _directive;
        std::string _arguments;

        std::string _file;
        int _line;
    };

    inline std::ostream& operator<<(std::ostream& ostr, const Metadata& metadata)
    {
        ostr << metadata._directive;
        if (!metadata._arguments.empty())
        {
            ostr << ":" << metadata._arguments;
        }
        return ostr;
    }

    // ----------------------------------------------------------------------
    // DefinitionContext
    // ----------------------------------------------------------------------

    class DefinitionContext final
    {
    public:
        DefinitionContext(int includeLevel, MetadataList metadata);

        [[nodiscard]] std::string filename() const;
        [[nodiscard]] int includeLevel() const;
        [[nodiscard]] bool seenDefinition() const;

        void setFilename(const std::string& filename);
        void setSeenDefinition();

        [[nodiscard]] MetadataList getMetadata() const;
        void setMetadata(MetadataList metadata);
        void appendMetadata(MetadataList metadata);
        [[nodiscard]] bool hasMetadata(std::string_view directive) const;
        [[nodiscard]] std::optional<std::string> getMetadataArgs(std::string_view directive) const;

        [[nodiscard]] bool isSuppressed(WarningCategory category) const;

    private:
        void initSuppressedWarnings();

        int _includeLevel;
        MetadataList _metadata;
        std::string _filename;
        bool _seenDefinition{false};
        std::set<WarningCategory> _suppressedWarnings;
    };
    using DefinitionContextPtr = std::shared_ptr<DefinitionContext>;

    // ----------------------------------------------------------------------
    // DocComment
    // ----------------------------------------------------------------------

    /// Functions of this type are used by `DocComment::parseFrom` to map link tags into each language's link syntax.
    /// In Slice, links are of the form: '{@link <rawLink>}'.
    ///
    /// The first argument (`rawLink`) is the raw link text, taken verbatim from the doc-comment.
    /// The second argument (`source`) is a pointer to the Slice element that the doc comment (and link) are written on.
    /// The third argument (`target`) is a pointer to the Slice element that is being linked to.
    /// If the parser could not resolve the link, this will be `nullptr`.
    ///
    /// This function should return the fully formatted link.
    /// `DocComment::parseFrom` replaces the entire '{@link <rawLink>}' by the string this function returns.
    using DocLinkFormatter =
        std::string (*)(const std::string& rawLink, const ContainedPtr& source, const SyntaxTreeBasePtr& target);

    class DocComment final
    {
    public:
        /// Parses the raw doc-comment attached to `p` into a structured `DocComment`.
        ///
        /// @param p The slice element whose doc-comment should be parsed.
        /// @param linkFormatter A function used to format links according to the target language's syntax.
        /// @param stripMarkup If true, removes all HTML markup from the parsed comment. Defaults to false.
        /// @param escapeXml If true, escapes all XML special characters in the parsed comment. Defaults to false.
        ///
        /// @return A `DocComment` instance containing a parsed representation of `p`'s doc-comment, if a doc-comment
        /// was present. If no doc-comment was present (or it contained only whitespace) this returns `nullopt` instead.
        [[nodiscard]] static std::optional<DocComment> parseFrom(
            const ContainedPtr& p,
            DocLinkFormatter linkFormatter,
            bool stripMarkup = false,
            bool escapeXml = false);

        [[nodiscard]] bool isDeprecated() const;
        [[nodiscard]] StringList deprecated() const;

        /// Contains all introductory lines up to the first tag.
        [[nodiscard]] StringList overview() const;
        /// Targets of '@see' tags.
        [[nodiscard]] StringList seeAlso() const;

        /// Description of an operation's return value.
        [[nodiscard]] StringList returns() const;
        /// Parameter descriptions for an op. Key is parameter name.
        [[nodiscard]] std::map<std::string, StringList> parameters() const;
        /// Exception descriptions for an op. Key is exception name.
        [[nodiscard]] std::map<std::string, StringList> exceptions() const;

    private:
        bool _isDeprecated{false};
        StringList _deprecated;
        StringList _overview;
        StringList _seeAlso;

        StringList _returns;
        std::map<std::string, StringList> _parameters;
        std::map<std::string, StringList> _exceptions;
    };

    // ----------------------------------------------------------------------
    // SyntaxTreeBase
    // ----------------------------------------------------------------------

    class SyntaxTreeBase : public GrammarBase
    {
    public:
        SyntaxTreeBase(UnitPtr unit);
        virtual void destroy();
        [[nodiscard]] UnitPtr unit() const;
        virtual void visit(ParserVisitor* visitor);

    protected:
        UnitPtr _unit;
    };

    // ----------------------------------------------------------------------
    // Type
    // ----------------------------------------------------------------------

    class Type : public virtual SyntaxTreeBase
    {
    public:
        [[nodiscard]] virtual bool isClassType() const;
        [[nodiscard]] virtual bool usesClasses() const;
        [[nodiscard]] virtual size_t minWireSize() const = 0;
        [[nodiscard]] virtual std::string getOptionalFormat() const = 0;
        [[nodiscard]] virtual bool isVariableLength() const = 0;
    };

    // ----------------------------------------------------------------------
    // Builtin
    // ----------------------------------------------------------------------

    class Builtin final : public virtual Type
    {
    public:
        enum Kind
        {
            KindByte,
            KindBool,
            KindShort,
            KindInt,
            KindLong,
            KindFloat,
            KindDouble,
            KindString,
            KindObject,
            KindObjectProxy,
            KindValue
        };

        Builtin(const UnitPtr& unit, Kind kind);

        [[nodiscard]] bool isClassType() const final;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;

        [[nodiscard]] bool isNumericType() const;
        [[nodiscard]] bool isIntegralType() const;

        [[nodiscard]] Kind kind() const;
        [[nodiscard]] std::string kindAsString() const;
        static std::optional<Kind> kindFromString(std::string_view str);

        // NOLINTNEXTLINE(cert-err58-cpp)
        inline static const std::array<std::string, 11> builtinTable =
            {"byte", "bool", "short", "int", "long", "float", "double", "string", "Object", "Object*", "Value"};

    private:
        const Kind _kind;
    };

    // ----------------------------------------------------------------------
    // Contained
    // ----------------------------------------------------------------------

    class Contained : public virtual SyntaxTreeBase
    {
    public:
        [[nodiscard]] ContainerPtr container() const;

        /// Returns the Slice identifier of this element.
        [[nodiscard]] std::string name() const;
        /// Returns the Slice scope that this element is contained within (with a trailing '::').
        [[nodiscard]] std::string scope() const;
        /// Returns the fully-scoped Slice identifier of this element (equivalent to `scope() + name()`).
        [[nodiscard]] std::string scoped() const;

        /// Returns the mapped identifier that this element will use in the target language.
        [[nodiscard]] std::string mappedName() const;
        /// Returns the mapped scope that this element will be generated in in the target language.
        /// (equivalent to `mappedScope(separator) + mappedName()`).
        [[nodiscard]] std::string mappedScoped(const std::string& separator = "::") const;
        /// Returns the mapped fully-scoped identifier that this element will use in the target language.
        [[nodiscard]] std::string mappedScope(const std::string& separator = "::") const;

        [[nodiscard]] std::string file() const;
        [[nodiscard]] int line() const;

        [[nodiscard]] std::string docComment() const;

        [[nodiscard]] int includeLevel() const;
        [[nodiscard]] DefinitionContextPtr definitionContext() const;

        [[nodiscard]] virtual MetadataList getMetadata() const;
        virtual void setMetadata(MetadataList metadata);
        virtual void appendMetadata(MetadataList metadata);
        [[nodiscard]] bool hasMetadata(std::string_view directive) const;
        [[nodiscard]] std::optional<std::string> getMetadataArgs(std::string_view directive) const;

        [[nodiscard]] std::optional<FormatType> parseFormatMetadata() const;

        /// Returns true if this item is deprecated, due to the presence of 'deprecated' metadata.
        /// @return True if this item has 'deprecated' metadata on it, false otherwise.
        [[nodiscard]] bool isDeprecated() const;

        /// If this item is deprecated, return its deprecation message (if present).
        /// This is the string argument that can be optionally provided with 'deprecated' metadata.
        /// @return The message provided to the 'deprecated' metadata, if present.
        [[nodiscard]] std::optional<std::string> getDeprecationReason() const;

        [[nodiscard]] virtual std::string kindOf() const = 0;

    protected:
        Contained(const ContainerPtr& container, std::string name);

        ContainerPtr _container;
        std::string _name;
        std::string _file;
        int _line;
        std::string _docComment;
        int _includeLevel;
        DefinitionContextPtr _definitionContext;
        MetadataList _metadata;
    };

    // ----------------------------------------------------------------------
    // Container
    // ----------------------------------------------------------------------

    class Container : public virtual SyntaxTreeBase, public std::enable_shared_from_this<Container>
    {
    public:
        Container(const UnitPtr& unit);
        void destroy() override;
        ModulePtr createModule(const std::string& name);
        [[nodiscard]] ClassDefPtr createClassDef(const std::string& name, int id, const ClassDefPtr& base);
        [[nodiscard]] ClassDeclPtr createClassDecl(const std::string& name);
        [[nodiscard]] InterfaceDefPtr createInterfaceDef(const std::string& name, const InterfaceList& bases);
        [[nodiscard]] InterfaceDeclPtr createInterfaceDecl(const std::string& name);
        [[nodiscard]] ExceptionPtr
        createException(const std::string& name, const ExceptionPtr& base, NodeType nodeType = Real);
        [[nodiscard]] StructPtr createStruct(const std::string& name, NodeType nodeType = Real);
        [[nodiscard]] SequencePtr
        createSequence(const std::string& name, const TypePtr& type, MetadataList metadata, NodeType nodeType = Real);
        [[nodiscard]] DictionaryPtr createDictionary(
            const std::string& name,
            const TypePtr& keyType,
            MetadataList keyMetadata,
            const TypePtr& valueType,
            MetadataList valueMetadata,
            NodeType nodeType = Real);
        [[nodiscard]] EnumPtr createEnum(const std::string& name, NodeType nodeType = Real);
        [[nodiscard]] ConstPtr createConst(
            const std::string& name,
            const TypePtr& constType,
            MetadataList metadata,
            const SyntaxTreeBasePtr& valueType,
            const std::string& value,
            NodeType nodeType = Real);
        [[nodiscard]] TypeList lookupType(const std::string& identifier);
        [[nodiscard]] TypeList
        lookupTypeNoBuiltin(const std::string& identifier, bool emitErrors, bool ignoreUndefined = false);
        [[nodiscard]] ContainedList lookupContained(const std::string& identifier, bool emitErrors);
        [[nodiscard]] ExceptionPtr lookupException(const std::string& identifier, bool emitErrors);
        [[nodiscard]] UnitPtr unit() const;
        [[nodiscard]] ModuleList modules() const;
        [[nodiscard]] InterfaceList interfaces() const;
        [[nodiscard]] EnumList enums() const;
        [[nodiscard]] EnumeratorList enumerators() const;
        [[nodiscard]] EnumeratorList enumerators(const std::string& identifier) const;
        [[nodiscard]] ContainedList contents() const;
        void visit(ParserVisitor* visitor) override;

        bool checkIntroduced(const std::string& scopedName, ContainedPtr namedThing = nullptr);

        /// Returns true if this container is the global scope (i.e. it's of type `Unit`), and false otherwise.
        /// If false, we emit an error message. So this function should only be called for types which cannot appear at
        /// global scope... so everything except for `Module`s.
        bool checkForGlobalDefinition(const char* definitionKindPlural);

        /// Returns true if this contains elements of the specified type.
        /// This check is recursive, so it will still return true even if the type is only contained indirectly.
        template<typename T> [[nodiscard]] bool contains() const
        {
            for (const auto& p : contents())
            {
                if (std::dynamic_pointer_cast<T>(p))
                {
                    return true;
                }
                ContainerPtr childContainer = std::dynamic_pointer_cast<Container>(p);
                if (childContainer && childContainer->contains<T>())
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] std::string thisScope() const;

    protected:
        bool validateConstant(
            const std::string& name,
            const TypePtr& type,
            SyntaxTreeBasePtr& valueType,
            const std::string& valueString,
            bool isConstant);

        ContainedList _contents;
        std::map<std::string, ContainedPtr, CICompare> _introducedMap;
    };

    // ----------------------------------------------------------------------
    // Module
    // ----------------------------------------------------------------------

    class Module final : public virtual Container, public virtual Contained
    {
    public:
        Module(const ContainerPtr& container, const std::string& name);
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;
    };

    // ----------------------------------------------------------------------
    // Constructed
    // ----------------------------------------------------------------------

    class Constructed : public virtual Type, public virtual Contained
    {
    public:
        Constructed();
    };

    // ----------------------------------------------------------------------
    // ClassDecl
    // ----------------------------------------------------------------------

    class ClassDecl final : public virtual Constructed, public std::enable_shared_from_this<ClassDecl>
    {
    public:
        ClassDecl(const ContainerPtr& container, const std::string& name);
        void destroy() final;
        [[nodiscard]] ClassDefPtr definition() const;
        [[nodiscard]] bool isClassType() const final;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        void visit(ParserVisitor* visitor) final;
        [[nodiscard]] std::string kindOf() const final;

    private:
        friend class Container;

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
    class ClassDef final : public virtual Container, public virtual Contained
    {
    public:
        ClassDef(const ContainerPtr& container, const std::string& name, int id, ClassDefPtr base);
        void destroy() final;
        DataMemberPtr createDataMember(
            const std::string& name,
            const TypePtr& type,
            bool isOptional,
            int tag,
            const SyntaxTreeBasePtr& defaultValueType,
            const std::string& defaultValueString);
        [[nodiscard]] ClassDeclPtr declaration() const;
        [[nodiscard]] ClassDefPtr base() const;
        [[nodiscard]] ClassList allBases() const;
        [[nodiscard]] DataMemberList dataMembers() const;
        [[nodiscard]] DataMemberList orderedOptionalDataMembers() const;
        [[nodiscard]] DataMemberList allDataMembers() const;
        [[nodiscard]] DataMemberList classDataMembers() const;
        [[nodiscard]] bool canBeCyclic() const;
        void visit(ParserVisitor* visitor) final;
        [[nodiscard]] int compactId() const;
        [[nodiscard]] std::string kindOf() const final;

        // Class metadata is always stored on the underlying decl type, not the definition.
        // So we override these `xMetadata` functions to forward to `_declarations->xMetadata()` instead.
        [[nodiscard]] MetadataList getMetadata() const final;
        void setMetadata(MetadataList metadata) final;
        void appendMetadata(MetadataList metadata) final;

    private:
        friend class Container;

        ClassDeclPtr _declaration;
        ClassDefPtr _base;
        int _compactId;
    };

    // ----------------------------------------------------------------------
    // InterfaceDecl
    // ----------------------------------------------------------------------

    class InterfaceDecl final : public virtual Constructed, public std::enable_shared_from_this<InterfaceDecl>
    {
    public:
        InterfaceDecl(const ContainerPtr& container, const std::string& name);
        void destroy() final;
        [[nodiscard]] InterfaceDefPtr definition() const;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        void visit(ParserVisitor* visitor) final;
        [[nodiscard]] std::string kindOf() const final;

        static void checkBasesAreLegal(const std::string& name, const InterfaceList& bases, const UnitPtr& unit);

    private:
        friend class Container;

        InterfaceDefPtr _definition;

        using GraphPartitionList = std::list<InterfaceList>;
        using StringPartitionList = std::list<StringList>;

        static bool isInList(const GraphPartitionList& gpl, const InterfaceDefPtr& interfaceDef);

        static void addPartition(
            GraphPartitionList& partitions,
            const GraphPartitionList::reverse_iterator& tail,
            const InterfaceDefPtr& base);
        static StringPartitionList toStringPartitionList(const GraphPartitionList& partitions);
        static void checkPairIntersections(
            const StringPartitionList& stringPartitions,
            const std::string& name,
            const UnitPtr& unit);
    };

    // ----------------------------------------------------------------------
    // Operation
    // ----------------------------------------------------------------------

    class Operation final : public virtual Contained, public virtual Container
    {
    public:
        //
        // Note: The values of enumerators here *must* match the values for
        // enumerators of Ice::OperationMode in slice/Ice/OperationMode.ice!
        //
        enum Mode
        {
            Normal = 0,
            Idempotent = 2
        };

        Operation(const ContainerPtr&, const std::string&, TypePtr, bool, int, Mode);
        [[nodiscard]] InterfaceDefPtr interface() const;
        [[nodiscard]] TypePtr returnType() const;
        [[nodiscard]] bool returnIsOptional() const;
        [[nodiscard]] int returnTag() const;
        [[nodiscard]] Mode mode() const;
        [[nodiscard]] bool hasMarshaledResult() const;

        ParameterPtr
        createParameter(const std::string& name, const TypePtr& type, bool isOutParam, bool isOptional, int tag);

        [[nodiscard]] ParameterList parameters() const;
        [[nodiscard]] ParameterList inParameters() const;
        void inParameters(ParameterList& required, ParameterList& optional) const;
        [[nodiscard]] ParameterList outParameters() const;
        void outParameters(ParameterList& required, ParameterList& optional) const;
        [[nodiscard]] ExceptionList throws() const;
        void setExceptionList(const ExceptionList& exceptions);
        [[nodiscard]] bool sendsClasses() const;
        [[nodiscard]] bool returnsClasses() const;
        [[nodiscard]] bool returnsData() const;
        [[nodiscard]] bool returnsMultipleValues() const;
        [[nodiscard]] bool sendsOptionals() const;
        [[nodiscard]] std::optional<FormatType> format() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        TypePtr _returnType;
        bool _returnIsOptional;
        int _returnTag;
        ExceptionList _throws;
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
    class InterfaceDef final : public virtual Container, public virtual Contained
    {
    public:
        InterfaceDef(const ContainerPtr& container, const std::string& name, InterfaceList bases);
        void destroy() final;
        OperationPtr createOperation(
            const std::string& name,
            const TypePtr& returnType,
            bool isOptional,
            int tag,
            Operation::Mode mode = Operation::Normal);

        [[nodiscard]] InterfaceDeclPtr declaration() const;
        [[nodiscard]] InterfaceList bases() const;
        [[nodiscard]] InterfaceList allBases() const;
        [[nodiscard]] OperationList operations() const;
        [[nodiscard]] OperationList allOperations() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

        // Returns the type IDs of all the interfaces in the inheritance tree, in alphabetical order.
        [[nodiscard]] StringList ids() const;

        // Interface metadata is always stored on the underlying decl type, not the definition.
        // So we override these `xMetadata` functions to forward to `_declarations->xMetadata()` instead.
        [[nodiscard]] MetadataList getMetadata() const final;
        void setMetadata(MetadataList metadata) final;
        void appendMetadata(MetadataList metadata) final;

    private:
        friend class Container;

        // Returns true if name does not collide with any base name; otherwise, false.
        bool checkBaseOperationNames(const std::string& name, const std::vector<std::string>& baseNames);

        InterfaceDeclPtr _declaration;
        InterfaceList _bases;
    };

    // ----------------------------------------------------------------------
    // Exception
    // ----------------------------------------------------------------------

    // No inheritance from Constructed, as this is not a Type
    class Exception final : public virtual Container, public virtual Contained
    {
    public:
        Exception(const ContainerPtr& container, const std::string& name, ExceptionPtr base);
        void destroy() final;
        DataMemberPtr createDataMember(
            const std::string& name,
            const TypePtr& type,
            bool isOptional,
            int tag,
            const SyntaxTreeBasePtr& defaultValueType,
            const std::string& defaultValueString);
        [[nodiscard]] DataMemberList dataMembers() const;
        [[nodiscard]] DataMemberList orderedOptionalDataMembers() const;
        [[nodiscard]] DataMemberList allDataMembers() const;
        [[nodiscard]] DataMemberList classDataMembers() const;
        [[nodiscard]] ExceptionPtr base() const;
        [[nodiscard]] ExceptionList allBases() const;
        [[nodiscard]] bool isBaseOf(const ExceptionPtr& otherException) const;
        [[nodiscard]] bool usesClasses() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        ExceptionPtr _base;
    };

    // ----------------------------------------------------------------------
    // Struct
    // ----------------------------------------------------------------------

    class Struct final : public virtual Container, public virtual Constructed
    {
    public:
        Struct(const ContainerPtr& container, const std::string& name);
        DataMemberPtr createDataMember(
            const std::string& name,
            const TypePtr& type,
            bool isOptional,
            int tag,
            const SyntaxTreeBasePtr& defaultValueType,
            const std::string& defaultValueString);
        [[nodiscard]] DataMemberList dataMembers() const;
        [[nodiscard]] DataMemberList classDataMembers() const;
        [[nodiscard]] bool usesClasses() const final;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;
    };

    // ----------------------------------------------------------------------
    // Sequence
    // ----------------------------------------------------------------------

    class Sequence final : public virtual Constructed, public std::enable_shared_from_this<Sequence>
    {
    public:
        Sequence(const ContainerPtr& container, const std::string& name, TypePtr type, MetadataList typeMetadata);
        [[nodiscard]] TypePtr type() const;
        [[nodiscard]] MetadataList typeMetadata() const;
        void setTypeMetadata(MetadataList metadata);
        [[nodiscard]] bool usesClasses() const final;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        TypePtr _type;
        MetadataList _typeMetadata;
    };

    // ----------------------------------------------------------------------
    // Dictionary
    // ----------------------------------------------------------------------

    class Dictionary final : public virtual Constructed, public std::enable_shared_from_this<Dictionary>
    {
    public:
        Dictionary(
            const ContainerPtr& container,
            const std::string& name,
            TypePtr keyType,
            MetadataList keyMetadata,
            TypePtr valueType,
            MetadataList valueMetadata);
        [[nodiscard]] TypePtr keyType() const;
        [[nodiscard]] TypePtr valueType() const;
        [[nodiscard]] MetadataList keyMetadata() const;
        [[nodiscard]] MetadataList valueMetadata() const;
        void setKeyMetadata(MetadataList metadata);
        void setValueMetadata(MetadataList metadata);
        [[nodiscard]] bool usesClasses() const final;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

        static bool isLegalKeyType(const TypePtr& type);

    private:
        TypePtr _keyType;
        TypePtr _valueType;
        MetadataList _keyMetadata;
        MetadataList _valueMetadata;
    };

    // ----------------------------------------------------------------------
    // Enum
    // ----------------------------------------------------------------------

    class Enum final : public virtual Container, public virtual Constructed
    {
    public:
        Enum(const ContainerPtr& container, const std::string& name);
        void destroy() final;
        EnumeratorPtr createEnumerator(const std::string& name, std::optional<int> explicitValue);
        [[nodiscard]] bool hasExplicitValues() const;
        [[nodiscard]] int minValue() const;
        [[nodiscard]] int maxValue() const;
        [[nodiscard]] size_t minWireSize() const final;
        [[nodiscard]] std::string getOptionalFormat() const final;
        [[nodiscard]] bool isVariableLength() const final;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        bool _hasExplicitValues{false};
        std::int64_t _minValue;
        std::int64_t _maxValue{0};
        int _lastValue{-1};
    };

    // ----------------------------------------------------------------------
    // Enumerator
    // ----------------------------------------------------------------------

    class Enumerator final : public virtual Contained
    {
    public:
        Enumerator(const ContainerPtr& container, const std::string& name, int value, bool hasExplicitValue);
        [[nodiscard]] EnumPtr type() const;
        [[nodiscard]] std::string kindOf() const final;

        [[nodiscard]] bool hasExplicitValue() const;
        [[nodiscard]] int value() const;

    private:
        bool _hasExplicitValue;
        int _value;
    };

    // ----------------------------------------------------------------------
    // Const
    // ----------------------------------------------------------------------

    class Const final : public virtual Contained, public std::enable_shared_from_this<Const>
    {
    public:
        Const(
            const ContainerPtr& container,
            const std::string& name,
            TypePtr type,
            MetadataList typeMetadata,
            SyntaxTreeBasePtr valueType,
            std::string valueString);
        [[nodiscard]] TypePtr type() const;
        [[nodiscard]] MetadataList typeMetadata() const;
        void setTypeMetadata(MetadataList metadata);
        [[nodiscard]] SyntaxTreeBasePtr valueType() const;
        [[nodiscard]] std::string value() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        TypePtr _type;
        MetadataList _typeMetadata;
        SyntaxTreeBasePtr _valueType;
        std::string _value;
    };

    // ----------------------------------------------------------------------
    // Parameter
    // ----------------------------------------------------------------------

    class Parameter final : public virtual Contained, public std::enable_shared_from_this<Parameter>
    {
    public:
        Parameter(
            const ContainerPtr& container,
            const std::string& name,
            TypePtr type,
            bool isOutParam,
            bool isOptional,
            int tag);
        [[nodiscard]] TypePtr type() const;
        [[nodiscard]] bool isOutParam() const;
        [[nodiscard]] bool optional() const;
        [[nodiscard]] int tag() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        TypePtr _type;
        bool _isOutParam;
        bool _optional;
        int _tag;
    };

    // ----------------------------------------------------------------------
    // DataMember
    // ----------------------------------------------------------------------

    class DataMember final : public virtual Contained, public std::enable_shared_from_this<DataMember>
    {
    public:
        DataMember(
            const ContainerPtr& container,
            const std::string& name,
            TypePtr type,
            bool isOptional,
            int tag,
            SyntaxTreeBasePtr defaultValueType,
            std::string defaultValueString);
        [[nodiscard]] TypePtr type() const;
        [[nodiscard]] bool optional() const;
        [[nodiscard]] int tag() const;
        [[nodiscard]] std::string defaultValue() const;
        [[nodiscard]] SyntaxTreeBasePtr defaultValueType() const;
        [[nodiscard]] std::string kindOf() const final;
        void visit(ParserVisitor* visitor) final;

    private:
        TypePtr _type;
        bool _optional;
        int _tag;
        SyntaxTreeBasePtr _defaultValueType;
        std::string _defaultValue;
    };

    // ----------------------------------------------------------------------
    // Unit
    // ----------------------------------------------------------------------

    class Unit final : public virtual Container
    {
    public:
        static UnitPtr
        createUnit(std::string languageName, bool all, const StringList& defaultFileMetadata = StringList());

        [[nodiscard]] std::string languageName() const;

        void setDocComment(const std::string& comment);
        void addToDocComment(const std::string& comment);
        std::string currentDocComment(); // Not const, as this function removes the current doc-comment.
        [[nodiscard]] std::string currentFile() const;
        [[nodiscard]] std::string topLevelFile() const;
        [[nodiscard]] int currentLine() const;

        int setCurrentFile(const std::string& currentFile, int lineNumber);
        [[nodiscard]] int currentIncludeLevel() const;

        void addFileMetadata(MetadataList metadata);

        void setSeenDefinition();

        void error(std::string_view message);
        void error(std::string_view file, int line, std::string_view message);
        void warning(WarningCategory category, std::string_view message) const;
        void warning(std::string_view file, int line, WarningCategory category, std::string_view message) const;

        [[nodiscard]] ContainerPtr currentContainer() const;
        void pushContainer(const ContainerPtr& container);
        void popContainer();

        [[nodiscard]] DefinitionContextPtr currentDefinitionContext() const;
        [[nodiscard]] DefinitionContextPtr findDefinitionContext(std::string_view file) const;

        void addContent(const ContainedPtr& contained);
        [[nodiscard]] ContainedList findContents(const std::string& scopedName) const;

        void addTypeId(int compactId, const std::string& typeId);
        [[nodiscard]] std::string getTypeId(int compactId) const;

        // Returns the path names of the files included directly by the top-level file.
        [[nodiscard]] StringList includeFiles() const;

        // Returns the path names of all files parsed by this unit.
        [[nodiscard]] StringList allFiles() const;

        int parse(const std::string& filename, FILE* file, bool debugMode);

        void destroy() final;
        void visit(ParserVisitor* visitor) final;

        // Not const, as builtins are created on the fly. (Lazy initialization.)
        BuiltinPtr createBuiltin(Builtin::Kind kind);

        void addTopLevelModule(const std::string& file, const std::string& module);
        [[nodiscard]] std::set<std::string> getTopLevelModules(const std::string& file) const;

    private:
        Unit(std::string languageName, bool all, MetadataList defaultFileMetadata);

        void pushDefinitionContext();
        void popDefinitionContext();

        const std::string _languageName;
        bool _all;
        MetadataList _defaultFileMetadata;
        int _errors{0};
        std::string _currentDocComment;
        int _currentIncludeLevel{0};
        std::string _topLevelFile;
        std::stack<DefinitionContextPtr> _definitionContextStack;
        StringList _includeFiles;
        std::stack<ContainerPtr> _containerStack;
        std::map<Builtin::Kind, BuiltinPtr> _builtins;
        std::map<std::string, ContainedList> _contentMap;
        std::map<std::string, DefinitionContextPtr, std::less<>> _definitionContextMap;
        std::map<int, std::string> _typeIds;
        std::map<std::string, std::set<std::string>> _fileTopLevelModules;
    };

    extern Unit* currentUnit; // The current parser for bison/flex
}

#endif
