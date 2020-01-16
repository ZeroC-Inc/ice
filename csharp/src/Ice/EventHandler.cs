//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace IceInternal
{

    public abstract class EventHandler
    {
        //
        // Called to start a new asynchronous read or write operation.
        //
        public abstract bool StartAsync(int op, AsyncCallback cb, ref bool completedSynchronously);

        public abstract bool FinishAsync(int op);

        //
        // Called when there's a message ready to be processed.
        //
        public abstract void Message(ref ThreadPoolCurrent op);

        //
        // Called when the event handler is unregistered.
        //
        public abstract void Finished(ref ThreadPoolCurrent op);

        internal int _ready = 0;
        internal int _pending = 0;
        internal int _started = 0;
        internal bool _finish = false;

        internal bool _hasMoreData = false;
        internal int _registered = 0;
    }

}
