// Copyright (c) ZeroC, Inc.

#ifndef C_PLUS_PLUS_UTIL_H
#define C_PLUS_PLUS_UTIL_H

#include "../Ice/OutputUtil.h"
#include "../Slice/Parser.h"
#include "TypeContext.h"

namespace Slice
{
    extern std::string paramPrefix;

    struct ToIfdef
    {
        char operator()(char);
    };

    void printHeader(IceInternal::Output&);
    void printVersionCheck(IceInternal::Output&);
    void printDllExportStuff(IceInternal::Output&, const std::string&);

    bool isMovable(const TypePtr&);

    std::string getUnqualified(const std::string&, const std::string&);

    /// Gets the C++ type for a Slice parameter or field.
    std::string typeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const MetadataList& = MetadataList(),
        TypeContext = TypeContext::None);

    // TODO: find a better name.
    /// Gets the C++ type for a Slice parameter to be marshaled.
    std::string inputTypeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const MetadataList& = MetadataList(),
        TypeContext = TypeContext::None);

    // TODO: find a better name.
    /// Gets the C++ type for a Slice out parameter when mapped to a C++ out parameter.
    std::string outputTypeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const MetadataList& = MetadataList(),
        TypeContext = TypeContext::None);

    std::string operationModeToString(Operation::Mode);
    std::string opFormatTypeToString(const OperationPtr&);

    void writeMarshalCode(IceInternal::Output&, const ParameterList&, const OperationPtr&);
    void writeUnmarshalCode(IceInternal::Output&, const ParameterList&, const OperationPtr&);
    void
    writeAllocateCode(IceInternal::Output&, const ParameterList&, const OperationPtr&, const std::string&, TypeContext);

    /// Writes the StreamReader specialization for a struct.
    void writeStreamReader(IceInternal::Output&, const StructPtr&, const DataMemberList&);

    /// Reads or writes the data members of a class or exception slice.
    void readDataMembers(IceInternal::Output&, const DataMemberList&);
    void writeDataMembers(IceInternal::Output&, const DataMemberList&);

    void writeIceTuple(IceInternal::Output&, const DataMemberList&, TypeContext);

    std::string findMetadata(const MetadataList&, TypeContext = TypeContext::None);
    bool inWstringModule(const SequencePtr&);
}

#endif
