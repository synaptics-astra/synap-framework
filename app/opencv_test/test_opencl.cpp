#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/utility.hpp>

#include <iostream>
#include <fstream>
#include <iterator>

using namespace cv;
using namespace std;

int test_opencl(const CommandLineParser& args)
{
    string image_file = args.get<string>("i");
    cout << "OpenCL processing input: " << image_file << endl;

    cv::ocl::Context ctx = cv::ocl::Context::getDefault();
    if (!ctx.ptr())
    {
        cerr << "OpenCL is not available" << endl;
        return 1;
    }

    cout << "Find " << ctx.ndevices() << " OpenCL supported device" << endl;

    for (int i = 0; i < ctx.ndevices(); i++) {
        cv::ocl::Device device = ctx.device(i);
        cout << "name                   : " << device.name() << endl;
        cout << "available              : " << device.available() << endl;
        cout << "imageSupport           : " << device.imageSupport() << endl;
        cout << "OpenCL_C_Version       : " << device.OpenCL_C_Version() << endl;
        cout << "image2DMaxResolution   : " << device.image2DMaxWidth() << "x" << device.image2DMaxHeight() << endl;
        cout << "imageBaseAddrAlignment : " << device.imageBaseAddressAlignment() << endl;
        cout << "localMemSize           : " << device.localMemSize() << endl;
        cout << "localMemType           : " << device.localMemType() << endl;
        cout << "maxClockFrequency      : " << device.maxClockFrequency() << endl;
        cout << "maxComputeUnits        : " << device.maxComputeUnits() << endl;
        cout << "maxWorkGroupSize       : " << device.maxWorkGroupSize() << endl;
        cout << "maxWorkItemDims        : " << device.maxWorkItemDims() << endl;
        cout << endl;
    }

    cv::ocl::Device device = cv::ocl::Device::getDefault();
    if (!device.compilerAvailable())
    {
        cerr << "OpenCL compiler is not available" << endl;
        return 1;
    }

    UMat src;
    if (!image_file.empty()) {
        Mat image = imread(samples::findFile(image_file));
        if (image.empty())
        {
            cout << "error read image: " << image_file << endl;
            return 1;
        }
        cvtColor(image, src, COLOR_BGR2GRAY);

    } else {

        Mat frame(cv::Size(640, 480), CV_8U, Scalar::all(128));

        Point p(frame.cols / 2, frame.rows / 2);
        line(frame, Point(0, frame.rows / 2), Point(frame.cols, frame.rows / 2), 1);
        circle(frame, p, 200, Scalar(32, 32, 32), 8, LINE_AA);

        string str = "OpenCL";
        int baseLine = 0;
        Size box = getTextSize(str, FONT_HERSHEY_COMPLEX, 2, 5, &baseLine);
        putText(frame, str, Point((frame.cols - box.width) / 2, (frame.rows - box.height) / 2 + baseLine),
                FONT_HERSHEY_COMPLEX, 2, Scalar(255, 255, 255), 5, LINE_AA);

        frame.copyTo(src);
    }

    std::ifstream ifs("magnitude_filter_8u.cl");
    if (ifs.fail()) {
        cerr << "magnitude_filter_8u.cl missing" << endl;
        return 1;
    }
    std::string kernelSource((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    cv::ocl::ProgramSource source(kernelSource);

    // Compiler config for reference
    /* cv::String buildopt = cv::format("-D dstT=%s", cv::ocl::typeToStr(umat_dst.depth())); // "-D dstT=float" */
    /* cv::ocl::Program program = context.getProg(programSource, buildopt, errmsg); */

    // Compile/build OpenCL for current OpenCL device
    cv::String errmsg;
    cv::ocl::Program program(source, "", errmsg);
    if (program.ptr() == NULL) {
        cerr << "Can't compile OpenCL program:" << endl << errmsg << endl;
        return 1;
    }

    if (!errmsg.empty()) {
        cout << "OpenCL program build log:" << endl << errmsg << endl;
    }

    //Get OpenCL kernel by name
    cv::ocl::Kernel k("magnutude_filter_8u", program);
    if (k.empty())
    {
        cerr << "Can't get OpenCL kernel" << endl;
        return 1;
    }

    UMat result(src.size(), CV_8UC1);

    // Define kernel parameters and run
    size_t globalSize[2] = {(size_t)src.cols, (size_t)src.rows};
    size_t localSize[2] = {8, 8};
    bool executionResult = k
        .args(
              cv::ocl::KernelArg::ReadOnlyNoSize(src), // size is not used (similar to 'dst' size)
              cv::ocl::KernelArg::WriteOnly(result),
              (float)2.0
             )
        .run(2, globalSize, localSize, true);
    if (!executionResult)
    {
        cerr << "OpenCL kernel launch failed" << endl;
        return 1;
    }

    imwrite("opencl-test-src.jpg", src);
    imwrite("opencl-test-result.jpg", result);

    cout << "Done, find opencl-test-src[result].jpg under current directory" << endl;

    return 0;
}

