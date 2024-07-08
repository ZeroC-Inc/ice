// Copyright (c) ZeroC, Inc.

package com.zeroc.IceInternal;

import com.zeroc.Ice.ConnectionI;
import com.zeroc.Ice.LocalException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

// Decorates Transceiver to send heartbeats and optionally detect when no byte is received/read for
// a while.
// This decorator must not be applied on UDP connections.
public class IdleTimeoutTransceiverDecorator implements Transceiver {
    private final Transceiver _decoratee;
    private final int _idleTimeout;
    private final ScheduledExecutorService _scheduledExecutorService;

    private final Runnable _idleCheck;
    private final Runnable _sendHeartbeat;

    // All calls to a transceiver are serialized by the parent ConnectionI's lock.
    private ScheduledFuture<?> _readTimerFuture;
    private ScheduledFuture<?> _writeTimerFuture;

    @Override
    public java.nio.channels.SelectableChannel fd() {
        return _decoratee.fd();
    }

    @Override
    public void setReadyCallback(ReadyCallback callback) {
        _decoratee.setReadyCallback(callback);
    }

    @Override
    public int initialize(Buffer readBuffer, Buffer writeBuffer) {
        int op = _decoratee.initialize(readBuffer, writeBuffer);

        if (op == SocketOperation.None) { // connected
            rescheduleReadTimer();
            rescheduleWriteTimer();
        }

        return op;
    }

    @Override
    public int closing(boolean initiator, LocalException ex) {
        return _decoratee.closing(initiator, ex);
    }

    @Override
    public void close() {
        cancelReadTimer();
        cancelWriteTimer();
        _decoratee.close();
    }

    @Override
    public EndpointI bind() {
        return _decoratee.bind();
    }

    @Override
    public int write(Buffer buf) {
        cancelWriteTimer();
        int op = _decoratee.write(buf);
        if (op == SocketOperation.None) { // write completed
            rescheduleWriteTimer();
        }
        return op;
    }

    @Override
    public int read(Buffer buf) {
        // We don't want the idle check to run while we're reading, so we reschedule it before
        // reading.
        rescheduleReadTimer();
        return _decoratee.read(buf);
    }

    @Override
    public boolean isWaitingToBeRead() {
        return _decoratee.isWaitingToBeRead();
    }

    @Override
    public String protocol() {
        return _decoratee.protocol();
    }

    @Override
    public String toString() {
        return _decoratee.toString();
    }

    @Override
    public String toDetailedString() {
        return _decoratee.toDetailedString();
    }

    @Override
    public com.zeroc.Ice.ConnectionInfo getInfo() {
        return _decoratee.getInfo();
    }

    @Override
    public void checkSendSize(Buffer buf) {
        _decoratee.checkSendSize(buf);
    }

    @Override
    public void setBufferSize(int rcvSize, int sndSize) {
        _decoratee.setBufferSize(rcvSize, sndSize);
    }

    public IdleTimeoutTransceiverDecorator(
            Transceiver decoratee,
            ConnectionI connection,
            int idleTimeout,
            boolean enableIdleCheck,
            ScheduledExecutorService scheduledExecutorService) {
        _decoratee = decoratee;
        _idleTimeout = idleTimeout;
        _scheduledExecutorService = scheduledExecutorService;

        _idleCheck =
                enableIdleCheck
                        ? () ->
                                connection.idleCheck(
                                        idleTimeout,
                                        this::isReadTimerTaskStarted,
                                        this::rescheduleReadTimer)
                        : null;
        _sendHeartbeat = () -> connection.sendHeartbeat();
    }

    private void cancelReadTimer() {
        if (_readTimerFuture != null) {
            _readTimerFuture.cancel(false);
            _readTimerFuture = null;
        }
    }

    private boolean isReadTimerTaskStarted() {
        return _readTimerFuture != null && _readTimerFuture.getDelay(TimeUnit.NANOSECONDS) <= 0;
    }

    private void cancelWriteTimer() {
        if (_writeTimerFuture != null) {
            _writeTimerFuture.cancel(false);
            _writeTimerFuture = null;
        }
    }

    private void rescheduleReadTimer() {
        if (_idleCheck != null) {
            cancelReadTimer();
            _readTimerFuture =
                    _scheduledExecutorService.schedule(_idleCheck, _idleTimeout, TimeUnit.SECONDS);
        }
    }

    private void rescheduleWriteTimer() {
        cancelWriteTimer();
        _writeTimerFuture =
                _scheduledExecutorService.schedule(
                        _sendHeartbeat, _idleTimeout * 1000 / 2, TimeUnit.MILLISECONDS);
    }
}
