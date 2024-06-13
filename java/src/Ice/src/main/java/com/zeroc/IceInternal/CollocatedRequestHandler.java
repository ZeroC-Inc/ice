//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceInternal;

import com.zeroc.Ice.IncomingRequest;
import com.zeroc.Ice.InputStream;
import com.zeroc.Ice.LocalException;
import com.zeroc.Ice.Object;
import com.zeroc.Ice.OutgoingResponse;
import com.zeroc.Ice.OutputStream;
import com.zeroc.Ice.UnknownException;
import java.util.concurrent.CompletionStage;

public class CollocatedRequestHandler implements RequestHandler {
  private class InvokeAllAsync extends DispatchWorkItem {
    private InvokeAllAsync(
        OutgoingAsyncBase outAsync,
        com.zeroc.Ice.OutputStream os,
        int requestId,
        int batchRequestNum) {
      _outAsync = outAsync;
      _os = os;
      _requestId = requestId;
      _batchRequestNum = batchRequestNum;
    }

    @Override
    public void run() {
      if (sentAsync(_outAsync)) {
        dispatchAll(_os, _requestId, _batchRequestNum);
      }
    }

    private final OutgoingAsyncBase _outAsync;
    private com.zeroc.Ice.OutputStream _os;
    private final int _requestId;
    private final int _batchRequestNum;
  }

  public CollocatedRequestHandler(Reference ref, com.zeroc.Ice.ObjectAdapter adapter) {
    _reference = ref;
    _dispatcher = ref.getInstance().initializationData().dispatcher != null;
    _adapter = (com.zeroc.Ice.ObjectAdapterI) adapter;
    _response = _reference.getMode() == Reference.ModeTwoway;

    _logger =
        _reference.getInstance().initializationData().logger; // Cached for better performance.
    _traceLevels = _reference.getInstance().traceLevels(); // Cached for better performance.
    _requestId = 0;
  }

  @Override
  public RequestHandler update(RequestHandler previousHandler, RequestHandler newHandler) {
    return previousHandler == this ? newHandler : this;
  }

  @Override
  public int sendAsyncRequest(ProxyOutgoingAsyncBase outAsync) {
    return outAsync.invokeCollocated(this);
  }

  @Override
  public synchronized void asyncRequestCanceled(
      OutgoingAsyncBase outAsync, com.zeroc.Ice.LocalException ex) {
    Integer requestId = _sendAsyncRequests.remove(outAsync);
    if (requestId != null) {
      if (requestId > 0) {
        _asyncRequests.remove(requestId);
      }
      if (outAsync.completed(ex)) {
        outAsync.invokeCompletedAsync();
      }
      _adapter.decDirectCount(); // dispatchAll won't be called, decrease the direct count.
      return;
    }

    if (outAsync instanceof OutgoingAsync) {
      for (java.util.Map.Entry<Integer, OutgoingAsyncBase> e : _asyncRequests.entrySet()) {
        if (e.getValue() == outAsync) {
          _asyncRequests.remove(e.getKey());
          if (outAsync.completed(ex)) {
            outAsync.invokeCompletedAsync();
          }
          return;
        }
      }
    }
  }

  @Override
  public Reference getReference() {
    return _reference;
  }

  @Override
  public com.zeroc.Ice.ConnectionI getConnection() {
    return null;
  }

  int invokeAsyncRequest(OutgoingAsyncBase outAsync, int batchRequestNum, boolean sync) {
    //
    // Increase the direct count to prevent the thread pool from being destroyed before
    // dispatchAll is called. This will also throw if the object adapter has been deactivated.
    //
    _adapter.incDirectCount();

    int requestId = 0;
    try {
      synchronized (this) {
        outAsync.cancelable(this); // This will throw if the request is canceled

        if (_response) {
          requestId = ++_requestId;
          _asyncRequests.put(requestId, outAsync);
        }

        _sendAsyncRequests.put(outAsync, requestId);
      }
    } catch (Exception ex) {
      _adapter.decDirectCount();
      throw ex;
    }

    outAsync.attachCollocatedObserver(_adapter, requestId);

    if (!sync
        || !_response
        || _reference.getInstance().queueRequests()
        || _reference.getInvocationTimeout() > 0) {
      _adapter
          .getThreadPool()
          .dispatch(new InvokeAllAsync(outAsync, outAsync.getOs(), requestId, batchRequestNum));
    } else if (_dispatcher) {
      _adapter
          .getThreadPool()
          .dispatchFromThisThread(
              new InvokeAllAsync(outAsync, outAsync.getOs(), requestId, batchRequestNum));
    } else // Optimization: directly call dispatchAll if there's no dispatcher.
    {
      if (sentAsync(outAsync)) {
        dispatchAll(outAsync.getOs(), requestId, batchRequestNum);
      }
    }
    return AsyncStatus.Queued;
  }

  private boolean sentAsync(final OutgoingAsyncBase outAsync) {
    synchronized (this) {
      if (_sendAsyncRequests.remove(outAsync) == null) {
        return false; // The request timed-out.
      }

      //
      // This must be called within the synchronization to
      // ensure completed(ex) can't be called concurrently if
      // the request is canceled.
      //
      if (!outAsync.sent()) {
        return true;
      }
    }

    outAsync.invokeSent();
    return true;
  }

  private void dispatchAll(com.zeroc.Ice.OutputStream os, int requestId, int requestCount) {
    if (_traceLevels.protocol >= 1) {
      fillInValue(os, 10, os.size());
      if (requestId > 0) {
        fillInValue(os, Protocol.headerSize, requestId);
      } else if (requestCount > 0) {
        fillInValue(os, Protocol.headerSize, requestCount);
      }
      TraceUtil.traceSend(os, _logger, _traceLevels);
    }

    com.zeroc.Ice.InputStream is =
        new com.zeroc.Ice.InputStream(os.instance(), os.getEncoding(), os.getBuffer(), false);

    if (requestCount > 0) {
      is.pos(Protocol.requestBatchHdr.length);
    } else {
      is.pos(Protocol.requestHdr.length);
    }

    int dispatchCount = requestCount > 0 ? requestCount : 1;
    assert !_response || dispatchCount == 1;

    Object dispatcher = _adapter.dispatchPipeline();
    assert dispatcher != null;

    try {
      while (dispatchCount > 0) {
        //
        // Increase the direct count for the dispatch. We increase it again here for
        // each dispatch. It's important for the direct count to be > 0 until the last
        // collocated request response is sent to make sure the thread pool isn't
        // destroyed before.
        //
        try {
          _adapter.incDirectCount();
        } catch (com.zeroc.Ice.ObjectAdapterDeactivatedException ex) {
          handleException(ex, requestId, false);
          break;
        }

        var request = new IncomingRequest(requestId, null, _adapter, is);
        CompletionStage<OutgoingResponse> response = null;
        try {
          response = dispatcher.dispatch(request);
        } catch (Throwable ex) { // UserException or an unchecked exception
          sendResponse(request.current.createOutgoingResponse(ex), requestId, false);
        }

        if (response != null) {
          response.whenComplete(
              (result, exception) -> {
                if (exception != null) {
                  sendResponse(request.current.createOutgoingResponse(exception), requestId, true);
                } else {
                  sendResponse(result, requestId, true);
                }
                // Any exception thrown by this closure is effectively ignored.
              });
        }

        --dispatchCount;
      }
      is.clear();
    } catch (com.zeroc.Ice.LocalException ex) {
      dispatchException(ex, requestId, false); // Fatal dispatch exception
    } catch (RuntimeException | java.lang.Error ex) {
      // A runtime exception or an error was thrown outside of servant code (i.e., by Ice code).
      // Note
      // that this does NOT
      // send a response to the client. = new com.zeroc.Ice.UnknownException(ex);
      var uex = new UnknownException(ex);
      var sw = new java.io.StringWriter();
      var pw = new java.io.PrintWriter(sw);
      ex.printStackTrace(pw);
      pw.flush();
      uex.unknown = sw.toString();
      _logger.error(uex.unknown);
      dispatchException(uex, requestId, false);
    } finally {
      _adapter.decDirectCount();
    }
  }

  private void sendResponse(OutgoingResponse response, int requestId, boolean amd) {
    if (_response) {
      OutgoingAsyncBase outAsync = null;
      OutputStream outputStream = response.outputStream;

      synchronized (this) {
        if (_traceLevels.protocol >= 1) {
          fillInValue(outputStream, 10, outputStream.size());
        }

        // Adopt the OutputStream's buffer.
        var inputStream =
            new InputStream(
                outputStream.instance(),
                outputStream.getEncoding(),
                outputStream.getBuffer(),
                true); // adopt: true

        inputStream.pos(Protocol.replyHdr.length + 4);

        if (_traceLevels.protocol >= 1) {
          TraceUtil.traceRecv(inputStream, _logger, _traceLevels);
        }

        outAsync = _asyncRequests.remove(requestId);
        if (outAsync != null && !outAsync.completed(inputStream)) {
          outAsync = null;
        }
      }

      if (outAsync != null) {
        //
        // If called from an AMD dispatch, invoke asynchronously
        // the completion callback since this might be called from
        // the user code.
        //
        if (amd) {
          outAsync.invokeCompletedAsync();
        } else {
          outAsync.invokeCompleted();
        }
      }
    }
    _adapter.decDirectCount();
  }

  private void dispatchException(LocalException ex, int requestId, boolean amd) {
    handleException(ex, requestId, amd);
    _adapter.decDirectCount();
  }

  private void handleException(com.zeroc.Ice.Exception ex, int requestId, boolean amd) {
    if (requestId == 0) {
      return; // Ignore exception for oneway messages.
    }

    OutgoingAsyncBase outAsync = null;
    synchronized (this) {
      outAsync = _asyncRequests.remove(requestId);
      if (outAsync != null && !outAsync.completed(ex)) {
        outAsync = null;
      }
    }

    if (outAsync != null) {
      //
      // If called from an AMD dispatch, invoke asynchronously
      // the completion callback since this might be called from
      // the user code.
      //
      if (amd) {
        outAsync.invokeCompletedAsync();
      } else {
        outAsync.invokeCompleted();
      }
    }
  }

  private void fillInValue(com.zeroc.Ice.OutputStream os, int pos, int value) {
    os.rewriteInt(value, pos);
  }

  private final Reference _reference;
  private final boolean _dispatcher;
  private final boolean _response;
  private final com.zeroc.Ice.ObjectAdapterI _adapter;
  private final com.zeroc.Ice.Logger _logger;
  private final TraceLevels _traceLevels;

  private int _requestId;

  // A map of outstanding requests that can be canceled. A request
  // can be canceled if it has an invocation timeout, or we support
  // interrupts.
  private java.util.Map<OutgoingAsyncBase, Integer> _sendAsyncRequests = new java.util.HashMap<>();

  private java.util.Map<Integer, OutgoingAsyncBase> _asyncRequests = new java.util.HashMap<>();
}
