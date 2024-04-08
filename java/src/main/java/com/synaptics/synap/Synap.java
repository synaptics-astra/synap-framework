package com.synaptics.synap;

public class Synap {

    private static Synap mInstance;

    private Synap() {
        if (!SynapNative.init()) {
            throw new SynapException("Error connecting to SyNAP device");
        }
    }

    @Override
    protected void finalize() throws Throwable {
        SynapNative.deinit();
        super.finalize();
    }

    public static Synap getInstance() {
        if (mInstance == null) {
            mInstance = new Synap();
        }

        return mInstance;
    }

    public Network createNetwork(byte[] code) throws SynapException {

        long handle = SynapNative.createNetwork(code);

        if (handle == -1) {
            throw new SynapException("Error while creating network");
        }

        return new Network(handle);

    }

    public IoBuffer createIoBuffer(long size) throws SynapException {
        long handle = SynapNative.createIoBuffer(size);

        if (handle == 0) {
            throw new SynapException("Error while IoBuffer");
        }

        return new IoBuffer(handle);

    }


}
