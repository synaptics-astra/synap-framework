/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
///
/// Synap example program.
///
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

#define w 400

/// Function headers
void MyEllipse( Mat img, double angle );

int test_ellipse()
{
    /// Create black empty images
    Mat atom_image = Mat::zeros( w, w, CV_8UC3 );
    //![create_images]
    //
    /// 1.a. Creating ellipses
    MyEllipse( atom_image, 90 );
    MyEllipse( atom_image, 0 );
    MyEllipse( atom_image, 45 );
    MyEllipse( atom_image, -45 );

    const uint32_t uiHeight = 480;
    const uint32_t uiWidth = 640;

    uint32_t sizeIn = uiHeight * uiWidth * 3 / 2;
    uint32_t sizeOut = sizeIn;

    uint8_t *dataYuv = (uint8_t *)malloc(sizeIn);

    FILE *fpIn = fopen("/data/1.yuv", "rb");
    fread(dataYuv, sizeIn, 1, fpIn);
    fclose(fpIn);

    Mat yuv12 = Mat(uiHeight * 3/2, uiWidth, CV_8UC1, dataYuv);
    Mat rgb24 = Mat(uiHeight, uiWidth, CV_8UC3);

    //color space conversion
    cvtColor(yuv12, rgb24, COLOR_YUV2RGB_NV12);

    //Mat data field, good
    FILE *fpOut = fopen("/data/out.yuv", "wb");
    fwrite(yuv12.data, sizeOut, 1, fpOut);
    fclose(fpOut);

    //crop, ROI,
    FILE *fpOutCrop = fopen("/data/out_crop.yuv", "wb");
    Rect roi;
    roi.x = 20;
    roi.y = 30;
    roi.width = 320;
    roi.height = 240;
    uint32_t outCropSize = roi.width * roi.height * 3/2;

    //yuvcrop issue
    Mat cropYuv = yuv12(roi);
    fwrite(cropYuv.data, outCropSize, 1, fpOutCrop);
    fclose(fpOutCrop);
    return 0;
}

/// Function Declaration

/**
 * @function MyEllipse
 * @brief Draw a fixed-size ellipse with different angles
 */
//![my_ellipse]
void MyEllipse( Mat img, double angle )
{
  int thickness = 2;
  int lineType = 8;

  ellipse( img,
       Point( w/2, w/2 ),
       Size( w/4, w/16 ),
       angle,
       0,
       360,
       Scalar( 255, 0, 0 ),
       thickness,
       lineType );
}
//![my_ellipse]
