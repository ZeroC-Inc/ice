//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_BATCH_REQUEST_QUEUE_H
#define ICE_BATCH_REQUEST_QUEUE_H

#include <Ice/BatchRequestInterceptor.h>
#include <Ice/BatchRequestQueueF.h>
#include <Ice/InstanceF.h>
#include <Ice/OutputStream.h>

#include <mutex>
#include <condition_variable>

namespace IceInternal
{
    class BatchRequestQueue
    {
    public:
        BatchRequestQueue(const InstancePtr&, bool);

        void prepareBatchRequest(Ice::OutputStream*);
        void finishBatchRequest(Ice::OutputStream*, const Ice::ObjectPrx&, std::string_view);
        void abortBatchRequest(Ice::OutputStream*);

        int swap(Ice::OutputStream*, bool&);

        void destroy(std::exception_ptr);
        bool isEmpty();

        void enqueueBatchRequest(const Ice::ObjectPrx&);

    private:
        std::function<void(const Ice::BatchRequest&, int, int)> _interceptor;
        Ice::OutputStream _batchStream;
        bool _batchStreamInUse;
        bool _batchStreamCanFlush;
        bool _batchCompress;
        int _batchRequestNum;
        size_t _batchMarker;
        std::exception_ptr _exception;
        size_t _maxSize;

        std::mutex _mutex;
        std::condition_variable _conditionVariable;
    };

};

#endif
