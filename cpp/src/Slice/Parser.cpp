// Copyright (c) ZeroC, Inc.

#include "Parser.h"
#include "GrammarUtil.h"
#include "Ice/StringUtil.h"
#include "Util.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <limits>

using namespace std;
using namespace Slice;

bool
Slice::containedCompare(const ContainedPtr& lhs, const ContainedPtr& rhs)
{
    return lhs->scoped() < rhs->scoped();
}

bool
Slice::containedEqual(const ContainedPtr& lhs, const ContainedPtr& rhs)
{
    return lhs->scoped() == rhs->scoped();
}

template<typename T>
bool
compareTag(const T& lhs, const T& rhs)
{
    return lhs->tag() < rhs->tag();
}

// NOTE: It is important that this list is kept in alphabetical order!
constexpr string_view languages[] = {"cpp", "cs", "java", "js", "matlab", "php", "python", "ruby", "swift"};

// Forward declare things from Bison and Flex the parser can use.
extern int slice_parse();
extern int slice_lineno;
extern FILE* slice_in;
extern int slice_debug;
extern int slice__flex_debug; // NOLINT(cert-dcl37-c,cert-dcl51-cpp)

//
// Operation attributes
//
// read + supports must be 0 (the default)
//

namespace
{
    DataMemberList filterOrderedOptionalDataMembers(const DataMemberList& members)
    {
        DataMemberList result;
        copy_if(members.begin(), members.end(), back_inserter(result), [](const auto& p) { return p->optional(); });
        result.sort(compareTag<DataMemberPtr>);
        return result;
    }

    bool isMutableAfterReturnType(const TypePtr& type)
    {
        // Returns true if the type contains data types which can be referenced by user code and mutated after a
        // dispatch returns.

        if (type->isClassType())
        {
            return true;
        }

        if (dynamic_pointer_cast<Sequence>(type) || dynamic_pointer_cast<Dictionary>(type))
        {
            return true;
        }

        if (dynamic_pointer_cast<Struct>(type))
        {
            return true;
        }

        return false;
    }
}

namespace Slice
{
    Unit* currentUnit;
}

// ----------------------------------------------------------------------
// Metadata
// ----------------------------------------------------------------------
Slice::Metadata::Metadata(string rawMetadata, string file, int line)
{
    _file = std::move(file);
    _line = line;

    size_t splitPos = string::npos;

    size_t firstColonPos = rawMetadata.find(':');
    if (firstColonPos != string::npos)
    {
        // Check if the metadata starts with a language prefix.
        string prefix = rawMetadata.substr(0, firstColonPos);
        bool hasLangPrefix = binary_search(&languages[0], &languages[sizeof(languages) / sizeof(*languages)], prefix);
        if (hasLangPrefix)
        {
            // If the metadata does start with a language prefix, we need to check for a 2nd ':' separator then.
            // If one isn't present (i.e. the metadata only contains 1 ':' separator), then there are no arguments.
            // If one is present, then that's the boundary between directive and arguments.
            splitPos = rawMetadata.find(':', firstColonPos + 1);
        }
        else
        {
            // If the metadata doesn't start with a language prefix, then everything after the first ':' are arguments.
            splitPos = firstColonPos;
        }
    }

    // Split the metadata into a 'directive' substring and an 'arguments' substring if necessary.
    if (splitPos != string::npos)
    {
        _directive = rawMetadata.substr(0, splitPos);
        _arguments = rawMetadata.substr(splitPos + 1);
    }
    else
    {
        _directive = std::move(rawMetadata);
        _arguments = "";
    }
}

const string&
Slice::Metadata::directive() const
{
    return _directive;
}

const string&
Slice::Metadata::arguments() const
{
    return _arguments;
}

string
Slice::Metadata::file() const
{
    return _file;
}

int
Slice::Metadata::line() const
{
    return _line;
}

// ----------------------------------------------------------------------
// DefinitionContext
// ----------------------------------------------------------------------

Slice::DefinitionContext::DefinitionContext(int includeLevel, MetadataList metadata)
    : _includeLevel(includeLevel),
      _metadata(std::move(metadata))
{
    initSuppressedWarnings();
}

string
Slice::DefinitionContext::filename() const
{
    return _filename;
}

int
Slice::DefinitionContext::includeLevel() const
{
    return _includeLevel;
}

bool
Slice::DefinitionContext::seenDefinition() const
{
    return _seenDefinition;
}

void
Slice::DefinitionContext::setFilename(const string& filename)
{
    _filename = filename;
}

void
Slice::DefinitionContext::setSeenDefinition()
{
    _seenDefinition = true;
}

MetadataList
Slice::DefinitionContext::getMetadata() const
{
    return _metadata;
}

void
Slice::DefinitionContext::setMetadata(MetadataList metadata)
{
    _metadata = std::move(metadata);
    initSuppressedWarnings();
}

void
Slice::DefinitionContext::appendMetadata(MetadataList metadata)
{
    _metadata.splice(_metadata.end(), std::move(metadata));
    initSuppressedWarnings();
}

bool
Slice::DefinitionContext::hasMetadata(string_view directive) const
{
    for (const auto& p : getMetadata())
    {
        if (p->directive() == directive)
        {
            return true;
        }
    }
    return false;
}

optional<string>
Slice::DefinitionContext::getMetadataArgs(string_view directive) const
{
    for (const auto& p : getMetadata())
    {
        if (p->directive() == directive)
        {
            return p->arguments();
        }
    }
    return nullopt;
}

bool
Slice::DefinitionContext::isSuppressed(WarningCategory category) const
{
    return _suppressedWarnings.find(category) != _suppressedWarnings.end() ||
           _suppressedWarnings.find(All) != _suppressedWarnings.end();
}

void
Slice::DefinitionContext::initSuppressedWarnings()
{
    _suppressedWarnings.clear();
    for (const auto& metadata : getMetadata())
    {
        if (metadata->directive() != "suppress-warning")
        {
            continue;
        }

        string_view category = metadata->arguments();
        if (category.empty())
        {
            _suppressedWarnings.insert(All);
        }
        else
        {
            vector<string> result;
            IceInternal::splitString(category, ",", result);
            for (const auto& p : result)
            {
                string s = IceInternal::trim(p);
                if (s == "all")
                {
                    _suppressedWarnings.insert(All);
                }
                else if (s == "deprecated")
                {
                    _suppressedWarnings.insert(Deprecated);
                }
                else if (s == "invalid-comment")
                {
                    _suppressedWarnings.insert(InvalidComment);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------
// DocComment
// ----------------------------------------------------------------------

namespace
{
    void trimLines(StringList& lines)
    {
        // Remove empty trailing lines.
        while (!lines.empty() && lines.back().empty())
        {
            lines.pop_back();
        }
    }

    StringList splitComment(string comment, bool stripMarkup, bool escapeXml)
    {
        string::size_type pos = 0;

        if (stripMarkup)
        {
            // Strip HTML markup.
            while ((pos = comment.find('<', pos)) != string::npos)
            {
                string::size_type endpos = comment.find('>', pos);
                if (endpos == string::npos)
                {
                    break;
                }
                comment.erase(pos, endpos - pos + 1);
            }
        }

        // Escape XML entities.
        if (escapeXml)
        {
            const string amp = "&amp;";
            const string lt = "&lt;";
            const string gt = "&gt;";

            pos = 0;
            while ((pos = comment.find_first_of("&<>", pos)) != string::npos)
            {
                switch (comment[pos])
                {
                    case '&':
                        comment.replace(pos, 1, amp);
                        pos += amp.size();
                        break;
                    case '<':
                        comment.replace(pos, 1, lt);
                        pos += lt.size();
                        break;
                    case '>':
                        comment.replace(pos, 1, gt);
                        pos += gt.size();
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }

        // Split the comment into separate lines, and removing any trailing whitespace and lines from it.
        StringList result;
        pos = 0;
        string::size_type nextPos;
        while ((nextPos = comment.find_first_of('\n', pos)) != string::npos)
        {
            result.push_back(IceInternal::trim(comment.substr(pos, nextPos - pos)));
            pos = nextPos + 1;
        }
        result.push_back(IceInternal::trim(comment.substr(pos)));

        trimLines(result);
        return result;
    }

    bool parseNamedCommentLine(string_view l, string_view tag, string& name, string& doc)
    {
        if (l.find(tag) == 0)
        {
            const string ws = " \t";

            auto nameStart = l.find_first_not_of(ws, tag.size());
            if (nameStart == string::npos)
            {
                return false; // Malformed line, ignore it.
            }

            auto nameEnd = l.find_first_of(ws, nameStart);
            if (nameEnd == string::npos)
            {
                return false; // Malformed line, ignore it.
            }
            name = l.substr(nameStart, nameEnd - nameStart);

            // Store whatever remains of the doc comment in the `doc` string.
            auto docSplitPos = l.find_first_not_of(ws, nameEnd);
            if (docSplitPos != string::npos)
            {
                doc = l.substr(docSplitPos);
            }

            return true;
        }
        return false;
    }

    bool parseCommentLine(string_view l, string_view tag, string& doc)
    {
        if (l.find(tag) == 0)
        {
            // Find the first whitespace that appears after the tag. Everything after it is part of the `doc` string.
            auto docSplitPos = l.find_first_not_of(" \t", tag.size());
            if (docSplitPos != string::npos)
            {
                doc = l.substr(docSplitPos);
            }
            return true;
        }
        return false;
    }

    /// Returns a pointer to the Slice element referenced by `linkText`, relative to the scope of `source`.
    /// If the link cannot be resolved, `nullptr` is returned instead.
    SyntaxTreeBasePtr resolveDocLink(string linkText, const ContainedPtr& source)
    {
        // First we check if the link is to a builtin type.
        if (auto kind = Builtin::kindFromString(linkText))
        {
            return source->unit()->createBuiltin(kind.value());
        }

        // Then, before checking for user-defined types, we determine which scope we'll be searching relative to.
        ContainerPtr linkSourceScope = dynamic_pointer_cast<Container>(source);
        if (!linkSourceScope)
        {
            linkSourceScope = source->container();
        }

        // Perform the actual lookup.
        auto separatorPos = linkText.find('#');
        if (separatorPos == 0)
        {
            // If the link starts with '#', it is explicitly relative to the `linkSourceScope` container.
            ContainedList results = source->unit()->findContents(linkSourceScope->thisScope() + linkText.substr(1));
            return (results.empty() ? nullptr : results.front());
        }
        else if (separatorPos != string::npos)
        {
            // If the link has a '#' anywhere else, convert it to '::' so we can look it up.
            linkText.replace(separatorPos, 1, "::");
        }
        ContainedList results = linkSourceScope->lookupContained(linkText, false);
        return (results.empty() ? nullptr : results.front());
    }
}

optional<DocComment>
Slice::DocComment::parseFrom(const ContainedPtr& p, DocLinkFormatter linkFormatter, bool stripMarkup, bool escapeXml)
{
    string rawComment = p->docComment();

    // Fix any link tags using the provided link formatter.
    const string link = "{@link ";
    auto pos = rawComment.find(link);
    while (pos != string::npos)
    {
        auto endpos = rawComment.find('}', pos);
        if (endpos != string::npos)
        {
            // Extract the linked-to identifier.
            auto identStart = rawComment.find_first_not_of(" \t", pos + link.size());
            auto identEnd = rawComment.find_last_not_of(" \t", endpos);
            string linkText = rawComment.substr(identStart, identEnd - identStart);

            // Then erase the entire '{@link foo}' tag from the comment.
            rawComment.erase(pos, endpos - pos + 1);

            // Attempt to resolve the link, and issue a warning if the link is invalid.
            SyntaxTreeBasePtr linkTarget = resolveDocLink(linkText, p);
            if (!linkTarget)
            {
                string msg = "no Slice element with identifier '" + linkText + "' could be found in this context";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
            }

            // Finally, insert a correctly formatted link where the '{@link foo}' used to be.
            string formattedLink = linkFormatter(linkText, p, linkTarget);
            rawComment.insert(pos, formattedLink);
            pos += formattedLink.length();
        }
        pos = rawComment.find(link, pos);
    }

    // Split the comment's raw text up into lines.
    StringList lines = splitComment(rawComment, stripMarkup, escapeXml);
    if (lines.empty())
    {
        return nullopt;
    }

    // Some tags are only valid if they're applied to an operation.
    // If they aren't, we want to ignore the tag and issue a warning.
    bool isOperation = (bool)dynamic_pointer_cast<Operation>(p);

    const string ws = " \t";
    const string paramTag = "@param";
    const string throwsTag = "@throws";
    const string exceptionTag = "@exception";
    const string seeTag = "@see";
    const string returnTag = "@return";
    const string deprecatedTag = "@deprecated";

    DocComment comment;
    StringList* currentSection = &comment._overview;
    string lineText;
    string name;

    // Parse the comment's text.
    for (const auto& l : lines)
    {
        lineText.clear();

        if (parseNamedCommentLine(l, paramTag, name, lineText))
        {
            if (!isOperation)
            {
                // If '@param' was put on anything other than an operation, ignore it and issue a warning.
                string msg = "the '" + paramTag + "' tag is only valid on operations";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                currentSection = nullptr;
            }
            else
            {
                comment._parameters[name] = {};
                currentSection = &(comment._parameters[name]);
            }
        }
        else if (parseNamedCommentLine(l, throwsTag, name, lineText))
        {
            if (!isOperation)
            {
                // If '@throws' was put on anything other than an operation, ignore it and issue a warning.
                string msg = "the '" + throwsTag + "' tag is only valid on operations";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                currentSection = nullptr;
            }
            else
            {
                comment._exceptions[name] = {};
                currentSection = &(comment._exceptions[name]);
            }
        }
        else if (parseNamedCommentLine(l, exceptionTag, name, lineText))
        {
            if (!isOperation)
            {
                // If '@exception' was put on anything other than an operation, ignore it and issue a warning.
                string msg = "the '" + exceptionTag + "' tag is only valid on operations";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                currentSection = nullptr;
            }
            else
            {
                comment._exceptions[name] = {};
                currentSection = &(comment._exceptions[name]);
            }
        }
        else if (parseCommentLine(l, seeTag, lineText))
        {
            currentSection = &(comment._seeAlso);

            // Remove any leading and trailing whitespace from the line.
            // There's no concern of losing formatting for `@see` due to its simplicity.
            lineText = IceInternal::trim(lineText);
            if (lineText.empty())
            {
                p->unit()
                    ->warning(p->file(), p->line(), InvalidComment, "missing link target after '" + seeTag + "' tag");
            }
            else if (lineText.back() == '.')
            {
                // '@see' tags aren't allowed to end with periods.
                // They do not take sentences, and the trailing period will trip up some language's doc-comments.
                string msg = "ignoring trailing '.' character in '" + seeTag + "' tag";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                lineText.pop_back();
            }
        }
        else if (parseCommentLine(l, returnTag, lineText))
        {
            if (!isOperation)
            {
                // If '@return' was put on anything other than an operation, ignore it and issue a warning.
                string msg = "the '" + returnTag + "' tag is only valid on operations";
                p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                currentSection = nullptr;
            }
            else
            {
                currentSection = &(comment._returns);
            }
        }
        else if (parseCommentLine(l, deprecatedTag, lineText))
        {
            comment._isDeprecated = true;
            currentSection = &(comment._deprecated);
        }
        else // This line didn't introduce a new tag. Either we're in the overview or a tag whose content is multi-line.
        {
            if (!l.empty())
            {
                // We've encountered an unknown doc tag.
                if (l[0] == '@')
                {
                    auto unknownTag = l.substr(0, l.find_first_of(" \t:"));
                    string msg = "ignoring unknown doc tag '" + unknownTag + "' in comment";
                    p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                    currentSection = nullptr;
                }

                // '@see' tags are not allowed to span multiple lines.
                if (currentSection == &(comment._seeAlso))
                {
                    string msg = "'@see' tags cannot span multiple lines and must be of the form: '@see identifier'";
                    p->unit()->warning(p->file(), p->line(), InvalidComment, msg);
                    currentSection = nullptr;
                }
            }

            // Here we allow empty lines, since they could be used for formatting to separate lines.
            if (currentSection)
            {
                currentSection->push_back(l);
            }
            continue;
        }

        // Reaching here means that this line introduced a new tag. We reject empty lines to handle comments which
        // are formatted like: `@param myVeryCoolParam\nvery long explanation that\nspans multiple lines`.
        // We don't want an empty line at the top just because the user's content didn't start until the next line.
        if (currentSection && !lineText.empty())
        {
            currentSection->push_back(lineText);
        }
    }

    trimLines(comment._overview);
    trimLines(comment._deprecated);
    trimLines(comment._returns);

    return comment;
}

bool
Slice::DocComment::isDeprecated() const
{
    return _isDeprecated;
}

StringList
Slice::DocComment::deprecated() const
{
    return _deprecated;
}

StringList
Slice::DocComment::overview() const
{
    return _overview;
}

StringList
Slice::DocComment::seeAlso() const
{
    return _seeAlso;
}

StringList
Slice::DocComment::returns() const
{
    return _returns;
}

map<string, StringList>
Slice::DocComment::parameters() const
{
    return _parameters;
}

map<string, StringList>
Slice::DocComment::exceptions() const
{
    return _exceptions;
}

// ----------------------------------------------------------------------
// SyntaxTreeBase
// ----------------------------------------------------------------------

void
Slice::SyntaxTreeBase::destroy()
{
    _unit = nullptr;
}

UnitPtr
Slice::SyntaxTreeBase::unit() const
{
    return _unit;
}

void
Slice::SyntaxTreeBase::visit(ParserVisitor* /*visitor*/)
{
}

Slice::SyntaxTreeBase::SyntaxTreeBase(UnitPtr unit) : _unit(std::move(unit)) {}

// ----------------------------------------------------------------------
// Type
// ----------------------------------------------------------------------

bool
Slice::Type::isClassType() const
{
    return false;
}

bool
Slice::Type::usesClasses() const
{
    return isClassType();
}

// ----------------------------------------------------------------------
// Builtin
// ----------------------------------------------------------------------

bool
Slice::Builtin::isClassType() const
{
    return _kind == KindObject || _kind == KindValue;
}

size_t
Slice::Builtin::minWireSize() const
{
    switch (_kind)
    {
        case KindBool:
            return 1;
        case KindByte:
            return 1;
        case KindShort:
            return 2;
        case KindInt:
            return 4;
        case KindLong:
            return 8;
        case KindFloat:
            return 4;
        case KindDouble:
            return 8;
        case KindString:
            return 1; // at least one byte for an empty string.
        case KindObject:
            return 1; // at least one byte (to marshal an index instead of an instance).
        case KindObjectProxy:
            return 2; // at least an empty identity for a nil proxy, that is, 2 bytes.
        case KindValue:
            return 1; // at least one byte (to marshal an index instead of an instance).
        default:
            // TODO remove after converting `kind` to an `enum class`.
            throw logic_error("no min wire size");
    }
}

string
Slice::Builtin::getOptionalFormat() const
{
    switch (_kind)
    {
        case KindBool:
        case KindByte:
            return "F1";
        case KindShort:
            return "F2";
        case KindInt:
        case KindFloat:
            return "F4";
        case KindLong:
        case KindDouble:
            return "F8";
        case KindString:
            return "VSize";
        case KindObject:
        case KindValue:
            return "Class";
        case KindObjectProxy:
            return "FSize";
        default:
            // TODO remove after converting `kind` to an `enum class`.
            throw logic_error("no optional format");
    }
}

bool
Slice::Builtin::isVariableLength() const
{
    switch (_kind)
    {
        case KindString:
        case KindObject:
        case KindObjectProxy:
        case KindValue:
            return true;
        default:
            return false;
    }
}

bool
Slice::Builtin::isNumericType() const
{
    switch (_kind)
    {
        case KindByte:
        case KindShort:
        case KindInt:
        case KindLong:
        case KindFloat:
        case KindDouble:
            return true;
        default:
            return false;
    }
}

bool
Slice::Builtin::isIntegralType() const
{
    switch (_kind)
    {
        case KindByte:
        case KindShort:
        case KindInt:
        case KindLong:
            return true;
        default:
            return false;
    }
}

Builtin::Kind
Slice::Builtin::kind() const
{
    return _kind;
}

string
Slice::Builtin::kindAsString() const
{
    return builtinTable[_kind];
}

optional<Slice::Builtin::Kind>
Slice::Builtin::kindFromString(string_view str)
{
    for (size_t i = 0; i < builtinTable.size(); i++)
    {
        if (str == builtinTable[i])
        {
            return static_cast<Kind>(i);
        }
    }
    return nullopt;
}

Slice::Builtin::Builtin(const UnitPtr& unit, Kind kind) : SyntaxTreeBase(unit), Type(), _kind(kind) {}

// ----------------------------------------------------------------------
// Contained
// ----------------------------------------------------------------------

ContainerPtr
Slice::Contained::container() const
{
    return _container;
}

string
Slice::Contained::name() const
{
    return _name;
}

string
Slice::Contained::scoped() const
{
    return scope() + name();
}

string
Slice::Contained::scope() const
{
    string scoped;
    if (auto container = dynamic_pointer_cast<Contained>(_container))
    {
        scoped = container->scoped();
    }
    return scoped + "::";
}

string
Slice::Contained::mappedName() const
{
    const string languageName = _unit->languageName();
    assert(!languageName.empty());

    // First check if any 'xxx:identifier' has been applied to this element.
    // If so, we return that instead of the element's Slice identifier.
    const string metadata = languageName + ":identifier";
    if (auto customName = getMetadataArgs(metadata))
    {
        return *customName;
    }

    return _name;
}

string
Slice::Contained::mappedScoped(const string& separator) const
{
    return mappedScope(separator) + mappedName();
}

string
Slice::Contained::mappedScope(const string& separator) const
{
    string scoped;
    if (auto container = dynamic_pointer_cast<Contained>(_container))
    {
        scoped = container->mappedScoped(separator);
    }
    return scoped + separator;
}

string
Slice::Contained::file() const
{
    return _file;
}

int
Slice::Contained::line() const
{
    return _line;
}

string
Slice::Contained::docComment() const
{
    return _docComment;
}

int
Slice::Contained::includeLevel() const
{
    return _includeLevel;
}

DefinitionContextPtr
Slice::Contained::definitionContext() const
{
    return _definitionContext;
}

MetadataList
Slice::Contained::getMetadata() const
{
    return _metadata;
}

void
Slice::Contained::setMetadata(MetadataList metadata)
{
    _metadata = std::move(metadata);
}

void
Slice::Contained::appendMetadata(MetadataList metadata)
{
    _metadata.splice(_metadata.end(), std::move(metadata));
}

bool
Slice::Contained::hasMetadata(string_view directive) const
{
    for (const auto& p : getMetadata())
    {
        if (p->directive() == directive)
        {
            return true;
        }
    }
    return false;
}

optional<string>
Slice::Contained::getMetadataArgs(string_view directive) const
{
    for (const auto& p : getMetadata())
    {
        if (p->directive() == directive)
        {
            return p->arguments();
        }
    }
    return nullopt;
}

std::optional<FormatType>
Slice::Contained::parseFormatMetadata() const
{
    if (auto metadata = getMetadataArgs("format"))
    {
        const string& arg = *metadata;
        if (arg == "compact")
        {
            return CompactFormat;
        }
        else if (arg == "sliced")
        {
            return SlicedFormat;
        }
        else
        {
            // TODO: Allow "default" to be specified as a format value?
            assert(arg == "default");
        }
    }
    return nullopt;
}

bool
Slice::Contained::isDeprecated() const
{
    return (hasMetadata("deprecate") || hasMetadata("deprecated"));
}

optional<string>
Slice::Contained::getDeprecationReason() const
{
    string reasonMessage;
    if (auto reason = getMetadataArgs("deprecate"))
    {
        reasonMessage = *reason;
    }
    if (auto reason = getMetadataArgs("deprecated"))
    {
        reasonMessage = *reason;
    }

    return (reasonMessage.empty()) ? nullopt : optional{reasonMessage};
}

Slice::Contained::Contained(const ContainerPtr& container, string name)
    : _container(container),
      _name(std::move(name))
{
    assert(_unit);
    _file = _unit->currentFile();
    _line = _unit->currentLine();
    _docComment = _unit->currentDocComment();
    _includeLevel = _unit->currentIncludeLevel();
    _definitionContext = _unit->currentDefinitionContext();
}

// ----------------------------------------------------------------------
// Container
// ----------------------------------------------------------------------

void
Slice::Container::destroy()
{
    for (const auto& i : _contents)
    {
        i->destroy();
    }
    _contents.clear();
    _introducedMap.clear();
    SyntaxTreeBase::destroy();
}

ModulePtr
Slice::Container::createModule(const string& name)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    matches.sort(containedCompare); // Modules can occur many times...
    matches.unique(containedEqual); // ... but we only want one instance of each.

    if (thisScope() == "::")
    {
        _unit->addTopLevelModule(_unit->currentFile(), name);
    }

    for (const auto& p : matches)
    {
        bool differsOnlyInCase = matches.front()->name() != name;
        ModulePtr module = dynamic_pointer_cast<Module>(p);
        if (module)
        {
            if (differsOnlyInCase) // Modules can be reopened only if they are capitalized correctly.
            {
                ostringstream os;
                os << "module '" << name << "' is capitalized inconsistently with its previous name: '"
                   << module->name() << "'";
                _unit->error(os.str());
                return nullptr;
            }
        }
        else if (!differsOnlyInCase)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << matches.front()->name() << "' as module";
            _unit->error(os.str());
            return nullptr;
        }
        else
        {
            ostringstream os;
            os << "module '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " name '" << matches.front()->name() << "'";
            _unit->error(os.str());
            return nullptr;
        }
    }

    if (!checkIdentifier(name))
    {
        return nullptr;
    }

    ModulePtr q = make_shared<Module>(shared_from_this(), name);
    _unit->addContent(q);
    _contents.push_back(q);
    return q;
}

ClassDefPtr
Slice::Container::createClassDef(const string& name, int id, const ClassDefPtr& base)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    for (const auto& p : matches)
    {
        ClassDeclPtr decl = dynamic_pointer_cast<ClassDecl>(p);
        if (decl)
        {
            continue; // all good
        }

        bool differsOnlyInCase = matches.front()->name() != name;
        ClassDefPtr def = dynamic_pointer_cast<ClassDef>(p);
        if (def)
        {
            if (differsOnlyInCase)
            {
                ostringstream os;
                os << "class definition '" << name << "' is capitalized inconsistently with its previous name: '"
                   << def->name() << "'";
                _unit->error(os.str());
            }
            else
            {
                ostringstream os;
                os << "redefinition of class '" << name << "'";
                _unit->error(os.str());
            }
        }
        else if (differsOnlyInCase)
        {
            ostringstream os;
            os << "class definition '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " name '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "class '" << name << "' was previously defined as " << prependA(matches.front()->kindOf());
            _unit->error(os.str());
        }
        return nullptr;
    }

    if (!checkIdentifier(name) || !checkForGlobalDefinition("classes"))
    {
        return nullptr;
    }

    // Implicitly create a class declaration for each class definition.
    // This way the code generator can rely on always having a class declaration available for lookup.
    ClassDefPtr def = make_shared<ClassDef>(shared_from_this(), name, id, base);
    ClassDeclPtr decl = createClassDecl(name);
    def->_declaration = decl;
    decl->_definition = def;

    // Patch forward declarations which may of been created in other openings of this class' module.
    for (const auto& q : matches)
    {
        dynamic_pointer_cast<ClassDecl>(q)->_definition = def;
    }

    _unit->addContent(def);
    _contents.push_back(def);
    return def;
}

ClassDeclPtr
Slice::Container::createClassDecl(const string& name)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    for (const auto& p : matches)
    {
        ClassDefPtr clDef = dynamic_pointer_cast<ClassDef>(p);
        if (clDef)
        {
            continue;
        }

        ClassDeclPtr clDecl = dynamic_pointer_cast<ClassDecl>(p);
        if (clDecl)
        {
            continue;
        }

        bool differsOnlyInCase = matches.front()->name() != name;
        if (differsOnlyInCase)
        {
            ostringstream os;
            os << "class declaration '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " name '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "class '" << name << "' was previously defined as " << prependA(matches.front()->kindOf());
            _unit->error(os.str());
        }
        return nullptr;
    }

    if (!checkIdentifier(name) || !checkForGlobalDefinition("classes"))
    {
        return nullptr;
    }

    //
    // Multiple declarations are permissible. But if we do already
    // have a declaration for the class in this container, we don't
    // create another one.
    //
    for (const auto& q : _contents)
    {
        if (q->name() == name)
        {
            ClassDeclPtr decl = dynamic_pointer_cast<ClassDecl>(q);
            if (decl)
            {
                return decl;
            }
        }
    }

    ClassDeclPtr decl = make_shared<ClassDecl>(shared_from_this(), name);
    _unit->addContent(decl);
    _contents.push_back(decl);
    return decl;
}

InterfaceDefPtr
Slice::Container::createInterfaceDef(const string& name, const InterfaceList& bases)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    for (const auto& p : matches)
    {
        InterfaceDeclPtr decl = dynamic_pointer_cast<InterfaceDecl>(p);
        if (decl)
        {
            continue; // all good
        }

        bool differsOnlyInCase = matches.front()->name() != name;
        InterfaceDefPtr def = dynamic_pointer_cast<InterfaceDef>(p);
        if (def)
        {
            if (differsOnlyInCase)
            {
                ostringstream os;
                os << "interface definition '" << name << "' is capitalized inconsistently with its previous name: '"
                   << def->name() + "'";
                _unit->error(os.str());
            }
            else
            {
                ostringstream os;
                os << "redefinition of interface '" << name << "'";
                _unit->error(os.str());
            }
        }
        else if (differsOnlyInCase)
        {
            ostringstream os;
            os << "interface definition '" << name << "' differs only in capitalization from "
               << matches.front()->kindOf() << " name '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "interface '" << name << "' was previously defined as " << prependA(matches.front()->kindOf());
            _unit->error(os.str());
        }
        return nullptr;
    }

    if (!checkIdentifier(name) || !checkForGlobalDefinition("interfaces"))
    {
        return nullptr;
    }

    InterfaceDecl::checkBasesAreLegal(name, bases, _unit);

    // Implicitly create an interface declaration for each interface definition.
    // This way the code generator can rely on always having an interface declaration available for lookup.
    InterfaceDefPtr def = make_shared<InterfaceDef>(shared_from_this(), name, bases);
    InterfaceDeclPtr decl = createInterfaceDecl(name);
    def->_declaration = decl;
    decl->_definition = def;

    // Patch forward declarations which may of been created in other openings of this interface's module.
    for (const auto& q : matches)
    {
        dynamic_pointer_cast<InterfaceDecl>(q)->_definition = def;
    }

    _unit->addContent(def);
    _contents.push_back(def);
    return def;
}

InterfaceDeclPtr
Slice::Container::createInterfaceDecl(const string& name)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    for (const auto& p : matches)
    {
        InterfaceDefPtr interfaceDef = dynamic_pointer_cast<InterfaceDef>(p);
        if (interfaceDef)
        {
            continue;
        }

        InterfaceDeclPtr interfaceDecl = dynamic_pointer_cast<InterfaceDecl>(p);
        if (interfaceDecl)
        {
            continue;
        }

        bool differsOnlyInCase = matches.front()->name() != name;
        if (differsOnlyInCase)
        {
            ostringstream os;
            os << "interface declaration '" << name << "' differs only in capitalization from "
               << matches.front()->kindOf() << " name '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "interface '" << name << "' was previously defined as " << prependA(matches.front()->kindOf());
            _unit->error(os.str());
        }
        return nullptr;
    }

    if (!checkIdentifier(name) || !checkForGlobalDefinition("interfaces"))
    {
        return nullptr;
    }

    // Multiple declarations are permissible. But if we do already have a declaration for the interface in this
    // container, we don't create another one.
    for (const auto& q : _contents)
    {
        if (q->name() == name)
        {
            InterfaceDeclPtr decl = dynamic_pointer_cast<InterfaceDecl>(q);
            if (decl)
            {
                return decl;
            }
        }
    }

    InterfaceDeclPtr decl = make_shared<InterfaceDecl>(shared_from_this(), name);
    _unit->addContent(decl);
    _contents.push_back(decl);
    return decl;
}

ExceptionPtr
Slice::Container::createException(const string& name, const ExceptionPtr& base, NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as exception";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "exception '" << name << "' differs only in capitalization from " << matches.front()->kindOf() << " '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the exception anyway

    if (nodeType == Real)
    {
        checkForGlobalDefinition("exceptions"); // Don't return here -- we create the exception anyway
    }

    ExceptionPtr p = make_shared<Exception>(shared_from_this(), name, base);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

StructPtr
Slice::Container::createStruct(const string& name, NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as struct";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "struct '" << name << "' differs only in capitalization from " << matches.front()->kindOf() << " '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the struct anyway.

    if (nodeType == Real)
    {
        checkForGlobalDefinition("structs"); // Don't return here -- we create the struct anyway.
    }

    StructPtr p = make_shared<Struct>(shared_from_this(), name);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

SequencePtr
Slice::Container::createSequence(const string& name, const TypePtr& type, MetadataList metadata, NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as sequence";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "sequence '" << name << "' differs only in capitalization from " << matches.front()->kindOf() << " '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the sequence anyway.

    if (nodeType == Real)
    {
        checkForGlobalDefinition("sequences"); // Don't return here -- we create the sequence anyway.
    }

    SequencePtr p = make_shared<Sequence>(shared_from_this(), name, type, std::move(metadata));
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

DictionaryPtr
Slice::Container::createDictionary(
    const string& name,
    const TypePtr& keyType,
    MetadataList keyMetadata,
    const TypePtr& valueType,
    MetadataList valueMetadata,
    NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as dictionary";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "dictionary '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the dictionary anyway.

    if (nodeType == Real)
    {
        checkForGlobalDefinition("dictionaries"); // Don't return here -- we create the dictionary anyway.

        if (!Dictionary::isLegalKeyType(keyType))
        {
            ostringstream os;
            os << "dictionary '" << name << "' uses an illegal key type";
            _unit->error(os.str());
            return nullptr;
        }
    }

    DictionaryPtr p = make_shared<Dictionary>(
        shared_from_this(),
        name,
        keyType,
        std::move(keyMetadata),
        valueType,
        std::move(valueMetadata));
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

EnumPtr
Slice::Container::createEnum(const string& name, NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as enumeration";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "enumeration '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the enumeration anyway.

    if (nodeType == Real)
    {
        checkForGlobalDefinition("enums"); // Don't return here -- we create the enumeration anyway.
    }

    EnumPtr p = make_shared<Enum>(shared_from_this(), name);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

ConstPtr
Slice::Container::createConst(
    const string& name,
    const TypePtr& type,
    MetadataList metadata,
    const SyntaxTreeBasePtr& valueType,
    const string& valueString,
    NodeType nodeType)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as constant";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "constant '" << name << "' differs only in capitalization from " << matches.front()->kindOf() << " '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        return nullptr;
    }

    checkIdentifier(name); // Don't return here -- we create the constant anyway.

    if (nodeType == Real)
    {
        checkForGlobalDefinition("constants"); // Don't return here -- we create the constant anyway.
    }

    SyntaxTreeBasePtr resolvedValueType = valueType;

    // Validate the constant and its value; for enums, find enumerator
    if (nodeType == Real && !validateConstant(name, type, resolvedValueType, valueString, true))
    {
        return nullptr;
    }

    ConstPtr p =
        make_shared<Const>(shared_from_this(), name, type, std::move(metadata), resolvedValueType, valueString);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

TypeList
Slice::Container::lookupType(const string& identifier)
{
    // Remove whitespace.
    string sc = identifier;
    string::size_type pos;
    while ((pos = sc.find_first_of(" \t\r\n")) != string::npos)
    {
        sc.erase(pos, 1);
    }

    // Check for builtin type.
    auto kind = Builtin::kindFromString(sc);
    if (kind)
    {
        return {_unit->createBuiltin(*kind)};
    }

    // Not a builtin type, try to look up a constructed type.
    return lookupTypeNoBuiltin(identifier, true);
}

TypeList
Slice::Container::lookupTypeNoBuiltin(const string& identifier, bool emitErrors, bool ignoreUndefined)
{
    // Remove whitespace.
    string sc = identifier;
    string::size_type pos;
    while ((pos = sc.find_first_of(" \t\r\n")) != string::npos)
    {
        sc.erase(pos, 1);
    }

    // Absolute scoped name?
    if (sc.size() >= 2 && sc[0] == ':')
    {
        return _unit->lookupTypeNoBuiltin(sc.substr(2), emitErrors);
    }

    TypeList results;
    bool typeError = false;
    vector<string> errors;

    ContainedList matches = _unit->findContents(thisScope() + sc);
    for (const auto& p : matches)
    {
        if (dynamic_pointer_cast<InterfaceDef>(p) || dynamic_pointer_cast<ClassDef>(p))
        {
            continue; // Ignore interface and class definitions.
        }

        if (emitErrors && matches.front()->scoped() != (thisScope() + sc))
        {
            ostringstream os;
            os << p->kindOf() << " name '" << identifier << "' is capitalized inconsistently with its previous name: '"
               << matches.front()->scoped() << "'";
            errors.push_back(os.str());
        }

        ExceptionPtr ex = dynamic_pointer_cast<Exception>(p);
        if (ex)
        {
            if (emitErrors)
            {
                ostringstream os;
                os << "'" << sc << "' is an exception, which cannot be used as a type";
                _unit->error(os.str());
            }
            return {};
        }

        TypePtr type = dynamic_pointer_cast<Type>(p);
        if (!type)
        {
            typeError = true;
            if (emitErrors)
            {
                ostringstream os;
                os << "'" << sc << "' is not a type";
                errors.push_back(os.str());
            }
            break; // Possible that correct match is higher in scope
        }
        results.push_back(type);
    }

    if (results.empty())
    {
        ContainedPtr contained = dynamic_pointer_cast<Contained>(shared_from_this());
        if (contained)
        {
            results = contained->container()->lookupTypeNoBuiltin(sc, emitErrors, typeError || ignoreUndefined);
        }
        else if (!typeError)
        {
            if (emitErrors && !ignoreUndefined)
            {
                ostringstream os;
                os << "'" << sc << "' is not defined";
                _unit->error(os.str());
            }
            return {};
        }
    }

    // Do not emit errors if there was a type error but a match was found in a higher scope.
    if (emitErrors && !(typeError && !results.empty()))
    {
        for (const auto& error : errors)
        {
            _unit->error(error);
        }
    }
    return results;
}

ContainedList
Slice::Container::lookupContained(const string& identifier, bool emitErrors)
{
    // Remove whitespace.
    string sc = identifier;
    string::size_type pos;
    while ((pos = sc.find_first_of(" \t\r\n")) != string::npos)
    {
        sc.erase(pos, 1);
    }

    // Absolute scoped name?
    if (sc.size() >= 2 && sc[0] == ':')
    {
        return _unit->lookupContained(sc.substr(2), emitErrors);
    }

    ContainedList matches = _unit->findContents(thisScope() + sc);
    ContainedList results;
    for (const auto& p : matches)
    {
        if (dynamic_pointer_cast<InterfaceDef>(p) || dynamic_pointer_cast<ClassDef>(p))
        {
            continue; // ignore definitions
        }

        results.push_back(p);

        if (emitErrors && p->scoped() != (thisScope() + sc))
        {
            ostringstream os;
            os << p->kindOf() << " name '" << identifier << "' is capitalized inconsistently with its previous name: '"
               << p->scoped() << "'";
            _unit->error(os.str());
        }
    }

    if (results.empty())
    {
        ContainedPtr contained = dynamic_pointer_cast<Contained>(shared_from_this());
        if (!contained)
        {
            if (emitErrors)
            {
                ostringstream os;
                os << "'" << sc << "' is not defined";
                _unit->error(os.str());
            }
            return {};
        }
        return contained->container()->lookupContained(sc, emitErrors);
    }
    else
    {
        return results;
    }
}

ExceptionPtr
Slice::Container::lookupException(const string& identifier, bool emitErrors)
{
    ContainedList contained = lookupContained(identifier, emitErrors);
    if (contained.empty())
    {
        return nullptr;
    }

    ExceptionList exceptions;
    for (const auto& p : contained)
    {
        ExceptionPtr ex = dynamic_pointer_cast<Exception>(p);
        if (!ex)
        {
            if (emitErrors)
            {
                ostringstream os;
                os << "'" << identifier << "' is not an exception";
                _unit->error(os.str());
            }
            return nullptr;
        }
        exceptions.push_back(ex);
    }
    assert(exceptions.size() == 1);
    return exceptions.front();
}

UnitPtr
Slice::Container::unit() const
{
    return SyntaxTreeBase::unit();
}

ModuleList
Slice::Container::modules() const
{
    ModuleList result;
    for (const auto& p : _contents)
    {
        ModulePtr q = dynamic_pointer_cast<Module>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

InterfaceList
Slice::Container::interfaces() const
{
    InterfaceList result;
    for (const auto& p : _contents)
    {
        InterfaceDefPtr q = dynamic_pointer_cast<InterfaceDef>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

EnumList
Slice::Container::enums() const
{
    EnumList result;
    for (const auto& p : _contents)
    {
        EnumPtr q = dynamic_pointer_cast<Enum>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

EnumeratorList
Slice::Container::enumerators() const
{
    EnumeratorList result;
    for (const auto& p : _contents)
    {
        EnumeratorPtr q = dynamic_pointer_cast<Enumerator>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

// Find enumerators using the old unscoped enumerators lookup
EnumeratorList
Slice::Container::enumerators(const string& identifier) const
{
    EnumeratorList result;
    string::size_type lastColon = identifier.rfind(':');

    if (lastColon == string::npos)
    {
        // check all enclosing scopes
        ContainerPtr container = const_pointer_cast<Container>(shared_from_this());
        do
        {
            EnumList enums = container->enums();
            for (const auto& p : enums)
            {
                ContainedList cl = p->lookupContained(identifier, false);
                if (!cl.empty())
                {
                    result.push_back(dynamic_pointer_cast<Enumerator>(cl.front()));
                }
            }

            ContainedPtr contained = dynamic_pointer_cast<Contained>(container);
            container = contained ? contained->container() : nullptr;
        } while (result.empty() && container);
    }
    else
    {
        // Find the referenced scope
        ContainerPtr container = const_pointer_cast<Container>(shared_from_this());
        string scope = identifier.substr(0, identifier.rfind("::"));
        ContainedList cl = container->lookupContained(scope, false);
        if (!cl.empty())
        {
            container = dynamic_pointer_cast<Container>(cl.front());
            if (container)
            {
                EnumList enums = container->enums();
                string name = identifier.substr(lastColon + 1);
                for (const auto& p : enums)
                {
                    ContainedList cl2 = p->lookupContained(name, false);
                    if (!cl2.empty())
                    {
                        result.push_back(dynamic_pointer_cast<Enumerator>(cl2.front()));
                    }
                }
            }
        }
    }

    return result;
}

ContainedList
Slice::Container::contents() const
{
    return _contents;
}

string
Slice::Container::thisScope() const
{
    string s;
    ContainedPtr contained = dynamic_pointer_cast<Contained>(const_pointer_cast<Container>(shared_from_this()));
    if (contained)
    {
        s = contained->scoped();
    }
    s += "::";
    return s;
}

void
Slice::Container::visit(ParserVisitor* visitor)
{
    for (const auto& p : _contents)
    {
        if (visitor->shouldVisitIncludedDefinitions() || p->includeLevel() == 0)
        {
            p->visit(visitor);
        }
    }
}

bool
Slice::Container::checkIntroduced(const string& scopedName, ContainedPtr namedThing)
{
    if (scopedName[0] == ':') // Only unscoped names introduce anything.
    {
        return true;
    }

    // Split off first component.
    string::size_type pos = scopedName.find("::");
    string firstComponent = pos == string::npos ? scopedName : scopedName.substr(0, pos);

    // If we don't have a type, the thing that is introduced is the contained for the first component.
    if (namedThing == nullptr)
    {
        ContainedList cl = lookupContained(firstComponent, false);
        if (cl.empty())
        {
            return true; // Ignore types whose creation failed previously.
        }
        namedThing = cl.front();
    }
    else
    {
        // For each scope, get the container until we have the container for the first scope (which is the introduced
        // one).
        ContainerPtr c;
        bool first = true;
        while (pos != string::npos)
        {
            if (first)
            {
                c = namedThing->container();
            }
            else
            {
                ContainedPtr contained = dynamic_pointer_cast<Contained>(c);
                if (contained)
                {
                    c = contained->container();
                }
            }
            first = false;
            if (pos != string::npos)
            {
                pos = scopedName.find("::", pos + 2);
            }
        }

        if (dynamic_pointer_cast<Contained>(c))
        {
            namedThing = dynamic_pointer_cast<Contained>(c);
        }
    }

    // Check if the first component is in the introduced map of this scope.
    auto it = _introducedMap.find(firstComponent);
    if (it == _introducedMap.end())
    {
        // We've just introduced the first component to the current scope.
        _introducedMap[firstComponent] = namedThing; // No, insert it
    }
    else
    {
        // We've previously introduced the first component to the current scope, check that it has not changed meaning.
        if (it->second->scoped() != namedThing->scoped())
        {
            // Parameter are in its own scope.
            if ((dynamic_pointer_cast<Parameter>(it->second) && !dynamic_pointer_cast<Parameter>(namedThing)) ||
                (!dynamic_pointer_cast<Parameter>(it->second) && dynamic_pointer_cast<Parameter>(namedThing)))
            {
                return true;
            }

            // Data members are in its own scope.
            if ((dynamic_pointer_cast<DataMember>(it->second) && !dynamic_pointer_cast<DataMember>(namedThing)) ||
                (!dynamic_pointer_cast<DataMember>(it->second) && dynamic_pointer_cast<DataMember>(namedThing)))
            {
                return true;
            }

            _unit->error("'" + firstComponent + "' has changed meaning");

            return false;
        }
    }
    return true;
}

bool
Slice::Container::checkForGlobalDefinition(const char* definitionKindPlural)
{
    if (dynamic_cast<Unit*>(this))
    {
        ostringstream os;
        os << definitionKindPlural << " can only be defined within a module";
        _unit->error(os.str());
        return false;
    }
    return true;
}

Slice::Container::Container(const UnitPtr& unit) : SyntaxTreeBase(unit) {}

bool
Slice::Container::validateConstant(
    const string& name,
    const TypePtr& type,
    SyntaxTreeBasePtr& valueType,
    const string& valueString,
    bool isConstant)
{
    // isConstant indicates whether a constant or a data member (with a default value) is being defined.

    const string desc = isConstant ? "constant" : "data member";

    // If valueType is a ConstPtr, it means the constant or data member being defined refers to another constant.
    const ConstPtr constant = dynamic_pointer_cast<Const>(valueType);

    // First verify that it is legal to specify a constant or default value for the given type.
    BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
    EnumPtr e = dynamic_pointer_cast<Enum>(type);

    if (b)
    {
        if (b->usesClasses() || b->kind() == Builtin::KindObjectProxy)
        {
            if (isConstant)
            {
                ostringstream os;
                os << "constant '" << name << "' has illegal type: '" << b->kindAsString() << "'";
                _unit->error(os.str());
            }
            else
            {
                ostringstream os;
                os << "default value not allowed for data member '" << name << "' of type '" << b->kindAsString()
                   << "'";
                _unit->error(os.str());
            }
            return false;
        }
    }
    else if (!e)
    {
        if (isConstant)
        {
            ostringstream os;
            os << "constant '" << name << "' has illegal type";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "default value not allowed for data member '" << name << "'";
            _unit->error(os.str());
        }
        return false;
    }

    // Next, verify that the type of the constant or data member is compatible with the given value.
    if (b)
    {
        BuiltinPtr lt;

        if (constant)
        {
            lt = dynamic_pointer_cast<Builtin>(constant->type());
        }
        else
        {
            lt = dynamic_pointer_cast<Builtin>(valueType);
        }

        if (lt)
        {
            bool ok = false;
            if (b->kind() == lt->kind())
            {
                ok = true;
            }
            else if (b->isIntegralType())
            {
                ok = lt->isIntegralType();
            }
            else if (b->isNumericType())
            {
                ok = lt->isNumericType();
            }

            if (!ok)
            {
                ostringstream os;
                os << "initializer of type '" << lt->kindAsString() << "' is incompatible with the type '"
                   << b->kindAsString() << "' of " << desc << " '" << name << "'";
                _unit->error(os.str());
                return false;
            }
        }
        else
        {
            ostringstream os;
            os << "type of initializer is incompatible with the type '" << b->kindAsString() << "' of " << desc << " '"
               << name << "'";
            _unit->error(os.str());
            return false;
        }

        switch (b->kind())
        {
            case Builtin::KindByte:
            {
                int64_t l = std::stoll(valueString, nullptr, 0);
                if (l < numeric_limits<uint8_t>::min() || l > numeric_limits<uint8_t>::max())
                {
                    ostringstream os;
                    os << "initializer '" << valueString << "' for " << desc << " '" << name
                       << "' out of range for type byte";
                    _unit->error(os.str());
                    return false;
                }
                break;
            }
            case Builtin::KindShort:
            {
                int64_t l = std::stoll(valueString, nullptr, 0);
                if (l < numeric_limits<int16_t>::min() || l > numeric_limits<int16_t>::max())
                {
                    ostringstream os;
                    os << "initializer '" << valueString << "' for " << desc << " '" << name
                       << "' out of range for type short";
                    _unit->error(os.str());
                    return false;
                }
                break;
            }
            case Builtin::KindInt:
            {
                int64_t l = std::stoll(valueString, nullptr, 0);
                if (l < numeric_limits<int32_t>::min() || l > numeric_limits<int32_t>::max())
                {
                    ostringstream os;
                    os << "initializer '" << valueString << "' for " + desc << " '" << name
                       << "' out of range for type int";
                    _unit->error(os.str());
                    return false;
                }
                break;
            }

            default:
            {
                break;
            }
        }
    }

    if (e)
    {
        if (constant)
        {
            EnumPtr ec = dynamic_pointer_cast<Enum>(constant->type());
            if (e != ec)
            {
                ostringstream os;
                os << "type of initializer is incompatible with the type of " << desc << " '" << name << "'";
                _unit->error(os.str());
                return false;
            }
        }
        else
        {
            if (valueType)
            {
                EnumeratorPtr lte = dynamic_pointer_cast<Enumerator>(valueType);

                if (!lte)
                {
                    ostringstream os;
                    os << "type of initializer is incompatible with the type of " << desc << " '" << name << "'";
                    _unit->error(os.str());
                    return false;
                }
                EnumeratorList elist = e->enumerators();
                if (find(elist.begin(), elist.end(), lte) == elist.end())
                {
                    ostringstream os;
                    os << "enumerator '" << valueString << "' is not defined in enumeration '" << e->scoped() << "'";
                    _unit->error(os.str());
                    return false;
                }
            }
            else
            {
                // Check if value designates an enumerator of e
                string newVal = valueString;
                string::size_type lastColon = valueString.rfind(':');
                if (lastColon != string::npos && lastColon + 1 < valueString.length())
                {
                    newVal =
                        valueString.substr(0, lastColon + 1) + e->name() + "::" + valueString.substr(lastColon + 1);
                }

                ContainedList clist = e->lookupContained(newVal, false);
                if (clist.empty())
                {
                    ostringstream os;
                    os << "'" << valueString << "' does not designate an enumerator of '" << e->scoped() << "'";
                    _unit->error(os.str());
                    return false;
                }

                EnumeratorPtr lte = dynamic_pointer_cast<Enumerator>(clist.front());
                if (lte)
                {
                    valueType = lte;
                }
                else
                {
                    ostringstream os;
                    os << "type of initializer is incompatible with the type of " << desc << " '" << name << "'";
                    _unit->error(os.str());
                    return false;
                }
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------------
// Module
// ----------------------------------------------------------------------

string
Slice::Module::kindOf() const
{
    return "module";
}

void
Slice::Module::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<Module>(shared_from_this());
    if (visitor->visitModuleStart(self))
    {
        Container::visit(visitor);
        visitor->visitModuleEnd(self);
    }
}

Slice::Module::Module(const ContainerPtr& container, const string& name)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Contained(container, name)
{
}

// ----------------------------------------------------------------------
// Constructed
// ----------------------------------------------------------------------

Slice::Constructed::Constructed() {}

// ----------------------------------------------------------------------
// ClassDecl
// ----------------------------------------------------------------------

void
Slice::ClassDecl::destroy()
{
    _definition = nullptr;
    SyntaxTreeBase::destroy();
}

ClassDefPtr
Slice::ClassDecl::definition() const
{
    return _definition;
}

bool
Slice::ClassDecl::isClassType() const
{
    return true;
}

size_t
Slice::ClassDecl::minWireSize() const
{
    return 1; // At least four bytes for an instance, if the instance is marshaled as an index.
}

string
Slice::ClassDecl::getOptionalFormat() const
{
    return "Class";
}

bool
Slice::ClassDecl::isVariableLength() const
{
    return true;
}

string
Slice::ClassDecl::kindOf() const
{
    return "class";
}

void
Slice::ClassDecl::visit(ParserVisitor* visitor)
{
    visitor->visitClassDecl(shared_from_this());
}

Slice::ClassDecl::ClassDecl(const ContainerPtr& container, const string& name)
    : SyntaxTreeBase(container->unit()),
      Type(),
      Contained(container, name),
      Constructed()
{
}

// ----------------------------------------------------------------------
// ClassDef
// ----------------------------------------------------------------------

void
Slice::ClassDef::destroy()
{
    _declaration = nullptr;
    _base = nullptr;
    Container::destroy();
}

DataMemberPtr
Slice::ClassDef::createDataMember(
    const string& name,
    const TypePtr& type,
    bool isOptional,
    int tag,
    const SyntaxTreeBasePtr& defaultValueType,
    const string& defaultValueString)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() != name)
        {
            ostringstream os;
            os << "data member '" << name << "' differs only in capitalization from " << matches.front()->kindOf()
               << " '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "redefinition of " << matches.front()->kindOf() << " '" << name << "' as data member '" << name
               << "'";
            _unit->error(os.str());
            return nullptr;
        }
    }

    checkIdentifier(name); // Don't return here -- we create the data member anyway.

    //
    // Check whether any bases have defined something with the same name already.
    //
    if (_base)
    {
        for (const auto& dataMember : _base->allDataMembers())
        {
            if (dataMember->name() == name)
            {
                ostringstream os;
                os << "data member '" << name << "' is already defined as a data member in a base class";
                _unit->error(os.str());
                return nullptr;
            }

            string baseName = IceInternal::toLower(dataMember->name());
            string newName = IceInternal::toLower(name);
            if (baseName == newName)
            {
                ostringstream os;
                os << "data member '" << name << "' differs only in capitalization from data member '"
                   << dataMember->name() << "', which is defined in a base class";
                _unit->error(os.str());
            }
        }
    }

    SyntaxTreeBasePtr dlt = defaultValueType;
    string dv = defaultValueString;

    if (dlt || (dynamic_pointer_cast<Enum>(type) && !dv.empty()))
    {
        // Validate the default value.
        if (!validateConstant(name, type, dlt, dv, false))
        {
            // Create the data member anyway, just without the default value.
            dlt = nullptr;
            dv.clear();
        }
    }

    if (isOptional)
    {
        // Validate the tag.
        for (const auto& q : dataMembers())
        {
            if (q->optional() && tag == q->tag())
            {
                ostringstream os;
                os << "tag for optional data member '" << name << "' is already in use";
                _unit->error(os.str());
                break;
            }
        }
    }

    DataMemberPtr member = make_shared<DataMember>(shared_from_this(), name, type, isOptional, tag, dlt, dv);
    _unit->addContent(member);
    _contents.push_back(member);
    return member;
}

ClassDeclPtr
Slice::ClassDef::declaration() const
{
    return _declaration;
}

ClassDefPtr
Slice::ClassDef::base() const
{
    return _base;
}

ClassList
Slice::ClassDef::allBases() const
{
    ClassList result;
    if (_base)
    {
        result.push_back(_base);
        result.merge(_base->allBases());
    }
    return result;
}

DataMemberList
Slice::ClassDef::dataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

DataMemberList
Slice::ClassDef::orderedOptionalDataMembers() const
{
    return filterOrderedOptionalDataMembers(dataMembers());
}

// Return the data members of this class and its parent classes, in base-to-derived order.
DataMemberList
Slice::ClassDef::allDataMembers() const
{
    DataMemberList result;

    // Check if we have a base class. If so, recursively get the data members of the base(s).
    if (_base)
    {
        result = _base->allDataMembers();
    }

    // Append this class's data members.
    DataMemberList myMembers = dataMembers();
    result.splice(result.end(), myMembers);

    return result;
}

DataMemberList
Slice::ClassDef::classDataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q && q->type()->isClassType())
        {
            result.push_back(q);
        }
    }
    return result;
}

bool
Slice::ClassDef::canBeCyclic() const
{
    if (_base && _base->canBeCyclic())
    {
        return true;
    }

    for (const auto& i : dataMembers())
    {
        if (i->type()->usesClasses())
        {
            return true;
        }
    }
    return false;
}

string
Slice::ClassDef::kindOf() const
{
    return "class";
}

void
Slice::ClassDef::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<ClassDef>(shared_from_this());
    if (visitor->visitClassDefStart(self))
    {
        Container::visit(visitor);
        visitor->visitClassDefEnd(self);
    }
}

int
Slice::ClassDef::compactId() const
{
    return _compactId;
}

MetadataList
Slice::ClassDef::getMetadata() const
{
    return _declaration->getMetadata();
}

void
Slice::ClassDef::setMetadata(MetadataList metadata)
{
    _declaration->setMetadata(std::move(metadata));
}

void
Slice::ClassDef::appendMetadata(MetadataList metadata)
{
    _declaration->appendMetadata(std::move(metadata));
}

Slice::ClassDef::ClassDef(const ContainerPtr& container, const string& name, int id, ClassDefPtr base)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Contained(container, name),
      _base(std::move(base)),
      _compactId(id)
{
    if (_compactId >= 0)
    {
        _unit->addTypeId(_compactId, scoped());
    }
}

// ----------------------------------------------------------------------
// InterfaceDecl
// ----------------------------------------------------------------------

void
Slice::InterfaceDecl::destroy()
{
    _definition = nullptr;
    SyntaxTreeBase::destroy();
}

InterfaceDefPtr
Slice::InterfaceDecl::definition() const
{
    return _definition;
}

size_t
Slice::InterfaceDecl::minWireSize() const
{
    return 2; // a null proxy is encoded on 2 bytes
}

string
Slice::InterfaceDecl::getOptionalFormat() const
{
    return "FSize"; // with the 1.1 encoding, the size of a proxy is encoded using a fixed-length size
}

bool
Slice::InterfaceDecl::isVariableLength() const
{
    return true;
}

string
Slice::InterfaceDecl::kindOf() const
{
    return "interface";
}

void
Slice::InterfaceDecl::visit(ParserVisitor* visitor)
{
    visitor->visitInterfaceDecl(shared_from_this());
}

void
Slice::InterfaceDecl::checkBasesAreLegal(const string& name, const InterfaceList& bases, const UnitPtr& unit)
{
    // Check whether, for multiple inheritance, any of the bases define the same operations.
    if (bases.size() > 1)
    {
        //
        // We have multiple inheritance. Build a list of paths through the
        // inheritance graph, such that multiple inheritance is legal if
        // the union of the names defined in classes on each path are disjoint.
        //
        GraphPartitionList gpl;
        for (const auto& base : bases)
        {
            InterfaceList cl;
            gpl.push_back(cl);
            addPartition(gpl, gpl.rbegin(), base);
        }

        //
        // We now have a list of partitions, with each partition containing
        // a list of class definitions. Turn the list of partitions of class
        // definitions into a list of sets of strings, with each
        // set containing the names of operations and data members defined in
        // the classes in each partition.
        //
        StringPartitionList spl = toStringPartitionList(gpl);

        //
        // Multiple inheritance is legal if no two partitions contain a common
        // name (that is, if the union of the intersections of all possible pairs
        // of partitions is empty).
        //
        checkPairIntersections(spl, name, unit);
    }
}

Slice::InterfaceDecl::InterfaceDecl(const ContainerPtr& container, const string& name)
    : SyntaxTreeBase(container->unit()),
      Type(),
      Contained(container, name),
      Constructed()
{
}

// Return true if the interface definition `idp` is on one of the interface lists in `gpl`, false otherwise.
bool
Slice::InterfaceDecl::isInList(const GraphPartitionList& gpl, const InterfaceDefPtr& interfaceDef)
{
    for (const auto& i : gpl)
    {
        if (find_if(
                i.begin(),
                i.end(),
                [scope = interfaceDef->scoped()](const auto& other) { return other->scoped() == scope; }) != i.end())
        {
            return true;
        }
    }
    return false;
}

void
Slice::InterfaceDecl::addPartition(
    GraphPartitionList& partitions,
    const GraphPartitionList::reverse_iterator& tail,
    const InterfaceDefPtr& base)
{
    // If this base is on one of the partition lists already, do nothing.
    if (isInList(partitions, base))
    {
        return;
    }

    // Put the current base at the end of the current partition.
    tail->push_back(base);

    // If the base has bases in turn, recurse, adding the first base of base (the left-most "grandbase") to the current
    // partition.
    if (base->bases().size())
    {
        addPartition(partitions, tail, *(base->bases().begin()));
    }

    // If the base has multiple bases, each of the "grandbases" except for the left-most (which we just dealt with)
    // adds a new partition.
    if (base->bases().size() > 1)
    {
        InterfaceList grandBases = base->bases();
        auto i = grandBases.begin();
        while (++i != grandBases.end())
        {
            InterfaceList cl;
            partitions.push_back(cl);
            addPartition(partitions, partitions.rbegin(), *i);
        }
    }
}

//
// Convert the list of partitions of interface definitions into a
// list of lists, with each member list containing the operation
// names defined by the interfaces in each partition.
//
Slice::InterfaceDecl::StringPartitionList
Slice::InterfaceDecl::toStringPartitionList(const GraphPartitionList& partitions)
{
    StringPartitionList spl;
    for (const auto& interfaces : partitions)
    {
        StringList sl;
        for (const auto& interfaceDefinition : interfaces)
        {
            for (const auto& operation : interfaceDefinition->operations())
            {
                sl.push_back(operation->name());
            }
        }
        spl.push_back(std::move(sl));
    }
    return spl;
}

//
// For all (unique) pairs of string lists, check whether an identifier in one list occurs
// in the other and, if so, complain.
//
void
Slice::InterfaceDecl::checkPairIntersections(
    const StringPartitionList& stringPartitions,
    const string& name,
    const UnitPtr& unit)
{
    set<string> reported;
    for (auto i = stringPartitions.begin(); i != stringPartitions.end(); ++i)
    {
        auto cursor = i;
        ++cursor;
        for (auto j = cursor; j != stringPartitions.end(); ++j)
        {
            for (const auto& s1 : *i)
            {
                for (const auto& s2 : *j)
                {
                    if (s1 == s2 && reported.find(s1) == reported.end())
                    {
                        ostringstream os;
                        os << "ambiguous multiple inheritance: '" << name << "' inherits operation '" << s1
                           << "' from two or more unrelated base interfaces";
                        unit->error(os.str());
                        reported.insert(s1);
                    }
                    else if (
                        !CICompare()(s1, s2) && !CICompare()(s2, s1) && reported.find(s1) == reported.end() &&
                        reported.find(s2) == reported.end())
                    {
                        ostringstream os;
                        os << "ambiguous multiple inheritance: '" << name << "' inherits operations '" << s1
                           << "' and '" << s2
                           << "', which differ only in capitalization, from unrelated base interfaces";
                        unit->error(os.str());
                        reported.insert(s1);
                        reported.insert(s2);
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------
// InterfaceDef
// ----------------------------------------------------------------------

void
Slice::InterfaceDef::destroy()
{
    _declaration = nullptr;
    _bases.clear();
    Container::destroy();
}

OperationPtr
Slice::InterfaceDef::createOperation(
    const string& name,
    const TypePtr& returnType,
    bool isOptional,
    int tag,
    Operation::Mode mode)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() != name)
        {
            ostringstream os;
            os << "operation '" << name << "' differs only in capitalization from " << matches.front()->kindOf() << " '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        ostringstream os;
        os << "redefinition of " << matches.front()->kindOf() << " '" << matches.front()->name() << "' as operation '"
           << name << "'";
        _unit->error(os.str());
        return nullptr;
    }

    // Check whether enclosing interface has the same name.
    if (name == this->name())
    {
        ostringstream os;
        os << "interface name '" << name << "' cannot be used as operation name";
        _unit->error(os.str());
        return nullptr;
    }

    string newName = IceInternal::toLower(name);
    string thisName = IceInternal::toLower(this->name());
    if (newName == thisName)
    {
        ostringstream os;
        os << "operation '" << name << "' differs only in capitalization from enclosing interface name '"
           << this->name() << "'";
        _unit->error(os.str());
        return nullptr;
    }

    // Check whether any base has an operation with the same name already
    for (const auto& baseInterface : _bases)
    {
        vector<string> baseNames;
        for (const auto& op : baseInterface->allOperations())
        {
            baseNames.push_back(op->name());
        }
        if (!checkBaseOperationNames(name, baseNames))
        {
            return nullptr;
        }
    }

    // Check the operations of the Object pseudo-interface.
    if (!checkBaseOperationNames(name, {"ice_id", "ice_ids", "ice_ping", "ice_isA"}))
    {
        return nullptr;
    }

    OperationPtr op = make_shared<Operation>(shared_from_this(), name, returnType, isOptional, tag, mode);
    _unit->addContent(op);
    _contents.push_back(op);
    return op;
}

bool
Slice::InterfaceDef::checkBaseOperationNames(const string& name, const vector<string>& baseNames)
{
    string nameInLowercase = IceInternal::toLower(name);

    for (const auto& baseName : baseNames)
    {
        if (baseName == name)
        {
            ostringstream os;
            os << "operation '" << name << "' is already defined as an operation in a base interface";
            _unit->error(os.str());
            return false;
        }

        string baseNameInLowercase = IceInternal::toLower(baseName);

        if (baseNameInLowercase == nameInLowercase)
        {
            ostringstream os;
            os << "operation '" << name << "' differs only in capitalization from operation"
               << " '" << baseName << "', which is defined in a base interface";
            _unit->error(os.str());
            return false;
        }
    }
    return true;
}

InterfaceDeclPtr
Slice::InterfaceDef::declaration() const
{
    return _declaration;
}

InterfaceList
Slice::InterfaceDef::bases() const
{
    return _bases;
}

InterfaceList
Slice::InterfaceDef::allBases() const
{
    InterfaceList result = _bases;
    result.sort(containedCompare);
    for (const auto& p : _bases)
    {
        result.merge(p->allBases(), containedCompare);
        result.unique(containedEqual);
    }
    return result;
}

OperationList
Slice::InterfaceDef::operations() const
{
    OperationList result;
    for (const auto& p : _contents)
    {
        OperationPtr q = dynamic_pointer_cast<Operation>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

OperationList
Slice::InterfaceDef::allOperations() const
{
    OperationList result;
    for (const auto& p : _bases)
    {
        for (const auto& q : p->allOperations())
        {
            if (find_if(
                    result.begin(),
                    result.end(),
                    [name = q->name()](const auto& other) { return other->name() == name; }) == result.end())
            {
                result.push_back(q);
            }
        }
    }

    for (const auto& q : operations())
    {
        if (find_if(
                result.begin(),
                result.end(),
                [name = q->name()](const auto& other) { return other->name() == name; }) == result.end())
        {
            result.push_back(q);
        }
    }
    return result;
}

string
Slice::InterfaceDef::kindOf() const
{
    return "interface";
}

void
Slice::InterfaceDef::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<InterfaceDef>(shared_from_this());
    if (visitor->visitInterfaceDefStart(self))
    {
        Container::visit(visitor);
        visitor->visitInterfaceDefEnd(self);
    }
}

StringList
Slice::InterfaceDef::ids() const
{
    StringList ids;
    InterfaceList bases = allBases();
    std::transform(bases.begin(), bases.end(), back_inserter(ids), [](const auto& c) { return c->scoped(); });
    ids.push_back(scoped());
    ids.emplace_back("::Ice::Object");
    ids.sort();
    ids.unique();
    return ids;
}

MetadataList
Slice::InterfaceDef::getMetadata() const
{
    return _declaration->getMetadata();
}

void
Slice::InterfaceDef::setMetadata(MetadataList metadata)
{
    _declaration->setMetadata(std::move(metadata));
}

void
Slice::InterfaceDef::appendMetadata(MetadataList metadata)
{
    _declaration->appendMetadata(std::move(metadata));
}

Slice::InterfaceDef::InterfaceDef(const ContainerPtr& container, const string& name, InterfaceList bases)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Contained(container, name),
      _bases(std::move(bases))
{
}

// ----------------------------------------------------------------------
// Operation
// ----------------------------------------------------------------------

InterfaceDefPtr
Slice::Operation::interface() const
{
    return dynamic_pointer_cast<InterfaceDef>(_container);
}

TypePtr
Slice::Operation::returnType() const
{
    return _returnType;
}

bool
Slice::Operation::returnIsOptional() const
{
    return _returnIsOptional;
}

int
Slice::Operation::returnTag() const
{
    return _returnTag;
}

Operation::Mode
Slice::Operation::mode() const
{
    return _mode;
}

bool
Slice::Operation::hasMarshaledResult() const
{
    InterfaceDefPtr intf = interface();
    assert(intf);
    if (intf->hasMetadata("marshaled-result") || hasMetadata("marshaled-result"))
    {
        if (returnType() && isMutableAfterReturnType(returnType()))
        {
            return true;
        }

        for (const auto& p : _contents)
        {
            ParameterPtr q = dynamic_pointer_cast<Parameter>(p);
            if (q->isOutParam() && isMutableAfterReturnType(q->type()))
            {
                return true;
            }
        }
    }
    return false;
}

ParameterPtr
Slice::Operation::createParameter(const string& name, const TypePtr& type, bool isOutParam, bool isOptional, int tag)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() != name)
        {
            ostringstream os;
            os << "parameter '" << name << "' differs only in capitalization from parameter '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "redefinition of parameter '" << name << "'";
            _unit->error(os.str());
            return nullptr;
        }
    }

    checkIdentifier(name); // Don't return here -- we create the parameter anyway.

    //
    // Check that in parameters don't follow out parameters.
    //
    if (!_contents.empty())
    {
        ParameterPtr p = dynamic_pointer_cast<Parameter>(_contents.back());
        assert(p);
        if (p->isOutParam() && !isOutParam)
        {
            ostringstream os;
            os << "'" << name << "': in parameters cannot follow out parameters";
            _unit->error(os.str());
        }
    }

    if (isOptional)
    {
        // Check for a duplicate tag.
        ostringstream os;
        os << "tag for optional parameter '" << name << "' is already in use";
        if (_returnIsOptional && tag == _returnTag)
        {
            _unit->error(os.str());
        }
        else
        {
            for (const auto& p : parameters())
            {
                if (p->optional() && p->tag() == tag)
                {
                    _unit->error(os.str());
                    break;
                }
            }
        }
    }

    ParameterPtr p = make_shared<Parameter>(shared_from_this(), name, type, isOutParam, isOptional, tag);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

ParameterList
Slice::Operation::parameters() const
{
    ParameterList result;
    for (const auto& p : _contents)
    {
        ParameterPtr q = dynamic_pointer_cast<Parameter>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

ParameterList
Slice::Operation::inParameters() const
{
    ParameterList result;
    for (const auto& p : _contents)
    {
        ParameterPtr q = dynamic_pointer_cast<Parameter>(p);
        if (q && !q->isOutParam())
        {
            result.push_back(q);
        }
    }
    return result;
}

void
Slice::Operation::inParameters(ParameterList& required, ParameterList& optional) const
{
    for (const auto& pli : inParameters())
    {
        if (pli->optional())
        {
            optional.push_back(pli);
        }
        else
        {
            required.push_back(pli);
        }
    }
    optional.sort(compareTag<ParameterPtr>);
}

ParameterList
Slice::Operation::outParameters() const
{
    ParameterList result;

    for (const auto& p : _contents)
    {
        ParameterPtr q = dynamic_pointer_cast<Parameter>(p);
        if (q && q->isOutParam())
        {
            result.push_back(q);
        }
    }
    return result;
}

void
Slice::Operation::outParameters(ParameterList& required, ParameterList& optional) const
{
    for (const auto& pli : outParameters())
    {
        if (pli->optional())
        {
            optional.push_back(pli);
        }
        else
        {
            required.push_back(pli);
        }
    }
    optional.sort(compareTag<ParameterPtr>);
}

ExceptionList
Slice::Operation::throws() const
{
    return _throws;
}

void
Slice::Operation::setExceptionList(const ExceptionList& exceptions)
{
    _throws = exceptions;

    // Check that no exception occurs more than once in the throws clause.
    ExceptionList uniqueExceptions = exceptions;
    uniqueExceptions.sort(containedCompare);
    uniqueExceptions.unique(containedEqual);
    if (uniqueExceptions.size() != exceptions.size())
    {
        // At least one exception appears twice.
        ExceptionList tmp = exceptions;
        tmp.sort(containedCompare);
        ExceptionList duplicates;
        set_difference(
            tmp.begin(),
            tmp.end(),
            uniqueExceptions.begin(),
            uniqueExceptions.end(),
            back_inserter(duplicates),
            containedCompare);
        ostringstream os;
        os << "operation '" << name() << "' has a throws clause with ";
        if (duplicates.size() == 1)
        {
            os << "a ";
        }
        os << "duplicate exception";
        if (duplicates.size() > 1)
        {
            os << "s";
        }
        auto i = duplicates.begin();
        os << ": '" << (*i)->name() << "'";
        for (i = ++i; i != duplicates.end(); ++i)
        {
            os << ", '" << (*i)->name() << "'";
        }
        _unit->error(os.str());
    }
}

bool
Slice::Operation::sendsClasses() const
{
    for (const auto& i : parameters())
    {
        if (!i->isOutParam() && i->type()->usesClasses())
        {
            return true;
        }
    }
    return false;
}

bool
Slice::Operation::returnsClasses() const
{
    TypePtr t = returnType();
    if (t && t->usesClasses())
    {
        return true;
    }

    for (const auto& i : parameters())
    {
        if (i->isOutParam() && i->type()->usesClasses())
        {
            return true;
        }
    }
    return false;
}

bool
Slice::Operation::returnsData() const
{
    TypePtr t = returnType();
    if (t)
    {
        return true;
    }

    for (const auto& i : parameters())
    {
        if (i->isOutParam())
        {
            return true;
        }
    }

    if (!throws().empty())
    {
        return true;
    }
    return false;
}

bool
Slice::Operation::returnsMultipleValues() const
{
    size_t count = outParameters().size();

    if (returnType())
    {
        ++count;
    }

    return count > 1;
}

bool
Slice::Operation::sendsOptionals() const
{
    for (const auto& i : inParameters())
    {
        if (i->optional())
        {
            return true;
        }
    }
    return false;
}

std::optional<FormatType>
Slice::Operation::format() const
{
    std::optional<FormatType> format = parseFormatMetadata();
    if (!format)
    {
        ContainedPtr cont = dynamic_pointer_cast<Contained>(container());
        assert(cont);
        format = cont->parseFormatMetadata();
    }
    return format;
}

string
Slice::Operation::kindOf() const
{
    return "operation";
}

void
Slice::Operation::visit(ParserVisitor* visitor)
{
    visitor->visitOperation(dynamic_pointer_cast<Operation>(shared_from_this()));
}

Slice::Operation::Operation(
    const ContainerPtr& container,
    const string& name,
    TypePtr returnType,
    bool returnIsOptional,
    int returnTag,
    Mode mode)
    : SyntaxTreeBase(container->unit()),
      Contained(container, name),
      Container(container->unit()),
      _returnType(std::move(returnType)),
      _returnIsOptional(returnIsOptional),
      _returnTag(returnTag),
      _mode(mode)
{
}

// ----------------------------------------------------------------------
// Exception
// ----------------------------------------------------------------------

void
Slice::Exception::destroy()
{
    _base = nullptr;
    Container::destroy();
}

DataMemberPtr
Slice::Exception::createDataMember(
    const string& name,
    const TypePtr& type,
    bool isOptional,
    int tag,
    const SyntaxTreeBasePtr& defaultValueType,
    const string& defaultValueString)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() != name)
        {
            ostringstream os;
            os << "exception member '" << name << "' differs only in capitalization from exception member '"
               << matches.front()->name() << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "redefinition of exception member '" << name << "'";
            _unit->error(os.str());
            return nullptr;
        }
    }

    checkIdentifier(name); // Don't return here -- we create the data member anyway.

    // Check whether any bases have defined a member with the same name already.
    for (const auto& q : allBases())
    {
        for (const auto& member : q->dataMembers())
        {
            if (member->name() == name)
            {
                ostringstream os;
                os << "exception member '" << name << "' is already defined in a base exception";
                _unit->error(os.str());
                return nullptr;
            }

            string baseName = IceInternal::toLower(member->name());
            string newName = IceInternal::toLower(name);
            if (baseName == newName) // TODO use ciCompare
            {
                ostringstream os;
                os << "exception member '" << name << "' differs only in capitalization from exception member '"
                   << member->name() << "', which is defined in a base exception";
                _unit->error(os.str());
            }
        }
    }

    SyntaxTreeBasePtr dlt = defaultValueType;
    string dv = defaultValueString;

    if (dlt || (dynamic_pointer_cast<Enum>(type) && !dv.empty()))
    {
        // Validate the default value.
        if (!validateConstant(name, type, dlt, dv, false))
        {
            // Create the data member anyway, just without the default value.
            dlt = nullptr;
            dv.clear();
        }
    }

    if (isOptional)
    {
        // Validate the tag.
        for (const auto& q : dataMembers())
        {
            if (q->optional() && tag == q->tag())
            {
                ostringstream os;
                os << "tag for optional data member '" << name << "' is already in use";
                _unit->error(os.str());
                break;
            }
        }
    }

    DataMemberPtr p = make_shared<DataMember>(shared_from_this(), name, type, isOptional, tag, dlt, dv);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

DataMemberList
Slice::Exception::dataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

DataMemberList
Slice::Exception::orderedOptionalDataMembers() const
{
    return filterOrderedOptionalDataMembers(dataMembers());
}

// Return the data members of this exception and its parent exceptions, in base-to-derived order.
DataMemberList
Slice::Exception::allDataMembers() const
{
    DataMemberList result;

    // Check if we have a base exception. If so, recursively get the data members of the base exception(s).
    if (base())
    {
        result = base()->allDataMembers();
    }

    // Append this exceptions's data members.
    DataMemberList myMembers = dataMembers();
    result.splice(result.end(), myMembers);
    return result;
}

DataMemberList
Slice::Exception::classDataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q && q->type()->isClassType())
        {
            result.push_back(q);
        }
    }
    return result;
}

ExceptionPtr
Slice::Exception::base() const
{
    return _base;
}

ExceptionList
Slice::Exception::allBases() const
{
    ExceptionList result;
    if (_base)
    {
        result = _base->allBases();
        result.push_front(_base);
    }
    return result;
}

bool
Slice::Exception::isBaseOf(const ExceptionPtr& otherException) const
{
    string scope = scoped();
    if (scope == otherException->scoped())
    {
        return false;
    }

    for (const auto& i : otherException->allBases())
    {
        if (i->scoped() == scope)
        {
            return true;
        }
    }
    return false;
}

bool
Slice::Exception::usesClasses() const
{
    for (const auto& i : dataMembers())
    {
        if (i->type()->usesClasses())
        {
            return true;
        }
    }

    if (_base)
    {
        return _base->usesClasses();
    }
    return false;
}

string
Slice::Exception::kindOf() const
{
    return "exception";
}

void
Slice::Exception::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<Exception>(shared_from_this());
    if (visitor->visitExceptionStart(self))
    {
        Container::visit(visitor);
        visitor->visitExceptionEnd(self);
    }
}

Slice::Exception::Exception(const ContainerPtr& container, const string& name, ExceptionPtr base)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Contained(container, name),
      _base(std::move(base))
{
}

// ----------------------------------------------------------------------
// Struct
// ----------------------------------------------------------------------

DataMemberPtr
Slice::Struct::createDataMember(
    const string& name,
    const TypePtr& type,
    bool isOptional,
    int tag,
    const SyntaxTreeBasePtr& defaultValueType,
    const string& defaultValueString)
{
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        if (matches.front()->name() != name)
        {
            ostringstream os;
            os << "member '" << name << "' differs only in capitalization from member '" << matches.front()->name()
               << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "redefinition of struct member '" << name << "'";
            _unit->error(os.str());
            return nullptr;
        }
    }

    checkIdentifier(name); // Don't return here -- we create the data member anyway.

    // Structures cannot contain themselves.
    if (type.get() == this)
    {
        ostringstream os;
        os << "struct '" << this->name() << "' cannot contain itself";
        _unit->error(os.str());
        return nullptr;
    }

    SyntaxTreeBasePtr dlt = defaultValueType;
    string dv = defaultValueString;

    if (dlt || (dynamic_pointer_cast<Enum>(type) && !dv.empty()))
    {
        // Validate the default value.
        if (!validateConstant(name, type, dlt, dv, false))
        {
            // Create the data member anyway, just without the default value.
            dlt = nullptr;
            dv.clear();
        }
    }

    DataMemberPtr p = make_shared<DataMember>(shared_from_this(), name, type, isOptional, tag, dlt, dv);
    _unit->addContent(p);
    _contents.push_back(p);
    return p;
}

DataMemberList
Slice::Struct::dataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q)
        {
            result.push_back(q);
        }
    }
    return result;
}

DataMemberList
Slice::Struct::classDataMembers() const
{
    DataMemberList result;
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q && q->type()->isClassType())
        {
            result.push_back(q);
        }
    }
    return result;
}

string
Slice::Struct::getOptionalFormat() const
{
    return isVariableLength() ? "FSize" : "VSize";
}

bool
Slice::Struct::usesClasses() const
{
    for (const auto& p : _contents)
    {
        DataMemberPtr q = dynamic_pointer_cast<DataMember>(p);
        if (q)
        {
            if (q->type()->usesClasses())
            {
                return true;
            }
        }
    }
    return false;
}

size_t
Slice::Struct::minWireSize() const
{
    // At least the sum of the minimum member sizes.
    size_t sz = 0;
    for (const auto& i : dataMembers())
    {
        sz += i->type()->minWireSize();
    }
    return sz;
}

bool
Slice::Struct::isVariableLength() const
{
    for (const auto& i : dataMembers())
    {
        if (i->type()->isVariableLength())
        {
            return true;
        }
    }
    return false;
}

string
Slice::Struct::kindOf() const
{
    return "struct";
}

void
Slice::Struct::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<Struct>(shared_from_this());
    if (visitor->visitStructStart(self))
    {
        Container::visit(visitor);
        visitor->visitStructEnd(self);
    }
}

Slice::Struct::Struct(const ContainerPtr& container, const string& name)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Type(),
      Contained(container, name),
      Constructed()
{
}

// ----------------------------------------------------------------------
// Sequence
// ----------------------------------------------------------------------

TypePtr
Slice::Sequence::type() const
{
    return _type;
}

MetadataList
Slice::Sequence::typeMetadata() const
{
    return _typeMetadata;
}

void
Slice::Sequence::setTypeMetadata(MetadataList metadata)
{
    _typeMetadata = std::move(metadata);
}

bool
Slice::Sequence::usesClasses() const
{
    return _type->usesClasses();
}

size_t
Slice::Sequence::minWireSize() const
{
    return 1; // An empty sequence.
}

bool
Slice::Sequence::isVariableLength() const
{
    return true;
}

string
Slice::Sequence::getOptionalFormat() const
{
    return _type->isVariableLength() ? "FSize" : "VSize";
}

string
Slice::Sequence::kindOf() const
{
    return "sequence";
}

void
Slice::Sequence::visit(ParserVisitor* visitor)
{
    visitor->visitSequence(shared_from_this());
}

Slice::Sequence::Sequence(const ContainerPtr& container, const string& name, TypePtr type, MetadataList typeMetadata)
    : SyntaxTreeBase(container->unit()),
      Type(),
      Contained(container, name),
      Constructed(),
      _type(std::move(type)),
      _typeMetadata(std::move(typeMetadata))
{
}

// ----------------------------------------------------------------------
// Dictionary
// ----------------------------------------------------------------------

TypePtr
Slice::Dictionary::keyType() const
{
    return _keyType;
}

TypePtr
Slice::Dictionary::valueType() const
{
    return _valueType;
}

MetadataList
Slice::Dictionary::keyMetadata() const
{
    return _keyMetadata;
}

MetadataList
Slice::Dictionary::valueMetadata() const
{
    return _valueMetadata;
}

void
Slice::Dictionary::setKeyMetadata(MetadataList metadata)
{
    _keyMetadata = std::move(metadata);
}

void
Slice::Dictionary::setValueMetadata(MetadataList metadata)
{
    _valueMetadata = std::move(metadata);
}

bool
Slice::Dictionary::usesClasses() const
{
    return _valueType->usesClasses();
}

size_t
Slice::Dictionary::minWireSize() const
{
    return 1; // An empty dictionary.
}

string
Slice::Dictionary::getOptionalFormat() const
{
    return _keyType->isVariableLength() || _valueType->isVariableLength() ? "FSize" : "VSize";
}

bool
Slice::Dictionary::isVariableLength() const
{
    return true;
}

string
Slice::Dictionary::kindOf() const
{
    return "dictionary";
}

void
Slice::Dictionary::visit(ParserVisitor* visitor)
{
    visitor->visitDictionary(shared_from_this());
}

// Checks whether the provided type is a legal dictionary key type.
// Legal key types are integral types, string, and structs that only contain other legal key types.
bool
Slice::Dictionary::isLegalKeyType(const TypePtr& type)
{
    BuiltinPtr bp = dynamic_pointer_cast<Builtin>(type);
    if (bp)
    {
        switch (bp->kind())
        {
            case Builtin::KindByte:
            case Builtin::KindBool:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            case Builtin::KindString:
            {
                return true;
            }

            case Builtin::KindFloat:
            case Builtin::KindDouble:
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            case Builtin::KindValue:
            {
                return false;
            }
        }
    }

    if (dynamic_pointer_cast<Enum>(type))
    {
        return true;
    }

    StructPtr structPtr = dynamic_pointer_cast<Struct>(type);
    if (structPtr)
    {
        for (const auto& dm : structPtr->dataMembers())
        {
            if (!isLegalKeyType(dm->type()))
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

Slice::Dictionary::Dictionary(
    const ContainerPtr& container,
    const string& name,
    TypePtr keyType,
    MetadataList keyMetadata,
    TypePtr valueType,
    MetadataList valueMetadata)
    : SyntaxTreeBase(container->unit()),
      Type(),
      Contained(container, name),
      Constructed(),
      _keyType(std::move(keyType)),
      _valueType(std::move(valueType)),
      _keyMetadata(std::move(keyMetadata)),
      _valueMetadata(std::move(valueMetadata))
{
}

// ----------------------------------------------------------------------
// Enum
// ----------------------------------------------------------------------

void
Slice::Enum::destroy()
{
    SyntaxTreeBase::destroy();
}

EnumeratorPtr
Slice::Enum::createEnumerator(const string& name, optional<int> explicitValue)
{
    // Validate the enumerator's name.
    ContainedList matches = _unit->findContents(thisScope() + name);
    if (!matches.empty())
    {
        EnumeratorPtr p = dynamic_pointer_cast<Enumerator>(matches.front());
        if (matches.front()->name() == name)
        {
            ostringstream os;
            os << "redefinition of enumerator '" << name << "'";
            _unit->error(os.str());
        }
        else
        {
            ostringstream os;
            os << "enumerator '" << name << "' differs only in capitalization from '" << matches.front()->name() << "'";
            _unit->error(os.str());
        }
    }
    checkIdentifier(name); // Ignore return value.

    // Determine the enumerator's value, and check that it's valid.
    int nextValue;
    if (explicitValue)
    {
        // If an explicit value was provided, the parser already checks that it's between `0` and `int32_t::max`.
        _hasExplicitValues = true;
        nextValue = *explicitValue;
    }
    else
    {
        if (_lastValue == numeric_limits<int32_t>::max())
        {
            ostringstream os;
            os << "value for enumerator '" << name << "' is out of range";
            _unit->error(os.str());
        }
        // If the enumerator was not assigned an explicit value,
        // we automatically assign it one more than the previous enumerator.
        nextValue = _lastValue + 1;
    }

    // Check if the enumerator's value is already in use.
    bool checkForDuplicates = true;
    if (nextValue > _maxValue)
    {
        _maxValue = nextValue;
        checkForDuplicates = false;
    }
    if (nextValue < _minValue)
    {
        _minValue = nextValue;
        checkForDuplicates = false;
    }
    if (checkForDuplicates)
    {
        for (const auto& r : enumerators())
        {
            if (r->value() == nextValue)
            {
                ostringstream os;
                os << "enumerator '" << name << "' has the same value as enumerator '" << r->name() << "'";
                _unit->error(os.str());
            }
        }
    }

    // Create the enumerator.
    ContainerPtr cont = shared_from_this();
    EnumeratorPtr p = make_shared<Enumerator>(cont, name, nextValue, explicitValue.has_value());
    _unit->addContent(p);
    _contents.push_back(p);
    _lastValue = nextValue;
    return p;
}

bool
Slice::Enum::hasExplicitValues() const
{
    return _hasExplicitValues;
}

int
Slice::Enum::minValue() const
{
    return static_cast<int>(_minValue);
}

int
Slice::Enum::maxValue() const
{
    return static_cast<int>(_maxValue);
}

size_t
Slice::Enum::minWireSize() const
{
    return 1;
}

string
Slice::Enum::getOptionalFormat() const
{
    return "Size";
}

bool
Slice::Enum::isVariableLength() const
{
    return true;
}

string
Slice::Enum::kindOf() const
{
    return "enumeration";
}

void
Slice::Enum::visit(ParserVisitor* visitor)
{
    visitor->visitEnum(dynamic_pointer_cast<Enum>(shared_from_this()));
}

Slice::Enum::Enum(const ContainerPtr& container, const string& name)
    : SyntaxTreeBase(container->unit()),
      Container(container->unit()),
      Type(),
      Contained(container, name),
      Constructed(),
      _minValue(numeric_limits<int32_t>::max())
{
}

// ----------------------------------------------------------------------
// Enumerator
// ----------------------------------------------------------------------

EnumPtr
Slice::Enumerator::type() const
{
    return dynamic_pointer_cast<Enum>(container());
}

string
Slice::Enumerator::kindOf() const
{
    return "enumerator";
}

bool
Slice::Enumerator::hasExplicitValue() const
{
    return _hasExplicitValue;
}

int
Slice::Enumerator::value() const
{
    return _value;
}

Slice::Enumerator::Enumerator(const ContainerPtr& container, const string& name, int value, bool hasExplicitValue)
    : SyntaxTreeBase(container->unit()),
      Contained(container, name),
      _hasExplicitValue(hasExplicitValue),
      _value(value)
{
}

// ----------------------------------------------------------------------
// Const
// ----------------------------------------------------------------------

TypePtr
Slice::Const::type() const
{
    return _type;
}

MetadataList
Slice::Const::typeMetadata() const
{
    return _typeMetadata;
}

void
Slice::Const::setTypeMetadata(MetadataList metadata)
{
    _typeMetadata = std::move(metadata);
}

SyntaxTreeBasePtr
Slice::Const::valueType() const
{
    return _valueType;
}

string
Slice::Const::value() const
{
    return _value;
}

string
Slice::Const::kindOf() const
{
    return "constant";
}

void
Slice::Const::visit(ParserVisitor* visitor)
{
    visitor->visitConst(shared_from_this());
}

Slice::Const::Const(
    const ContainerPtr& container,
    const string& name,
    TypePtr type,
    MetadataList typeMetadata,
    SyntaxTreeBasePtr valueType,
    string valueString)
    : SyntaxTreeBase(container->unit()),
      Contained(container, name),
      _type(std::move(type)),
      _typeMetadata(std::move(typeMetadata)),
      _valueType(std::move(valueType)),
      _value(std::move(valueString))
{
}

// ----------------------------------------------------------------------
// Parameter
// ----------------------------------------------------------------------

TypePtr
Slice::Parameter::type() const
{
    return _type;
}

bool
Slice::Parameter::isOutParam() const
{
    return _isOutParam;
}

bool
Slice::Parameter::optional() const
{
    return _optional;
}

int
Slice::Parameter::tag() const
{
    return _tag;
}

string
Slice::Parameter::kindOf() const
{
    return "parameter declaration";
}

void
Slice::Parameter::visit(ParserVisitor* visitor)
{
    visitor->visitParameter(shared_from_this());
}

Slice::Parameter::Parameter(
    const ContainerPtr& container,
    const string& name,
    TypePtr type,
    bool isOutParam,
    bool isOptional,
    int tag)
    : SyntaxTreeBase(container->unit()),
      Contained(container, name),
      _type(std::move(type)),
      _isOutParam(isOutParam),
      _optional(isOptional),
      _tag(tag)
{
}

// ----------------------------------------------------------------------
// DataMember
// ----------------------------------------------------------------------

TypePtr
Slice::DataMember::type() const
{
    return _type;
}

bool
Slice::DataMember::optional() const
{
    return _optional;
}

int
Slice::DataMember::tag() const
{
    return _tag;
}

string
Slice::DataMember::defaultValue() const
{
    return _defaultValue;
}

SyntaxTreeBasePtr
Slice::DataMember::defaultValueType() const
{
    return _defaultValueType;
}

string
Slice::DataMember::kindOf() const
{
    return "data member";
}

void
Slice::DataMember::visit(ParserVisitor* visitor)
{
    visitor->visitDataMember(shared_from_this());
}

Slice::DataMember::DataMember(
    const ContainerPtr& container,
    const string& name,
    TypePtr type,
    bool isOptional,
    int tag,
    SyntaxTreeBasePtr defaultValueType,
    string defaultValueString)
    : SyntaxTreeBase(container->unit()),
      Contained(container, name),
      _type(std::move(type)),
      _optional(isOptional),
      _tag(tag),
      _defaultValueType(std::move(defaultValueType)),
      _defaultValue(std::move(defaultValueString))
{
}

// ----------------------------------------------------------------------
// Unit
// ----------------------------------------------------------------------

UnitPtr
Slice::Unit::createUnit(string languageName, bool all, const StringList& defaultFileMetadata)
{
    MetadataList defaultMetadata;
    for (const auto& metadataString : defaultFileMetadata)
    {
        defaultMetadata.push_back(make_shared<Metadata>(metadataString, "<command-line>", 0));
    }

    UnitPtr unit{new Unit{std::move(languageName), all, std::move(defaultMetadata)}};
    unit->_unit = unit;
    return unit;
}

string
Slice::Unit::languageName() const
{
    return _languageName;
}

void
Slice::Unit::setDocComment(const string& comment)
{
    _currentDocComment = "";

    string::size_type end = 0;
    while (true)
    {
        string::size_type begin;
        if (end == 0)
        {
            // Skip past the initial whitespace.
            begin = comment.find_first_not_of(" \t\r\n*", end);
        }
        else
        {
            // Skip more whitespace but retain blank lines.
            begin = comment.find_first_not_of(" \t*", end);
        }

        if (begin == string::npos)
        {
            break;
        }

        end = comment.find('\n', begin);
        if (end != string::npos)
        {
            if (end + 1 > begin)
            {
                _currentDocComment += comment.substr(begin, end + 1 - begin);
            }
            ++end;
        }
        else
        {
            end = comment.find_last_not_of(" \t\r\n*");
            if (end != string::npos)
            {
                if (end + 1 > begin)
                {
                    _currentDocComment += comment.substr(begin, end + 1 - begin);
                }
            }
            break;
        }
    }
}

void
Slice::Unit::addToDocComment(const string& comment)
{
    if (!_currentDocComment.empty())
    {
        _currentDocComment += '\n';
    }
    _currentDocComment += comment;
}

string
Slice::Unit::currentDocComment()
{
    string comment;
    comment.swap(_currentDocComment);
    return comment;
}

string
Slice::Unit::currentFile() const
{
    DefinitionContextPtr dc = currentDefinitionContext();
    if (dc)
    {
        return dc->filename();
    }
    else
    {
        return {};
    }
}

string
Slice::Unit::topLevelFile() const
{
    return _topLevelFile;
}

int
Slice::Unit::currentLine() const
{
    return slice_lineno;
}

int
Slice::Unit::setCurrentFile(const std::string& currentFile, int lineNumber)
{
    assert(!currentFile.empty());

    enum LineType
    {
        File,
        Push,
        Pop
    };

    LineType type = File;

    if (lineNumber == 0)
    {
        if (_currentIncludeLevel > 0 || currentFile != _topLevelFile)
        {
            type = Push;
        }
    }
    else
    {
        DefinitionContextPtr dc = currentDefinitionContext();
        if (dc != nullptr && !dc->filename().empty() && dc->filename() != currentFile)
        {
            type = Pop;
        }
    }

    switch (type)
    {
        case Push:
        {
            if (++_currentIncludeLevel == 1)
            {
                if (find(_includeFiles.begin(), _includeFiles.end(), currentFile) == _includeFiles.end())
                {
                    _includeFiles.push_back(currentFile);
                }
            }
            pushDefinitionContext();
            _currentDocComment = "";
            break;
        }
        case Pop:
        {
            --_currentIncludeLevel;
            popDefinitionContext();
            _currentDocComment = "";
            break;
        }
        default:
        {
            break; // Do nothing
        }
    }

    DefinitionContextPtr dc = currentDefinitionContext();
    assert(dc);
    dc->setFilename(currentFile);
    _definitionContextMap.insert(make_pair(currentFile, dc));

    return static_cast<int>(type);
}

int
Slice::Unit::currentIncludeLevel() const
{
    if (_all)
    {
        return 0;
    }
    else
    {
        return _currentIncludeLevel;
    }
}

void
Slice::Unit::addFileMetadata(MetadataList metadata)
{
    DefinitionContextPtr dc = currentDefinitionContext();
    assert(dc);
    if (dc->seenDefinition())
    {
        error("file metadata must appear before any definitions");
    }
    else
    {
        dc->appendMetadata(std::move(metadata));
    }
}

void
Slice::Unit::setSeenDefinition()
{
    DefinitionContextPtr dc = currentDefinitionContext();
    assert(dc);
    dc->setSeenDefinition();
}

void
Slice::Unit::error(string_view message)
{
    error(currentFile(), currentLine(), message);
}

void
Slice::Unit::error(string_view file, int line, string_view message)
{
    emitError(file, line, message);
    _errors++;
}

void
Slice::Unit::warning(WarningCategory category, string_view message) const
{
    warning(currentFile(), currentLine(), category, message);
}

void
Slice::Unit::warning(string_view file, int line, WarningCategory category, string_view message) const
{
    DefinitionContextPtr dc = findDefinitionContext(file);
    assert(dc);

    // Determine whether the warning should be ignored by checking for any 'suppress-warning' metadata in this context.
    if (!dc->isSuppressed(category))
    {
        emitWarning(file, line, message);
    }
}

ContainerPtr
Slice::Unit::currentContainer() const
{
    assert(!_containerStack.empty());
    return _containerStack.top();
}

void
Slice::Unit::pushContainer(const ContainerPtr& container)
{
    _containerStack.push(container);
}

void
Slice::Unit::popContainer()
{
    assert(!_containerStack.empty());
    _containerStack.pop();
}

DefinitionContextPtr
Slice::Unit::currentDefinitionContext() const
{
    DefinitionContextPtr dc;
    if (!_definitionContextStack.empty())
    {
        dc = _definitionContextStack.top();
    }
    return dc;
}

void
Slice::Unit::pushDefinitionContext()
{
    _definitionContextStack.push(make_shared<DefinitionContext>(_currentIncludeLevel, _defaultFileMetadata));
}

void
Slice::Unit::popDefinitionContext()
{
    assert(!_definitionContextStack.empty());
    _definitionContextStack.pop();
}

DefinitionContextPtr
Slice::Unit::findDefinitionContext(string_view file) const
{
    auto p = _definitionContextMap.find(file);
    if (p != _definitionContextMap.end())
    {
        return p->second;
    }
    return nullptr;
}

void
Slice::Unit::addContent(const ContainedPtr& contained)
{
    string scoped = IceInternal::toLower(contained->scoped());
    _contentMap[scoped].push_back(contained);
}

ContainedList
Slice::Unit::findContents(const string& scopedName) const
{
    assert(!scopedName.empty());
    assert(scopedName[0] == ':');

    string name = IceInternal::toLower(scopedName);
    auto p = _contentMap.find(name);
    if (p != _contentMap.end())
    {
        return p->second;
    }
    else
    {
        return {};
    }
}

void
Slice::Unit::addTypeId(int compactId, const std::string& typeId)
{
    _typeIds.insert(make_pair(compactId, typeId));
}

std::string
Slice::Unit::getTypeId(int compactId) const
{
    auto p = _typeIds.find(compactId);
    if (p != _typeIds.end())
    {
        return p->second;
    }
    return {};
}

StringList
Slice::Unit::includeFiles() const
{
    return _includeFiles;
}

StringList
Slice::Unit::allFiles() const
{
    StringList result;
    for (const auto& [key, value] : _definitionContextMap)
    {
        result.push_back(key);
    }
    return result;
}

int
Slice::Unit::parse(const string& filename, FILE* file, bool debugMode)
{
    slice_debug = debugMode ? 1 : 0;
    slice__flex_debug = debugMode ? 1 : 0;

    assert(!Slice::currentUnit);
    Slice::currentUnit = this;

    _currentDocComment = "";
    _currentIncludeLevel = 0;
    _topLevelFile = fullPath(filename);
    pushContainer(shared_from_this());
    pushDefinitionContext();
    setCurrentFile(_topLevelFile, 0);

    slice_in = file;
    int status = slice_parse();
    if (_errors)
    {
        status = EXIT_FAILURE;
    }

    if (status == EXIT_FAILURE)
    {
        while (!_containerStack.empty())
        {
            popContainer();
        }
        while (!_definitionContextStack.empty())
        {
            popDefinitionContext();
        }
    }
    else
    {
        assert(_containerStack.size() == 1);
        popContainer();
        assert(_definitionContextStack.size() == 1);
        popDefinitionContext();
    }

    Slice::currentUnit = nullptr;
    return status;
}

void
Slice::Unit::destroy()
{
    _contentMap.clear();
    _builtins.clear();
    Container::destroy();
}

void
Slice::Unit::visit(ParserVisitor* visitor)
{
    auto self = dynamic_pointer_cast<Unit>(shared_from_this());
    if (visitor->visitUnitStart(self))
    {
        Container::visit(visitor);
        visitor->visitUnitEnd(self);
    }
}

BuiltinPtr
Slice::Unit::createBuiltin(Builtin::Kind kind)
{
    auto p = _builtins.find(kind);
    if (p != _builtins.end())
    {
        return p->second;
    }
    auto builtin = make_shared<Builtin>(dynamic_pointer_cast<Unit>(shared_from_this()), kind);
    _builtins.insert(make_pair(kind, builtin));
    return builtin;
}

void
Slice::Unit::addTopLevelModule(const string& file, const string& module)
{
    auto i = _fileTopLevelModules.find(file);
    if (i == _fileTopLevelModules.end())
    {
        set<string> modules;
        modules.insert(module);
        _fileTopLevelModules.insert(make_pair(file, modules));
    }
    else
    {
        i->second.insert(module);
    }
}

set<string>
Slice::Unit::getTopLevelModules(const string& file) const
{
    auto i = _fileTopLevelModules.find(file);
    if (i == _fileTopLevelModules.end())
    {
        return {};
    }
    else
    {
        return i->second;
    }
}

Slice::Unit::Unit(string languageName, bool all, MetadataList defaultFileMetadata)
    : SyntaxTreeBase(nullptr),
      Container(nullptr),
      _languageName(std::move(languageName)),
      _all(all),
      _defaultFileMetadata(std::move(defaultFileMetadata))
{
    if (!languageName.empty())
    {
        assert(binary_search(&languages[0], &languages[sizeof(languages) / sizeof(*languages)], _languageName));
    }
}

// ----------------------------------------------------------------------
// CICompare
// ----------------------------------------------------------------------

bool
Slice::CICompare::operator()(const string& lhs, const string& rhs) const
{
    string::const_iterator p1 = lhs.begin();
    string::const_iterator p2 = rhs.begin();
    while (p1 != lhs.end() && p2 != rhs.end() &&
           ::tolower(static_cast<unsigned char>(*p1)) == ::tolower(static_cast<unsigned char>(*p2)))
    {
        ++p1;
        ++p2;
    }
    if (p1 == lhs.end() && p2 == rhs.end())
    {
        return false;
    }
    else if (p1 == lhs.end())
    {
        return true;
    }
    else if (p2 == rhs.end())
    {
        return false;
    }
    else
    {
        return ::tolower(static_cast<unsigned char>(*p1)) < ::tolower(static_cast<unsigned char>(*p2));
    }
}

// ----------------------------------------------------------------------
// DerivedToBaseCompare
// ----------------------------------------------------------------------

bool
Slice::DerivedToBaseCompare::operator()(const ExceptionPtr& lhs, const ExceptionPtr& rhs) const
{
    return rhs->isBaseOf(lhs);
}
