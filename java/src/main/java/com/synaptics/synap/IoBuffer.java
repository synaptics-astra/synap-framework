package com.synaptics.synap;

public class IoBuffer {

    final long mHandle;
    private boolean mReleased;

    IoBuffer(long handle) {
        mHandle = handle;
        mReleased = false;
    }

    public void copyToBuffer(byte[] buffer, long srcOffset, long dstOffset, long count)
            throws SynapException {
        if (!SynapNative.copyDataFromIoBuffer(mHandle, buffer, srcOffset, dstOffset, count)) {
            throw new SynapException("Error while coping data from IoBuffer");
        }
    }

    public void copyFromBuffer(byte[] buffer, long srcOffset, long dstOffset, long count)
            throws SynapException {
        if (!SynapNative.copyDataToIoBuffer(mHandle, buffer, srcOffset, dstOffset, count)) {
            throw new SynapException("Error while coping data to IoBuffer");
        }
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    public void release() {
        if (!mReleased) {
            SynapNative.releaseIoBuffer(mHandle);
            mReleased = true;
        }
    }

}
