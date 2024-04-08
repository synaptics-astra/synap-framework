#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/c/c_api_experimental.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"

#include <stdio.h>
#include <assert.h>

int main(int argc, char** argv)
{
    printf("Hello from TensorFlow-Lite C library version %s\n", TfLiteVersion());

    TfLiteModel* model = TfLiteModelCreateFromFile("testdata/add.tflite");
    if (!model) {
        fprintf(stderr, "Create model failed\n");
        return 1;
    }

    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    if (!options) {
        fprintf(stderr, "Create interpreter options failed\n");
        return 1;
    }

    TfLiteInterpreterOptionsSetNumThreads(options, 4);
    TfLiteInterpreterOptionsSetEnableDelegateFallback(options, true);

    TfLiteInterpreter* interpreter = TfLiteInterpreterCreate(model, options);
    if (!interpreter) {
        fprintf(stderr, "Create interpreter failed\n");
        return -1;
    }

    TfLiteDelegate* xnnpack_delegate;
    TfLiteDelegate* gpu_delegate;

    // Add GPU delegate
    TfLiteGpuDelegateOptionsV2 gpu_opts = TfLiteGpuDelegateOptionsV2Default();
    gpu_opts.inference_preference = TFLITE_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED;
    gpu_opts.inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY;
    gpu_opts.inference_priority2 = TFLITE_GPU_INFERENCE_PRIORITY_MAX_PRECISION;
    gpu_delegate = TfLiteGpuDelegateV2Create(&gpu_opts);
    if (TfLiteInterpreterModifyGraphWithDelegate(interpreter, gpu_delegate) != kTfLiteOk) {
        fprintf(stderr, "ModifyGraphWith GPU Delegate failed, fallback to the others\n");
        // TfLiteGpuDelegateV2Delete(gpu_delegate);

        // TODO: check tflite modifyGraphWithDelegate flow
        // tensor delegate pointer doesn't clean after modifying graph with failed delegate
        // issue: subgraph.cc:600 tensor->delegate == nullptr || tensor->delegate == delegate was not true.
        // re-create interpreter to avoid this issue
        TfLiteInterpreterDelete(interpreter);
        interpreter = TfLiteInterpreterCreate(model, options);

        // Add XNNPack delegate
        TfLiteXNNPackDelegateOptions xnnpack_opts = TfLiteXNNPackDelegateOptionsDefault();
        xnnpack_opts.num_threads = 4;
        // vs640 and dvf120 could enable fp16 for fp model to get better performance
        // xnnpack_opts.flags |= TFLITE_XNNPACK_DELEGATE_FLAG_FORCE_FP16;
        xnnpack_delegate = TfLiteXNNPackDelegateCreate(&xnnpack_opts);
        if (TfLiteInterpreterModifyGraphWithDelegate(interpreter, xnnpack_delegate) != kTfLiteOk) {
            fprintf(stderr, "ModifyGraphWith XNNPACK Delegate failed, fallback to CPU\n");
        }
    }

    if (TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        fprintf(stderr, "AllocateTensors failed\n");
        return 1;
    }
    size_t input_count = TfLiteInterpreterGetInputTensorCount(interpreter);
    size_t output_count = TfLiteInterpreterGetOutputTensorCount(interpreter);
    if (input_count <= 0 || output_count <= 0) {
        fprintf(stderr, "Wrong model as no input or output\n");
        return 1;
    }
    // for this case, we know we have only 1 input and 1 output
    assert(input_count == 1);
    assert(output_count == 1);

    int input_dims[1] = {2};
    if (TfLiteInterpreterResizeInputTensor(interpreter, 0, input_dims, 1) != kTfLiteOk) {
        fprintf(stderr, "Interpreter ResizeInputTensor failed\n");
        return 1;
    }

    if (TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        fprintf(stderr, "AllocateTensors failed\n");
        return 1;
    }

    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
    if (!input_tensor) {
        fprintf(stderr, "Get input tensor pointer failed\n");
        return 1;
    }

    float input[2] = {1.f, 3.f};
    if (TfLiteTensorCopyFromBuffer(input_tensor, input, 2 * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "CopyFromBuffer failed\n");
        return 1;
    }

    if (TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
        fprintf(stderr, "Invoke failed\n");
        return 1;
    }

    const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
    if (!output_tensor) {
        fprintf(stderr, "Get output tensor pointer failed\n");
        return 1;
    }

    float output[2];
    if (TfLiteTensorCopyToBuffer(output_tensor, output, 2 * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "CopyToBuffer failed\n");
        return 1;
    }
    assert(output[0] == 3.f);
    assert(output[1] == 9.f);

    if (gpu_delegate) TfLiteGpuDelegateV2Delete(gpu_delegate);
    if (xnnpack_delegate) TfLiteXNNPackDelegateDelete(xnnpack_delegate);

    TfLiteInterpreterDelete(interpreter);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(model);

    printf("Finish Tflite C API testcases\n");

    return 0;
}