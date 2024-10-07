//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

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

    void printHeader(::IceInternal::Output&);
    void printVersionCheck(::IceInternal::Output&);
    void printDllExportStuff(::IceInternal::Output&, const std::string&);

    bool isMovable(const TypePtr&);

    std::string getUnqualified(const std::string&, const std::string&);

    // Gets the C++ type for a Slice parameter or field.
    std::string typeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const StringList& = StringList(),
        TypeContext = TypeContext::None);

    // TODO: find a better name.
    // Gets the C++ type for a Slice parameter to be marshaled.
    std::string inputTypeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const StringList& = StringList(),
        TypeContext = TypeContext::None);

    // TODO: find a better name.
    // Gets the C++ type for a Slice out parameter when mapped to a C++ out parameter.
    std::string outputTypeToString(
        const TypePtr&,
        bool,
        const std::string& = "",
        const StringList& = StringList(),
        TypeContext = TypeContext::None);

    std::string operationModeToString(Operation::Mode);
    std::string opFormatTypeToString(const OperationPtr&);

    std::string fixKwd(const std::string&);

    void writeMarshalCode(::IceInternal::Output&, const ParamDeclList&, const OperationPtr&);
    void writeUnmarshalCode(::IceInternal::Output&, const ParamDeclList&, const OperationPtr&);
    void writeAllocateCode(
        ::IceInternal::Output&,
        const ParamDeclList&,
        const OperationPtr&,
        const std::string&,
        TypeContext);

    // TODO: remove from header file.
    void writeMarshalUnmarshalAllInHolder(IceInternal::Output&, const std::string&, const DataMemberList&, bool, bool);

    // Writes the StreamReader specialization for a struct.
    void writeStreamReader(IceInternal::Output&, const StructPtr&, const DataMemberList&);

    // Reads or writes the data members of a class or exceptions slice.
    void readDataMembers(IceInternal::Output&, const DataMemberList&);
    void writeDataMembers(IceInternal::Output&, const DataMemberList&);

    void writeIceTuple(::IceInternal::Output&, const DataMemberList&, TypeContext);

    std::string findMetadata(const StringList&, TypeContext = TypeContext::None);
    bool inWstringModule(const SequencePtr&);
}

#endif
