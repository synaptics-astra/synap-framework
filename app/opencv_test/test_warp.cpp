#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utility.hpp>

#include <iostream>

using namespace cv;
using namespace std;

// test cmd: opencv_test --warp -i=/data/112x112.jpg
// timing on VS680A0:
// Matrix [112 x 112] channel 3 rotate angle -50 CPU warp affine timing : 0.87648ms
// Matrix [112 x 112] channel 3 rotate angle -50 OpenCL(gpu) warp affine timing : 43.4791ms

int test_warp(const CommandLineParser & parser)
{
    string image_file = parser.get<string>("i");
    cout << "Warp test: processing input file " << image_file << endl;

    Mat src;
    if (!image_file.empty()) {
        src = imread(samples::findFile(image_file));
        if (src.empty()) {
            cout << "error read image: " << image_file << endl;
            return 1;
        }
    }

    Point center = Point( src.cols/2, src.rows/2 );
    double angle = -50.0;
    double scale = 0.6;
    Mat rot_mat = getRotationMatrix2D( center, angle, scale );

    TickMeter tm;
    Mat dst;
    tm.start();
    warpAffine( src, dst, rot_mat, src.size() );
    tm.stop();
    cout << " Matrix " << src.size() << " channel " << src.channels() << " rotate angle " << angle <<
        " CPU warp affine timing : " << tm.getTimeMilli() << endl;

    UMat warp_rotate_dst;
    tm.start();
    warpAffine( src, warp_rotate_dst, rot_mat, src.size() );
    tm.stop();
    cout << " Matrix " << src.size() << " channel " << src.channels() << " rotate angle " << angle <<
        " OpenCL(gpu) warp affine timing : " << tm.getTimeMilli() << endl;

    imwrite("warp_rotate-image.jpg", warp_rotate_dst);
    return 0;
}
