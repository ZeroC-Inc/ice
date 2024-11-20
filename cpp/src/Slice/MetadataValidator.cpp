// Copyright (c) ZeroC, Inc.

#include "MetadataValidator.h"
#include "Ice/StringUtil.h"
#include "Util.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using namespace std;
using namespace Slice;

Slice::MetadataValidator::MetadataValidator(string language, map<string, MetadataInfo> metadataSpecification)
    : _language(std::move(language)),
      _metadataSpecification(std::move(metadataSpecification))
{
    // We want to perform all the metadata validation in the same pass, to keep all the diagnostics in order.
    // So, we add all the language-agnostic metadata validation into the provided list:

    // "amd"
    MetadataInfo amdInfo = {
        {typeid(InterfaceDef), typeid(Operation)},
        MetadataArgumentKind::NoArguments,
    };
    _metadataSpecification.emplace("amd", std::move(amdInfo));

    // "deprecated"
    MetadataInfo deprecatedInfo = {
        {typeid(InterfaceDecl),
         typeid(InterfaceDef),
         typeid(ClassDecl),
         typeid(ClassDef),
         typeid(Operation),
         typeid(Exception),
         typeid(Struct),
         typeid(Sequence),
         typeid(Dictionary),
         typeid(Enum),
         typeid(Enumerator),
         typeid(Const),
         typeid(DataMember)},
        MetadataArgumentKind::OptionalTextArgument,
    };
    _metadataSpecification.emplace("deprecated", std::move(deprecatedInfo));

    // "format"
    MetadataInfo formatInfo = {
        {typeid(InterfaceDef), typeid(Operation)},
        MetadataArgumentKind::SingleArgument,
        {{"compact", "sliced", "default"}},
    };
    _metadataSpecification.emplace("format", std::move(formatInfo));

    // "marshaled-result"
    MetadataInfo marshaledResultInfo = {
        {typeid(InterfaceDef), typeid(Operation)},
        MetadataArgumentKind::NoArguments,
    };
    _metadataSpecification.emplace("marshaled-result", std::move(marshaledResultInfo));

    // "protected"
    MetadataInfo protectedInfo = {
        {typeid(ClassDef), typeid(Slice::Exception), typeid(Struct), typeid(DataMember)},
        MetadataArgumentKind::NoArguments,
    };
    _metadataSpecification.emplace("protected", std::move(protectedInfo));

    // "suppress-warning"
    MetadataInfo suppressWarningInfo = {
        {typeid(Unit)},
        MetadataArgumentKind::AnyNumberOfArguments,
        {{"all", "deprecated", "invalid-metadata", "invalid-comment"}},
    };
    suppressWarningInfo.mustBeUnique = false;
    _metadataSpecification.emplace("suppress-warning", std::move(suppressWarningInfo));

    // TODO: we should probably just remove this metadata. It's only checked by slice2java,
    // and there's already a 'java:UserException' metadata that we also check... better to only keep that one.
    // "UserException"
    MetadataInfo userExceptionInfo = {
        {typeid(Operation)},
        MetadataArgumentKind::NoArguments,
    };
    _metadataSpecification.emplace("UserException", std::move(userExceptionInfo));
}

bool
Slice::MetadataValidator::visitUnitStart(const UnitPtr& p)
{
    DefinitionContextPtr dc = p->findDefinitionContext(p->topLevelFile());
    assert(dc);
    dc->setMetadata(validate(dc->getMetadata(), p));
    return true;
}

bool
Slice::MetadataValidator::visitModuleStart(const ModulePtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    return true;
}

void
Slice::MetadataValidator::visitClassDecl(const ClassDeclPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
}

bool
Slice::MetadataValidator::visitClassDefStart(const ClassDefPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    return true;
}

void
Slice::MetadataValidator::visitInterfaceDecl(const InterfaceDeclPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
}

bool
Slice::MetadataValidator::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    return true;
}

bool
Slice::MetadataValidator::visitExceptionStart(const ExceptionPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    return true;
}

bool
Slice::MetadataValidator::visitStructStart(const StructPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    return true;
}

void
Slice::MetadataValidator::visitOperation(const OperationPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    for (const auto& param : p->parameters())
    {
        param->visit(this);
    }
}

void
Slice::MetadataValidator::visitParamDecl(const ParamDeclPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
}

void
Slice::MetadataValidator::visitDataMember(const DataMemberPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
}

void
Slice::MetadataValidator::visitSequence(const SequencePtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    p->setTypeMetadata(validate(p->typeMetadata(), p->type(), true));
}

void
Slice::MetadataValidator::visitDictionary(const DictionaryPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    p->setKeyMetadata(validate(p->keyMetadata(), p->keyType(), true));
    p->setValueMetadata(validate(p->valueMetadata(), p->valueType(), true));
}

void
Slice::MetadataValidator::visitEnum(const EnumPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
}

void
Slice::MetadataValidator::visitConst(const ConstPtr& p)
{
    p->setMetadata(validate(p->getMetadata(), p));
    p->setTypeMetadata(validate(p->typeMetadata(), p->type(), true));
}

MetadataList
Slice::MetadataValidator::validate(MetadataList metadata, const SyntaxTreeBasePtr& p, bool isTypeContext)
{
    // Reset the set of 'seenDirectives' now that we're visiting a new Slice element.
    _seenDirectives.clear();

    // Iterate through the provided metadata and check each one for validity.
    // If we come across any invalid metadata, we remove it from the list (i.e. we filter out invalid metadata).
    for (MetadataList::const_iterator i = metadata.begin(); i != metadata.end();)
    {
        const string& directive = (*i)->directive();

        // If the directive contains a ':' character, but is for a different language than what we're checking,
        // we mark it for removal, but perform no additional validation of it.
        if (directive.find(':') != string::npos && directive.find(_language) != 0)
        {
            i = metadata.erase(i);
        }
        else
        {
            // If the metadata is invalid, remove it. Otherwise we advance to the next metadata like normal.
            bool isValid = isMetadataValid(*i, p, isTypeContext);
            i = isValid ? std::next(i, 1) : metadata.erase(i);
        }
    }

    return metadata;
}

bool
Slice::MetadataValidator::isMetadataValid(const MetadataPtr& metadata, const SyntaxTreeBasePtr& p, bool isTypeContext)
{
    // First, we check if the metadata is one we know of. If it isn't, we issue a warning and immediately return.
    const string& directive = metadata->directive();
    auto lookupResult = _metadataSpecification.find(directive);
    if (lookupResult == _metadataSpecification.end())
    {
        ostringstream msg;
        msg << "ignoring unknown metadata: '" << *metadata << '\'';
        p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg.str());
        return false;
    }
    const MetadataInfo& info = lookupResult->second;

    // If we get here, then it's metadata that we know about!
    bool isValid = true;

    // Second, we check to make sure that the correct number of arguments were provided.
    const string& arguments = metadata->arguments();
    switch (info.acceptedArguments)
    {
        case MetadataArgumentKind::NoArguments:
            if (!arguments.empty())
            {
                string msg = "the '" + directive + "' metadata does not take any arguments";
                p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
                isValid = false;
            }
            break;

        case MetadataArgumentKind::SingleArgument:
            // Make sure there's no commas in the arguments (i.e. make sure it's not a list).
            if (arguments.find(',') != string::npos)
            {
                string msg = "the '" + directive + "' metadata only accepts one argument but a list was provided";
                p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
                isValid = false;
            }
            // Then intentionally fall through to the non-empty check below, since we require an argument.
            [[fallthrough]];

        case MetadataArgumentKind::RequiredTextArgument:
            if (arguments.empty())
            {
                string msg = "missing required argument for '" + directive + "' metadata";
                p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
                isValid = false;
            }
            break;

        // We don't need to validate the number of arguments if it allows "any number of arguments".
        case MetadataArgumentKind::AnyNumberOfArguments:
        // Or if it's `OptionalTextArgument` since it can be anything, including the empty string.
        case MetadataArgumentKind::OptionalTextArgument:
            break;
    }

    // Third, we check if the argument values are valid (if the metadata restricts what values can be supplied).
    if (info.validArgumentValues.has_value())
    {
        // Split the arguments into a string delimited list and check each one.
        vector<string> argList;
        IceInternal::splitString(arguments, ",", argList);

        const StringList& validValues = *info.validArgumentValues;
        for (const auto& arg : argList)
        {
            string trimmedArg = IceInternal::trim(arg);
            if (std::find(validValues.begin(), validValues.end(), trimmedArg) == validValues.end())
            {
                string msg = "invalid argument '" + trimmedArg + "' supplied to '" + directive + "' metadata";
                p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
                isValid = false;
            }
        }
    }

    // Fourth, we check what the metadata was applied to - does that Slice definition support this metadata?
    SyntaxTreeBasePtr appliedTo;
    if (info.acceptedContexts == MetadataApplicationContext::Definitions)
    {
        if (isTypeContext)
        {
            string msg = '\'' + directive + "' metadata can only be applied to definitions and declarations" +
                         ", it cannot be applied to type references";
            p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
            isValid = false;
        }
        else
        {
            appliedTo = p;
        }
    }
    else
    {
        // Some metadata can only be applied to parameters (and return types). This bool stores that information.
        bool isAppliedToParameterType = false;

        // If the metadata we're validating can be applied to type references, but it was applied to an operation,
        // we treat the metadata as if it was applied to that operation's return type.
        // Same thing if this metadata has been applied to a parameter or data member as well.
        if (auto op = dynamic_pointer_cast<Operation>(p))
        {
            isAppliedToParameterType = true;
            if (const auto returnType = op->returnType())
            {
                appliedTo = returnType;
            }
            else
            {
                string msg = '\'' + directive + "' metadata cannot be applied to operations with void return type";
                p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
                isValid = false;
            }
        }
        else if (auto param = dynamic_pointer_cast<ParamDecl>(p))
        {
            isAppliedToParameterType = true;
            appliedTo = param->type();
        }
        else if (auto dm = dynamic_pointer_cast<DataMember>(p))
        {
            appliedTo = dm->type();
        }
        else
        {
            // Otherwise if there's nothing special going on, and we know that the metadata was applied directly to `p`.
            appliedTo = p;
        }

        // If this metadata is only valid in the context of parameters, issue a warning if that condition wasn't met.
        if (info.acceptedContexts == MetadataApplicationContext::ParameterTypeReferences && !isAppliedToParameterType)
        {
            string msg = '\'' + directive + "' metadata can only be applied to operation parameters and return types";
            p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
            isValid = false;
        }
    }

    // Now that we've deduced exactly what Slice element the metadata should be applied to, check that it's supported.
    const list<reference_wrapper<const type_info>>& validOn = info.validOn;
    if (!validOn.empty() && appliedTo) // 'appliedTo' will be null if we already found a problem and should stop.
    {
        auto appliedToPtr = appliedTo.get();
        auto typeComparator = [&](reference_wrapper<const type_info> t) { return t.get() == typeid(*appliedToPtr); };
        if (std::find_if(validOn.begin(), validOn.end(), typeComparator) == validOn.end())
        {
            string message = misappliedMetadataMessage(metadata, appliedTo);
            p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, message);
            isValid = false;
        }
    }

    // Fifth we check if this metadata is a duplicate, i.e. has the same directive already been applied in this context?
    if (info.mustBeUnique)
    {
        // 'insert' only returns `true` if the value wasn't already present in the set.
        bool wasInserted = _seenDirectives.insert(directive).second;
        if (!wasInserted)
        {
            string msg = "ignoring duplicate metadata: '" + directive + "' has already been applied in this context";
            p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, msg);
            isValid = false;
        }
    }

    // Finally, if a custom validation function is specified for this metadata, we run it.
    if (info.extraValidation && appliedTo) // 'appliedTo' will be null if we already found a problem and should stop.
    {
        // This function will return `nullopt` to signal everything is okay.
        // So if we get a string back, we know that the custom validation failed.
        if (auto result = info.extraValidation(metadata, appliedTo))
        {
            p->unit()->warning(metadata->file(), metadata->line(), InvalidMetadata, *result);
            isValid = false;
        }
    }

    return isValid;
}

std::string
Slice::MetadataValidator::misappliedMetadataMessage(const MetadataPtr& metadata, const SyntaxTreeBasePtr& p)
{
    string message = '\'' + metadata->directive() + "' metadata cannot be ";
    if (dynamic_pointer_cast<Unit>(p))
    {
        message += "specified as file metadata";
    }
    else if (dynamic_pointer_cast<Builtin>(p))
    {
        message += "applied to builtin types";
    }
    else
    {
        const ContainedPtr contained = dynamic_pointer_cast<Contained>(p);
        assert(contained);
        message += "applied to " + pluralKindOf(contained);
    }
    return message;
}
