package com.synaptics.synap;

public class Network {

    final long mHandle;
    private boolean mReleased;

    Network(long handle) {
        mHandle = handle;
        mReleased = false;
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    public void release() {
        if (!mReleased) {
            SynapNative.releaseNetwork(mHandle);
            mReleased = true;
        }
    }

    public void run() throws SynapException {
        SynapNative.runNetwork(mHandle);
    }

    public Attachment attachIoBuffer(IoBuffer buffer) throws SynapException {

        long attachment = SynapNative.attachBufferToNetwork(mHandle, buffer.mHandle);

        if (attachment == -1) {
            throw new SynapException("Unable to attach buffer");
        }

        return new Attachment(attachment, mHandle);
    }

}
