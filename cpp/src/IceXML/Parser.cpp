//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Parser.h"
#include "../Ice/FileUtil.h"

#include <expat.h>

#include <cassert>
#include <fstream>
#include <list>
#include <sstream>

using namespace std;
using namespace IceXML;

//
// ParserException
//

const char*
IceXML::ParserException::ice_id() const noexcept
{
    return "::IceXML::ParserException";
}

//
// Handler
//
IceXML::Handler::~Handler() {}

//
// expat callbacks
//
struct CallbackData
{
    XML_Parser parser;
    Handler* handler;
};

extern "C"
{
    static void startElementHandler(void* data, const XML_Char* name, const XML_Char** attr)
    {
        CallbackData* cb = static_cast<CallbackData*>(data);

        Attributes attributes;
        for (int i = 0; attr[i]; i += 2)
        {
            attributes[attr[i]] = attr[i + 1];
        }

        int line = static_cast<int>(XML_GetCurrentLineNumber(cb->parser));
        int column = static_cast<int>(XML_GetCurrentColumnNumber(cb->parser));
        cb->handler->startElement(name, attributes, line, column);
    }

    static void endElementHandler(void* data, const XML_Char* name)
    {
        CallbackData* cb = static_cast<CallbackData*>(data);
        int line = static_cast<int>(XML_GetCurrentLineNumber(cb->parser));
        int column = static_cast<int>(XML_GetCurrentColumnNumber(cb->parser));
        cb->handler->endElement(name, line, column);
    }

    static void characterDataHandler(void* data, const XML_Char* s, int len)
    {
        CallbackData* cb = static_cast<CallbackData*>(data);

        string str(s, static_cast<size_t>(len));
        int line = static_cast<int>(XML_GetCurrentLineNumber(cb->parser));
        int column = static_cast<int>(XML_GetCurrentColumnNumber(cb->parser));
        cb->handler->characters(str, line, column);
    }
}

//
// Parser
//
void
IceXML::Parser::parse(const string& file, Handler& handler) // The given filename must be UTF-8 encoded
{
    ifstream in(IceInternal::streamFilename(file).c_str());
    if (!in.good())
    {
        ostringstream out;
        out << "unable to open file `" << file << "'";
        throw ParserException(__FILE__, __LINE__, out.str());
    }
    parse(in, handler);
}

void
IceXML::Parser::parse(istream& in, Handler& handler)
{
    XML_Parser parser = XML_ParserCreate(nullptr);
    CallbackData cb;
    cb.parser = parser;
    cb.handler = &handler;
    XML_SetUserData(parser, &cb);
    XML_SetElementHandler(parser, startElementHandler, endElementHandler);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    try
    {
        char buff[1024];
        int isFinal = 0;
        while (!isFinal)
        {
            in.read(buff, 1024);
            if (in.gcount() < 1024)
            {
                isFinal = 1;
            }
            if (XML_Parse(parser, buff, static_cast<int>(in.gcount()), isFinal) != 1)
            {
                handler.error(
                    XML_ErrorString(XML_GetErrorCode(parser)),
                    static_cast<int>(XML_GetCurrentLineNumber(parser)),
                    static_cast<int>(XML_GetCurrentColumnNumber(parser)));
                return;
            }
        }
    }
    catch (...)
    {
        XML_ParserFree(parser);
        throw;
    }

    XML_ParserFree(parser);
}
