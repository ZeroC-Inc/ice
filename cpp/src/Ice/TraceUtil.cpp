//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/StringUtil.h>
#include <Ice/TraceUtil.h>
#include <Ice/Instance.h>
#include <Ice/Object.h>
#include <Ice/Proxy.h>
#include <Ice/TraceLevels.h>
#include <Ice/Logger.h>
#include <Ice/OutputStream.h>
#include <Ice/InputStream.h>
#include <Ice/Protocol.h>
#include <Ice/ReplyStatus.h>

#include <mutex>
#include <set>

using namespace std;
using namespace Ice;
using namespace IceInternal;

static void
printIdentityFacetOperation(ostream& s, InputStream& stream)
{
    ToStringMode toStringMode = ToStringMode::Unicode;
    if (stream.instance())
    {
        toStringMode = stream.instance()->toStringMode();
    }

    Identity identity;
    stream.read(identity);
    s << "\nidentity = " << Ice::identityToString(identity, toStringMode);

    vector<string> facet;
    stream.read(facet);
    s << "\nfacet = ";
    if (!facet.empty())
    {
        s << escapeString(facet[0], "", toStringMode);
    }

    string operation;
    stream.read(operation, false);
    s << "\noperation = " << operation;
}

static string
getMessageTypeAsString(uint8_t type)
{
    switch (type)
    {
        case requestMsg:
            return "request";
        case requestBatchMsg:
            return "batch request";
        case replyMsg:
            return "reply";
        case closeConnectionMsg:
            return "close connection";
        case validateConnectionMsg:
            return "validate connection";
        default:
            return "unknown";
    }
}

static void
printRequestHeader(ostream& s, InputStream& stream)
{
    printIdentityFacetOperation(s, stream);

    uint8_t mode;
    stream.read(mode);
    s << "\nmode = " << static_cast<int>(mode) << ' ';
    switch (static_cast<OperationMode>(mode))
    {
        case OperationMode::Normal:
        {
            s << "(normal)";
            break;
        }

        case OperationMode::Nonmutating:
        {
            s << "(nonmutating)";
            break;
        }

        case OperationMode::Idempotent:
        {
            s << "(idempotent)";
            break;
        }

        default:
        {
            s << "(unknown)";
            break;
        }
    }

    int32_t sz = stream.readSize();
    s << "\ncontext = ";
    while (sz--)
    {
        pair<string, string> pair;
        stream.read(pair.first);
        stream.read(pair.second);
        s << pair.first << '/' << pair.second;
        if (sz)
        {
            s << ", ";
        }
    }

    Ice::EncodingVersion v = stream.skipEncapsulation();
    if (v > Ice::Encoding_1_0)
    {
        s << "\nencoding = " << v;
    }
}

static uint8_t
printHeader(ostream& s, InputStream& stream)
{
    uint8_t magicNumber;
    stream.read(magicNumber); // Don't bother printing the magic number
    stream.read(magicNumber);
    stream.read(magicNumber);
    stream.read(magicNumber);

    uint8_t pMajor;
    uint8_t pMinor;
    stream.read(pMajor);
    stream.read(pMinor);
    //    s << "\nprotocol version = " << static_cast<unsigned>(pMajor)
    //      << "." << static_cast<unsigned>(pMinor);

    uint8_t eMajor;
    uint8_t eMinor;
    stream.read(eMajor);
    stream.read(eMinor);
    //    s << "\nencoding version = " << static_cast<unsigned>(eMajor)
    //      << "." << static_cast<unsigned>(eMinor);

    uint8_t type;
    stream.read(type);
    s << "\nmessage type = " << static_cast<int>(type) << " (" << getMessageTypeAsString(type) << ')';

    uint8_t compress;
    stream.read(compress);
    s << "\ncompression status = " << static_cast<int>(compress) << ' ';

    switch (compress)
    {
        case 0:
        {
            s << "(not compressed; do not compress response, if any)";
            break;
        }

        case 1:
        {
            s << "(not compressed; compress response, if any)";
            break;
        }

        case 2:
        {
            s << "(compressed; compress response, if any)";
            break;
        }

        default:
        {
            s << "(unknown)";
            break;
        }
    }

    int32_t size;
    stream.read(size);
    s << "\nmessage size = " << size;

    return type;
}

static void
printRequest(ostream& s, InputStream& stream)
{
    int32_t requestId;
    stream.read(requestId);
    s << "\nrequest id = " << requestId;
    if (requestId == 0)
    {
        s << " (oneway)";
    }

    printRequestHeader(s, stream);
}

static void
printBatchRequest(ostream& s, InputStream& stream)
{
    int batchRequestNum;
    stream.read(batchRequestNum);
    s << "\nnumber of requests = " << batchRequestNum;

    for (int i = 0; i < batchRequestNum; ++i)
    {
        s << "\nrequest #" << i << ':';
        printRequestHeader(s, stream);
    }
}

static void
printReply(ostream& s, InputStream& stream)
{
    int32_t requestId;
    stream.read(requestId);
    s << "\nrequest id = " << requestId;

    uint8_t replyStatus;
    stream.read(replyStatus);
    s << "\nreply status = " << static_cast<int>(replyStatus) << ' ';
    switch (replyStatus)
    {
        case replyOK:
        {
            s << "(ok)";
            break;
        }

        case replyUserException:
        {
            s << "(user exception)";
            break;
        }

        case replyObjectNotExist:
        case replyFacetNotExist:
        case replyOperationNotExist:
        {
            switch (replyStatus)
            {
                case replyObjectNotExist:
                {
                    s << "(object not exist)";
                    break;
                }

                case replyFacetNotExist:
                {
                    s << "(facet not exist)";
                    break;
                }

                case replyOperationNotExist:
                {
                    s << "(operation not exist)";
                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }

            printIdentityFacetOperation(s, stream);
            break;
        }

        case replyUnknownException:
        case replyUnknownLocalException:
        case replyUnknownUserException:
        {
            switch (replyStatus)
            {
                case replyUnknownException:
                {
                    s << "(unknown exception)";
                    break;
                }

                case replyUnknownLocalException:
                {
                    s << "(unknown local exception)";
                    break;
                }

                case replyUnknownUserException:
                {
                    s << "(unknown user exception)";
                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }

            string unknown;
            stream.read(unknown, false);
            s << "\nunknown = " << unknown;
            break;
        }

        default:
        {
            s << "(unknown)";
            break;
        }
    }

    if (replyStatus == replyOK || replyStatus == replyUserException)
    {
        Ice::EncodingVersion v = stream.skipEncapsulation();
        if (v > Ice::Encoding_1_0)
        {
            s << "\nencoding = " << v;
        }
    }
}

static uint8_t
printMessage(ostream& s, InputStream& stream)
{
    uint8_t type = printHeader(s, stream);

    switch (type)
    {
        case closeConnectionMsg:
        case validateConnectionMsg:
        {
            // We're done.
            break;
        }

        case requestMsg:
        {
            printRequest(s, stream);
            break;
        }

        case requestBatchMsg:
        {
            printBatchRequest(s, stream);
            break;
        }

        case replyMsg:
        {
            printReply(s, stream);
            break;
        }

        default:
        {
            break;
        }
    }

    return type;
}

namespace
{

    mutex slicingMutex;

}

void
IceInternal::traceSlicing(const char* kind, string_view typeId, const char* slicingCat, const LoggerPtr& logger)
{
    lock_guard lock(slicingMutex);
    static set<string, std::less<>> slicingIds;
    if (slicingIds.find(typeId) == slicingIds.end())
    {
        string newTypeId(typeId);
        slicingIds.insert(newTypeId);
        string s("unknown ");
        s += kind;
        s += " type `" + newTypeId + "'";
        logger->trace(slicingCat, s);
    }
}

void
IceInternal::traceSend(const OutputStream& str, const LoggerPtr& logger, const TraceLevelsPtr& tl)
{
    if (tl->protocol >= 1)
    {
        OutputStream& stream = const_cast<OutputStream&>(str);
        InputStream is(stream.instance(), stream.getEncoding(), stream);
        is.i = is.b.begin();

        ostringstream s;
        uint8_t type = printMessage(s, is);

        logger->trace(tl->protocolCat, "sending " + getMessageTypeAsString(type) + " " + s.str());
    }
}

void
IceInternal::traceRecv(const InputStream& str, const LoggerPtr& logger, const TraceLevelsPtr& tl)
{
    if (tl->protocol >= 1)
    {
        InputStream& stream = const_cast<InputStream&>(str);
        InputStream::Container::iterator p = stream.i;
        stream.i = stream.b.begin();

        ostringstream s;
        uint8_t type = printMessage(s, stream);

        logger->trace(tl->protocolCat, "received " + getMessageTypeAsString(type) + " " + s.str());
        stream.i = p;
    }
}

void
IceInternal::trace(const char* heading, const OutputStream& str, const LoggerPtr& logger, const TraceLevelsPtr& tl)
{
    if (tl->protocol >= 1)
    {
        OutputStream& stream = const_cast<OutputStream&>(str);
        InputStream is(stream.instance(), stream.getEncoding(), stream);
        is.i = is.b.begin();

        ostringstream s;
        s << heading;
        printMessage(s, is);

        logger->trace(tl->protocolCat, s.str());
    }
}

void
IceInternal::trace(const char* heading, const InputStream& str, const LoggerPtr& logger, const TraceLevelsPtr& tl)
{
    if (tl->protocol >= 1)
    {
        InputStream& stream = const_cast<InputStream&>(str);
        InputStream::Container::iterator p = stream.i;
        stream.i = stream.b.begin();

        ostringstream s;
        s << heading;
        printMessage(s, stream);

        logger->trace(tl->protocolCat, s.str());
        stream.i = p;
    }
}
