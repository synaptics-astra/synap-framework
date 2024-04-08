package com.synaptics.synap;

public class InferenceEngine {

    /**
     * Perform inference using the model passed in  data
     *
     * @param model EBG model
     * @param inputs arrays containing model input data, one byte array per network input,
     *               of the size expected by the network
     * @param outputs arrays where to store output of the network, one byte array per network
     *                output, of the size expected by the network
     */
    public static void infer(byte[] model, byte[][] inputs, byte[][] outputs) {

        Synap synap = Synap.getInstance();

        // load the network
        Network network = synap.createNetwork(model);

        // create input buffers and attach them to the network
        IoBuffer[] inputBuffers = new IoBuffer[inputs.length];
        Attachment[] inputAttachments = new Attachment[inputs.length];

        for (int i = 0; i < inputs.length; i++) {
            // create the input buffer of the desired length
            inputBuffers[i] = synap.createIoBuffer(inputs[i].length);

            // attach the buffer to the network (make sure you keep a reference to the
            // attachment to avoid it is garbage collected and destroyed)
            inputAttachments[i] = network.attachIoBuffer(inputBuffers[i]);

            // set the buffer as the i-th input of the network
            inputAttachments[i].useAsInput(i);

            // copy the input data to the buffer
            inputBuffers[i].copyFromBuffer(inputs[i], 0, 0, inputs[i].length);
        }

        // create the output buffers and attach them to the network
        IoBuffer[] outputBuffers = new IoBuffer[outputs.length];
        Attachment[] outputAttachments = new Attachment[inputs.length];

        for (int i = 0; i < outputs.length; i++) {
            // create the output buffer of the desired length
            outputBuffers[i] = synap.createIoBuffer(outputs[i].length);

            // attach the buffer to the network (make sure you keep a reference to the
            // attachment to avoid it is garbage collected and destroyed)
            outputAttachments[i] = network.attachIoBuffer(outputBuffers[i]);

            // set the buffer as the i-th output of the network
            outputAttachments[i].useAsOutput(i);
        }

        // run the network
        network.run();

        // copy the result data to the output buffers
        for (int i = 0; i < outputs.length; i++) {
            outputBuffers[i].copyToBuffer(outputs[i], 0, 0, outputs[i].length);
        }

        // release resources (it will be done automatically when the objects are garbage
        // collected but this may take some time so it is better to release them explicitly
        // as soon as possible)

        network.release();  // this will automatically release the attachments

        for (int i = 0 ; i < inputs.length; i++) {
            inputBuffers[i].release();
        }

        for (int i = 0 ; i < outputs.length; i++) {
            outputBuffers[i].release();
        }

    }

}
