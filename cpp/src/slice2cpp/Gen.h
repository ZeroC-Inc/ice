//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GEN_H
#define GEN_H

#include "Slice/Parser.h"
#include "IceUtil/OutputUtil.h"

namespace Slice
{

class Gen
{
public:

    Gen(const std::string&,
        const std::string&,
        const std::string&,
        const std::vector<std::string>&,
        const std::string&,
        const std::vector<std::string>&,
        const std::string&,
        const std::string&);
    ~Gen();

    Gen(const Gen&) = delete;

    void generate(const UnitPtr&);
    void closeOutput();

    static int setUseWstring(ContainedPtr, std::list<int>&, int);
    static int resetUseWstring(std::list<int>&);

private:

    void writeExtraHeaders(::IceUtilInternal::Output&);

    //
    // Returns the header extension defined in the file metadata for a given file,
    // or an empty string if no file metadata was found.
    //
    std::string getHeaderExt(const std::string& file, const UnitPtr& unit);

    //
    // Returns the source extension defined in the file metadata for a given file,
    // or an empty string if no file metadata was found.
    //
    std::string getSourceExt(const std::string& file, const UnitPtr& unit);

    ::IceUtilInternal::Output H;
    ::IceUtilInternal::Output C;

    ::IceUtilInternal::Output implH;
    ::IceUtilInternal::Output implC;

    std::string _base;
    std::string _headerExtension;
    std::string _implHeaderExtension;
    std::string _sourceExtension;
    std::vector<std::string> _extraHeaders;
    std::string _include;
    std::vector<std::string> _includePaths;
    std::string _dllExport;
    std::string _dir;

    // Visitors, in code-generation order.

    // Generates forward declarations for classes, proxies and structs. Also generates using aliases for sequences and
    // dictionaries, enum definitions and constants.
    class ForwardDeclVisitor final : public ParserVisitor
    {
    public:

        ForwardDeclVisitor(::IceUtilInternal::Output&);
        ForwardDeclVisitor(const ForwardDeclVisitor&) = delete;

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        void visitClassDecl(const ClassDeclPtr&) final;
        void visitInterfaceDecl(const InterfaceDeclPtr&) final;
        bool visitStructStart(const StructPtr&) final;
        void visitSequence(const SequencePtr&) final;
        void visitDictionary(const DictionaryPtr&) final;
        void visitEnum(const EnumPtr&) final;
        void visitConst(const ConstPtr&) final;

    private:

        ::IceUtilInternal::Output& H;

        int _useWstring;
        std::list<int> _useWstringHist;
    };

    // Generates the code that registers the default class and exception factories.
    class DefaultFactoryVisitor final : public ParserVisitor
    {
    public:

        DefaultFactoryVisitor(::IceUtilInternal::Output&);
        DefaultFactoryVisitor(const DefaultFactoryVisitor&) = delete;

        bool visitUnitStart(const UnitPtr&) final;
        void visitUnitEnd(const UnitPtr&) final;
        bool visitClassDefStart(const ClassDefPtr&) final;
        bool visitExceptionStart(const ExceptionPtr&) final;

    private:

        ::IceUtilInternal::Output& C;
    };

    // Generates code for proxies. We need to generate this code before the code for structs, classes and exceptions
    // because a data member with a proxy type (e.g., std::optional<GreeterPrx>) needs to see a complete type.
    class ProxyVisitor final : public ParserVisitor
    {
    public:

        ProxyVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);
        ProxyVisitor(const ProxyVisitor&) = delete;

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
        void visitInterfaceDefEnd(const InterfaceDefPtr&) final;
        void visitOperation(const OperationPtr&) final;

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    // Generates code for definitions with data members - structs, classes and exceptions.
    class DataDefVisitor final : public ParserVisitor
    {
    public:

        DataDefVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);
        DataDefVisitor(const DataDefVisitor&) = delete;

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        bool visitStructStart(const StructPtr&) final;
        void visitStructEnd(const StructPtr&) final;
        bool visitExceptionStart(const ExceptionPtr&) final;
        void visitExceptionEnd(const ExceptionPtr&) final;
        bool visitClassDefStart(const ClassDefPtr&) final;
        void visitClassDefEnd(const ClassDefPtr&) final;
        void visitDataMember(const DataMemberPtr&) final;

    private:

        bool emitBaseInitializers(const ClassDefPtr&);
        void emitOneShotConstructor(const ClassDefPtr&);
        void emitDataMember(const DataMemberPtr&);

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        std::string _dllClassExport;
        std::string _dllMemberExport;
        bool _doneStaticSymbol;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    // Generates the server-side classes that applications use to implement Ice objects.
    class InterfaceVisitor final : public ParserVisitor
    {
    public:

        InterfaceVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);
        InterfaceVisitor(const InterfaceVisitor&) = delete;

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
        void visitInterfaceDefEnd(const InterfaceDefPtr&) final;
        void visitOperation(const OperationPtr&) final;

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    // Generates internal StreamHelper template specializations for enums, structs, classes and exceptions.
    class StreamVisitor final : public ParserVisitor
    {
    public:

        StreamVisitor(::IceUtilInternal::Output&);
        StreamVisitor(const StreamVisitor&) = delete;

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        bool visitStructStart(const StructPtr&) final;
        bool visitClassDefStart(const ClassDefPtr&) final;
        bool visitExceptionStart(const ExceptionPtr&) final;
        void visitExceptionEnd(const ExceptionPtr&) final;
        void visitEnum(const EnumPtr&) final;

    private:

        ::IceUtilInternal::Output& H;
    };

    class MetaDataVisitor final : public ParserVisitor
    {
    public:

        bool visitUnitStart(const UnitPtr&) final;
        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        void visitClassDecl(const ClassDeclPtr&) final;
        bool visitClassDefStart(const ClassDefPtr&) final;
        void visitClassDefEnd(const ClassDefPtr&) final;
        bool visitExceptionStart(const ExceptionPtr&) final;
        void visitExceptionEnd(const ExceptionPtr&) final;
        bool visitStructStart(const StructPtr&) final;
        void visitStructEnd(const StructPtr&) final;
        void visitOperation(const OperationPtr&) final;
        void visitDataMember(const DataMemberPtr&) final;
        void visitSequence(const SequencePtr&) final;
        void visitDictionary(const DictionaryPtr&) final;
        void visitEnum(const EnumPtr&) final;
        void visitConst(const ConstPtr&) final;

    private:

        StringList validate(const SyntaxTreeBasePtr&, const StringList&, const std::string&, const std::string&,
                            bool = false);
    };

    static void validateMetaData(const UnitPtr&);
};

}

#endif
