// Copyright (c) ZeroC, Inc.

#include "JavaUtil.h"
#include "../Ice/FileUtil.h"
#include "../Slice/FileTracker.h"
#include "../Slice/MetadataValidation.h"
#include "../Slice/Util.h"
#include "Ice/StringUtil.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sys/types.h>

#ifdef _WIN32
#    include <direct.h>
#endif

#ifndef _WIN32
#    include <unistd.h>
#endif

using namespace std;
using namespace Slice;
using namespace IceInternal;

namespace
{
    static const char* builtinNameTable[] = {"Byte", "Bool", "Short", "Int", "Long", "Float", "Double", "String"};

    string typeToBufferString(const TypePtr& type)
    {
        static const char* builtinBufferTable[] = {
            "java.nio.ByteBuffer",
            "???",
            "java.nio.ShortBuffer",
            "java.nio.IntBuffer",
            "java.nio.LongBuffer",
            "java.nio.FloatBuffer",
            "java.nio.DoubleBuffer",
            "???",
            "???",
            "???",
            "???"};

        BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
        if (!builtin)
        {
            return "???";
        }
        else
        {
            return builtinBufferTable[builtin->kind()];
        }
    }

    string lookupKwd(const string& name)
    {
        //
        // Keyword list. *Must* be kept in alphabetical order. Note that checkedCast and uncheckedCast
        // are not Java keywords, but are in this list to prevent illegal code being generated if
        // someone defines Slice operations with that name.
        //
        // NOTE: Any changes made to this list must also be made in BasicStream.java.
        //
        static const string keywordList[] = {
            "abstract", "assert",      "boolean",    "break",     "byte",       "case",         "catch",
            "char",     "checkedCast", "class",      "clone",     "const",      "continue",     "default",
            "do",       "double",      "else",       "enum",      "equals",     "extends",      "false",
            "final",    "finalize",    "finally",    "float",     "for",        "getClass",     "goto",
            "hashCode", "if",          "implements", "import",    "instanceof", "int",          "interface",
            "long",     "native",      "new",        "notify",    "notifyAll",  "null",         "package",
            "permits",  "private",     "protected",  "public",    "record",     "return",       "sealed",
            "short",    "static",      "strictfp",   "super",     "switch",     "synchronized", "this",
            "throw",    "throws",      "toString",   "transient", "true",       "try",          "uncheckedCast",
            "var",      "void",        "volatile",   "wait",      "when",       "while",        "yield"};
        bool found = binary_search(&keywordList[0], &keywordList[sizeof(keywordList) / sizeof(*keywordList)], name);
        return found ? "_" + name : name;
    }
}

string
Slice::getSerialVersionUID(const ContainedPtr& p)
{
    int64_t serialVersionUID;

    // Check if the user provided their own UID value with metadata.
    if (auto meta = p->getMetadataArgs("java:serialVersionUID"))
    {
        serialVersionUID = std::stoll(*meta, nullptr, 0);
    }
    else
    {
        serialVersionUID = computeDefaultSerialVersionUID(p);
    }

    ostringstream os;
    os << "private static final long serialVersionUID = " << serialVersionUID << "L;";
    return os.str();
}

int64_t
Slice::computeDefaultSerialVersionUID(const ContainedPtr& p)
{
    string name = p->scoped();
    DataMemberList members;
    optional<string> baseName;
    if (ClassDefPtr cl = dynamic_pointer_cast<ClassDef>(p))
    {
        members = cl->dataMembers();
        baseName = (cl->base()) ? cl->base()->scoped() : "";
    }
    if (ExceptionPtr ex = dynamic_pointer_cast<Exception>(p))
    {
        members = ex->dataMembers();
        baseName = nullopt;
    }
    if (StructPtr st = dynamic_pointer_cast<Struct>(p))
    {
        members = st->dataMembers();
        baseName = nullopt;
    }

    // Actually compute the `SerialVersionUID` value.
    ostringstream os;
    os << name << ":";
    if (baseName)
    {
        os << *baseName;
    }
    os << ";";
    for (const auto& member : members)
    {
        const MetadataList metadata = member->getMetadata();
        const string typeString = JavaGenerator::typeToString(member->type(), TypeModeMember, "", metadata);
        os << member->name() << ":" << typeString << ",";
    }

    // We use a custom hash instead of relying on `std::hash` to ensure cross-platform consistency.
    const string data = os.str();
    int64_t hashCode = 5381;
    for (const auto& c : data)
    {
        hashCode = ((hashCode << 5) + hashCode) ^ c;
    }
    return hashCode;
}

bool
Slice::isValidMethodParameterList(const DataMemberList& members, int additionalUnits)
{
    // The maximum length of a method parameter list is 255 units, including the implicit 'this' parameter.
    // Each parameter is 1 unit, except for long and double parameters, which are 2 units.
    // Start the length at 1 to account for the implicit 'this' parameter (plus any additional units).
    int length = 1 + additionalUnits;
    for (const auto& member : members)
    {
        BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(member->type());
        if (builtin && (builtin->kind() == Builtin::KindLong || builtin->kind() == Builtin::KindDouble))
        {
            length += 2;
        }
        else
        {
            length++;
        }
    }
    return length <= 255;
}

bool
Slice::mapsToJavaBuiltinType(const TypePtr& p)
{
    if (auto builtin = dynamic_pointer_cast<Builtin>(p))
    {
        return builtin->kind() < Builtin::KindObject;
    }
    return false;
}

Slice::JavaOutput::JavaOutput() = default;

Slice::JavaOutput::JavaOutput(ostream& os) : Output(os) {}

Slice::JavaOutput::JavaOutput(const char* s) : Output(s) {}

void
Slice::JavaOutput::openClass(const string& cls, const string& prefix, const string& sliceFile)
{
    string package;
    string file;
    string path = prefix;

    string::size_type pos = cls.rfind('.');
    if (pos != string::npos)
    {
        package = cls.substr(0, pos);
        file = cls.substr(pos + 1);
        string dir = package;

        //
        // Create package directories if necessary.
        //
        string::size_type start = 0;
        do
        {
            if (!path.empty())
            {
                path += "/";
            }
            pos = dir.find('.', start);
            if (pos != string::npos)
            {
                path += dir.substr(start, pos - start);
                start = pos + 1;
            }
            else
            {
                path += dir.substr(start);
            }

            IceInternal::structstat st;
            if (!IceInternal::stat(path, &st))
            {
                if (!(st.st_mode & S_IFDIR))
                {
                    ostringstream os;
                    os << "failed to create package directory '" << path
                       << "': file already exists and is not a directory";
                    throw FileException(__FILE__, __LINE__, os.str());
                }
                continue;
            }

            int err = IceInternal::mkdir(path, 0777);
            // If slice2java is run concurrently, it's possible that another instance of slice2java has already
            // created the directory.
            if (err == 0 || (errno == EEXIST && IceInternal::directoryExists(path)))
            {
                // Directory successfully created or already exists.
            }
            else
            {
                ostringstream os;
                os << "cannot create directory '" << path << "': " << IceInternal::errorToString(errno);
                throw FileException(__FILE__, __LINE__, os.str());
            }
            FileTracker::instance()->addDirectory(path);
        } while (pos != string::npos);
    }
    else
    {
        file = cls;
    }
    file += ".java";

    //
    // Open class file.
    //
    if (!path.empty())
    {
        path += "/";
    }
    path += file;

    open(path.c_str());
    if (isOpen())
    {
        FileTracker::instance()->addFile(path);
        printHeader();
        printGeneratedHeader(*this, sliceFile);
        if (!package.empty())
        {
            separator();
            print("package ");
            print(package.c_str());
            print(";");
        }
    }
    else
    {
        ostringstream os;
        os << "cannot open file '" << path << "': " << IceInternal::errorToString(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
}

void
Slice::JavaOutput::printHeader()
{
    print("// Copyright (c) ZeroC, Inc.\n");
    print("//\n");
    print("// Ice version ");
    print(ICE_STRING_VERSION);
    print("\n");
    print("//\n");
}

Slice::JavaGenerator::JavaGenerator(string dir) : _dir(std::move(dir)) {}

Slice::JavaGenerator::~JavaGenerator()
{
    // If open throws an exception other generators could be left open
    // during the stack unwind.
    if (_out != nullptr)
    {
        close();
    }
    assert(_out == nullptr);
}

void
Slice::JavaGenerator::open(const string& absolute, const string& file)
{
    assert(_out == nullptr);

    JavaOutput* out = createOutput();
    try
    {
        out->openClass(absolute, _dir, file);
    }
    catch (const FileException&)
    {
        delete out;
        throw;
    }
    _out = out;
}

void
Slice::JavaGenerator::close()
{
    assert(_out != nullptr);
    *_out << nl;
    delete _out;
    _out = nullptr;
}

Output&
Slice::JavaGenerator::output() const
{
    assert(_out != nullptr);
    return *_out;
}

//
// If the passed name is a scoped name, return the identical scoped name,
// but with all components that are Java keywords replaced by
// their "_"-prefixed version; otherwise, if the passed name is
// not scoped, but a Java keyword, return the "_"-prefixed name;
// otherwise, return the name unchanged.
//
string
Slice::JavaGenerator::fixKwd(const string& name)
{
    if (name.empty())
    {
        return name;
    }
    if (name[0] != ':')
    {
        return lookupKwd(name);
    }
    vector<string> ids = splitScopedName(name);
    transform(ids.begin(), ids.end(), ids.begin(), [](const string& id) -> string { return lookupKwd(id); });
    stringstream result;
    for (const auto& id : ids)
    {
        result << "::" + id;
    }
    return result.str();
}

string
Slice::JavaGenerator::convertScopedName(const string& scoped, const string& prefix, const string& suffix)
{
    string result;
    string::size_type start = 0;
    string fscoped = fixKwd(scoped);

    //
    // Skip leading "::"
    //
    if (fscoped[start] == ':')
    {
        assert(fscoped[start + 1] == ':');
        start += 2;
    }

    //
    // Convert all occurrences of "::" to "."
    //
    string::size_type pos;
    do
    {
        pos = fscoped.find(':', start);
        string fix;
        if (pos == string::npos)
        {
            string s = fscoped.substr(start);
            if (!s.empty())
            {
                fix = prefix + fixKwd(s) + suffix;
            }
        }
        else
        {
            assert(fscoped[pos + 1] == ':');
            fix = fixKwd(fscoped.substr(start, pos - start));
            start = pos + 2;
        }

        if (!result.empty() && !fix.empty())
        {
            result += ".";
        }
        result += fix;
    } while (pos != string::npos);

    return result;
}

string
Slice::JavaGenerator::getPackagePrefix(const ContainedPtr& cont)
{
    //
    // Traverse to the top-level module.
    //
    ModulePtr m;
    ContainedPtr p = cont;
    while (true)
    {
        if (dynamic_pointer_cast<Module>(p))
        {
            m = dynamic_pointer_cast<Module>(p);
        }

        ContainerPtr c = p->container();
        p = dynamic_pointer_cast<Contained>(c); // This cast fails for Unit.
        if (!p)
        {
            break;
        }
    }

    assert(m);

    // The 'java:package' metadata can be defined as file metadata or applied to a top-level module.
    // We check for the metadata at the top-level module first and then fall back to the global scope.
    if (auto metadataArgs = m->getMetadataArgs("java:package"))
    {
        return *metadataArgs;
    }
    string file = cont->file();
    DefinitionContextPtr dc = cont->unit()->findDefinitionContext(file);
    assert(dc);
    return dc->getMetadataArgs("java:package").value_or("");
}

string
Slice::JavaGenerator::getPackage(const ContainedPtr& cont)
{
    string scope = convertScopedName(cont->scope());
    string prefix = getPackagePrefix(cont);
    if (!prefix.empty())
    {
        if (!scope.empty())
        {
            return prefix + "." + scope;
        }
        else
        {
            return prefix;
        }
    }

    return scope;
}

string
Slice::JavaGenerator::getUnqualified(const std::string& type, const std::string& package)
{
    if (type.find('.') != string::npos && type.find(package) == 0 && type.find('.', package.size() + 1) == string::npos)
    {
        return type.substr(package.size() + 1);
    }
    return type;
}

string
Slice::JavaGenerator::getUnqualified(
    const ContainedPtr& cont,
    const string& package,
    const string& prefix,
    const string& suffix)
{
    string name = cont->name();
    if (prefix == "" && suffix == "")
    {
        name = fixKwd(name);
    }
    string contPkg = getPackage(cont);
    if (contPkg == package)
    {
        return prefix + name + suffix;
    }
    else if (!contPkg.empty())
    {
        return contPkg + "." + prefix + name + suffix;
    }
    else
    {
        return prefix + name + suffix;
    }
}

string
Slice::JavaGenerator::getStaticId(const TypePtr& type, const string& package)
{
    BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);

    assert((b && b->usesClasses()) || cl);

    if (b && b->kind() == Builtin::KindObject)
    {
        return "com.zeroc.Ice.Object.ice_staticId()";
    }
    else if (b && b->kind() == Builtin::KindValue)
    {
        return "com.zeroc.Ice.Value.ice_staticId()";
    }
    else
    {
        return getUnqualified(cl, package) + ".ice_staticId()";
    }
}

string
Slice::JavaGenerator::getOptionalFormat(const TypePtr& type)
{
    return "com.zeroc.Ice.OptionalFormat." + type->getOptionalFormat();
}

string
Slice::JavaGenerator::typeToString(
    const TypePtr& type,
    TypeMode mode,
    const string& package,
    const MetadataList& metadata,
    bool formal,
    bool optional)
{
    static const char* builtinTable[] = {
        "byte",
        "boolean",
        "short",
        "int",
        "long",
        "float",
        "double",
        "String",
        "com.zeroc.Ice.Object",
        "com.zeroc.Ice.ObjectPrx",
        "com.zeroc.Ice.Value"};

    static const char* builtinOptionalTable[] = {
        "java.util.Optional<java.lang.Byte>",
        "java.util.Optional<java.lang.Boolean>",
        "java.util.Optional<java.lang.Short>",
        "java.util.OptionalInt",
        "java.util.OptionalLong",
        "java.util.Optional<java.lang.Float>",
        "java.util.OptionalDouble",
        "???",
        "???",
        "???",
        "???"};

    if (!type)
    {
        assert(mode == TypeModeReturn);
        return "void";
    }

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        if (optional)
        {
            switch (builtin->kind())
            {
                case Builtin::KindByte:
                case Builtin::KindBool:
                case Builtin::KindShort:
                case Builtin::KindInt:
                case Builtin::KindLong:
                case Builtin::KindFloat:
                case Builtin::KindDouble:
                {
                    return getUnqualified(builtinOptionalTable[builtin->kind()], package);
                }
                case Builtin::KindString:
                case Builtin::KindObject:
                case Builtin::KindObjectProxy:
                case Builtin::KindValue:
                {
                    break;
                }
            }
        }
        else
        {
            if (builtin->kind() == Builtin::KindObject)
            {
                return getUnqualified(builtinTable[Builtin::KindValue], package);
            }
            else
            {
                return getUnqualified(builtinTable[builtin->kind()], package);
            }
        }
    }

    if (optional)
    {
        return "java.util.Optional<" + typeToObjectString(type, mode, package, metadata, formal) + ">";
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        return getUnqualified(cl, package);
    }

    InterfaceDeclPtr proxy = dynamic_pointer_cast<InterfaceDecl>(type);
    if (proxy)
    {
        return getUnqualified(proxy, package, "", "Prx");
    }

    DictionaryPtr dict = dynamic_pointer_cast<Dictionary>(type);
    if (dict)
    {
        string instanceType, formalType;
        getDictionaryTypes(dict, package, metadata, instanceType, formalType);
        return formal ? formalType : instanceType;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        string instanceType, formalType;
        getSequenceTypes(seq, package, metadata, instanceType, formalType);
        return formal ? formalType : instanceType;
    }

    ContainedPtr contained = dynamic_pointer_cast<Contained>(type);
    if (contained)
    {
        if (mode == TypeModeOut)
        {
            return getUnqualified(contained, package, "", "Holder");
        }
        else
        {
            return getUnqualified(contained, package);
        }
    }

    return "???";
}

string
Slice::JavaGenerator::typeToObjectString(
    const TypePtr& type,
    TypeMode mode,
    const string& package,
    const MetadataList& metadata,
    bool formal)
{
    static const char* builtinTable[] = {
        "java.lang.Byte",
        "java.lang.Boolean",
        "java.lang.Short",
        "java.lang.Integer",
        "java.lang.Long",
        "java.lang.Float",
        "java.lang.Double",
        "java.lang.String",
        "com.zeroc.Ice.Value",
        "com.zeroc.Ice.ObjectPrx",
        "com.zeroc.Ice.Value"};

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin && mode != TypeModeOut)
    {
        return builtinTable[builtin->kind()];
    }

    return typeToString(type, mode, package, metadata, formal, false);
}

void
Slice::JavaGenerator::writeMarshalUnmarshalCode(
    Output& out,
    const string& package,
    const TypePtr& type,
    OptionalMode mode,
    bool optionalMapping,
    int tag,
    const string& param,
    bool marshal,
    int& iter,
    const string& customStream,
    const MetadataList& metadata,
    const string& patchParams)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    const bool optionalParam = mode == OptionalInParam || mode == OptionalOutParam || mode == OptionalReturnParam;
    string typeS = typeToString(type, TypeModeIn, package, metadata);

    assert(!marshal || mode != OptionalMember); // Only support OptionalMember for un-marshaling

    const BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindByte:
            case Builtin::KindBool:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            case Builtin::KindFloat:
            case Builtin::KindDouble:
            case Builtin::KindString:
            {
                string s = builtinNameTable[builtin->kind()];
                if (marshal)
                {
                    if (optionalParam)
                    {
                        out << nl << stream << ".write" << s << "(" << tag << ", " << param << ");";
                    }
                    else
                    {
                        out << nl << stream << ".write" << s << "(" << param << ");";
                    }
                }
                else
                {
                    if (optionalParam)
                    {
                        out << nl << param << " = " << stream << ".read" << s << "(" << tag << ");";
                    }
                    else
                    {
                        out << nl << param << " = " << stream << ".read" << s << "();";
                    }
                }
                return;
            }
            case Builtin::KindObject:
            case Builtin::KindValue:
            {
                // Handled by isClassType below.
                break;
            }
            case Builtin::KindObjectProxy:
            {
                if (marshal)
                {
                    if (optionalParam)
                    {
                        out << nl << stream << ".writeProxy(" << tag << ", " << param << ");";
                    }
                    else
                    {
                        out << nl << stream << ".writeProxy(" << param << ");";
                    }
                }
                else
                {
                    if (optionalParam)
                    {
                        out << nl << param << " = " << stream << ".readProxy(" << tag << ");";
                    }
                    else if (mode == OptionalMember)
                    {
                        out << nl << stream << ".skip(4);";
                        out << nl << param << " = " << stream << ".readProxy();";
                    }
                    else
                    {
                        out << nl << param << " = " << stream << ".readProxy();";
                    }
                }
                return;
            }
        }
    }

    InterfaceDeclPtr prx = dynamic_pointer_cast<InterfaceDecl>(type);
    if (prx)
    {
        if (marshal)
        {
            if (optionalParam)
            {
                out << nl << stream << ".writeProxy(" << tag << ", " << param << ");";
            }
            else
            {
                out << nl << stream << ".writeProxy(" << param << ");";
            }
        }
        else
        {
            if (optionalParam)
            {
                out << nl << param << " = " << stream << ".readProxy(" << tag << ", " << typeS << "::uncheckedCast);";
            }
            else if (mode == OptionalMember)
            {
                out << nl << stream << ".skip(4);";
                out << nl << param << " = " << typeS << ".uncheckedCast(" << stream << ".readProxy());";
            }
            else
            {
                out << nl << param << " = " << typeS << ".uncheckedCast(" << stream << ".readProxy());";
            }
        }
        return;
    }

    if (type->isClassType())
    {
        assert(!optionalParam); // Optional classes are disallowed by the parser.
        if (marshal)
        {
            out << nl << stream << ".writeValue(" << param << ");";
        }
        else
        {
            assert(!patchParams.empty());
            out << nl << stream << ".readValue(" << patchParams << ");";
        }
        return;
    }

    DictionaryPtr dict = dynamic_pointer_cast<Dictionary>(type);
    if (dict)
    {
        if (optionalParam || mode == OptionalMember)
        {
            string instanceType, formalType, origInstanceType, origFormalType;
            getDictionaryTypes(dict, "", metadata, instanceType, formalType);
            getDictionaryTypes(dict, "", MetadataList(), origInstanceType, origFormalType);
            if (formalType == origFormalType && (marshal || instanceType == origInstanceType))
            {
                //
                // If we can use the helper, it's easy.
                //
                string helper = getUnqualified(dict, package, "", "Helper");
                if (marshal)
                {
                    out << nl << helper << ".write" << spar << stream << tag << param << epar << ";";
                    return;
                }
                else if (mode != OptionalMember)
                {
                    out << nl << param << " = " << helper << ".read" << spar << stream << tag << epar << ";";
                    return;
                }
            }

            TypePtr keyType = dict->keyType();
            TypePtr valueType = dict->valueType();
            if (marshal)
            {
                if (optionalParam)
                {
                    out << nl;
                    if (optionalMapping)
                    {
                        out << "if(" << param << " != null && " << param << ".isPresent() && " << stream
                            << ".writeOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    }
                    else
                    {
                        out << "if(" << stream << ".writeOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    }
                    out << sb;
                }

                if (keyType->isVariableLength() || valueType->isVariableLength())
                {
                    string d = optionalParam && optionalMapping ? param + ".get()" : param;
                    out << nl << "int pos = " << stream << ".startSize();";
                    writeDictionaryMarshalUnmarshalCode(
                        out,
                        package,
                        dict,
                        d,
                        marshal,
                        iter,
                        true,
                        customStream,
                        metadata);
                    out << nl << stream << ".endSize(pos);";
                }
                else
                {
                    const size_t sz = keyType->minWireSize() + valueType->minWireSize();
                    string d = optionalParam && optionalMapping ? param + ".get()" : param;
                    out << nl << "final int optSize = " << d << " == null ? 0 : " << d << ".size();";
                    out << nl << stream << ".writeSize(optSize > 254 ? optSize * " << sz << " + 5 : optSize * " << sz
                        << " + 1);";
                    writeDictionaryMarshalUnmarshalCode(
                        out,
                        package,
                        dict,
                        d,
                        marshal,
                        iter,
                        true,
                        customStream,
                        metadata);
                }

                if (optionalParam)
                {
                    out << eb;
                }
            }
            else
            {
                string d = optionalParam ? "optDict" : param;
                if (optionalParam)
                {
                    out << nl << "if(" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    out << sb;
                    out << nl << typeS << ' ' << d << ';';
                }
                if (keyType->isVariableLength() || valueType->isVariableLength())
                {
                    out << nl << stream << ".skip(4);";
                }
                else
                {
                    out << nl << stream << ".skipSize();";
                }
                writeDictionaryMarshalUnmarshalCode(out, package, dict, d, marshal, iter, true, customStream, metadata);
                if (optionalParam)
                {
                    out << nl << param << " = java.util.Optional.of(" << d << ");";
                    out << eb;
                    out << nl << "else";
                    out << sb;
                    out << nl << param << " = java.util.Optional.empty();";
                    out << eb;
                }
            }
        }
        else
        {
            writeDictionaryMarshalUnmarshalCode(out, package, dict, param, marshal, iter, true, customStream, metadata);
        }
        return;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        if (optionalParam || mode == OptionalMember)
        {
            TypePtr elemType = seq->type();
            BuiltinPtr eltBltin = dynamic_pointer_cast<Builtin>(elemType);
            if (!hasTypeMetadata(seq, metadata) && mapsToJavaBuiltinType(eltBltin))
            {
                string bs = builtinNameTable[eltBltin->kind()];
                if (marshal)
                {
                    out << nl << stream << ".write" << bs << "Seq(" << tag << ", " << param << ");";
                    return;
                }
                else if (mode != OptionalMember)
                {
                    out << nl << param << " = " << stream << ".read" << bs << "Seq(" << tag << ");";
                    return;
                }
            }
            else if (seq->hasMetadata("java:serializable"))
            {
                if (marshal)
                {
                    out << nl << stream << ".writeSerializable" << spar << tag << param << epar << ";";
                    return;
                }
                else if (mode != OptionalMember)
                {
                    out << nl << param << " = " << stream << ".readSerializable" << spar << tag << typeS + ".class"
                        << epar << ";";
                    return;
                }
            }
            // Check if either 1) No type metadata was applied to this sequence at all or 2) 'java:type' was applied to
            // where the sequence is used (which overrides any metadata on the definition) or 3) 'java:type' was applied
            // to the sequence definition, and there is no metadata overriding it where the sequence is used.
            else if (
                !hasTypeMetadata(seq, metadata) || hasMetadata("java:type", metadata) ||
                (seq->hasMetadata("java:type") && !hasMetadata("java:buffer", metadata)))
            {
                string instanceType, formalType, origInstanceType, origFormalType;
                getSequenceTypes(seq, "", metadata, instanceType, formalType);
                getSequenceTypes(seq, "", MetadataList(), origInstanceType, origFormalType);
                if (formalType == origFormalType && (marshal || instanceType == origInstanceType))
                {
                    string helper = getUnqualified(seq, package, "", "Helper");
                    if (marshal)
                    {
                        out << nl << helper << ".write" << spar << stream << tag << param << epar << ";";
                        return;
                    }
                    else if (mode != OptionalMember)
                    {
                        out << nl << param << " = " << helper << ".read" << spar << stream << tag << epar << ";";
                        return;
                    }
                }
            }

            if (marshal)
            {
                if (optionalParam)
                {
                    out << nl;
                    if (optionalMapping)
                    {
                        out << "if(" << param << " != null && " << param << ".isPresent() && " << stream
                            << ".writeOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    }
                    else
                    {
                        out << "if(" << stream << ".writeOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    }
                    out << sb;
                }

                if (elemType->isVariableLength())
                {
                    string s = optionalParam && optionalMapping ? param + ".get()" : param;
                    out << nl << "int pos = " << stream << ".startSize();";
                    writeSequenceMarshalUnmarshalCode(out, package, seq, s, true, iter, true, customStream, metadata);
                    out << nl << stream << ".endSize(pos);";
                }
                else
                {
                    const size_t sz = elemType->minWireSize();
                    string s = optionalParam && optionalMapping ? param + ".get()" : param;
                    if (sz > 1)
                    {
                        out << nl << "final int optSize = " << s << " == null ? 0 : ";

                        // Check the local metadata before checking metadata on the sequence definition.
                        if (hasMetadata("java:type", metadata))
                        {
                            out << s << ".size();";
                        }
                        else if (hasMetadata("java:buffer", metadata) || seq->hasMetadata("java:buffer"))
                        {
                            out << s << ".remaining() / " << sz << ";";
                        }
                        else if (seq->hasMetadata("java:type"))
                        {
                            out << s << ".size();";
                        }
                        else
                        {
                            out << s << ".length;";
                        }
                        out << nl << stream << ".writeSize(optSize > 254 ? optSize * " << sz << " + 5 : optSize * "
                            << sz << " + 1);";
                    }
                    writeSequenceMarshalUnmarshalCode(out, package, seq, s, true, iter, true, customStream, metadata);
                }

                if (optionalParam)
                {
                    out << eb;
                }
            }
            else
            {
                const size_t sz = elemType->minWireSize();
                string s = optionalParam ? "optSeq" : param;
                if (optionalParam)
                {
                    out << nl << "if(" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(type) << "))";
                    out << sb;
                    out << nl << typeS << ' ' << s << ';';
                }
                if (elemType->isVariableLength())
                {
                    out << nl << stream << ".skip(4);";
                }
                else if (sz > 1)
                {
                    out << nl << stream << ".skipSize();";
                }
                writeSequenceMarshalUnmarshalCode(out, package, seq, s, false, iter, true, customStream, metadata);
                if (optionalParam)
                {
                    out << nl << param << " = java.util.Optional.of(" << s << ");";
                    out << eb;
                    out << nl << "else";
                    out << sb;
                    out << nl << param << " = java.util.Optional.empty();";
                    out << eb;
                }
            }
        }
        else
        {
            writeSequenceMarshalUnmarshalCode(out, package, seq, param, marshal, iter, true, customStream, metadata);
        }
        return;
    }

    ConstructedPtr constructed = dynamic_pointer_cast<Constructed>(type);
    StructPtr st = dynamic_pointer_cast<Struct>(type);
    assert(constructed);
    if (marshal)
    {
        if (optionalParam)
        {
            out << nl << typeS << ".ice_write(" << stream << ", " << tag << ", " << param << ");";
        }
        else
        {
            out << nl << typeS << ".ice_write(" << stream << ", " << param << ");";
        }
    }
    else
    {
        if (optionalParam)
        {
            out << nl << param << " = " << typeS << ".ice_read(" << stream << ", " << tag << ");";
        }
        else if (mode == OptionalMember && st)
        {
            out << nl << stream << (st->isVariableLength() ? ".skip(4);" : ".skipSize();");
            out << nl << param << " = " << typeS << ".ice_read(" << stream << ");";
        }
        else
        {
            out << nl << param << " = " << typeS << ".ice_read(" << stream << ");";
        }
    }
}

void
Slice::JavaGenerator::writeDictionaryMarshalUnmarshalCode(
    Output& out,
    const string& package,
    const DictionaryPtr& dict,
    const string& param,
    bool marshal,
    int& iter,
    bool useHelper,
    const string& customStream,
    const MetadataList& metadata)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    //
    // We have to determine whether it's possible to use the
    // type's generated helper class for this marshal/unmarshal
    // task. Since the user may have specified a custom type in
    // metadata, it's possible that the helper class is not
    // compatible and therefore we'll need to generate the code
    // in-line instead.
    //
    // Specifically, there may be "local" metadata (i.e., from
    // a data member or parameter definition) that overrides the
    // original type. We'll compare the mapped types with and
    // without local metadata to determine whether we can use
    // the helper.
    //
    string instanceType, formalType, origInstanceType, origFormalType;
    getDictionaryTypes(dict, "", metadata, instanceType, formalType);
    getDictionaryTypes(dict, "", MetadataList(), origInstanceType, origFormalType);
    if (useHelper && formalType == origFormalType && (marshal || instanceType == origInstanceType))
    {
        //
        // If we can use the helper, it's easy.
        //
        string helper = getUnqualified(dict, package, "", "Helper");
        if (marshal)
        {
            out << nl << helper << ".write" << spar << stream << param << epar << ";";
        }
        else
        {
            out << nl << param << " = " << helper << ".read" << spar << stream << epar << ";";
        }
        return;
    }

    TypePtr key = dict->keyType();
    TypePtr value = dict->valueType();

    string keyS = typeToString(key, TypeModeIn, package);
    string valueS = typeToString(value, TypeModeIn, package);

    ostringstream o;
    o << iter;
    string iterS = o.str();
    iter++;

    if (marshal)
    {
        out << nl << "if(" << param << " == null)";
        out << sb;
        out << nl << "ostr.writeSize(0);";
        out << eb;
        out << nl << "else";
        out << sb;
        out << nl << "ostr.writeSize(" << param << ".size());";
        string keyObjectS = typeToObjectString(key, TypeModeIn, package);
        string valueObjectS = typeToObjectString(value, TypeModeIn, package);
        out << nl;
        out << "for(java.util.Map.Entry<" << keyObjectS << ", " << valueObjectS << "> e : " << param << ".entrySet())";
        out << sb;
        for (int i = 0; i < 2; i++)
        {
            string arg;
            TypePtr type;
            if (i == 0)
            {
                arg = "e.getKey()";
                type = key;
            }
            else
            {
                arg = "e.getValue()";
                type = value;
            }
            writeMarshalUnmarshalCode(out, package, type, OptionalNone, false, 0, arg, true, iter, customStream);
        }
        out << eb;
        out << eb;
    }
    else
    {
        out << nl << param << " = new " << instanceType << "();";
        out << nl << "int sz" << iterS << " = " << stream << ".readSize();";
        out << nl << "for(int i" << iterS << " = 0; i" << iterS << " < sz" << iterS << "; i" << iterS << "++)";
        out << sb;

        if (value->isClassType())
        {
            out << nl << "final " << keyS << " key;";
            writeMarshalUnmarshalCode(out, package, key, OptionalNone, false, 0, "key", false, iter, customStream);

            valueS = typeToObjectString(value, TypeModeIn, package);
            ostringstream patchParams;
            patchParams << "value -> " << param << ".put(key, value), " << valueS << ".class";
            writeMarshalUnmarshalCode(
                out,
                package,
                value,
                OptionalNone,
                false,
                0,
                "value",
                false,
                iter,
                customStream,
                MetadataList(),
                patchParams.str());
        }
        else
        {
            out << nl << keyS << " key;";
            writeMarshalUnmarshalCode(out, package, key, OptionalNone, false, 0, "key", false, iter, customStream);

            out << nl << valueS << " value;";
            writeMarshalUnmarshalCode(out, package, value, OptionalNone, false, 0, "value", false, iter, customStream);

            out << nl << "" << param << ".put(key, value);";
        }
        out << eb;
    }
}

void
Slice::JavaGenerator::writeSequenceMarshalUnmarshalCode(
    Output& out,
    const string& package,
    const SequencePtr& seq,
    const string& param,
    bool marshal,
    int& iter,
    bool useHelper,
    const string& customStream,
    const MetadataList& metadata)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    string typeS = typeToString(seq, TypeModeIn, package);

    // Check for the serializable metadata to get rid of this case first.
    if (seq->hasMetadata("java:serializable"))
    {
        if (marshal)
        {
            out << nl << stream << ".writeSerializable(" << param << ");";
        }
        else
        {
            out << nl << param << " = " << stream << ".readSerializable(" << typeS << ".class);";
        }
        return;
    }

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(seq->type());
    static const string bytebuffer = "java:buffer";
    if ((seq->hasMetadata(bytebuffer) || hasMetadata(bytebuffer, metadata)) && !hasMetadata("java:type", metadata))
    {
        if (marshal)
        {
            out << nl << stream << ".write" << builtinNameTable[builtin->kind()] << "Buffer(" << param << ");";
        }
        else
        {
            out << nl << param << " = " << stream << ".read" << builtinNameTable[builtin->kind()] << "Buffer();";
        }
        return;
    }

    if (!hasTypeMetadata(seq, metadata) && mapsToJavaBuiltinType(builtin))
    {
        if (marshal)
        {
            out << nl << stream << ".write" << builtinNameTable[builtin->kind()] << "Seq(" << param << ");";
        }
        else
        {
            out << nl << param << " = " << stream << ".read" << builtinNameTable[builtin->kind()] << "Seq();";
        }
        return;
    }

    //
    // We have to determine whether it's possible to use the
    // type's generated helper class for this marshal/unmarshal
    // task. Since the user may have specified a custom type in
    // metadata, it's possible that the helper class is not
    // compatible and therefore we'll need to generate the code
    // in-line instead.
    //
    // Specifically, there may be "local" metadata (i.e., from
    // a data member or parameter definition) that overrides the
    // original type. We'll compare the mapped types with and
    // without local metadata to determine whether we can use
    // the helper.
    //
    string instanceType, formalType, origInstanceType, origFormalType;
    bool customType = getSequenceTypes(seq, "", metadata, instanceType, formalType);
    getSequenceTypes(seq, "", MetadataList(), origInstanceType, origFormalType);
    if (useHelper && formalType == origFormalType && (marshal || instanceType == origInstanceType))
    {
        //
        // If we can use the helper, it's easy.
        //
        string helper = getUnqualified(seq, package, "", "Helper");
        if (marshal)
        {
            out << nl << helper << ".write" << spar << stream << param << epar << ";";
        }
        else
        {
            out << nl << param << " = " << helper << ".read" << spar << stream << epar << ";";
        }
        return;
    }

    //
    // Determine sequence depth.
    //
    int depth = 0;
    TypePtr origContent = seq->type();
    SequencePtr s = dynamic_pointer_cast<Sequence>(origContent);
    while (s)
    {
        //
        // Stop if the inner sequence type has a custom, or serializable type.
        //
        if (hasTypeMetadata(s))
        {
            break;
        }
        depth++;
        origContent = s->type();
        s = dynamic_pointer_cast<Sequence>(origContent);
    }
    string origContentS = typeToString(origContent, TypeModeIn, package);

    TypePtr type = seq->type();

    if (customType)
    {
        //
        // Marshal/unmarshal a custom sequence type.
        //
        BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
        typeS = getUnqualified(seq, package);
        ostringstream o;
        o << origContentS;
        int d = depth;
        while (d--)
        {
            o << "[]";
        }
        string cont = o.str();
        if (marshal)
        {
            out << nl << "if(" << param << " == null)";
            out << sb;
            out << nl << stream << ".writeSize(0);";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << stream << ".writeSize(" << param << ".size());";
            string ctypeS = typeToString(type, TypeModeIn, package);
            out << nl << "for(" << ctypeS << " elem : " << param << ')';
            out << sb;
            writeMarshalUnmarshalCode(out, package, type, OptionalNone, false, 0, "elem", true, iter, customStream);
            out << eb;
            out << eb; // else
        }
        else
        {
            out << nl << param << " = new " << instanceType << "();";
            out << nl << "final int len" << iter << " = " << stream << ".readAndCheckSeqSize(" << type->minWireSize()
                << ");";
            out << nl << "for(int i" << iter << " = 0; i" << iter << " < len" << iter << "; i" << iter << "++)";
            out << sb;
            if (type->isClassType())
            {
                //
                // Add a null value to the list as a placeholder for the element.
                //
                out << nl << param << ".add(null);";
                ostringstream patchParams;
                out << nl << "final int fi" << iter << " = i" << iter << ";";
                patchParams << "value -> " << param << ".set(fi" << iter << ", value), " << origContentS << ".class";

                writeMarshalUnmarshalCode(
                    out,
                    package,
                    type,
                    OptionalNone,
                    false,
                    0,
                    "elem",
                    false,
                    iter,
                    customStream,
                    MetadataList(),
                    patchParams.str());
            }
            else
            {
                out << nl << cont << " elem;";
                writeMarshalUnmarshalCode(
                    out,
                    package,
                    type,
                    OptionalNone,
                    false,
                    0,
                    "elem",
                    false,
                    iter,
                    customStream);
                out << nl << param << ".add(elem);";
            }
            out << eb;
            iter++;
        }
    }
    else
    {
        BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
        if (mapsToJavaBuiltinType(b))
        {
            string_view kindName = builtinNameTable[b->kind()];
            if (marshal)
            {
                out << nl << stream << ".write" << kindName << "Seq(" << param << ");";
            }
            else
            {
                out << nl << param << " = " << stream << ".read" << kindName << "Seq();";
            }
        }
        else
        {
            if (marshal)
            {
                out << nl << "if(" << param << " == null)";
                out << sb;
                out << nl << stream << ".writeSize(0);";
                out << eb;
                out << nl << "else";
                out << sb;
                out << nl << stream << ".writeSize(" << param << ".length);";
                out << nl << "for(int i" << iter << " = 0; i" << iter << " < " << param << ".length; i" << iter
                    << "++)";
                out << sb;
                ostringstream o;
                o << param << "[i" << iter << "]";
                iter++;
                writeMarshalUnmarshalCode(
                    out,
                    package,
                    type,
                    OptionalNone,
                    false,
                    0,
                    o.str(),
                    true,
                    iter,
                    customStream);
                out << eb;
                out << eb;
            }
            else
            {
                bool isObject = false;
                ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(origContent);
                if ((b && b->usesClasses()) || cl)
                {
                    isObject = true;
                }
                out << nl << "final int len" << iter << " = " << stream << ".readAndCheckSeqSize("
                    << type->minWireSize() << ");";
                //
                // We cannot allocate an array of a generic type, such as
                //
                // arr = new Map<String, String>[sz];
                //
                // Attempting to compile this code results in a "generic array creation" error
                // message. This problem can occur when the sequence's element type is a
                // dictionary, or when the element type is a nested sequence that uses a custom
                // mapping.
                //
                // The solution is to rewrite the code as follows:
                //
                // arr = (Map<String, String>[])new Map[sz];
                //
                // Unfortunately, this produces an unchecked warning during compilation.
                //
                // A simple test is to look for a "<" character in the content type, which
                // indicates the use of a generic type.
                //
                string::size_type pos = origContentS.find('<');
                if (pos != string::npos)
                {
                    string nonGenericType = origContentS.substr(0, pos);
                    out << nl << param << " = (" << origContentS << "[]";
                    int d = depth;
                    while (d--)
                    {
                        out << "[]";
                    }
                    out << ")new " << nonGenericType << "[len" << iter << "]";
                }
                else
                {
                    out << nl << param << " = new " << origContentS << "[len" << iter << "]";
                }
                int d = depth;
                while (d--)
                {
                    out << "[]";
                }
                out << ';';
                out << nl << "for(int i" << iter << " = 0; i" << iter << " < len" << iter << "; i" << iter << "++)";
                out << sb;
                ostringstream o;
                o << param << "[i" << iter << "]";
                if (isObject)
                {
                    ostringstream patchParams;
                    out << nl << "final int fi" << iter << " = i" << iter << ";";
                    patchParams << "value -> " << param << "[fi" << iter << "] = value, " << origContentS << ".class";
                    writeMarshalUnmarshalCode(
                        out,
                        package,
                        type,
                        OptionalNone,
                        false,
                        0,
                        o.str(),
                        false,
                        iter,
                        customStream,
                        MetadataList(),
                        patchParams.str());
                }
                else
                {
                    writeMarshalUnmarshalCode(
                        out,
                        package,
                        type,
                        OptionalNone,
                        false,
                        0,
                        o.str(),
                        false,
                        iter,
                        customStream);
                }
                out << eb;
                iter++;
            }
        }
    }
}

bool
Slice::JavaGenerator::hasMetadata(const string& directive, const MetadataList& metadata)
{
    for (const auto& m : metadata)
    {
        if (m->directive() == directive)
        {
            return true;
        }
    }

    return false;
}

bool
Slice::JavaGenerator::getTypeMetadata(const MetadataList& metadata, string& instanceType, string& formalType)
{
    //
    // Extract the instance type and an optional formal type.
    // The correct syntax is "java:type:instance-type[:formal-type]".
    //
    for (const auto& m : metadata)
    {
        if (m->directive() == "java:type")
        {
            string arguments = m->arguments();
            string::size_type pos = arguments.find(':');
            if (pos != string::npos)
            {
                instanceType = arguments.substr(0, pos);
                formalType = arguments.substr(pos + 1);
            }
            else
            {
                instanceType = std::move(arguments);
                formalType.clear();
            }
            return true;
        }
    }

    return false;
}

bool
Slice::JavaGenerator::hasTypeMetadata(const SequencePtr& seq, const MetadataList& localMetadata)
{
    if (seq->hasMetadata("java:type") || seq->hasMetadata("java:buffer") || seq->hasMetadata("java:serializable"))
    {
        return true;
    }
    if (hasMetadata("java:type", localMetadata) || hasMetadata("java:buffer", localMetadata))
    {
        return true;
    }
    return false;
}

bool
Slice::JavaGenerator::getDictionaryTypes(
    const DictionaryPtr& dict,
    const string& package,
    const MetadataList& metadata,
    string& instanceType,
    string& formalType)
{
    //
    // Get the types of the key and value.
    //
    string keyTypeStr = typeToObjectString(dict->keyType(), TypeModeIn, package, MetadataList(), true);
    string valueTypeStr = typeToObjectString(dict->valueType(), TypeModeIn, package, MetadataList(), true);

    //
    // Collect metadata for a custom type.
    //
    if (getTypeMetadata(metadata, instanceType, formalType) ||
        getTypeMetadata(dict->getMetadata(), instanceType, formalType))
    {
        assert(!instanceType.empty());
        if (formalType.empty())
        {
            formalType = "java.util.Map<" + keyTypeStr + ", " + valueTypeStr + ">";
        }
        return true;
    }

    //
    // Return a default type for the platform.
    //
    instanceType = "java.util.HashMap<" + keyTypeStr + ", " + valueTypeStr + ">";
    formalType = "java.util.Map<" + keyTypeStr + ", " + valueTypeStr + ">";
    return false;
}

bool
Slice::JavaGenerator::getSequenceTypes(
    const SequencePtr& seq,
    const string& package,
    const MetadataList& metadata,
    string& instanceType,
    string& formalType)
{
    if (auto meta = seq->getMetadataArgs("java:serializable"))
    {
        instanceType = formalType = *meta;
        return true;
    }

    string prefix = "java:buffer";
    if ((seq->hasMetadata(prefix) || hasMetadata(prefix, metadata)) && !hasMetadata("java:type", metadata))
    {
        instanceType = formalType = typeToBufferString(seq->type());
        return true;
    }

    if (getTypeMetadata(metadata, instanceType, formalType) ||
        getTypeMetadata(seq->getMetadata(), instanceType, formalType))
    {
        assert(!instanceType.empty());
        if (formalType.empty())
        {
            formalType =
                "java.util.List<" + typeToObjectString(seq->type(), TypeModeIn, package, MetadataList(), true) + ">";
        }
        return true;
    }

    // The default mapping is a native array.
    instanceType = formalType = typeToString(seq->type(), TypeModeIn, package, metadata, true, false) + "[]";
    return false;
}

JavaOutput*
Slice::JavaGenerator::createOutput()
{
    return new JavaOutput;
}

void
Slice::JavaGenerator::validateMetadata(const UnitPtr& u)
{
    map<string, MetadataInfo> knownMetadata;

    // "java:buffer"
    MetadataInfo bufferInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::NoArguments,
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
        .extraValidation = [](const MetadataPtr&, const SyntaxTreeBasePtr& p) -> optional<string>
        {
            if (auto seq = dynamic_pointer_cast<Sequence>(p))
            {
                auto builtin = dynamic_pointer_cast<Builtin>(seq->type());
                if (!builtin || (builtin->kind() != Builtin::KindByte && builtin->kind() != Builtin::KindShort &&
                                 builtin->kind() != Builtin::KindInt && builtin->kind() != Builtin::KindLong &&
                                 builtin->kind() != Builtin::KindFloat && builtin->kind() != Builtin::KindDouble))
                {
                    return "the 'java:buffer' metadata can only be applied to sequences of bytes, shorts, ints, longs, "
                           "floats, or doubles";
                }

                // This check for 'hasMetadata("java:buffer")' looks redundant, but is necessary to ensure
                // that 'java:buffer' was placed on the _definition_ and not on where the sequence was used.
                if (seq->hasMetadata("java:type") && seq->hasMetadata("java:buffer"))
                {
                    return "the 'java:buffer' metadata cannot be used alongside 'java:type' - both change the mapped "
                           "type of this sequence";
                }
                if (seq->hasMetadata("java:serializable"))
                {
                    return "the 'java:buffer' metadata cannot be used alongside 'java:serializable' - both change the "
                           "mapped type of this sequence";
                }
            }
            return nullopt;
        },
    };
    knownMetadata.emplace("java:buffer", std::move(bufferInfo));

    // "java:getset"
    MetadataInfo getsetInfo = {
        .validOn = {typeid(ClassDecl), typeid(Slice::Exception), typeid(Struct), typeid(DataMember)},
        .acceptedArgumentKind = MetadataArgumentKind::NoArguments,
    };
    knownMetadata.emplace("java:getset", std::move(getsetInfo));

    // "java:package"
    MetadataInfo packageInfo = {
        .validOn = {typeid(Unit), typeid(Module)},
        .acceptedArgumentKind = MetadataArgumentKind::SingleArgument,
        .extraValidation = [](const MetadataPtr&, const SyntaxTreeBasePtr& p) -> optional<string>
        {
            // If 'java:package' is applied to a module, it must be a top-level module.
            // // Top-level modules are contained by the 'Unit'. Non-top-level modules are contained in 'Module's.
            if (auto mod = dynamic_pointer_cast<Module>(p); mod && !dynamic_pointer_cast<Unit>(mod->container()))
            {
                return "the 'java:package' metadata can only be applied at the file level or to top-level modules";
            }
            return nullopt;
        },
    };
    knownMetadata.emplace("java:package", std::move(packageInfo));

    // "java:serializable"
    MetadataInfo serializableInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::RequiredTextArgument,
        .extraValidation = [](const MetadataPtr&, const SyntaxTreeBasePtr& p) -> optional<string>
        {
            if (auto seq = dynamic_pointer_cast<Sequence>(p))
            {
                auto builtin = dynamic_pointer_cast<Builtin>(seq->type());
                if (!builtin || builtin->kind() != Builtin::KindByte)
                {
                    return "the 'java:serializable' metadata can only be applied to byte sequences (`sequence<byte>`)";
                }
            }
            // This metadata conflicts with 'java:type' and 'java:buffer', but we let the validation functions for those
            // metadata emit the conflict warnings instead of this one, since this can only go on the definition.
            return nullopt;
        },
    };
    knownMetadata.emplace("java:serializable", std::move(serializableInfo));

    // "java:serialVersionUID"
    MetadataInfo serialVersionUIDInfo = {
        .validOn = {typeid(ClassDecl), typeid(Slice::Exception), typeid(Struct)},
        .acceptedArgumentKind = MetadataArgumentKind::SingleArgument,
        .extraValidation = [](const MetadataPtr& meta, const SyntaxTreeBasePtr&) -> optional<string>
        {
            const string& value = meta->arguments();
            try
            {
                [[maybe_unused]] auto _ = std::stoll(value, nullptr, 0);
            }
            catch (const std::exception&)
            {
                return "serialVersionUID '" + value + "' is not a valid integer literal; using default value instead";
            }
            return nullopt;
        },
    };
    knownMetadata.emplace("java:serialVersionUID", std::move(serialVersionUIDInfo));

    // "java:type"
    MetadataInfo typeInfo = {
        .validOn = {typeid(Sequence), typeid(Dictionary)},
        .acceptedArgumentKind = MetadataArgumentKind::RequiredTextArgument,
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
        .extraValidation = [](const MetadataPtr&, const SyntaxTreeBasePtr& p) -> optional<string>
        {
            if (auto seq = dynamic_pointer_cast<Sequence>(p))
            {
                if (seq->hasMetadata("java:serializable"))
                {
                    return "the 'java:type' metadata cannot be used alongside 'java:serializable' - both change the "
                           "mapped type of this sequence";
                }
            }
            // This metadata conflicts with 'java:buffer', but we let the validation functions for that metadata
            // emit the conflict warnings instead of this one, since this can only go on the definition.
            return nullopt;
        },
    };
    knownMetadata.emplace("java:type", std::move(typeInfo));

    // "java:UserException"
    MetadataInfo userExceptionInfo = {
        .validOn = {typeid(Operation)},
        .acceptedArgumentKind = MetadataArgumentKind::NoArguments,
    };
    knownMetadata.emplace("java:UserException", std::move(userExceptionInfo));

    // Pass this information off to the parser's metadata validation logic.
    Slice::validateMetadata(u, "java", knownMetadata);
}
