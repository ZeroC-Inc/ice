// Copyright (c) ZeroC, Inc.

#include "Gen.h"
#include "../Ice/FileUtil.h"
#include "Ice/StringUtil.h"

#include <limits>
#ifndef _WIN32
#    include <unistd.h>
#else
#    include <direct.h>
#endif

#include "../Slice/FileTracker.h"
#include "../Slice/Util.h"
#include "Ice/UUID.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>

using namespace std;
using namespace Slice;
using namespace Ice;
using namespace IceInternal;

namespace
{
    void writeDocLines(Output& out, const StringList& lines)
    {
        for (const auto& line : lines)
        {
            out << nl << "///";
            if (!line.empty())
            {
                out << " " << line;
            }
        }
    }

    /// Returns a C# formatted link to the provided Slice identifier.
    /// TODO: this is temporary and will be replaced when we add 'cs:identifier' support.
    string csLinkFormatter(const string& rawLink, const ContainedPtr&, const SyntaxTreeBasePtr&)
    {
        string result = "<see cref=\"";

        auto hashPos = rawLink.find('#');
        if (hashPos != string::npos)
        {
            if (hashPos != 0)
            {
                result += rawLink.substr(0, hashPos);
                result += ".";
            }
            result += rawLink.substr(hashPos + 1);
        }
        else
        {
            result += rawLink;
        }
        return result + "\" />";
    }

    // TODO: this function should probably use the link formatter.
    string toCsIdent(const string& s)
    {
        string::size_type pos = s.find('#');
        if (pos == string::npos)
        {
            return s;
        }

        string result = s;
        if (pos == 0)
        {
            return result.erase(0, 1);
        }

        result[pos] = '.';
        return result;
    }

    void writeSeeAlso(Output& out, const StringList& lines)
    {
        for (const auto& line : lines)
        {
            if (!line.empty())
            {
                out << nl << "/// <seealso cref=\"" << toCsIdent(line) << "\" />";
            }
        }
    }

    string sliceModeToIceMode(Operation::Mode opMode)
    {
        string mode;
        switch (opMode)
        {
            case Operation::Normal:
            {
                mode = "Ice.OperationMode.Normal";
                break;
            }
            case Operation::Idempotent:
            {
                mode = "Ice.OperationMode.Idempotent";
                break;
            }
            default:
            {
                assert(false);
                break;
            }
        }
        return mode;
    }

    string opFormatTypeToString(const OperationPtr& op)
    {
        optional<FormatType> opFormat = op->format();
        if (opFormat)
        {
            switch (*opFormat)
            {
                case CompactFormat:
                    return "Ice.FormatType.CompactFormat";
                case SlicedFormat:
                    return "Ice.FormatType.SlicedFormat";
                default:
                    assert(false);
                    return "???";
            }
        }
        else
        {
            return "null";
        }
    }

    void emitObsoleteAttribute(const ContainedPtr& p1, Output& out)
    {
        if (p1->isDeprecated())
        {
            if (auto reason = p1->getDeprecationReason())
            {
                out << nl << "[global::System.Obsolete(\"" << *reason << "\")]";
            }
            else
            {
                out << nl << "[global::System.Obsolete]";
            }
        }
    }

    string getEscapedParamName(const OperationPtr& p, const string& name)
    {
        for (const auto& param : p->parameters())
        {
            if (param->mappedName() == name)
            {
                return name + "_";
            }
        }
        return name;
    }

    string resultStructReturnValueName(const ParameterList& outParams)
    {
        for (const auto& outParam : outParams)
        {
            if (outParam->mappedName() == "returnValue")
            {
                return "returnValue_";
            }
        }
        return "returnValue";
    }
}

Slice::CsVisitor::CsVisitor(Output& out) : _out(out) {}

Slice::CsVisitor::~CsVisitor() = default;

void
Slice::CsVisitor::writeMarshalUnmarshalParams(
    const ParameterList& params,
    const OperationPtr& op,
    bool marshal,
    const string& ns,
    bool resultStruct,
    bool publicNames,
    const string& customStream)
{
    ParameterList optionals;

    string paramPrefix = "";
    string returnValueS = "ret";

    if (op && resultStruct)
    {
        if ((op->returnType() && !params.empty()) || params.size() > 1)
        {
            paramPrefix = "ret.";
            returnValueS = resultStructReturnValueName(params);
        }
    }

    for (const auto& pli : params)
    {
        string param = pli->mappedName();
        if (paramPrefix.empty() && !publicNames)
        {
            param = "iceP_" + removeEscapePrefix(param);
        }
        TypePtr type = pli->type();
        if (!marshal && type->isClassType())
        {
            ostringstream os;
            os << '(' << typeToString(type, ns) << " v) => { " << paramPrefix << param << " = v; }";
            param = os.str();
        }
        else
        {
            param = paramPrefix + param;
        }

        if (pli->optional())
        {
            optionals.push_back(pli);
        }
        else
        {
            writeMarshalUnmarshalCode(_out, type, ns, param, marshal, customStream);
        }
    }

    TypePtr ret;

    if (op && op->returnType())
    {
        ret = op->returnType();
        string param;
        if (!marshal && ret->isClassType())
        {
            ostringstream os;
            os << '(' << typeToString(ret, ns) << " v) => {" << paramPrefix << returnValueS << " = v; }";
            param = os.str();
        }
        else
        {
            param = paramPrefix + returnValueS;
        }

        if (!op->returnIsOptional())
        {
            writeMarshalUnmarshalCode(_out, ret, ns, param, marshal, customStream);
        }
    }

    //
    // Sort optional parameters by tag.
    //
    class SortFn
    {
    public:
        static bool compare(const ParameterPtr& lhs, const ParameterPtr& rhs) { return lhs->tag() < rhs->tag(); }
    };
    optionals.sort(SortFn::compare);

    //
    // Handle optional parameters.
    //
    bool checkReturnType = op && op->returnIsOptional();
    for (const auto& optional : optionals)
    {
        if (checkReturnType && op->returnTag() < optional->tag())
        {
            string param;
            if (!marshal && ret->isClassType())
            {
                ostringstream os;
                os << '(' << typeToString(ret, ns) << " v) => {" << paramPrefix << returnValueS << " = v; }";
                param = os.str();
            }
            else
            {
                param = paramPrefix + returnValueS;
            }
            writeOptionalMarshalUnmarshalCode(_out, ret, ns, param, op->returnTag(), marshal, customStream);
            checkReturnType = false;
        }

        string param = optional->mappedName();
        if (paramPrefix.empty() && !publicNames)
        {
            param = "iceP_" + removeEscapePrefix(param);
        }
        TypePtr type = optional->type();
        if (!marshal && type->isClassType())
        {
            ostringstream os;
            os << '(' << typeToString(type, ns) << " v) => {" << paramPrefix << param << " = v; }";
            param = os.str();
        }
        else
        {
            param = paramPrefix + param;
        }

        writeOptionalMarshalUnmarshalCode(_out, type, ns, param, optional->tag(), marshal, customStream);
    }

    if (checkReturnType)
    {
        string param;
        if (!marshal && ret->isClassType())
        {
            ostringstream os;
            os << '(' << typeToString(ret, ns) << " v) => {" << paramPrefix << returnValueS << " = v; }";
            param = os.str();
        }
        else
        {
            param = paramPrefix + returnValueS;
        }
        writeOptionalMarshalUnmarshalCode(_out, ret, ns, param, op->returnTag(), marshal, customStream);
    }
}

void
Slice::CsVisitor::writeMarshalDataMember(
    const DataMemberPtr& member,
    const string& name,
    const string& ns,
    bool forStruct)
{
    if (member->optional())
    {
        assert(!forStruct);
        writeOptionalMarshalUnmarshalCode(_out, member->type(), ns, name, member->tag(), true, "ostr_");
    }
    else
    {
        string stream = forStruct ? "" : "ostr_";
        string memberName = name;
        if (forStruct)
        {
            memberName = "this." + memberName;
        }

        writeMarshalUnmarshalCode(_out, member->type(), ns, memberName, true, stream);
    }
}

void
Slice::CsVisitor::writeUnmarshalDataMember(
    const DataMemberPtr& member,
    const string& name,
    const string& ns,
    bool forStruct)
{
    string param = name;
    if (member->type()->isClassType())
    {
        ostringstream os;
        os << '(' << typeToString(member->type(), ns) << " v) => { this." << name << " = v; }";
        param = os.str();
    }
    else if (forStruct)
    {
        param = "this." + name;
    }

    if (member->optional())
    {
        assert(!forStruct);
        writeOptionalMarshalUnmarshalCode(_out, member->type(), ns, param, member->tag(), false, "istr_");
    }
    else
    {
        writeMarshalUnmarshalCode(_out, member->type(), ns, param, false, forStruct ? "" : "istr_");
    }
}

void
Slice::CsVisitor::writeMarshaling(const ClassDefPtr& p)
{
    string ns = getNamespace(p);
    ClassDefPtr base = p->base();

    //
    // Marshaling support
    //
    DataMemberList members = p->dataMembers();
    DataMemberList optionalMembers = p->orderedOptionalDataMembers();

    _out << sp;
    _out << nl << "protected override void iceWriteImpl(Ice.OutputStream ostr_)";
    _out << sb;
    _out << nl << "ostr_.startSlice(ice_staticId(), " << p->compactId() << (!base ? ", true" : ", false") << ");";
    for (const auto& member : members)
    {
        if (!member->optional())
        {
            writeMarshalDataMember(member, member->mappedName(), ns);
        }
    }
    for (const auto& optionalMember : optionalMembers)
    {
        writeMarshalDataMember(optionalMember, optionalMember->mappedName(), ns);
    }
    _out << nl << "ostr_.endSlice();";
    if (base)
    {
        _out << nl << "base.iceWriteImpl(ostr_);";
    }
    _out << eb;

    _out << sp;
    _out << nl << "protected override void iceReadImpl(Ice.InputStream istr_)";
    _out << sb;
    _out << nl << "istr_.startSlice();";
    for (const auto& member : members)
    {
        if (!member->optional())
        {
            writeUnmarshalDataMember(member, member->mappedName(), ns);
        }
    }
    for (const auto& optionalMember : optionalMembers)
    {
        writeUnmarshalDataMember(optionalMember, optionalMember->mappedName(), ns);
    }
    _out << nl << "istr_.endSlice();";
    if (base)
    {
        _out << nl << "base.iceReadImpl(istr_);";
    }
    _out << eb;
}

string
Slice::CsVisitor::getParamAttributes(const ParameterPtr& p)
{
    ostringstream ostr;
    for (const auto& metadata : p->getMetadata())
    {
        if (metadata->directive() == "cs:attribute")
        {
            ostr << "[" << metadata->arguments() << "] ";
        }
    }
    return ostr.str();
}

vector<string>
Slice::CsVisitor::getParams(const OperationPtr& op, const string& ns)
{
    vector<string> params;
    ParameterList paramList = op->parameters();
    InterfaceDefPtr interface = op->interface();
    for (const auto& q : paramList)
    {
        string param = getParamAttributes(q);
        if (q->isOutParam())
        {
            param += "out ";
        }
        param += typeToString(q->type(), ns, q->optional()) + " " + q->mappedName();
        params.push_back(param);
    }
    return params;
}

vector<string>
Slice::CsVisitor::getInParams(const OperationPtr& op, const string& ns, bool internal)
{
    vector<string> params;
    for (const auto& q : op->inParameters())
    {
        string param = (internal ? ("iceP_" + removeEscapePrefix(q->mappedName())) : q->mappedName());
        params.push_back(getParamAttributes(q) + typeToString(q->type(), ns, q->optional()) + " " + param);
    }
    return params;
}

vector<string>
Slice::CsVisitor::getOutParams(const OperationPtr& op, const string& ns, bool returnParam, bool outKeyword)
{
    vector<string> params;
    if (returnParam)
    {
        TypePtr ret = op->returnType();
        if (ret)
        {
            params.push_back(typeToString(ret, ns, op->returnIsOptional()) + " ret");
        }
    }

    ParameterList paramList = op->outParameters();
    for (const auto& q : paramList)
    {
        string s = getParamAttributes(q);
        if (outKeyword)
        {
            s += "out ";
        }
        s += typeToString(q->type(), ns, q->optional()) + ' ' + q->mappedName();
        params.push_back(s);
    }

    return params;
}

vector<string>
Slice::CsVisitor::getArgs(const OperationPtr& op)
{
    vector<string> args;
    ParameterList paramList = op->parameters();
    for (const auto& q : paramList)
    {
        string arg = q->mappedName();
        if (q->isOutParam())
        {
            arg = "out " + arg;
        }
        args.push_back(arg);
    }
    return args;
}

vector<string>
Slice::CsVisitor::getInArgs(const OperationPtr& op, bool internal)
{
    vector<string> args;
    ParameterList paramList = op->parameters();
    for (const auto& q : paramList)
    {
        if (!q->isOutParam())
        {
            string param = (internal ? ("iceP_" + removeEscapePrefix(q->mappedName())) : q->mappedName());
            args.push_back(param);
        }
    }
    return args;
}

string
Slice::CsVisitor::getDispatchParams(
    const OperationPtr& op,
    string& retS,
    vector<string>& params,
    vector<string>& args,
    const string& ns)
{
    string name = op->mappedName();
    InterfaceDefPtr interface = op->interface();
    ParameterList parameterss;

    if (interface->hasMetadata("amd") || op->hasMetadata("amd"))
    {
        name += "Async";
        params = getInParams(op, ns);
        args = getInArgs(op);
        parameterss = op->inParameters();
        retS = taskResultType(op, ns, true);
    }
    else if (op->hasMarshaledResult())
    {
        params = getInParams(op, ns);
        args = getInArgs(op);
        parameterss = op->inParameters();
        retS = resultType(op, ns, true);
    }
    else
    {
        params = getParams(op, ns);
        args = getArgs(op);
        parameterss = op->parameters();
        retS = typeToString(op->returnType(), ns, op->returnIsOptional());
    }

    string currentParamName = getEscapedParamName(op, "current");
    params.push_back("Ice.Current " + currentParamName);
    args.push_back(currentParamName);
    return name;
}

void
Slice::CsVisitor::emitAttributes(const ContainedPtr& p)
{
    for (const auto& metadata : p->getMetadata())
    {
        if (metadata->directive() == "cs:attribute")
        {
            _out << nl << '[' << metadata->arguments() << ']';
        }
    }
}

void
Slice::CsVisitor::emitNonBrowsableAttribute()
{
    _out << nl
         << "[global::System.ComponentModel.EditorBrowsable(global::System.ComponentModel.EditorBrowsableState.Never)]";
}

void
Slice::CsVisitor::writeConstantValue(const TypePtr& type, const SyntaxTreeBasePtr& valueType, const string& value)
{
    ConstPtr constant = dynamic_pointer_cast<Const>(valueType);
    if (constant)
    {
        _out << constant->mappedScoped(".") << ".value";
    }
    else
    {
        BuiltinPtr bp = dynamic_pointer_cast<Builtin>(type);
        if (bp && bp->kind() == Builtin::KindString)
        {
            _out << "\"" << toStringLiteral(value, "\a\b\f\n\r\t\v\0", "", UCN, 0) << "\"";
        }
        else if (bp && bp->kind() == Builtin::KindLong)
        {
            _out << value << "L";
        }
        else if (bp && bp->kind() == Builtin::KindFloat)
        {
            _out << value << "F";
        }
        else if (dynamic_pointer_cast<Enum>(type))
        {
            EnumeratorPtr lte = dynamic_pointer_cast<Enumerator>(valueType);
            assert(lte);
            _out << lte->mappedScoped(".");
        }
        else
        {
            _out << value;
        }
    }
}

void
Slice::CsVisitor::writeDataMemberInitializers(const DataMemberList& dataMembers)
{
    // Generates "= null!" for each required field. This wouldn't be necessary if we actually generated required
    // fields and properties.
    for (const auto& q : dataMembers)
    {
        if (isMappedToRequiredField(q))
        {
            _out << nl << "this." << q->mappedName() << " = null!;";
        }
    }
}

void
Slice::CsVisitor::writeDocComment(const ContainedPtr& p)
{
    optional<DocComment> comment = DocComment::parseFrom(p, csLinkFormatter, true, true);
    if (!comment)
    {
        return;
    }

    StringList overview = comment->overview();
    if (!overview.empty())
    {
        _out << nl << "/// <summary>";
        writeDocLines(_out, overview);
        _out << nl << "/// </summary>";
    }

    writeSeeAlso(_out, comment->seeAlso());
}

void
Slice::CsVisitor::writeOpDocComment(const OperationPtr& op, const vector<string>& extraParams, bool isAsync)
{
    optional<DocComment> comment = DocComment::parseFrom(op, csLinkFormatter, true, true);
    if (!comment)
    {
        return;
    }

    StringList overview = comment->overview();
    if (!overview.empty())
    {
        _out << nl << "/// <summary>";
        writeDocLines(_out, overview);
        _out << nl << "/// </summary>";
    }

    writeParameterDocComments(*comment, isAsync ? op->inParameters() : op->parameters());

    for (const auto& extraParam : extraParams)
    {
        _out << nl << "/// " << extraParam;
    }

    if (isAsync)
    {
        _out << nl << "/// <returns>A task that represents the asynchronous operation.</returns>";
    }
    else if (op->returnType())
    {
        StringList returns = comment->returns();
        if (!returns.empty())
        {
            _out << nl << "/// <returns>";
            writeDocLines(_out, returns);
            _out << nl << "/// </returns>";
        }
    }

    for (const auto& [exceptionName, exceptionLines] : comment->exceptions())
    {
        string name = exceptionName;
        ExceptionPtr ex = op->container()->lookupException(exceptionName, false);
        if (ex)
        {
            name = ex->mappedScoped(".");
        }

        if (!exceptionLines.empty())
        {
            _out << nl << "/// <exception cref=\"" << name << "\">";
            writeDocLines(_out, exceptionLines);
            _out << nl << "/// </exception>";
        }
    }

    writeSeeAlso(_out, comment->seeAlso());
}

void
Slice::CsVisitor::writeParameterDocComments(const DocComment& comment, const ParameterList& parameters)
{
    auto commentParameters = comment.parameters();
    for (const auto& param : parameters)
    {
        auto q = commentParameters.find(param->name());
        if (q != commentParameters.end())
        {
            _out << nl << "/// <param name=\"" << removeEscapePrefix(param->mappedName()) << "\">";
            writeDocLines(_out, q->second);
            _out << nl << "/// </param>";
        }
    }
}

void
Slice::CsVisitor::moduleStart(const ModulePtr& p)
{
    if (!dynamic_pointer_cast<Contained>(p->container()))
    {
        string ns = getNamespacePrefix(p);
        if (!ns.empty())
        {
            _out << sp;
            _out << nl << "namespace " << ns;
            _out << sb;
        }
    }
}

void
Slice::CsVisitor::moduleEnd(const ModulePtr& p)
{
    if (!dynamic_pointer_cast<Contained>(p->container()))
    {
        if (!getNamespacePrefix(p).empty())
        {
            _out << eb;
        }
    }
}

Slice::Gen::Gen(const string& base, const vector<string>& includePaths, const string& dir) : _includePaths(includePaths)
{
    string fileBase = base;
    string::size_type pos = base.find_last_of("/\\");
    if (pos != string::npos)
    {
        fileBase = base.substr(pos + 1);
    }
    string file = fileBase + ".cs";
    if (!dir.empty())
    {
        file = dir + '/' + file;
    }

    _out.open(file.c_str());
    if (!_out)
    {
        ostringstream os;
        os << "cannot open '" << file << "': " << IceInternal::errorToString(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
    FileTracker::instance()->addFile(file);
    printHeader();

    printGeneratedHeader(_out, fileBase + ".ice");

    _out << nl << "#nullable enable";
    _out << sp;
    _out << nl << "[assembly:Ice.Slice(\"" << fileBase << ".ice\")]";

    _out << sp;
    _out << nl << "#pragma warning disable CS1591 // Missing XML Comment";
    _out << nl << "#pragma warning disable CS1573 // Parameter has no matching param tag in the XML comment";
    _out << nl << "#pragma warning disable CS0612 // Type or member is obsolete";
    _out << nl << "#pragma warning disable CS0618 // Type or member is obsolete";
    _out << nl << "#pragma warning disable CS0619 // Type or member is obsolete";
}

Slice::Gen::~Gen()
{
    if (_out.isOpen())
    {
        _out << '\n';
        _out.close();
    }
}

void
Slice::Gen::generate(const UnitPtr& p)
{
    CsGenerator::validateMetadata(p);

    UnitVisitor unitVisitor(_out);
    p->visit(&unitVisitor);

    TypesVisitor typesVisitor(_out);
    p->visit(&typesVisitor);

    ResultVisitor resultVisitor(_out);
    p->visit(&resultVisitor);

    ProxyVisitor proxyVisitor(_out);
    p->visit(&proxyVisitor);

    HelperVisitor helperVisitor(_out);
    p->visit(&helperVisitor);

    DispatcherVisitor dispatcherVisitor(_out);
    p->visit(&dispatcherVisitor);

    DispatchAdapterVisitor dispatchAdapterVisitor(_out);
    p->visit(&dispatchAdapterVisitor);
}

void
Slice::Gen::printHeader()
{
    _out << "// Copyright (c) ZeroC, Inc.\n";
    _out << "//\n";
    _out << "// Ice version " << ICE_STRING_VERSION << "\n";
    _out << "//\n";
}

Slice::Gen::UnitVisitor::UnitVisitor(IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::Gen::UnitVisitor::visitUnitStart(const UnitPtr& unit)
{
    DefinitionContextPtr dc = unit->findDefinitionContext(unit->topLevelFile());
    assert(dc);

    bool sep = false;
    for (const auto& metadata : dc->getMetadata())
    {
        if (metadata->directive() == "cs:attribute")
        {
            if (!sep)
            {
                _out << sp;
                sep = true;
            }
            _out << nl << '[' << metadata->arguments() << ']';
        }
    }
    return false;
}

Slice::Gen::TypesVisitor::TypesVisitor(IceInternal::Output& out) : CsVisitor(out) {}
bool
Slice::Gen::TypesVisitor::visitModuleStart(const ModulePtr& p)
{
    moduleStart(p);
    _out << sp;
    emitAttributes(p);
    _out << nl << "namespace " << p->mappedName();

    _out << sb;

    return true;
}

void
Slice::Gen::TypesVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

bool
Slice::Gen::TypesVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string ns = getNamespace(p);
    ClassDefPtr base = p->base();

    _out << sp;
    emitAttributes(p);

    _out << nl << "[Ice.SliceTypeId(\"" << p->scoped() << "\")]";
    if (p->compactId() >= 0)
    {
        _out << nl << "[Ice.CompactSliceTypeId(" << p->compactId() << ")]";
    }
    _out << nl << "public partial class " << p->mappedName() << " : ";

    if (base)
    {
        _out << getUnqualified(base, ns);
        ;
    }
    else
    {
        _out << "Ice.Value";
    }

    _out << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList allDataMembers = p->allDataMembers();
    ClassDefPtr base = p->base();

    _out << sp << nl << "partial void ice_initialize();";
    if (allDataMembers.empty())
    {
        _out << sp;
        _out << nl << "public " << name << spar << epar;
        _out << sb;
        _out << nl << "ice_initialize();";
        _out << eb;
    }
    else
    {
        // Primary constructor.
        _out << sp;
        _out << nl << "public " << name << spar;

        vector<string> parameters;
        vector<string> secondaryCtorParams;
        vector<string> secondaryCtorMemberNames;
        vector<string> secondaryCtorBaseParamNames;
        for (const auto& q : allDataMembers)
        {
            string memberName = q->mappedName();
            string memberType = typeToString(q->type(), ns, q->optional());
            parameters.push_back(memberType + " " + memberName);

            // The secondary constructor initializes the fields that would be marked "required" if we generated the
            // required keyword.
            if (isMappedToRequiredField(q))
            {
                secondaryCtorParams.push_back(memberType + " " + memberName);

                if (find(dataMembers.begin(), dataMembers.end(), q) != dataMembers.end())
                {
                    secondaryCtorMemberNames.push_back(memberName);
                }
            }
        }
        _out << parameters << epar;

        if (base && allDataMembers.size() != dataMembers.size())
        {
            _out << " : base" << spar;
            vector<string> baseParamNames;
            DataMemberList baseDataMembers = base->allDataMembers();
            for (const auto& q : baseDataMembers)
            {
                string memberName = q->mappedName();
                baseParamNames.push_back(memberName);

                // Look for required fields
                if (isMappedToRequiredField(q))
                {
                    secondaryCtorBaseParamNames.push_back(memberName);
                }
            }
            _out << baseParamNames << epar;
        }
        _out << sb;
        for (const auto& q : dataMembers)
        {
            const string memberName = q->mappedName();
            if (isMappedToNonNullableReference(q))
            {
                _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << memberName << ");";
            }
            _out << nl << "this." << memberName << " = " << memberName << ';';
        }
        _out << nl << "ice_initialize();";
        _out << eb;

        // Secondary constructor. Can be parameterless.
        if (secondaryCtorParams.size() != parameters.size())
        {
            _out << sp;
            _out << nl << "public " << name << spar << secondaryCtorParams << epar;
            if (base && secondaryCtorBaseParamNames.size() > 0)
            {
                _out << " : base" << spar << secondaryCtorBaseParamNames << epar;
            }
            _out << sb;
            for (const auto& q : secondaryCtorMemberNames)
            {
                // All these parameters/fields are non-nullable and we don't tolerate null values.
                _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << q << ");";
                _out << nl << "this." << q << " = " << q << ';';
            }
            _out << nl << "ice_initialize();";
            _out << eb;
        }

        // Parameterless constructor. Required for unmarshaling.
        if (secondaryCtorParams.size() > 0)
        {
            _out << sp;
            emitNonBrowsableAttribute();
            _out << nl << "public " << name << "()";
            _out << sb;
            writeDataMemberInitializers(dataMembers);
            _out << nl << "ice_initialize();";
            _out << eb;
        }
    }

    _out << sp;
    _out << nl << "public static new string ice_staticId() => \"" << p->scoped() << "\";";

    _out << nl << "public override string ice_id() => ice_staticId();";
    writeMarshaling(p);

    _out << eb;
}

bool
Slice::Gen::TypesVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string ns = getNamespace(p);

    _out << sp;
    emitAttributes(p);

    _out << nl << "[Ice.SliceTypeId(\"" << p->scoped() << "\")]";
    _out << nl << "public partial interface " << p->mappedName();

    list<string> baseNames;
    for (const auto& q : p->bases())
    {
        baseNames.push_back(getUnqualified(q, ns));
    }

    if (baseNames.empty())
    {
        baseNames.emplace_back("Ice.Object");
    }

    _out << " : ";
    bool emitSep = false;
    for (const auto& baseName : baseNames)
    {
        if (emitSep)
        {
            _out << ", ";
        }
        emitSep = true;
        _out << baseName;
    }

    _out << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitOperation(const OperationPtr& op)
{
    InterfaceDefPtr interface = op->interface();
    string ns = getNamespace(interface);

    const bool amd = interface->hasMetadata("amd") || op->hasMetadata("amd");
    string retS;
    vector<string> params, args;
    string opName = getDispatchParams(op, retS, params, args, ns);
    _out << sp;

    writeOpDocComment(op, {"<param name=\"" + args.back() + "\">The Current object for the dispatch.</param>"}, amd);

    emitAttributes(op);
    emitObsoleteAttribute(op, _out);
    _out << nl << retS << " " << opName << spar << params << epar << ";";
}

void
Slice::Gen::TypesVisitor::visitSequence(const SequencePtr&)
{
    //
    // No need to generate anything for sequences.
    //
}

bool
Slice::Gen::TypesVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    string ns = getNamespace(p);
    ExceptionPtr base = p->base();

    _out << sp;
    writeDocComment(p);
    emitObsoleteAttribute(p, _out);
    emitAttributes(p);
    _out << nl << "[Ice.SliceTypeId(\"" << p->scoped() << "\")]";
    _out << nl << "public partial class " << p->mappedName() << " : ";
    if (base)
    {
        _out << getUnqualified(base, ns);
    }
    else
    {
        _out << "Ice.UserException";
    }
    _out << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitExceptionEnd(const ExceptionPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);
    DataMemberList allDataMembers = p->allDataMembers();
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList optionalMembers = p->orderedOptionalDataMembers();
    ExceptionPtr base = p->base();

    if (!allDataMembers.empty())
    {
        // Primary constructor.
        _out << sp;
        _out << nl << "public " << name << spar;

        vector<string> parameters;
        vector<string> secondaryCtorParams;
        vector<string> secondaryCtorMemberNames;
        vector<string> secondaryCtorBaseParamNames;

        for (const auto& q : allDataMembers)
        {
            string memberName = q->mappedName();
            string memberType = typeToString(q->type(), ns, q->optional());
            parameters.push_back(memberType + " " + memberName);

            // The secondary constructor initializes the fields that would be marked "required" if we generated the
            // required keyword.
            if (isMappedToRequiredField(q))
            {
                secondaryCtorParams.push_back(memberType + " " + memberName);

                if (find(dataMembers.begin(), dataMembers.end(), q) != dataMembers.end())
                {
                    secondaryCtorMemberNames.push_back(memberName);
                }
            }
        }
        _out << parameters << epar;

        if (base && allDataMembers.size() != dataMembers.size())
        {
            _out << " : base" << spar;
            vector<string> baseParamNames;
            DataMemberList baseDataMembers = base->allDataMembers();
            for (const auto& q : baseDataMembers)
            {
                string memberName = q->mappedName();
                baseParamNames.push_back(memberName);

                // Look for required fields
                if (isMappedToRequiredField(q))
                {
                    secondaryCtorBaseParamNames.push_back(memberName);
                }
            }
            _out << baseParamNames << epar;
        }
        _out << sb;
        for (const auto& q : dataMembers)
        {
            const string memberName = q->mappedName();
            if (isMappedToNonNullableReference(q))
            {
                _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << memberName << ");";
            }
            _out << nl << "this." << memberName << " = " << memberName << ';';
        }
        _out << eb;

        // Secondary constructor. Can be parameterless.
        if (secondaryCtorParams.size() != parameters.size())
        {
            _out << sp;
            _out << nl << "public " << name << spar << secondaryCtorParams << epar;
            if (base && secondaryCtorBaseParamNames.size() > 0)
            {
                _out << " : base" << spar << secondaryCtorBaseParamNames << epar;
            }
            _out << sb;
            for (const auto& q : secondaryCtorMemberNames)
            {
                // All these parameters/fields are non-nullable and we don't tolerate null values.
                _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << q << ");";
                _out << nl << "this." << q << " = " << q << ';';
            }
            _out << eb;
        }

        // Parameterless constructor. Required for unmarshaling.
        if (secondaryCtorParams.size() > 0)
        {
            _out << sp;
            emitNonBrowsableAttribute();
            _out << nl << "public " << name << "()";
            _out << sb;
            writeDataMemberInitializers(dataMembers);
            _out << eb;
        }
    }

    string scoped = p->scoped();

    _out << sp;
    _out << nl << "public override string ice_id() => \"" << scoped << "\";";

    _out << sp;
    _out << nl << "protected override void iceWriteImpl(Ice.OutputStream ostr_)";
    _out << sb;
    _out << nl << "ostr_.startSlice(\"" << scoped << "\", -1, " << (!base ? "true" : "false") << ");";
    for (const auto& dataMember : dataMembers)
    {
        if (!dataMember->optional())
        {
            writeMarshalDataMember(dataMember, dataMember->mappedName(), ns);
        }
    }

    for (const auto& optionalMember : optionalMembers)
    {
        writeMarshalDataMember(optionalMember, optionalMember->mappedName(), ns);
    }

    _out << nl << "ostr_.endSlice();";
    if (base)
    {
        _out << nl << "base.iceWriteImpl(ostr_);";
    }
    _out << eb;

    _out << sp;
    _out << nl << "protected override void iceReadImpl(Ice.InputStream istr_)";
    _out << sb;
    _out << nl << "istr_.startSlice();";

    for (const auto& dataMember : dataMembers)
    {
        if (!dataMember->optional())
        {
            writeUnmarshalDataMember(dataMember, dataMember->mappedName(), ns);
        }
    }

    for (const auto& optionalMember : optionalMembers)
    {
        writeUnmarshalDataMember(optionalMember, optionalMember->mappedName(), ns);
    }
    _out << nl << "istr_.endSlice();";
    if (base)
    {
        _out << nl << "base.iceReadImpl(istr_);";
    }
    _out << eb;

    if (p->usesClasses() && !(base && base->usesClasses()))
    {
        _out << sp;
        _out << nl << "public override bool iceUsesClasses()";
        _out << sb;
        _out << nl << "return true;";
        _out << eb;
    }

    _out << eb;
}

bool
Slice::Gen::TypesVisitor::visitStructStart(const StructPtr& p)
{
    const bool classMapping = isMappedToClass(p);
    string name = p->mappedName();
    _out << sp;

    emitObsoleteAttribute(p, _out);

    emitAttributes(p);
    _out << nl << "public " << (classMapping ? "sealed partial record class" : "partial record struct") << ' ' << name;
    _out << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitStructEnd(const StructPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);
    DataMemberList dataMembers = p->dataMembers();

    _out << sp << nl << "partial void ice_initialize();";

    if (isMappedToClass(p))
    {
        // We generate a constructor that initializes all required fields (collections and structs mapped to
        // classes). It can be parameterless.

        vector<string> ctorParams;
        vector<string> ctorParamNames;
        for (const auto& q : dataMembers)
        {
            if (isMappedToRequiredField(q))
            {
                string memberName = q->mappedName();
                string memberType = typeToString(q->type(), ns, false);
                ctorParams.push_back(memberType + " " + memberName);
                ctorParamNames.push_back(memberName);
            }
        }

        // We only generate this ctor if it's different from the primary constructor.
        if (ctorParams.size() != dataMembers.size())
        {
            _out << sp;
            _out << nl << "public " << name << spar << ctorParams << epar;
            _out << sb;
            for (const auto& q : ctorParamNames)
            {
                _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << q << ");";
                _out << nl << "this." << q << " = " << q << ';';
            }
            // All the other fields keep their default values (explicit or implicit).
            _out << nl << "ice_initialize();";
            _out << eb;
        }
    }

    _out << sp;
    _out << nl << "public " << name << spar;
    vector<string> parameters;
    for (const auto& q : dataMembers)
    {
        parameters.push_back(typeToString(q->type(), ns, false) + " " + q->mappedName());
    }
    _out << parameters << epar;
    _out << sb;
    for (const auto& q : dataMembers)
    {
        string paramName = q->mappedName();
        if (isMappedToNonNullableReference(q))
        {
            _out << nl << "global::System.ArgumentNullException.ThrowIfNull(" << paramName << ");";
        }
        _out << nl << "this." << paramName << " = " << paramName << ';';
    }
    _out << nl << "ice_initialize();";
    _out << eb;

    // Unmarshaling constructor
    _out << sp;
    _out << nl << "public " << name << "(Ice.InputStream istr)";
    _out << sb;
    for (const auto& q : dataMembers)
    {
        writeUnmarshalDataMember(q, q->mappedName(), ns, true);
    }
    _out << nl << "ice_initialize();";
    _out << eb;

    _out << sp;
    _out << nl << "public void ice_writeMembers(Ice.OutputStream ostr)";
    _out << sb;
    for (const auto& dataMember : dataMembers)
    {
        writeMarshalDataMember(dataMember, dataMember->mappedName(), ns, true);
    }
    _out << eb;

    _out << sp;
    _out << nl << "public static void ice_write(Ice.OutputStream ostr, " << name << " v)";
    _out << sb;
    _out << nl << "v.ice_writeMembers(ostr);";
    _out << eb;

    _out << sp;
    _out << nl << "public static " << name << " ice_read(Ice.InputStream istr) => new(istr);";
    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitEnum(const EnumPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);
    EnumeratorList enumerators = p->enumerators();
    const bool hasExplicitValues = p->hasExplicitValues();

    _out << sp;
    emitObsoleteAttribute(p, _out);
    writeDocComment(p);
    emitAttributes(p);
    _out << nl << "public enum " << name;
    _out << sb;
    for (auto en = enumerators.begin(); en != enumerators.end(); ++en)
    {
        if (en != enumerators.begin())
        {
            _out << ',';
        }
        writeDocComment(*en);
        emitAttributes(*en);
        _out << nl << (*en)->mappedName();
        if (hasExplicitValues)
        {
            _out << " = " << (*en)->value();
        }
    }
    _out << eb;

    _out << sp;
    _out << nl << "public sealed class " << name << "Helper";
    _out << sb;
    _out << sp;
    _out << nl << "public static void write(Ice.OutputStream ostr, " << name << " v)";
    _out << sb;
    writeMarshalUnmarshalCode(_out, p, ns, "v", true);
    _out << eb;

    _out << sp;
    _out << nl << "public static " << name << " read(Ice.InputStream istr)";
    _out << sb;
    _out << nl << name << " v;";
    writeMarshalUnmarshalCode(_out, p, ns, "v", false);
    _out << nl << "return v;";
    _out << eb;

    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitConst(const ConstPtr& p)
{
    _out << sp;
    emitAttributes(p);
    _out << nl << "public abstract class " << p->mappedName();
    _out << sb;
    _out << sp << nl << "public const " << typeToString(p->type(), "") << " value = ";
    writeConstantValue(p->type(), p->valueType(), p->value());
    _out << ";";
    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitDataMember(const DataMemberPtr& p)
{
    ContainedPtr cont = dynamic_pointer_cast<Contained>(p->container());
    assert(cont);
    bool isProperty = cont->hasMetadata("cs:property");
    StructPtr st = dynamic_pointer_cast<Struct>(cont);
    string ns = getNamespace(cont);

    _out << sp;

    emitObsoleteAttribute(p, _out);

    string type = typeToString(p->type(), ns, p->optional());

    emitAttributes(p);
    _out << nl << "public" << ' ' << type << ' ' << p->mappedName();

    bool addSemicolon = true;
    if (isProperty)
    {
        _out << " { get; set; }";
        addSemicolon = false;
    }

    // Generate the default value for this field unless the enclosing type is a struct.
    if (!st || isMappedToClass(st))
    {
        if (p->defaultValueType())
        {
            _out << " = ";
            writeConstantValue(p->type(), p->defaultValueType(), p->defaultValue());
            addSemicolon = true;
        }
        else if (!p->optional())
        {
            BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(p->type());
            if (builtin && builtin->kind() == Builtin::KindString)
            {
                // This behavior is unfortunate but kept for backwards compatibility.
                _out << " = \"\"";
                addSemicolon = true;
            }
        }
    }

    if (addSemicolon)
    {
        _out << ';';
    }
}

Slice::Gen::ResultVisitor::ResultVisitor(::IceInternal::Output& out) : CsVisitor(out) {}

namespace
{
    bool hasResultType(const ModulePtr& p)
    {
        InterfaceList interfaces = p->interfaces();
        for (const auto& interface : interfaces)
        {
            OperationList operations = interface->operations();
            for (const auto& op : operations)
            {
                ParameterList outParams = op->outParameters();
                TypePtr ret = op->returnType();
                if (outParams.size() > 1 || (ret && outParams.size() > 0))
                {
                    return true;
                }
            }
        }

        for (const auto& module : p->modules())
        {
            if (hasResultType(module))
            {
                return true;
            }
        }

        return false;
    }
}

bool
Slice::Gen::ResultVisitor::visitModuleStart(const ModulePtr& p)
{
    if (hasResultType(p))
    {
        moduleStart(p);
        _out << sp << nl << "namespace " << p->mappedName();
        _out << sb;
        return true;
    }
    return false;
}

void
Slice::Gen::ResultVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

void
Slice::Gen::ResultVisitor::visitOperation(const OperationPtr& p)
{
    InterfaceDefPtr interface = p->interface();
    string ns = getNamespace(interface);
    ParameterList outParams = p->outParameters();
    TypePtr ret = p->returnType();

    if (outParams.size() > 1 || (ret && outParams.size() > 0))
    {
        string name = resultStructName(interface->mappedName(), p->mappedName());

        string retS;
        string retSName;
        if (ret)
        {
            retS = typeToString(ret, ns, p->returnIsOptional());
            retSName = resultStructReturnValueName(outParams);
        }

        _out << sp;
        _out << nl << "public record struct " << name;
        _out << spar;
        if (ret)
        {
            _out << (retS + " " + retSName);
        }

        for (const auto& q : outParams)
        {
            _out << (typeToString(q->type(), ns, q->optional()) + " " + q->mappedName());
        }
        _out << epar;
        _out << ";";
    }

    if (p->hasMarshaledResult())
    {
        string name = resultStructName(interface->mappedName(), p->mappedName(), true);

        _out << sp;
        _out << nl << "public readonly record struct " << name << " : Ice.MarshaledResult";
        _out << sb;

        //
        // Marshaling constructor
        //
        _out << nl << "public " << name << spar << getOutParams(p, ns, true, false) << "Ice.Current current" << epar;
        _out << sb;
        _out << nl << "_ostr = Ice.CurrentExtensions.startReplyStream(current);";
        _out << nl << "_ostr.startEncapsulation(current.encoding, " << opFormatTypeToString(p) << ");";
        writeMarshalUnmarshalParams(outParams, p, true, ns, false, true, "_ostr");
        if (p->returnsClasses())
        {
            _out << nl << "_ostr.writePendingValues();";
        }
        _out << nl << "_ostr.endEncapsulation();";
        _out << eb;
        _out << sp;
        _out << nl << "public Ice.OutputStream outputStream => _ostr;";
        _out << sp;
        _out << nl << "private readonly Ice.OutputStream _ostr;";
        _out << eb;
    }
}

Slice::Gen::ProxyVisitor::ProxyVisitor(IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::Gen::ProxyVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->contains<InterfaceDef>())
    {
        return false;
    }

    moduleStart(p);
    _out << sp << nl << "namespace " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::Gen::ProxyVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

bool
Slice::Gen::ProxyVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string ns = getNamespace(p);

    _out << sp;
    writeDocComment(p);
    _out << nl << "public interface " << p->mappedName() << "Prx : ";

    vector<string> baseInterfaces;
    for (const auto& base : p->bases())
    {
        baseInterfaces.push_back(getUnqualified(base, ns) + "Prx");
    }

    if (baseInterfaces.empty())
    {
        baseInterfaces.emplace_back("Ice.ObjectPrx");
    }

    for (auto q = baseInterfaces.begin(); q != baseInterfaces.end();)
    {
        _out << *q;
        if (++q != baseInterfaces.end())
        {
            _out << ", ";
        }
    }
    _out << sb;

    return true;
}

void
Slice::Gen::ProxyVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb;
}

void
Slice::Gen::ProxyVisitor::visitOperation(const OperationPtr& p)
{
    string ns = getNamespace(p->interface());
    string name = p->mappedName();
    vector<string> inParams = getInParams(p, ns);
    string retS = typeToString(p->returnType(), ns, p->returnIsOptional());

    {
        //
        // Write the synchronous version of the operation.
        //
        string context = getEscapedParamName(p, "context");
        _out << sp;
        writeOpDocComment(
            p,
            {"<param name=\"" + context + "\">The Context map to send with the invocation.</param>"},
            false);
        emitObsoleteAttribute(p, _out);
        _out << nl << retS << " " << name << spar << getParams(p, ns)
             << ("global::System.Collections.Generic.Dictionary<string, string>? " + context + " = null") << epar
             << ';';
    }

    {
        //
        // Write the async version of the operation (using Async Task API)
        //
        string context = getEscapedParamName(p, "context");
        string cancel = getEscapedParamName(p, "cancel");
        string progress = getEscapedParamName(p, "progress");

        _out << sp;
        writeOpDocComment(
            p,
            {"<param name=\"" + context + "\">Context map to send with the invocation.</param>",
             "<param name=\"" + progress + "\">Sent progress provider.</param>",
             "<param name=\"" + cancel + "\">A cancellation token that receives the cancellation requests.</param>"},
            true);
        emitObsoleteAttribute(p, _out);
        _out << nl << taskResultType(p, ns);
        _out << " " << name << "Async" << spar << inParams
             << ("global::System.Collections.Generic.Dictionary<string, string>? " + context + " = null")
             << ("global::System.IProgress<bool>? " + progress + " = null")
             << ("global::System.Threading.CancellationToken " + cancel + " = default") << epar << ";";
    }
}

Slice::Gen::DispatchAdapterVisitor::DispatchAdapterVisitor(IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::Gen::DispatchAdapterVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->contains<InterfaceDef>())
    {
        return false;
    }

    moduleStart(p);
    _out << sp << nl << "namespace " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::Gen::DispatchAdapterVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

bool
Slice::Gen::DispatchAdapterVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    if (p->allOperations().empty())
    {
        return false;
    }

    _out << sp;
    _out << nl << "public partial interface " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::Gen::DispatchAdapterVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb;
}

void
Slice::Gen::DispatchAdapterVisitor::visitOperation(const OperationPtr& op)
{
    InterfaceDefPtr interface = op->interface();
    string ns = getNamespace(interface);
    string opName = op->mappedName();
    const bool amd = interface->hasMetadata("amd") || op->hasMetadata("amd");

    _out << sp;
    _out << nl << "protected static " << (amd ? "async " : "")
         << "global::System.Threading.Tasks.ValueTask<Ice.OutgoingResponse> iceD_" << op->name() << "Async(";
    _out.inc();
    _out << nl << interface->mappedName() << " obj,";
    _out << nl << "Ice.IncomingRequest request)";
    _out.dec();
    _out << sb;

    TypePtr ret = op->returnType();
    ParameterList inParams = op->inParameters();
    ParameterList outParams = op->outParameters();

    _out << nl << "Ice.ObjectImpl.iceCheckMode(" << sliceModeToIceMode(op->mode()) << ", request.current.mode);";
    if (!inParams.empty())
    {
        // Unmarshal 'in' parameters.
        _out << nl << "var istr = request.inputStream;";
        _out << nl << "istr.startEncapsulation();";
        for (const auto& pli : inParams)
        {
            string param = "iceP_" + removeEscapePrefix(pli->mappedName());
            string typeS = typeToString(pli->type(), ns, pli->optional());

            _out << nl << typeS << ' ' << param << (pli->type()->isClassType() ? " = null;" : ";");
        }
        writeMarshalUnmarshalParams(inParams, nullptr, false, ns);
        if (op->sendsClasses())
        {
            _out << nl << "istr.readPendingValues();";
        }
        _out << nl << "istr.endEncapsulation();";
    }
    else
    {
        _out << nl << "request.inputStream.skipEmptyEncapsulation();";
    }

    vector<string> inArgs;
    for (const auto& pli : inParams)
    {
        inArgs.push_back("iceP_" + removeEscapePrefix(pli->mappedName()));
    }

    if (op->hasMarshaledResult())
    {
        if (amd)
        {
            _out << nl << "var result = await obj." << opName << "Async" << spar << inArgs << "request.current" << epar
                 << ".ConfigureAwait(false);";
            _out << nl << "return new Ice.OutgoingResponse(result.outputStream);";
        }
        else
        {
            _out << nl << "var result = obj." << opName << spar << inArgs << "request.current" << epar << ";";
            _out << nl << "return new (new Ice.OutgoingResponse(result.outputStream));";
        }
    }
    else if (amd)
    {
        string retS = resultType(op, ns);
        _out << nl;

        if (!retS.empty())
        {
            _out << "var result = ";
        }

        _out << "await obj." << opName << "Async" << spar << inArgs << "request.current" << epar
             << ".ConfigureAwait(false);";

        if (retS.empty())
        {
            _out << nl << "return Ice.CurrentExtensions.createEmptyOutgoingResponse(request.current);";
        }
        else
        {
            // Adapt to marshaling helper below.
            string resultParam =
                !ret && outParams.size() == 1 ? ("iceP_" + removeEscapePrefix(outParams.front()->mappedName())) : "ret";

            _out << nl << "return Ice.CurrentExtensions.createOutgoingResponse(";
            _out.inc();
            _out << nl << "request.current,";
            _out << nl << "result,";
            _out << nl << "static (ostr, " << resultParam << ") =>";
            _out << sb;
            writeMarshalUnmarshalParams(outParams, op, true, ns, true);
            if (op->returnsClasses())
            {
                _out << nl << "ostr.writePendingValues();";
            }
            _out << eb;
            if (op->format())
            {
                _out << "," << nl << opFormatTypeToString(op);
            }
            _out << ");";
            _out.dec();
        }
    }
    else
    {
        for (const auto& pli : outParams)
        {
            string typeS = typeToString(pli->type(), ns, pli->optional());
            _out << nl << typeS << " iceP_" << removeEscapePrefix(pli->mappedName()) << ";";
        }
        _out << nl;
        if (ret)
        {
            _out << "var ret = ";
        }
        _out << "obj." << opName << spar << inArgs;
        for (const auto& pli : outParams)
        {
            _out << "out iceP_" + removeEscapePrefix(pli->mappedName());
        }
        _out << "request.current" << epar << ";";

        if (outParams.empty() && !ret)
        {
            _out << nl << "return new(Ice.CurrentExtensions.createEmptyOutgoingResponse(request.current));";
        }
        else
        {
            _out << nl << "var ostr = Ice.CurrentExtensions.startReplyStream(request.current);";
            _out << nl << "ostr.startEncapsulation(request.current.encoding, " << opFormatTypeToString(op) << ");";
            writeMarshalUnmarshalParams(outParams, op, true, ns);
            if (op->returnsClasses())
            {
                _out << nl << "ostr.writePendingValues();";
            }
            _out << nl << "ostr.endEncapsulation();";
            _out << nl << "return new(new Ice.OutgoingResponse(ostr));";
        }
    }
    _out << eb;
}

Slice::Gen::HelperVisitor::HelperVisitor(IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::Gen::HelperVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->contains<InterfaceDef>() && !p->contains<Sequence>() && !p->contains<Dictionary>())
    {
        return false;
    }

    moduleStart(p);
    _out << sp << nl << "namespace " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::Gen::HelperVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

bool
Slice::Gen::HelperVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);

    _out << sp;
    _out << nl << "public sealed class " << name << "PrxHelper : "
         << "Ice.ObjectPrxHelperBase, " << name << "Prx";
    _out << sb;

    OperationList ops = p->allOperations();

    for (const auto& op : ops)
    {
        string opName = op->mappedName();
        TypePtr ret = op->returnType();
        string retS = typeToString(ret, ns, op->returnIsOptional());

        vector<string> params = getParams(op, ns);
        vector<string> argsAMI = getInArgs(op);

        ParameterList outParams = op->outParameters();

        string context = getEscapedParamName(op, "context");

        _out << sp;
        _out << nl << "public " << retS << " " << opName << spar << params
             << ("global::System.Collections.Generic.Dictionary<string, string>? " + context + " = null") << epar;
        _out << sb;
        _out << nl << "try";
        _out << sb;

        _out << nl;

        if (ret || !outParams.empty())
        {
            if (outParams.empty())
            {
                _out << "return ";
            }
            else if (ret || outParams.size() > 1)
            {
                _out << "var result_ = ";
            }
            else
            {
                _out << outParams.front()->mappedName() << " = ";
            }
        }
        _out << "_iceI_" << removeEscapePrefix(opName) << "Async" << spar << argsAMI << context << "null"
             << "global::System.Threading.CancellationToken.None"
             << "true" << epar;

        if (ret || outParams.size() > 0)
        {
            _out << ".Result;";
        }
        else
        {
            _out << ".Wait();";
        }

        if ((ret && outParams.size() > 0) || outParams.size() > 1)
        {
            for (const auto& param : outParams)
            {
                string paramName = param->mappedName();
                _out << nl << paramName << " = result_." << paramName << ";";
            }

            if (ret)
            {
                _out << nl << "return result_." << resultStructReturnValueName(outParams) << ";";
            }
        }
        _out << eb;
        _out << nl << "catch (global::System.AggregateException ex_)";
        _out << sb;
        _out << nl << "throw ex_.InnerException!;";
        _out << eb;
        _out << eb;
    }

    // Async invocation
    for (const auto& op : ops)
    {
        vector<string> paramsAMI = getInParams(op, ns);
        vector<string> argsAMI = getInArgs(op);

        string opName = op->mappedName();

        ParameterList inParams = op->inParameters();
        ParameterList outParams = op->outParameters();

        string context = getEscapedParamName(op, "context");
        string cancel = getEscapedParamName(op, "cancel");
        string progress = getEscapedParamName(op, "progress");

        TypePtr ret = op->returnType();

        string returnTypeS = resultType(op, ns);

        // Arrange exceptions into most-derived to least-derived order. If we don't
        // do this, a base exception handler can appear before a derived exception
        // handler, causing compiler warnings and resulting in the base exception
        // being marshaled instead of the derived exception.
        ExceptionList throws = op->throws();
        throws.sort(Slice::DerivedToBaseCompare());

        // Write the public Async method.
        _out << sp;
        _out << nl << "public global::System.Threading.Tasks.Task";
        if (!returnTypeS.empty())
        {
            _out << "<" << returnTypeS << ">";
        }
        _out << " " << opName << "Async" << spar << paramsAMI
             << ("global::System.Collections.Generic.Dictionary<string, string>? " + context + " = null")
             << ("global::System.IProgress<bool>? " + progress + " = null")
             << ("global::System.Threading.CancellationToken " + cancel + " = default") << epar;

        _out << sb;
        _out << nl << "return _iceI_" << removeEscapePrefix(opName) << "Async" << spar << argsAMI << context
             << progress << cancel << "false" << epar << ";";
        _out << eb;

        //
        // Write the Async method implementation.
        //
        _out << sp;
        _out << nl << "private global::System.Threading.Tasks.Task";
        if (!returnTypeS.empty())
        {
            _out << "<" << returnTypeS << ">";
        }
        _out << " _iceI_" << removeEscapePrefix(opName) << "Async" << spar << getInParams(op, ns, true)
             << "global::System.Collections.Generic.Dictionary<string, string>? context"
             << "global::System.IProgress<bool>? progress"
             << "global::System.Threading.CancellationToken cancel"
             << "bool synchronous" << epar;
        _out << sb;

        string flatName = "_" + removeEscapePrefix(opName) + "_name";
        if (op->returnsData())
        {
            _out << nl << "iceCheckTwowayOnly(" << flatName << ");";
        }
        if (returnTypeS.empty())
        {
            _out << nl << "var completed = "
                 << "new Ice.Internal.OperationTaskCompletionCallback<object>(progress, cancel);";
        }
        else
        {
            _out << nl << "var completed = "
                 << "new Ice.Internal.OperationTaskCompletionCallback<" << returnTypeS << ">(progress, cancel);";
        }

        _out << nl << "_iceI_" << removeEscapePrefix(opName) << spar << getInArgs(op, true) << "context"
             << "synchronous"
             << "completed" << epar << ";";
        _out << nl << "return completed.Task;";

        _out << eb;

        _out << sp << nl << "private const string " << flatName << " = \"" << op->name() << "\";";

        //
        // Write the common invoke method
        //
        _out << sp << nl;
        _out << "private void _iceI_" << removeEscapePrefix(opName) << spar << getInParams(op, ns, true)
             << "global::System.Collections.Generic.Dictionary<string, string>? context"
             << "bool synchronous"
             << "Ice.Internal.OutgoingAsyncCompletionCallback completed" << epar;
        _out << sb;

        if (returnTypeS.empty())
        {
            _out << nl << "var outAsync = getOutgoingAsync<object>(completed);";
        }
        else
        {
            _out << nl << "var outAsync = getOutgoingAsync<" << returnTypeS << ">(completed);";
        }

        _out << nl << "outAsync.invoke(";
        _out.inc();
        _out << nl << flatName << ",";
        _out << nl << sliceModeToIceMode(op->mode()) << ",";
        _out << nl << opFormatTypeToString(op) << ",";
        _out << nl << "context,";
        _out << nl << "synchronous";
        if (!inParams.empty())
        {
            _out << ",";
            _out << nl << "write: (Ice.OutputStream ostr) =>";
            _out << sb;
            writeMarshalUnmarshalParams(inParams, nullptr, true, ns);
            if (op->sendsClasses())
            {
                _out << nl << "ostr.writePendingValues();";
            }
            _out << eb;
        }

        if (!throws.empty())
        {
            _out << ",";
            _out << nl << "userException: (Ice.UserException ex) =>";
            _out << sb;
            _out << nl << "try";
            _out << sb;
            _out << nl << "throw ex;";
            _out << eb;

            // Generate a catch block for each legal user exception.
            for (const auto& thrown : throws)
            {
                _out << nl << "catch(" << getUnqualified(thrown, ns) << ")";
                _out << sb;
                _out << nl << "throw;";
                _out << eb;
            }

            _out << nl << "catch(Ice.UserException)";
            _out << sb;
            _out << eb;

            _out << eb;
        }

        if (ret || !outParams.empty())
        {
            _out << ",";
            _out << nl << "read: (Ice.InputStream istr) =>";
            _out << sb;
            if (outParams.empty())
            {
                _out << nl << returnTypeS << " ret" << (ret->isClassType() ? " = null;" : ";");
            }
            else if (ret || outParams.size() > 1)
            {
                // Generated OpResult struct
                _out << nl << "var ret = new " << returnTypeS << "();";
            }
            else
            {
                TypePtr t = outParams.front()->type();
                _out << nl << typeToString(t, ns, (outParams.front()->optional()))
                     << " iceP_" << removeEscapePrefix(outParams.front()->mappedName())
                     << (t->isClassType() ? " = null;" : ";");
            }

            writeMarshalUnmarshalParams(outParams, op, false, ns, true);
            if (op->returnsClasses())
            {
                _out << nl << "istr.readPendingValues();";
            }

            if (!ret && outParams.size() == 1)
            {
                _out << nl << "return iceP_" << removeEscapePrefix(outParams.front()->mappedName()) << ";";
            }
            else
            {
                _out << nl << "return ret;";
            }
            _out << eb;
        }
        _out << ");";
        _out.dec();
        _out << eb;
    }

    _out << sp << nl << "public static " << name
         << "Prx createProxy(Ice.Communicator communicator, string proxyString) =>";
    _out.inc();
    _out << nl << "new " << name << "PrxHelper(Ice.ObjectPrxHelper.createProxy(communicator, proxyString));";
    _out.dec();

    _out << sp << nl << "public static " << name
         << "Prx? checkedCast(Ice.ObjectPrx? b, global::System.Collections.Generic.Dictionary<string, string>? ctx = "
            "null) =>";
    _out.inc();
    _out << nl << "b is not null && b.ice_isA(ice_staticId(), ctx) ? new " << name << "PrxHelper(b) : null;";
    _out.dec();

    _out << sp << nl << "public static " << name
         << "Prx? checkedCast(Ice.ObjectPrx? b, string f, global::System.Collections.Generic.Dictionary<string, "
            "string>? ctx = null) =>";
    _out.inc();
    _out << nl << "checkedCast(b?.ice_facet(f), ctx);";
    _out.dec();

    _out << sp << nl << "[return: global::System.Diagnostics.CodeAnalysis.NotNullIfNotNull(nameof(b))]";
    _out << sp << nl << "public static " << name << "Prx? uncheckedCast(Ice.ObjectPrx? b) =>";
    _out.inc();
    _out << nl << "b is not null ? new " << name << "PrxHelper(b) : null;";
    _out.dec();

    _out << sp << nl << "[return: global::System.Diagnostics.CodeAnalysis.NotNullIfNotNull(nameof(b))]";
    _out << sp << nl << "public static " << name << "Prx? uncheckedCast(Ice.ObjectPrx? b, string f) =>";
    _out.inc();
    _out << nl << "uncheckedCast(b?.ice_facet(f));";
    _out.dec();

    //
    // Need static-readonly for arrays in C# (not const)
    //
    _out << sp << nl << "private static readonly string[] _ids =";
    _out << sb;

    StringList ids = p->ids();
    {
        auto q = ids.begin();
        while (q != ids.end())
        {
            _out << nl << '"' << *q << '"';
            if (++q != ids.end())
            {
                _out << ',';
            }
        }
    }
    _out << eb << ";";

    _out << sp << nl << "public static string ice_staticId() => \"" << p->scoped() << "\";";

    _out << sp << nl << "public static void write(Ice.OutputStream ostr, " << name << "Prx? v)";
    _out << sb;
    _out << nl << "ostr.writeProxy(v);";
    _out << eb;

    _out << sp << nl << "public static " << name << "Prx? read(Ice.InputStream istr) =>";
    _out.inc();
    _out << nl << "istr.readProxy() is Ice.ObjectPrx proxy ? new " << name << "PrxHelper(proxy) : null;";
    _out.dec();

    return true;
}

void
Slice::Gen::HelperVisitor::visitInterfaceDefEnd(const InterfaceDefPtr& p)
{
    string name = p->mappedName();

    _out << sp;
    _out << nl << "protected override Ice.ObjectPrxHelperBase iceNewInstance(Ice.Internal.Reference reference) => new "
         << name << "PrxHelper(reference);";

    _out << sp;
    _out << nl << "private " << name << "PrxHelper(Ice.ObjectPrx proxy)";
    _out.inc();
    _out << nl << ": base(proxy)";
    _out.dec();
    _out << sb;
    _out << eb;

    _out << sp;
    _out << nl << "private " << name << "PrxHelper(Ice.Internal.Reference reference)";
    _out.inc();
    _out << nl << ": base(reference)";
    _out.dec();
    _out << sb;
    _out << eb;

    _out << eb;
}

void
Slice::Gen::HelperVisitor::visitSequence(const SequencePtr& p)
{
    string ns = getNamespace(p);
    string name = p->mappedName();

    string typeS = typeToString(p, ns);
    _out << sp;
    _out << nl << "public sealed class " << name << "Helper";
    _out << sb;

    _out << sp << nl << "public static void write(Ice.OutputStream ostr, " << typeS << " v)";
    _out << sb;
    writeSequenceMarshalUnmarshalCode(_out, p, ns, "v", true, false);
    _out << eb;

    _out << sp << nl << "public static " << typeS << " read(Ice.InputStream istr)";
    _out << sb;
    _out << nl << typeS << " v;";
    writeSequenceMarshalUnmarshalCode(_out, p, ns, "v", false, false);
    _out << nl << "return v;";
    _out << eb;
    _out << eb;

    if (auto metadata = p->getMetadataArgs("cs:generic"))
    {
        string_view type = *metadata;
        if (type == "List" || type == "LinkedList" || type == "Queue" || type == "Stack")
        {
            return;
        }

        if (!p->type()->isClassType())
        {
            return;
        }

        //
        // The sequence is a custom sequence with elements of class type.
        // Emit a dummy class that causes a compile-time error if the
        // custom sequence type does not implement an indexer.
        //
        _out << sp;
        _out << nl << "public class " << name << "_Tester";
        _out << sb;
        _out << nl << name << "_Tester()";
        _out << sb;
        _out << nl << typeS << " test = new " << typeS << "();";
        _out << nl << "test[0] = null;";
        _out << eb;
        _out << eb;
    }
}

void
Slice::Gen::HelperVisitor::visitDictionary(const DictionaryPtr& p)
{
    TypePtr key = p->keyType();
    TypePtr value = p->valueType();

    string genericType = p->getMetadataArgs("cs:generic").value_or("Dictionary");

    string ns = getNamespace(p);
    string keyS = typeToString(key, ns);
    string valueS = typeToString(value, ns);
    string name = "global::System.Collections.Generic." + genericType + "<" + keyS + ", " + valueS + ">";

    _out << sp;
    _out << nl << "public sealed class " << p->mappedName() << "Helper";
    _out << sb;

    _out << sp << nl << "public static void write(";
    _out.useCurrentPosAsIndent();
    _out << "Ice.OutputStream ostr,";
    _out << nl << name << " v)";
    _out.restoreIndent();
    _out << sb;
    _out << nl << "if(v == null)";
    _out << sb;
    _out << nl << "ostr.writeSize(0);";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "ostr.writeSize(v.Count);";
    _out << nl << "foreach(global::System.Collections.";
    _out << "Generic.KeyValuePair<" << keyS << ", " << valueS << ">";
    _out << " e in v)";
    _out << sb;
    writeMarshalUnmarshalCode(_out, key, ns, "e.Key", true);
    writeMarshalUnmarshalCode(_out, value, ns, "e.Value", true);
    _out << eb;
    _out << eb;
    _out << eb;

    _out << sp << nl << "public static " << name << " read(Ice.InputStream istr)";
    _out << sb;
    _out << nl << "int sz = istr.readSize();";
    _out << nl << name << " r = new " << name << "();";
    _out << nl << "for(int i = 0; i < sz; ++i)";
    _out << sb;
    _out << nl << keyS << " k;";
    writeMarshalUnmarshalCode(_out, key, ns, "k", false);

    if (value->isClassType())
    {
        ostringstream os;
        os << '(' << typeToString(value, ns) << " v) => { r[k] = v; }";
        writeMarshalUnmarshalCode(_out, value, ns, os.str(), false);
    }
    else
    {
        _out << nl << valueS << " v;";
        writeMarshalUnmarshalCode(_out, value, ns, "v", false);
        _out << nl << "r[k] = v;";
    }
    _out << eb;
    _out << nl << "return r;";
    _out << eb;

    _out << eb;
}

Slice::Gen::DispatcherVisitor::DispatcherVisitor(::IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::Gen::DispatcherVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->contains<InterfaceDef>())
    {
        return false;
    }

    moduleStart(p);
    _out << sp << nl << "namespace " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::Gen::DispatcherVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    moduleEnd(p);
}

bool
Slice::Gen::DispatcherVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);

    _out << sp;
    _out << nl << "public abstract class " << name << "Disp_ : Ice.ObjectImpl, " << name;

    _out << sb;
    for (const auto& op : p->allOperations())
    {
        string retS;
        vector<string> params, args;
        string opName = getDispatchParams(op, retS, params, args, ns);
        _out << sp << nl << "public abstract " << retS << " " << opName << spar << params << epar << ';';
    }

    _out << sp;
    _out << nl << "public override string ice_id(Ice.Current current) => ice_staticId();";

    _out << sp;
    _out << nl << "public static new string ice_staticId() => \"" << p->scoped() << "\";";

    writeDispatch(p);

    return true;
}
void
Slice::Gen::DispatcherVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb;
}

void
Slice::Gen::DispatcherVisitor::writeDispatch(const InterfaceDefPtr& p)
{
    string ns = getNamespace(p);

    OperationList allOps = p->allOperations();
    if (!allOps.empty())
    {
        _out << sp;
        _out << nl
             << "public override global::System.Threading.Tasks.ValueTask<Ice.OutgoingResponse> "
                "dispatchAsync(Ice.IncomingRequest request) =>";
        _out.inc();
        _out << nl << "request.current.operation switch";
        _out << sb;
        for (const auto& op : allOps)
        {
            string opName = op->name();
            _out << nl << '"' << opName << "\" => " << getUnqualified(op->interface(), ns) << ".iceD_" << opName
                 << "Async(this, request),";
        }
        for (const auto& opName : {"ice_id", "ice_ids", "ice_isA", "ice_ping"})
        {
            _out << nl << '"' << opName << "\" => Ice.Object.iceD_" << opName << "Async(this, request),";
        }
        _out << nl << "_ => throw new Ice.OperationNotExistException()";
        _out << eb;
        _out << ";";
        _out.dec();
    }
}
