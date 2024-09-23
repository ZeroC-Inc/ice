//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

import com.zeroc.Ice.IceMX.DispatchMetrics;
import com.zeroc.Ice.IceMX.ObserverWithDelegate;
import com.zeroc.Ice.Instrumentation.DispatchObserver;

class DispatchObserverI extends ObserverWithDelegate<DispatchMetrics, DispatchObserver>
        implements DispatchObserver {
    @Override
    public void userException() {
        forEach(_userException);
        if (_delegate != null) {
            _delegate.userException();
        }
    }

    @Override
    public void reply(final int size) {
        forEach(
                new MetricsUpdate<DispatchMetrics>() {
                    @Override
                    public void update(DispatchMetrics v) {
                        v.replySize += size;
                    }
                });
        if (_delegate != null) {
            _delegate.reply(size);
        }
    }

    private final MetricsUpdate<DispatchMetrics> _userException =
            new MetricsUpdate<DispatchMetrics>() {
                @Override
                public void update(DispatchMetrics v) {
                    ++v.userException;
                }
            };
}
