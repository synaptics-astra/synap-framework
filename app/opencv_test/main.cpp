#include <opencv2/core/utility.hpp>

#include <iostream>

using namespace cv;
using namespace std;

int test_ellipse();
int test_opencl(const CommandLineParser & parser);
int test_warp(const CommandLineParser & parser);

int main( int argc, char** argv )
{
    const char* keys =
        "{ i input    | | specify input image }"
        "{ o output   | | specify output image }"
        "{ warp       | | warp test }"
        "{ opencl     | | opencl test }"
        "{ ellipse    | | ellipse test }"
        "{ h help     | | print help message }";

    cv::CommandLineParser args(argc, argv, keys);

    if (args.has("help")) {
        cout << "Usage : " << argv[0] << " [options]" << endl;
        cout << "Example : opencv_test --warp -i=sample.jpg" << endl;
        cout << "Available options:" << endl;
        args.printMessage();
        return EXIT_SUCCESS;
    }

    if (args.has("warp"))
        return test_warp(args);

    if (args.has("ellipse"))
        return test_ellipse();

    if (args.has("opencl"))
        return test_opencl(args);

}
