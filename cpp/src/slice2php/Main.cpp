// Copyright (c) ZeroC, Inc.

#include "../Ice/ConsoleUtil.h"
#include "../Ice/Options.h"
#include "../Ice/OutputUtil.h"
#include "../Slice/FileTracker.h"
#include "../Slice/MetadataValidation.h"
#include "../Slice/Parser.h"
#include "../Slice/Preprocessor.h"
#include "../Slice/Util.h"
#include "Ice/CtrlCHandler.h"
#include "Ice/StringUtil.h"
#include "PHPUtil.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <mutex>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#    include <direct.h>
#else
#    include <unistd.h>
#endif

// TODO: fix this warning!
#if defined(_MSC_VER)
#    pragma warning(disable : 4456) // shadow
#    pragma warning(disable : 4457) // shadow
#    pragma warning(disable : 4459) // shadow
#elif defined(__clang__)
#    pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#    pragma GCC diagnostic ignored "-Wshadow"
#endif

using namespace std;
using namespace Slice;
using namespace Slice::PHP;
using namespace IceInternal;

namespace
{
    // Get the fully-qualified name of the given definition. If a suffix is provided, it is prepended to the
    // definition's unqualified name. If the nameSuffix is provided, it is appended to the container's name.
    string
    getAbsolute(const ContainedPtr& cont, const string& pfx = std::string(), const string& suffix = std::string())
    {
        return scopedToName(cont->scope() + pfx + cont->name() + suffix, true);
    }
}

// CodeVisitor generates the PHP mapping for a translation unit.
class CodeVisitor final : public ParserVisitor
{
public:
    CodeVisitor(IceInternal::Output&);

    void visitClassDecl(const ClassDeclPtr&) final;
    bool visitClassDefStart(const ClassDefPtr&) final;
    void visitInterfaceDecl(const InterfaceDeclPtr&) final;
    bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
    bool visitExceptionStart(const ExceptionPtr&) final;
    bool visitStructStart(const StructPtr&) final;
    void visitSequence(const SequencePtr&) final;
    void visitDictionary(const DictionaryPtr&) final;
    void visitEnum(const EnumPtr&) final;
    void visitConst(const ConstPtr&) final;

private:
    void startNamespace(const ContainedPtr&);
    void endNamespace();

    // Return the PHP name for the given Slice type. When using namespaces, this name is a relative (unqualified) name,
    // otherwise this name is the flattened absolute name.
    string getName(const ContainedPtr&, const string& = string());

    // Return the PHP variable for the given object's type.
    string getTypeVar(const ContainedPtr&, const string& = string());

    // Emit the array for a Slice type.
    void writeType(const TypePtr&);
    string getType(const TypePtr&);

    // Write a default value for a given type.
    void writeDefaultValue(const DataMemberPtr&);

    struct MemberInfo
    {
        string fixedName;
        bool inherited;
        DataMemberPtr dataMember;
    };
    using MemberInfoList = list<MemberInfo>;

    // Write a member assignment statement for a constructor.
    void writeAssign(const MemberInfo&);

    // Write constant value.
    void writeConstantValue(const TypePtr&, const SyntaxTreeBasePtr&, const string&);

    // Write constructor parameters with default values.
    void writeConstructorParams(const MemberInfoList&);

    // Convert an operation mode into a string.
    string getOperationMode(Slice::Operation::Mode);

    void collectClassMembers(const ClassDefPtr&, MemberInfoList&, bool);
    void collectExceptionMembers(const ExceptionPtr&, MemberInfoList&, bool);

    Output& _out;
    set<string> _classHistory;
};

// CodeVisitor implementation.
CodeVisitor::CodeVisitor(Output& out) : _out(out) {}

void
CodeVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    // Handle forward declarations.
    string scoped = p->scoped();
    if (_classHistory.count(scoped) == 0)
    {
        startNamespace(p);

        string type = getTypeVar(p);
        _out << sp << nl << "global " << type << ';';
        _out << nl << type << " = IcePHP_declareClass('" << scoped << "');";

        endNamespace();

        _classHistory.insert(scoped); // Avoid redundant declarations.
    }
}

void
CodeVisitor::visitInterfaceDecl(const InterfaceDeclPtr& p)
{
    // Handle forward declarations.
    string scoped = p->scoped();
    if (_classHistory.count(scoped) == 0)
    {
        startNamespace(p);

        string type = getTypeVar(p);
        _out << sp << nl << "global " << type << ';';

        _out << nl << "global " << type << "Prx;";
        _out << nl << type << "Prx = IcePHP_declareProxy('" << scoped << "');";

        endNamespace();

        _classHistory.insert(scoped); // Avoid redundant declarations.
    }
}

bool
CodeVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string scoped = p->scoped();
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);
    ClassDefPtr base = p->base();
    DataMemberList members = p->dataMembers();

    startNamespace(p);

    _out << sp << nl << "global " << type << ';';

    _out << nl;
    _out << "class " << name;
    if (base)
    {
        _out << " extends " << getAbsolute(base);
    }
    else
    {
        _out << " extends \\Ice\\Value";
    }

    _out << sb;

    // __construct
    _out << nl << "public function __construct(";
    MemberInfoList allMembers;
    collectClassMembers(p, allMembers, false);
    writeConstructorParams(allMembers);
    _out << ")";
    _out << sb;
    if (base)
    {
        _out << nl << "parent::__construct(";
        int count = 0;
        for (const auto& allMember : allMembers)
        {
            if (allMember.inherited)
            {
                if (count)
                {
                    _out << ", ";
                }
                _out << '$' << allMember.fixedName;
                ++count;
            }
        }
        _out << ");";
    }
    {
        for (const auto& allMember : allMembers)
        {
            if (!allMember.inherited)
            {
                writeAssign(allMember);
            }
        }
    }
    _out << eb;

    // ice_ice
    _out << sp << nl << "public function ice_id()";
    _out << sb;
    _out << nl << "return '" << scoped << "';";
    _out << eb;

    // ice_staticId
    _out << sp << nl << "public static function ice_staticId()";
    _out << sb;
    _out << nl << "return '" << scoped << "';";
    _out << eb;

    // __toString
    _out << sp << nl << "public function __toString(): string";

    _out << sb;
    _out << nl << "global " << type << ';';
    _out << nl << "return IcePHP_stringify($this, " << type << ");";
    _out << eb;

    if (!members.empty())
    {
        _out << sp;
        for (const auto& member : members)
        {
            _out << nl << "public " << "$" << fixIdent(member->name()) << ";";
        }
    }

    _out << eb; // End of class.

    {
        string type;
        vector<string> seenType;
        _out << sp << nl << "global ";
        if (!base)
        {
            type = "$Ice__t_Value";
        }
        else
        {
            type = getTypeVar(base);
        }
        _out << type << ";";
        seenType.push_back(type);

        for (const auto& member : members)
        {
            string type = getType(member->type());
            if (find(seenType.begin(), seenType.end(), type) == seenType.end())
            {
                seenType.push_back(type);
                _out << nl << "global " << type << ";";
            }
        }
    }

    // Emit the type information.
    _out << nl << type << " = IcePHP_defineClass('" << scoped << "', '" << escapeName(abs) << "', " << p->compactId()
         << ", false, ";
    if (!base)
    {
        _out << "$Ice__t_Value";
    }
    else
    {
        _out << getTypeVar(base);
    }
    _out << ", ";
    // Members
    //
    // Data members are represented as an array:
    //
    //   ('MemberName', MemberType, Optional, Tag)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a constructed type.
    if (!members.empty())
    {
        _out << "array(";
        for (auto q = members.begin(); q != members.end(); ++q)
        {
            if (q != members.begin())
            {
                _out << ',';
            }
            _out.inc();
            _out << nl << "array('" << fixIdent((*q)->name()) << "', ";
            writeType((*q)->type());
            _out << ", " << ((*q)->optional() ? "true" : "false") << ", " << ((*q)->optional() ? (*q)->tag() : 0)
                 << ')';
            _out.dec();
        }
        _out << ')';
    }
    else
    {
        _out << "null";
    }
    _out << ");";

    endNamespace();

    return false;
}

bool
CodeVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string scoped = p->scoped();
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);
    string prxName = getName(p, "Prx");
    string prxType = getTypeVar(p, "Prx");
    string prxAbs = getAbsolute(p, "", "Prx");
    InterfaceList bases = p->bases();
    OperationList ops = p->operations();
    startNamespace(p);

    _out << sp << nl << "global " << type << ';';
    _out << nl << "global " << prxType << ';';

    // Define the proxy class.
    _out << sp << nl << "class " << prxName << "Helper";
    _out << sb;

    _out << sp << nl << "public static function createProxy($communicator, $proxyString)";
    _out << sb;
    _out << nl << "return  $communicator->stringToProxy($proxyString, '" << scoped << "');";
    _out << eb;

    _out << sp << nl << "public static function checkedCast($proxy, ...$args)";
    _out << sb;
    _out << nl << "return $proxy->ice_checkedCast('" << scoped << "', ...$args);";
    _out << eb;

    _out << sp << nl << "public static function uncheckedCast($proxy, $facet=null)";
    _out << sb;
    _out << nl << "return $proxy->ice_uncheckedCast('" << scoped << "', $facet);";
    _out << eb;

    _out << sp << nl << "public static function ice_staticId()";
    _out << sb;
    _out << nl << "return '" << scoped << "';";
    _out << eb;

    _out << eb;

    _out << sp << nl << "global ";
    _out << "$Ice__t_ObjectPrx";
    _out << ";";
    _out << nl << prxType << " = IcePHP_defineProxy('" << scoped << "', ";
    _out << "$Ice__t_ObjectPrx";
    _out << ", ";

    // Interfaces
    if (!bases.empty())
    {
        _out << "array(";
        for (auto q = bases.begin(); q != bases.end(); ++q)
        {
            if (q != bases.begin())
            {
                _out << ", ";
            }
            _out << getTypeVar(*q, "Prx");
        }
        _out << ')';
    }
    else
    {
        _out << "null";
    }
    _out << ");";

    // Define each operation. The arguments to IcePHP_defineOperation are:
    //
    // $ClassType, 'opName', Mode, FormatType, (InParams), (OutParams), ReturnParam, (Exceptions)
    //
    // where InParams and OutParams are arrays of type descriptions, and Exceptions
    // is an array of exception type ids.

    if (!ops.empty())
    {
        _out << sp;
        vector<string> seenTypes;
        for (const auto& op : ops)
        {
            ParameterList params = op->parameters();
            for (const auto& param : params)
            {
                string type = getType(param->type());
                if (find(seenTypes.begin(), seenTypes.end(), type) == seenTypes.end())
                {
                    seenTypes.push_back(type);
                    _out << nl << "global " << type << ";";
                }
            }

            if (op->returnType())
            {
                string type = getType(op->returnType());
                if (find(seenTypes.begin(), seenTypes.end(), type) == seenTypes.end())
                {
                    seenTypes.push_back(type);
                    _out << nl << "global " << type << ";";
                }
            }
        }

        for (const auto& op : ops)
        {
            ParameterList params = op->parameters();
            ParameterList::iterator t;
            int count;

            // We encode nullopt as -1.
            optional<FormatType> opFormat = op->format();
            int phpFormat = -1;
            if (opFormat)
            {
                phpFormat = static_cast<int>(*opFormat);
            }

            _out << nl << "IcePHP_defineOperation(" << prxType << ", '" << op->name() << "', "
                 << getOperationMode(op->mode()) << ", " << phpFormat << ", ";
            for (t = params.begin(), count = 0; t != params.end(); ++t)
            {
                if (!(*t)->isOutParam())
                {
                    if (count == 0)
                    {
                        _out << "array(";
                    }
                    else if (count > 0)
                    {
                        _out << ", ";
                    }
                    _out << "array(";
                    writeType((*t)->type());
                    if ((*t)->optional())
                    {
                        _out << ", " << (*t)->tag();
                    }
                    _out << ')';
                    ++count;
                }
            }
            if (count > 0)
            {
                _out << ')';
            }
            else
            {
                _out << "null";
            }
            _out << ", ";
            for (t = params.begin(), count = 0; t != params.end(); ++t)
            {
                if ((*t)->isOutParam())
                {
                    if (count == 0)
                    {
                        _out << "array(";
                    }
                    else if (count > 0)
                    {
                        _out << ", ";
                    }
                    _out << "array(";
                    writeType((*t)->type());
                    if ((*t)->optional())
                    {
                        _out << ", " << (*t)->tag();
                    }
                    _out << ')';
                    ++count;
                }
            }
            if (count > 0)
            {
                _out << ')';
            }
            else
            {
                _out << "null";
            }
            _out << ", ";
            TypePtr returnType = op->returnType();
            if (returnType)
            {
                // The return type has the same format as an in/out parameter:
                //
                // Type, Optional?, OptionalTag
                _out << "array(";
                writeType(returnType);
                if (op->returnIsOptional())
                {
                    _out << ", " << op->returnTag();
                }
                _out << ')';
            }
            else
            {
                _out << "null";
            }
            _out << ", ";

            ExceptionList exceptions = op->throws();
            if (!exceptions.empty())
            {
                _out << "array(";
                for (auto u = exceptions.begin(); u != exceptions.end(); ++u)
                {
                    if (u != exceptions.begin())
                    {
                        _out << ", ";
                    }
                    _out << getTypeVar(*u);
                }
                _out << ')';
            }
            else
            {
                _out << "null";
            }
            _out << ");";
        }
    }

    endNamespace();

    return false;
}

bool
CodeVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    string scoped = p->scoped();
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);

    startNamespace(p);

    _out << sp << nl << "global " << type << ';';
    _out << nl << "class " << name << " extends ";
    ExceptionPtr base = p->base();
    string baseName;
    if (base)
    {
        baseName = getAbsolute(base);
        _out << baseName;
    }
    else
    {
        _out << "\\Ice\\UserException";
    }
    _out << sb;

    // ice_id
    _out << sp << nl << "public function ice_id()";
    _out << sb;
    _out << nl << "return '" << scoped << "';";
    _out << eb;

    // __toString
    _out << sp << nl << "public function __toString(): string";

    _out << sb;
    _out << nl << "global " << type << ';';
    _out << nl << "return IcePHP_stringifyException($this, " << type << ");";
    _out << eb;

    DataMemberList members = p->dataMembers();

    if (!members.empty())
    {
        _out << sp;
        for (const auto& member : members)
        {
            _out << nl << "public $" << fixIdent(member->name()) << ";";
        }
    }

    _out << eb;

    vector<string> seenType;
    for (const auto& member : members)
    {
        string type = getType(member->type());
        if (find(seenType.begin(), seenType.end(), type) == seenType.end())
        {
            seenType.push_back(type);
            _out << nl << "global " << type << ";";
        }
    }

    // Emit the type information.
    _out << sp << nl << type << " = IcePHP_defineException('" << scoped << "', '" << escapeName(abs) << "', ";
    if (!base)
    {
        _out << "null";
    }
    else
    {
        _out << getTypeVar(base);
    }
    _out << ", ";
    // Data members are represented as an array:
    //
    //   ('MemberName', MemberType, Optional, Tag)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a constructed type.
    if (!members.empty())
    {
        _out << "array(";
        for (auto dmli = members.begin(); dmli != members.end(); ++dmli)
        {
            if (dmli != members.begin())
            {
                _out << ',';
            }
            _out.inc();
            _out << nl << "array('" << fixIdent((*dmli)->name()) << "', ";
            writeType((*dmli)->type());
            _out << ", " << ((*dmli)->optional() ? "true" : "false") << ", "
                 << ((*dmli)->optional() ? (*dmli)->tag() : 0) << ')';
            _out.dec();
        }
        _out << ')';
    }
    else
    {
        _out << "null";
    }
    _out << ");";

    endNamespace();

    return false;
}

bool
CodeVisitor::visitStructStart(const StructPtr& p)
{
    string scoped = p->scoped();
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);
    MemberInfoList memberList;

    {
        DataMemberList members = p->dataMembers();
        for (const auto& member : members)
        {
            memberList.emplace_back();
            memberList.back().fixedName = fixIdent(member->name());
            memberList.back().inherited = false;
            memberList.back().dataMember = member;
        }
    }

    startNamespace(p);

    _out << sp << nl << "global " << type << ';';

    _out << nl << "class " << name;
    _out << sb;
    _out << nl << "public function __construct(";
    writeConstructorParams(memberList);
    _out << ")";
    _out << sb;
    for (const auto& r : memberList)
    {
        writeAssign(r);
    }
    _out << eb;

    // __toString
    _out << sp << nl << "public function __toString(): string";

    _out << sb;
    _out << nl << "global " << type << ';';
    _out << nl << "return IcePHP_stringify($this, " << type << ");";
    _out << eb;

    if (!memberList.empty())
    {
        _out << sp;
        for (const auto& r : memberList)
        {
            _out << nl << "public $" << r.fixedName << ';';
        }
    }

    _out << eb;

    _out << sp;
    vector<string> seenType;
    for (const auto& r : memberList)
    {
        string type = getType(r.dataMember->type());
        if (find(seenType.begin(), seenType.end(), type) == seenType.end())
        {
            seenType.push_back(type);
            _out << nl << "global " << type << ";";
        }
    }

    // Emit the type information.
    _out << nl << type << " = IcePHP_defineStruct('" << scoped << "', '" << escapeName(abs) << "', array(";

    // Data members are represented as an array:
    //
    //   ('MemberName', MemberType)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a constructed type.
    for (auto r = memberList.begin(); r != memberList.end(); ++r)
    {
        if (r != memberList.begin())
        {
            _out << ",";
        }
        _out.inc();
        _out << nl << "array('" << r->fixedName << "', ";
        writeType(r->dataMember->type());
        _out << ')';
        _out.dec();
    }
    _out << "));";

    endNamespace();

    return false;
}

void
CodeVisitor::visitSequence(const SequencePtr& p)
{
    string type = getTypeVar(p);
    TypePtr content = p->type();

    startNamespace(p);

    // Emit the type information.
    string scoped = p->scoped();
    _out << sp << nl << "global " << type << ';';
    _out << sp << nl << "if(!isset(" << type << "))";
    _out << sb;
    _out << nl << "global " << getType(content) << ";";
    _out << nl << type << " = IcePHP_defineSequence('" << scoped << "', ";
    writeType(content);
    _out << ");";
    _out << eb;

    endNamespace();
}

void
CodeVisitor::visitDictionary(const DictionaryPtr& p)
{
    TypePtr keyType = p->keyType();
    BuiltinPtr b = dynamic_pointer_cast<Builtin>(keyType);
    if (b)
    {
        switch (b->kind())
        {
            // These types are acceptable as dictionary keys.
            case Slice::Builtin::KindBool:
            case Slice::Builtin::KindByte:
            case Slice::Builtin::KindShort:
            case Slice::Builtin::KindInt:
            case Slice::Builtin::KindLong:
            case Slice::Builtin::KindString:
                break;

            // These types have already been rejected as illegal key types by the parser.
            case Slice::Builtin::KindFloat:
            case Slice::Builtin::KindDouble:
            case Slice::Builtin::KindObject:
            case Slice::Builtin::KindObjectProxy:
            case Slice::Builtin::KindValue:
                assert(false);
        }
    }
    else if (!dynamic_pointer_cast<Enum>(keyType))
    {
        // See https://github.com/zeroc-ice/ice/issues/254
        p->unit()->warning(p->file(), p->line(), All, "dictionary key type not supported in PHP");
    }

    string type = getTypeVar(p);

    startNamespace(p);

    // Emit the type information.
    string scoped = p->scoped();
    _out << sp << nl << "global " << type << ';';
    _out << sp << nl << "if(!isset(" << type << "))";
    _out << sb;
    _out << nl << "global " << getType(p->keyType()) << ";";
    _out << nl << "global " << getType(p->valueType()) << ";";
    _out << nl << type << " = IcePHP_defineDictionary('" << scoped << "', ";
    writeType(p->keyType());
    _out << ", ";
    writeType(p->valueType());
    _out << ");";
    _out << eb;

    endNamespace();
}

void
CodeVisitor::visitEnum(const EnumPtr& p)
{
    string scoped = p->scoped();
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);
    EnumeratorList enumerators = p->enumerators();

    startNamespace(p);

    _out << sp << nl << "global " << type << ';';
    _out << nl << "class " << name;
    _out << sb;

    for (const auto& enumerator : enumerators)
    {
        _out << nl << "const " << fixIdent(enumerator->name()) << " = " << enumerator->value() << ';';
    }

    _out << eb;

    // Emit the type information.
    _out << sp << nl << type << " = IcePHP_defineEnum('" << scoped << "', array(";
    for (auto q = enumerators.begin(); q != enumerators.end(); ++q)
    {
        if (q != enumerators.begin())
        {
            _out << ", ";
        }
        _out << "'" << (*q)->name() << "', " << (*q)->value();
    }
    _out << "));";

    endNamespace();
}

void
CodeVisitor::visitConst(const ConstPtr& p)
{
    string name = getName(p);
    string type = getTypeVar(p);
    string abs = getAbsolute(p);

    startNamespace(p);

    _out << sp << nl << "if(!defined('" << escapeName(abs) << "'))";
    _out << sb;
    _out << sp << nl << "define(__NAMESPACE__ . '\\\\" << name << "', ";
    writeConstantValue(p->type(), p->valueType(), p->value());

    _out << ");";
    _out << eb;

    endNamespace();
}

void
CodeVisitor::startNamespace(const ContainedPtr& cont)
{
    string scope = cont->scope();
    scope = scope.substr(2);                     // Removing leading '::'
    scope = scope.substr(0, scope.length() - 2); // Removing trailing '::'
    _out << sp << nl << "namespace " << scopedToName(scope, true);
    _out << sb;
}

void
CodeVisitor::endNamespace()
{
    _out << eb;
}

string
CodeVisitor::getTypeVar(const ContainedPtr& p, const string& suffix)
{
    return "$" + scopedToName(p->scope() + "_t_" + p->name() + suffix, false);
}

string
CodeVisitor::getName(const ContainedPtr& p, const string& suffix)
{
    return fixIdent(p->name() + suffix);
}

void
CodeVisitor::writeType(const TypePtr& p)
{
    _out << getType(p);
}

string
CodeVisitor::getType(const TypePtr& p)
{
    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(p);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindBool:
            {
                return "$IcePHP__t_bool";
            }
            case Builtin::KindByte:
            {
                return "$IcePHP__t_byte";
            }
            case Builtin::KindShort:
            {
                return "$IcePHP__t_short";
            }
            case Builtin::KindInt:
            {
                return "$IcePHP__t_int";
            }
            case Builtin::KindLong:
            {
                return "$IcePHP__t_long";
            }
            case Builtin::KindFloat:
            {
                return "$IcePHP__t_float";
            }
            case Builtin::KindDouble:
            {
                return "$IcePHP__t_double";
            }
            case Builtin::KindString:
            {
                return "$IcePHP__t_string";
            }
            case Builtin::KindObject:
            case Builtin::KindValue:
            {
                return "$Ice__t_Value";
            }
            case Builtin::KindObjectProxy:
            {
                return "$Ice__t_ObjectPrx";
            }
        }
    }

    InterfaceDeclPtr prx = dynamic_pointer_cast<InterfaceDecl>(p);
    if (prx)
    {
        return getTypeVar(prx, "Prx");
    }

    ContainedPtr cont = dynamic_pointer_cast<Contained>(p);
    assert(cont);
    return getTypeVar(cont);
}

void
CodeVisitor::writeDefaultValue(const DataMemberPtr& m)
{
    TypePtr p = m->type();
    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(p);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindBool:
            {
                _out << "false";
                break;
            }
            case Builtin::KindByte:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            {
                _out << "0";
                break;
            }
            case Builtin::KindFloat:
            case Builtin::KindDouble:
            {
                _out << "0.0";
                break;
            }
            case Builtin::KindString:
            {
                _out << "''";
                break;
            }
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            case Builtin::KindValue:
            {
                _out << "null";
                break;
            }
        }
        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(p);
    if (en)
    {
        string firstEnumerator = en->enumerators().front()->name();
        _out << getAbsolute(en) << "::" << fixIdent(firstEnumerator);
        return;
    }

    // PHP does not allow the following construct:
    //
    // function foo($theStruct=new MyStructType)
    //
    // Instead we use null as the default value and allocate an instance in the constructor.
    if (dynamic_pointer_cast<Struct>(p))
    {
        _out << "null";
        return;
    }

    _out << "null";
}

void
CodeVisitor::writeAssign(const MemberInfo& info)
{
    StructPtr st = dynamic_pointer_cast<Struct>(info.dataMember->type());
    if (st)
    {
        _out << nl << "$this->" << info.fixedName << " = is_null($" << info.fixedName << ") ? new " << getAbsolute(st)
             << " : $" << info.fixedName << ';';
    }
    else
    {
        _out << nl << "$this->" << info.fixedName << " = $" << info.fixedName << ';';
    }
}

void
CodeVisitor::writeConstantValue(const TypePtr& type, const SyntaxTreeBasePtr& valueType, const string& value)
{
    ConstPtr constant = dynamic_pointer_cast<Const>(valueType);
    if (constant)
    {
        _out << getAbsolute(constant);
    }
    else
    {
        Slice::BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
        Slice::EnumPtr en = dynamic_pointer_cast<Enum>(type);
        if (b)
        {
            switch (b->kind())
            {
                case Slice::Builtin::KindBool:
                case Slice::Builtin::KindByte:
                case Slice::Builtin::KindShort:
                case Slice::Builtin::KindInt:
                case Slice::Builtin::KindFloat:
                case Slice::Builtin::KindDouble:
                {
                    _out << value;
                    break;
                }
                case Slice::Builtin::KindLong:
                {
                    int64_t l = std::stoll(value, nullptr, 0); // NOLINT(clang-analyzer-deadcode.DeadStores)
                    // The platform's 'long' type may not be 64 bits, so we store 64-bit values as a string.
                    if (sizeof(int64_t) > sizeof(long) && (l < INT32_MIN || l > INT32_MAX))
                    {
                        _out << "'" << value << "'";
                    }
                    else
                    {
                        _out << value;
                    }
                    break;
                }
                case Slice::Builtin::KindString:
                {
                    // PHP 7.x also supports an EC6UCN-like notation, see: https://wiki.php.net/rfc/unicode_escape
                    _out << "\"" << toStringLiteral(value, "\f\n\r\t\v\x1b", "$", Octal, 0) << "\"";
                    break;
                }
                case Slice::Builtin::KindObject:
                case Slice::Builtin::KindObjectProxy:
                case Slice::Builtin::KindValue:
                    assert(false);
            }
        }
        else if (en)
        {
            EnumeratorPtr lte = dynamic_pointer_cast<Enumerator>(valueType);
            assert(lte);
            _out << getAbsolute(en) << "::" << fixIdent(lte->name());
        }
        else
        {
            assert(false); // Unknown const type.
        }
    }
}

void
CodeVisitor::writeConstructorParams(const MemberInfoList& members)
{
    for (auto p = members.begin(); p != members.end(); ++p)
    {
        if (p != members.begin())
        {
            _out << ", ";
        }
        _out << '$' << p->fixedName << "=";

        const DataMemberPtr member = p->dataMember;
        if (member->defaultValueType())
        {
            writeConstantValue(member->type(), member->defaultValueType(), member->defaultValue());
        }
        else if (member->optional())
        {
            _out << "\\Ice\\None";
        }
        else
        {
            writeDefaultValue(member);
        }
    }
}

string
CodeVisitor::getOperationMode(Slice::Operation::Mode mode)
{
    ostringstream ostr;
    ostr << static_cast<int>(mode);
    return ostr.str();
}

void
CodeVisitor::collectClassMembers(const ClassDefPtr& p, MemberInfoList& allMembers, bool inherited)
{
    ClassDefPtr base = p->base();
    if (base)
    {
        collectClassMembers(base, allMembers, true);
    }

    DataMemberList members = p->dataMembers();

    for (const auto& member : members)
    {
        MemberInfo m;
        m.fixedName = fixIdent(member->name());
        m.inherited = inherited;
        m.dataMember = member;
        allMembers.push_back(m);
    }
}

void
CodeVisitor::collectExceptionMembers(const ExceptionPtr& p, MemberInfoList& allMembers, bool inherited)
{
    ExceptionPtr base = p->base();
    if (base)
    {
        collectExceptionMembers(base, allMembers, true);
    }

    DataMemberList members = p->dataMembers();

    for (const auto& member : members)
    {
        MemberInfo m;
        m.fixedName = fixIdent(member->name());
        m.inherited = inherited;
        m.dataMember = member;
        allMembers.push_back(m);
    }
}

static void
generate(const UnitPtr& un, bool all, const vector<string>& includePaths, Output& out)
{
    if (!all)
    {
        vector<string> paths = includePaths;
        for (auto& path : paths)
        {
            path = fullPath(path);
        }

        StringList includes = un->includeFiles();
        if (!includes.empty())
        {
            out << sp;
            out << nl << "namespace";
            out << sb;
            for (const auto& include : includes)
            {
                string file = changeInclude(include, paths);
                out << nl << "require_once '" << file << ".php';";
            }
            out << eb;
        }
    }

    // 'slice2php' doesn't have any language-specific metadata, so we call `validateMetadata` with an empty list.
    // This ensures that the validation still runs, and will reject any 'php' metadata the user might think exists.
    Slice::validateMetadata(un, "php", {});

    CodeVisitor codeVisitor(out);
    un->visit(&codeVisitor);

    out << nl; // Trailing newline.
}

static void
printHeader(IceInternal::Output& out)
{
    out << "// Copyright (c) ZeroC, Inc.\n";
    out << "//\n";
    out << "// Ice version " << ICE_STRING_VERSION << "\n";
    out << "//\n";
}

namespace
{
    mutex globalMutex;
    bool interrupted = false;
}

static void
interruptedCallback(int /*signal*/)
{
    lock_guard lock(globalMutex);
    interrupted = true;
}

static void
usage(const string& n)
{
    consoleErr << "Usage: " << n << " [options] slice-files...\n";
    consoleErr << "Options:\n"
                  "-h, --help               Show this message.\n"
                  "-v, --version            Display the Ice version.\n"
                  "-DNAME                   Define NAME as 1.\n"
                  "-DNAME=DEF               Define NAME as DEF.\n"
                  "-UNAME                   Remove any definition for NAME.\n"
                  "-IDIR                    Put DIR in the include file search path.\n"
                  "-E                       Print preprocessor output on stdout.\n"
                  "--output-dir DIR         Create files in the directory DIR.\n"
                  "-d, --debug              Print debug messages.\n"
                  "--depend                 Generate Makefile dependencies.\n"
                  "--depend-xml             Generate dependencies in XML format.\n"
                  "--depend-file FILE       Write dependencies to FILE instead of standard output.\n"
                  "--validate               Validate command line options.\n"
                  "--all                    Generate code for Slice definitions in included files.\n";
}

int
compile(const vector<string>& argv)
{
    IceInternal::Options opts;
    opts.addOpt("h", "help");
    opts.addOpt("v", "version");
    opts.addOpt("", "validate");
    opts.addOpt("D", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("U", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("I", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("E");
    opts.addOpt("", "output-dir", IceInternal::Options::NeedArg);
    opts.addOpt("", "depend");
    opts.addOpt("", "depend-xml");
    opts.addOpt("", "depend-file", IceInternal::Options::NeedArg, "");
    opts.addOpt("d", "debug");
    opts.addOpt("", "all");

    bool validate = find(argv.begin(), argv.end(), "--validate") != argv.end();

    vector<string> args;
    try
    {
        args = opts.parse(argv);
    }
    catch (const IceInternal::BadOptException& e)
    {
        consoleErr << argv[0] << ": error: " << e.what() << endl;
        if (!validate)
        {
            usage(argv[0]);
        }
        return EXIT_FAILURE;
    }

    if (opts.isSet("help"))
    {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (opts.isSet("version"))
    {
        consoleErr << ICE_STRING_VERSION << endl;
        return EXIT_SUCCESS;
    }

    vector<string> cppArgs;
    vector<string> optargs = opts.argVec("D");
    cppArgs.reserve(optargs.size()); // keep clang-tidy happy
    for (const auto& optarg : optargs)
    {
        cppArgs.push_back("-D" + optarg);
    }

    optargs = opts.argVec("U");
    for (const auto& optarg : optargs)
    {
        cppArgs.push_back("-U" + optarg);
    }

    vector<string> includePaths = opts.argVec("I");
    for (const auto& includePath : includePaths)
    {
        cppArgs.push_back("-I" + Preprocessor::normalizeIncludePath(includePath));
    }

    bool preprocess = opts.isSet("E");

    string output = opts.optArg("output-dir");

    bool depend = opts.isSet("depend");

    bool dependxml = opts.isSet("depend-xml");

    string dependFile = opts.optArg("depend-file");

    bool debug = opts.isSet("debug");

    bool all = opts.isSet("all");

    if (args.empty())
    {
        consoleErr << argv[0] << ": error: no input file" << endl;
        if (!validate)
        {
            usage(argv[0]);
        }
        return EXIT_FAILURE;
    }

    if (depend && dependxml)
    {
        consoleErr << argv[0] << ": error: cannot specify both --depend and --depend-xml" << endl;
        if (!validate)
        {
            usage(argv[0]);
        }
        return EXIT_FAILURE;
    }

    if (validate)
    {
        return EXIT_SUCCESS;
    }

    int status = EXIT_SUCCESS;

    Ice::CtrlCHandler ctrlCHandler;
    ctrlCHandler.setCallback(interruptedCallback);

    ostringstream os;
    if (dependxml)
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<dependencies>" << endl;
    }

    for (auto i = args.begin(); i != args.end(); ++i)
    {
        // Ignore duplicates.
        auto p = find(args.begin(), args.end(), *i);
        if (p != i)
        {
            continue;
        }

        if (depend || dependxml)
        {
            PreprocessorPtr icecpp = Preprocessor::create(argv[0], *i, cppArgs);
            FILE* cppHandle = icecpp->preprocess(false, "-D__SLICE2PHP__");

            if (cppHandle == nullptr)
            {
                return EXIT_FAILURE;
            }

            UnitPtr u = Unit::createUnit("php", false);
            int parseStatus = u->parse(*i, cppHandle, debug);
            u->destroy();

            if (parseStatus == EXIT_FAILURE)
            {
                return EXIT_FAILURE;
            }

            if (!icecpp->printMakefileDependencies(
                    os,
                    depend ? Preprocessor::PHP : Preprocessor::SliceXML,
                    includePaths,
                    "-D__SLICE2PHP__"))
            {
                return EXIT_FAILURE;
            }

            if (!icecpp->close())
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            PreprocessorPtr icecpp = Preprocessor::create(argv[0], *i, cppArgs);
            FILE* cppHandle = icecpp->preprocess(false, "-D__SLICE2PHP__");

            if (cppHandle == nullptr)
            {
                return EXIT_FAILURE;
            }

            if (preprocess)
            {
                char buf[4096];
                while (fgets(buf, static_cast<int>(sizeof(buf)), cppHandle) != nullptr)
                {
                    if (fputs(buf, stdout) == EOF)
                    {
                        return EXIT_FAILURE;
                    }
                }
                if (!icecpp->close())
                {
                    return EXIT_FAILURE;
                }
            }
            else
            {
                UnitPtr u = Unit::createUnit("php", all);
                int parseStatus = u->parse(*i, cppHandle, debug);

                if (!icecpp->close())
                {
                    u->destroy();
                    return EXIT_FAILURE;
                }

                if (parseStatus == EXIT_FAILURE)
                {
                    status = EXIT_FAILURE;
                }
                else
                {
                    string base = icecpp->getBaseName();
                    string::size_type pos = base.find_last_of("/\\");
                    if (pos != string::npos)
                    {
                        base.erase(0, pos + 1);
                    }

                    string file = base + ".php";
                    if (!output.empty())
                    {
                        file = output + '/' + file;
                    }

                    try
                    {
                        IceInternal::Output out;
                        out.open(file.c_str());
                        if (!out)
                        {
                            ostringstream os;
                            os << "cannot open '" << file << "': " << IceInternal::errorToString(errno);
                            throw FileException(__FILE__, __LINE__, os.str());
                        }
                        FileTracker::instance()->addFile(file);

                        out << "<?php\n";
                        printHeader(out);
                        printGeneratedHeader(out, base + ".ice");

                        // Generate the PHP mapping.
                        generate(u, all, includePaths, out);

                        out << "?>\n";
                        out.close();
                    }
                    catch (const Slice::FileException& ex)
                    {
                        // If a file could not be created, then cleanup any  created files.
                        FileTracker::instance()->cleanup();
                        u->destroy();
                        consoleErr << argv[0] << ": error: " << ex.what() << endl;
                        return EXIT_FAILURE;
                    }
                    catch (const exception& ex)
                    {
                        FileTracker::instance()->cleanup();
                        consoleErr << argv[0] << ": error: " << ex.what() << endl;
                        status = EXIT_FAILURE;
                    }
                }

                u->destroy();
            }
        }

        {
            lock_guard lock(globalMutex);
            if (interrupted)
            {
                FileTracker::instance()->cleanup();
                return EXIT_FAILURE;
            }
        }
    }

    if (dependxml)
    {
        os << "</dependencies>\n";
    }

    if (depend || dependxml)
    {
        writeDependencies(os.str(), dependFile);
    }

    return status;
}

#ifdef _WIN32
int
wmain(int argc, wchar_t* argv[])
#else
int
main(int argc, char* argv[])
#endif
{
    vector<string> args = Slice::argvToArgs(argc, argv);
    try
    {
        return compile(args);
    }
    catch (const std::exception& ex)
    {
        consoleErr << args[0] << ": error:" << ex.what() << endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        consoleErr << args[0] << ": error:"
                   << "unknown exception" << endl;
        return EXIT_FAILURE;
    }
}
