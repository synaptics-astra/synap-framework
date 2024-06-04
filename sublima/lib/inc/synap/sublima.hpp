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
/// Sublima HDR image processing
///

#include <memory>
#include <string>
#include <vector>


namespace synaptics {
namespace synap {

/// Convert SDR to HDR using Sublima algorithm
class Sublima {
public:
    Sublima();
    ~Sublima();

    struct JsonHDRInfo {
        uint32_t type{1}; // 1: HDR10, 2: HLG
        float PrimaryChromaticityR_X;
        float PrimaryChromaticityR_Y;
        float PrimaryChromaticityG_X;
        float PrimaryChromaticityG_Y;
        float PrimaryChromaticityB_X;
        float PrimaryChromaticityB_Y;
        float WhitePointChromaticity_X;
        float WhitePointChromaticity_Y;
        float LuminanceMax;
        float LuminanceMin;

        uint32_t MatrixCoefficients;
        uint32_t TransferCharacteristics;
        uint32_t ColorPrimaries;

        uint32_t MaxCLL;
        uint32_t MaxFALL;
    };

    /// Initialize the Sublima HDR processing
    /// @param lut2d: LUT2D file path (the LUT file is in CSV format)
    /// @param model: model file path
    /// @param meta: meta file path (deprecated, will be removed)
    /// @param hdrjson: hdr json parameters file path
    /// @return true if initialization was successful
    bool init(const std::string& lut2d,
              const std::string& model, const std::string& meta = "",
              const std::string& hdrjson = "");


    /// Get hdr parameter information
    /// @return hdr parameter
    JsonHDRInfo hdrinfo();
    
    /// Process the NV12 image to convert it to HDR
    /// @param nv12y: Y plane of the NV12 image
    /// @param nv12uv: UV plane of the NV12 image
    /// @param nv15y: Y plane of the NV15 image
    /// @param nv15uv: UV plane of the NV15 image
    /// @param width: image width
    /// @param height: image height
    /// @return true if processing was successful
    bool process(const uint8_t* nv12y, const uint8_t* nv12uv, uint8_t* nv15y, uint8_t* nv15uv, uint32_t width, uint32_t height);

    /// Enable/Disable HDR
    /// @param enable: true to enable HDR output, false to just convert to NV15
    /// @return true if success
    bool set_hdr(bool enable);

    /// Enable/Disable UV Conversion
    /// @param enable: true to disable UV conversion, only Y channel will be processed
    /// @return true if success
    bool set_only_y(bool enable);

    /// Get total processing timings (for debug)
    struct Timings {
        int assign;
        int infer;
        int nv15y;
        int nv15uv;
        int tot;
        int cnt;
    };
    Timings* timings();
    
private:
    // Implementation details
    struct Private;
    std::unique_ptr<Private> d;
};

}  // namespace synap
}  // namespace synaptics


