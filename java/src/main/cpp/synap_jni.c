#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

#include <jni.h>

#define LOG_TAG "synap_jni"

#include <android/log.h>

#include <synap_device.h>

#define ALOGE(format, args...) __android_log_print(ANDROID_LOG_ERROR, "synap_jni", format , ## args)

static int synapFd = -1;

struct ClientIoBuffer {
    jlong bid;
    int fd;
    void *addr;
    jlong size;
};

JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_init
        (JNIEnv *env, jclass clazz) {

    return synap_init();
}

JNIEXPORT jlong JNICALL Java_com_synaptics_synap_SynapNative_createNetwork
        (JNIEnv *env, jclass clazz, jbyteArray code) {

    jbyte *codeBytes = (*env)->GetByteArrayElements(env, code, NULL);
    jlong dataLen = (*env)->GetArrayLength(env, code);

    uint32_t network;

    bool success = synap_prepare_network(codeBytes, dataLen, &network);

    (*env)->ReleaseByteArrayElements(env, code, codeBytes, JNI_ABORT);

    if (!success) {
        ALOGE("error while creating network");
        return -1;
    }

    return network;

}

JNIEXPORT void JNICALL Java_com_synaptics_synap_SynapNative_releaseNetwork
        (JNIEnv *env, jclass clazz, jlong network) {

    synap_release_network(network);

}

JNIEXPORT jlong JNICALL Java_com_synaptics_synap_SynapNative_attachBufferToNetwork
        (JNIEnv *env, jclass clazz, jlong network, jlong ioBuffer) {

    struct ClientIoBuffer *clientIoBuffer = (struct ClientIoBuffer *) ioBuffer;

    uint32_t attachment;

    if (!synap_attach_io_buffer(network, clientIoBuffer->bid, &attachment)) {
        return -1;
    }

    return attachment;
}

JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_runNetwork
        (JNIEnv *env, jclass clazz, jlong network) {

    return synap_run_network(network);
}

JNIEXPORT void JNICALL Java_com_synaptics_synap_SynapNative_releaseAttachment
        (JNIEnv *env, jclass clazz, jlong network, jlong attachment) {

    synap_detach_io_buffer(network, attachment);
}

JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_useAttachmentAsInput
        (JNIEnv *env, jclass clazz, jlong network, jlong attachment, jint index) {

    return synap_set_input(network, attachment, index);

}

JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_useAttachmentAsOutput
        (JNIEnv *env, jclass clazz, jlong network, jlong attachment, jint index) {

    return synap_set_output(network, attachment, index);

}

JNIEXPORT jlong JNICALL Java_com_synaptics_synap_SynapNative_createIoBuffer
        (JNIEnv *env, jclass clazz, jlong size) {

    struct ClientIoBuffer *clientIoBuffer = malloc(sizeof(struct ClientIoBuffer));

    if (!clientIoBuffer) {
        return 0;
    }

    uint32_t fd;
    uint32_t bid;

    if (!synap_allocate_io_buffer(size, &bid, NULL, &fd)) {
        ALOGE("Unable to create io buffer");
        free(clientIoBuffer);
        return 0;
    }

    clientIoBuffer->bid = bid;
    clientIoBuffer->fd = (int) fd;
    clientIoBuffer->size = size;

    clientIoBuffer->addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                                clientIoBuffer->fd, 0);

    if (clientIoBuffer->addr == MAP_FAILED) {
        synap_destroy_io_buffer(bid);
        synap_destroy_io_buffer(bid);
        free(clientIoBuffer);
        return 0;
    }

    return (jlong) clientIoBuffer;
}

JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_copyDataFromIoBuffer
        (JNIEnv *env, jclass clazz, jlong ioBuffer, jbyteArray data,
         jlong srcOffset, jlong dstOffset, jlong count) {

    struct ClientIoBuffer *clientIoBuffer = (struct ClientIoBuffer *) ioBuffer;

    if (clientIoBuffer->size < srcOffset + count) {
        return false;
    }

    if (!synap_lock_io_buffer(clientIoBuffer->fd)) {
        return false;
    }

    (*env)->SetByteArrayRegion(env, data, (jsize) dstOffset, (jsize) count,
                               clientIoBuffer->addr + srcOffset);

    if (!synap_unlock_io_buffer(clientIoBuffer->fd)) {
        return false;
    }

    return true;
}


JNIEXPORT jboolean JNICALL Java_com_synaptics_synap_SynapNative_copyDataToIoBuffer
        (JNIEnv *env, jclass clazz, jlong ioBuffer, jbyteArray data, jlong srcOffset,
         jlong dstOffset, jlong count) {

    struct ClientIoBuffer *clientIoBuffer = (struct ClientIoBuffer *) ioBuffer;

    if (clientIoBuffer->size < dstOffset + count) {
        return false;
    }

    if (!synap_lock_io_buffer(clientIoBuffer->fd)) {
        return false;
    }

    (*env)->GetByteArrayRegion(env, data, (jsize) srcOffset, (jsize) count,
                               clientIoBuffer->addr + dstOffset);

    if (!synap_unlock_io_buffer(clientIoBuffer->fd)) {
        return false;
    }

    return true;

}

JNIEXPORT void JNICALL Java_com_synaptics_synap_SynapNative_releaseIoBuffer
        (JNIEnv *env, jclass clazz, jlong ioBuffer) {

    struct ClientIoBuffer *clientIoBuffer = (struct ClientIoBuffer *) ioBuffer;

    munmap(clientIoBuffer->addr, clientIoBuffer->size);

    close(clientIoBuffer->fd);

    synap_destroy_io_buffer(clientIoBuffer->bid);

    free(clientIoBuffer);

}

JNIEXPORT void JNICALL Java_com_synaptics_synap_SynapNative_deinit
        (JNIEnv *env, jclass clazz) {
    synap_deinit();
}
