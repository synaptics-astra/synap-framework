package com.synaptics.synap;

class SynapNative {

    static {
        System.loadLibrary("synap_jni");
    }

    static native boolean init();

    static native long createNetwork(byte[] code);

    static native void releaseNetwork(long network);

    static native long attachBufferToNetwork(long network, long buffer);

    static native boolean runNetwork(long network);

    static native void releaseAttachment(long network, long attachment);

    static native boolean useAttachmentAsInput(long network, long attachment, int index);

    static native boolean useAttachmentAsOutput(long network, long attachment, int index);

    static native long createIoBuffer(long size);

    static native void releaseIoBuffer(long buffer);

    static native boolean copyDataFromIoBuffer(long buffer, byte[] dest, long srcOffset, long dstOffset, long count);

    static native boolean copyDataToIoBuffer(long buffer, byte[] dest, long srcOffset, long dstOffset, long count);

    static native void deinit();

}
