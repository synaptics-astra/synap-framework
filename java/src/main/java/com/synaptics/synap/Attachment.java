package com.synaptics.synap;

public class Attachment {

    final long mHandle;
    final long mNetwork;
    private boolean mReleased;

    Attachment(long handle, long network) {
        mHandle = handle;
        mNetwork = network;
        mReleased = false;
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    public void release() {
        if (!mReleased) {
            SynapNative.releaseAttachment(mNetwork, mHandle);
            mReleased = true;
        }
    }

    public void useAsInput(int index) throws SynapException {
        if (!SynapNative.useAttachmentAsInput(mNetwork, mHandle, index)) {
            throw new SynapException("Unable to use attachment as input");
        }
    }

    public void useAsOutput(int index) throws SynapException {
        if (!SynapNative.useAttachmentAsOutput(mNetwork, mHandle, index)) {
            throw new SynapException("Unable to use attachment as output");
        }
    }

}
