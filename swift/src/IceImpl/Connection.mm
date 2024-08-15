// Copyright (c) ZeroC, Inc.
#import "include/Connection.h"
#import "include/Endpoint.h"
#import "include/IceUtil.h"
#import "include/ObjectAdapter.h"
#import "include/ObjectPrx.h"

#import "Convert.h"

#import "../../cpp/src/Ice/SSL/SSLUtil.h"

@implementation ICEConnection

- (std::shared_ptr<Ice::Connection>)connection
{
    return std::static_pointer_cast<Ice::Connection>(self.cppObject);
}

- (void)abort
{
    self.connection->abort();
}

- (void)close:(void (^)(NSError* _Nullable error))completionHandler
{
    self.connection->close([completionHandler](std::exception_ptr closeException)
    {
        // Keep this code in sync with the future-returning Ice::Connection::close in C++.
        try
        {
            rethrow_exception(closeException);
        }
        catch (const Ice::ConnectionClosedException&)
        {
            completionHandler(nil);
        }
        catch (const Ice::CloseConnectionException&)
        {
            completionHandler(nil);
        }
        catch (const Ice::CommunicatorDestroyedException&)
        {
            completionHandler(nil);
        }
        catch (const Ice::ObjectAdapterDeactivatedException&)
        {
            completionHandler(nil);
        }
        catch (...)
        {
            // TODO: explain why we need an autoreleasepool here.
            @autoreleasepool
            {
                completionHandler(convertException(closeException));
            }
        }
    });
}

- (nullable ICEObjectPrx*)createProxy:(NSString*)name category:(NSString*)category error:(NSError**)error
{
    try
    {
        auto prx = self.connection->createProxy(Ice::Identity{fromNSString(name), fromNSString(category)});
        return [[ICEObjectPrx alloc] initWithCppObjectPrx:prx];
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (BOOL)setAdapter:(ICEObjectAdapter* _Nullable)oa error:(NSError* _Nullable* _Nullable)error;
{
    try
    {
        self.connection->setAdapter(oa == nil ? nullptr : [oa objectAdapter]);
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (nullable ICEObjectAdapter*)getAdapter
{
    auto cppAdapter = self.connection->getAdapter();

    return [ICEObjectAdapter getHandle:cppAdapter];
}

- (ICEEndpoint*)getEndpoint
{
    auto endpoint = self.connection->getEndpoint();
    return [ICEEndpoint getHandle:endpoint];
}

- (void)flushBatchRequests:(std::uint8_t)compress exception:(void (^)(NSError*))exception sent:(void (^)(bool))sent
{
    try
    {
        self.connection->flushBatchRequestsAsync(
            Ice::CompressBatch(compress),
            [exception](std::exception_ptr e)
            {
                @autoreleasepool
                {
                    exception(convertException(e));
                }
            },
            [sent](bool sentSynchronously) { sent(sentSynchronously); });
    }
    catch (...)
    {
        // Typically CommunicatorDestroyedException.
        exception(convertException(std::current_exception()));
    }
}

- (BOOL)setCloseCallback:(void (^)(ICEConnection*))callback error:(NSError**)error
{
    try
    {
        if (!callback)
        {
            self.connection->setCloseCallback(nullptr);
        }
        else
        {
            self.connection->setCloseCallback(
                [callback](auto connection)
                {
                    ICEConnection* conn = [ICEConnection getHandle:connection];
                    assert(conn);
                    @autoreleasepool
                    {
                        callback(conn);
                    }
                });
        }
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (NSString*)type
{
    return toNSString(self.connection->type());
}

- (NSString*)toString
{
    return toNSString(self.connection->toString());
}

- (id)getInfo:(NSError**)error
{
    try
    {
        auto info = self.connection->getInfo();
        return createConnectionInfo(info);
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (BOOL)setBufferSize:(int32_t)rcvSize sndSize:(int32_t)sndSize error:(NSError**)error
{
    try
    {
        self.connection->setBufferSize(rcvSize, sndSize);
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (BOOL)throwException:(NSError**)error
{
    try
    {
        self.connection->throwException();
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}
@end

id
createConnectionInfo(std::shared_ptr<Ice::ConnectionInfo> infoPtr)
{
    id underlying = infoPtr->underlying ? createConnectionInfo(infoPtr->underlying) : [NSNull null];

    Class<ICEConnectionInfoFactory> factory = [ICEUtil connectionInfoFactory];

    auto ipInfo = std::dynamic_pointer_cast<Ice::IPConnectionInfo>(infoPtr);
    if (ipInfo)
    {
        [factory createIPConnectionInfo:underlying
                               incoming:infoPtr->incoming
                            adapterName:toNSString(infoPtr->adapterName)
                           connectionId:toNSString(ipInfo->connectionId)
                           localAddress:toNSString(ipInfo->localAddress)
                              localPort:ipInfo->localPort
                          remoteAddress:toNSString(ipInfo->remoteAddress)
                             remotePort:ipInfo->remotePort];
    }

    auto tcpInfo = std::dynamic_pointer_cast<Ice::TCPConnectionInfo>(infoPtr);
    if (tcpInfo)
    {
        return [factory createTCPConnectionInfo:underlying
                                       incoming:tcpInfo->incoming
                                    adapterName:toNSString(tcpInfo->adapterName)
                                   connectionId:toNSString(tcpInfo->connectionId)
                                   localAddress:toNSString(tcpInfo->localAddress)
                                      localPort:tcpInfo->localPort
                                  remoteAddress:toNSString(tcpInfo->remoteAddress)
                                     remotePort:tcpInfo->remotePort
                                        rcvSize:tcpInfo->rcvSize
                                        sndSize:tcpInfo->sndSize];
    }

    auto udpInfo = std::dynamic_pointer_cast<Ice::UDPConnectionInfo>(infoPtr);
    if (udpInfo)
    {
        return [factory createUDPConnectionInfo:underlying
                                       incoming:infoPtr->incoming
                                    adapterName:toNSString(infoPtr->adapterName)
                                   connectionId:toNSString(ipInfo->connectionId)
                                   localAddress:toNSString(ipInfo->localAddress)
                                      localPort:ipInfo->localPort
                                  remoteAddress:toNSString(ipInfo->remoteAddress)
                                     remotePort:ipInfo->remotePort
                                   mcastAddress:toNSString(udpInfo->mcastAddress)
                                      mcastPort:udpInfo->mcastPort
                                        rcvSize:udpInfo->rcvSize
                                        sndSize:udpInfo->sndSize];
    }

    auto wsInfo = std::dynamic_pointer_cast<Ice::WSConnectionInfo>(infoPtr);
    if (wsInfo)
    {
        return [factory createWSConnectionInfo:underlying
                                      incoming:wsInfo->incoming
                                   adapterName:toNSString(wsInfo->adapterName)
                                  connectionId:toNSString(wsInfo->adapterName)
                                       headers:toNSDictionary(wsInfo->headers)];
    }

    auto sslInfo = std::dynamic_pointer_cast<Ice::SSL::ConnectionInfo>(infoPtr);
    if (sslInfo)
    {
        std::string encoded;
        if (sslInfo->peerCertificate)
        {
            encoded = Ice::SSL::encodeCertificate(sslInfo->peerCertificate);
        }
        return [factory createSSLConnectionInfo:underlying
                                       incoming:sslInfo->incoming
                                    adapterName:toNSString(sslInfo->adapterName)
                                   connectionId:toNSString(sslInfo->connectionId)
                                peerCertificate:toNSString(encoded)];
    }

#if TARGET_OS_IPHONE

    auto iapInfo = std::dynamic_pointer_cast<IceIAP::ConnectionInfo>(infoPtr);
    if (iapInfo)
    {
        return [factory createIAPConnectionInfo:underlying
                                       incoming:iapInfo->incoming
                                    adapterName:toNSString(iapInfo->adapterName)
                                   connectionId:toNSString(iapInfo->connectionId)
                                           name:toNSString(iapInfo->name)
                                   manufacturer:toNSString(iapInfo->manufacturer)
                                    modelNumber:toNSString(iapInfo->modelNumber)
                               firmwareRevision:toNSString(iapInfo->firmwareRevision)
                               hardwareRevision:toNSString(iapInfo->hardwareRevision)
                                       protocol:toNSString(iapInfo->protocol)];
    }

#endif

    return [NSNull null];
}
