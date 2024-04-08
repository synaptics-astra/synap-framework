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

#include "synap/detector.hpp"
#include "synap/logging.hpp"
#include "synap/network.hpp"
#include "synap/timer.hpp"
#include "synap/string_utils.hpp"
#include "synap/image_convert.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#if SYNAP_NB_NEON
#include <arm_neon.h>
#endif

using namespace std;

namespace synaptics {
namespace synap {

/// Detection point
struct Pt {
    float x;
    float y;
};

/// Detection bounding box
struct Box {
    Pt tl, br;      // Top-left and Bottom-right corners
};

/// Detection info
struct Detection {
    float score;
    int class_index;
    Box box;
    vector<Landmark> lm;
};


//
// Detector implemetation classes
//

class Detector::Impl {
public:
    virtual bool init(const Tensors& tensors);
    virtual vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) = 0;
    virtual ~Impl() {}
    int index_base() const { return _index_base; }
    int landmarks_count() const { return _landmarks_count; }
    int visibility() const { return _visibility; }
    bool valid() const { return _valid; }
    bool validate() { _valid = true; return true; }

private:
    // Base index for the 1st class
    int _index_base{};

    // Number of 2D landmark points
    int _landmarks_count{};

    // Initializaton completed succesfully
    bool _valid{};

    // Visibility associated with the points
    int _visibility{};
};
class DetectorBoxesScores : public Detector::Impl {
public:
    bool init(const Tensors& tensors) override;
    virtual vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) override;

protected:
    struct Scaling {
        float y, x, h, w;
    };

    Scaling _scale{};
    std::vector<float> _anchors{};

private:
    virtual Box get_box(const float* deltas, const float* anchors, const Dim2d& dim) = 0;
};


class DetectorRetinanet : public DetectorBoxesScores {
private:
    Box get_box(const float* deltas, const float* anchors, const Dim2d& dim) override;
};


class DetectorTfliteODPostprocessIn : public DetectorBoxesScores {
public:
    DetectorTfliteODPostprocessIn() {
        _scale.y = _scale.x = 10;
        _scale.h = _scale.w = 5;
    }

private:
    Box get_box(const float* deltas, const float* anchors, const Dim2d& dim) override;
};


class DetectorTfliteODPostprocessOut : public Detector::Impl {
private:
    Dim2d _in_tensor_dim{};

    struct Outputs {
        const Tensor* boxes{};
        const Tensor* classes{};
        const Tensor* scores{};
        const Tensor* num_detections{};
    };
    
    /// Fill output tensor structure
    /// @return false if invalid tensor shapes
    static bool get_outputs(const Tensors& tensors, Outputs& outs);

public:
    bool init(const Tensors& tensors) override;
    vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) override;
};


class DetectorYoloBase : public Detector::Impl {
protected:
    Dim2d _in_tensor_dim{};
    bool _out_bb_norm{};

public:
    bool init(const Tensors& tensors) override;
};

class DetectorYolov5 : public DetectorYoloBase {

public:
    vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) override;
};

class DetectorYolov8 : public DetectorYoloBase {

public:
    vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) override;
};

class DetectorYolov5Pyramid : public Detector::Impl {
public:
    bool init(const Tensors& tensors) override;
    vector<Detection> get_detections(float min_score, const Tensors& tensors, Dim2d in_dim) override;
private:
    // Supported Tensor Layouts
    // Where:
    //  A: number of anchors per pyramid image
    //  D: detection size = (xywh/confidence/landmarks[][2]/classConfidence[])
    enum class OutLayout {
        hwad,   // Out shape: 1xHxWxAxD or 1xHxWx(A*D) [yolov5s-640x480-noreshape]
        ahwd,   // Out shape: 1xAxHxWxD (auto-detected) [yolov5s_face_640x480_onnx]
        adhw    // Out shape: 1xAxDxHxW or 1x(A*D)xHxW (requires "transposed=1")
                //            [yolov5s_face_640x480_onnx_notransp/noreshape]
    };
    
    // Detection content
    struct RawDetection {
        float x, y, w, h, confidence, lm_class_confidence[0];
    };

    vector<vector<int>> _anchors{};
    size_t _anchors_count{};  // N. of anchors per pyramid element. Each is represented by x & y
    int _class_count{};    // Number of classes
    int _pyramid_base{};   // Pyramid scale for 1st output (P0: full image, P1: scald by half, ..)
    size_t _x_axis{};      // Index of x-axis in the tensor shape
    size_t _y_axis{};      // Index of y-axis in the tensor shape
    OutLayout _ol{};       // Output layout
};


/// Determines whether the detection should be kept or not.
///
/// Detection is discarded if the box overlap with any one already selected is above threshold.
/// @param curr_idx: index of detectiont to be checked
/// @param detections: all detections
/// @param selected_indices: indexes of selected detections
/// @param iou_threshold: discard if box overlap above this threshold
/// @param use_min: use minimum area instead of intersection
/// @return true to keep, false to discard
static bool keep_detection(int32_t curr_idx, const vector<Detection>& detections,
                           const vector<int32_t>& selected_indices, float iou_threshold, bool use_min)
{
    const Box* curr_box = &detections[curr_idx].box;
    float curr_box_area = (curr_box->br.x - curr_box->tl.x) * (curr_box->br.y - curr_box->tl.y);

    // Check if any of the selected boxes has high overlap with the current one
    for (int32_t i = 0; i < selected_indices.size(); i++) {
        // Calculate intersection width and height.
        const Box* box = &detections[selected_indices[i]].box;
        float iw = min(curr_box->br.x, box->br.x) - max(curr_box->tl.x, box->tl.x);
        float ih = min(curr_box->br.y, box->br.y) - max(curr_box->tl.y, box->tl.y);
        float box_area = (box->br.x - box->tl.x) * (box->br.y - box->tl.y);
        // Calculate IOU and compare with the threshold for non-zero IOUs only.
        if (ih > 0.0 && iw > 0.0) {
            float ia = iw * ih;
            float ua = use_min ? min(curr_box_area, box_area) : curr_box_area + box_area - ia;
            float iou = ia / ua;
            if (iou > iou_threshold) {
                return false;
            }
        }
    }

    // Low overlap with all the detections already selected
    return true;
}


/// Select a subset of detections in descending order of score.
///
/// If NonMaxSuppression is enabled prunes away those with high intersection-over-union (IOU)
/// overlap with previously selected boxes.
/// @param max_detections: maximum number of detections to be selected (0: all)
/// @param detections: all detections
/// @param nms: if true apply NMS, else just pick the detections with highest score
/// @param iou_threshold: max allowed overlap for IOU in the range [0, 1]
/// @param iou_with_min: use min to compute IOU
///
/// @return: indexes of selected boxes in the 'boxes' array.
static vector<int32_t> select(int32_t max_detections, const vector<Detection>& detections, bool nms,
                              float iou_threshold, bool iou_with_min)
{
    // Sort detections in order of decreasing scores
    auto compare_score = [&detections](int32_t i, int32_t j) {
        return detections[i].score < detections[j].score;
    };
    vector<int32_t> indices(detections.size());
    iota(begin(indices), end(indices), 0);
    make_heap(begin(indices), end(indices), compare_score);

    // Generate indices of top scoring boxes that overlap less than the iou_threshold.
    vector<int32_t> selected_indexes;
    while (!indices.empty() && (max_detections == 0 || selected_indexes.size() < max_detections)) {
        if (!nms || keep_detection(indices.front(), detections, selected_indexes, iou_threshold, iou_with_min)) {
            selected_indexes.push_back(indices.front());
        }
        pop_heap(begin(indices), end(indices), compare_score);
        indices.pop_back();
    };

    return selected_indexes;
}


#if SYNAP_NB_NEON
/// Get index of max value in a vector using vectorized neon instructions
/// 
/// @param v: vector
/// @param size: number of elements in v[]
/// @param[out] consumed: number of items actually checked (may be less than size)
/// @return index of max item, or -1 if nothing done
inline int get_index_max_neon(const float32_t* v, size_t size, int32_t* consumed)
{
    if (size < 12) {
        // Not efficient for very small sizes
        *consumed = 0;
        return -1;
    }
    *consumed = size & ~(4 - 1);

    // Compare 4 elements at a time.
    static const uint32_t index_max_init[4] = {0, 1, 2, 3};
    uint32x4_t index_max = vld1q_u32(index_max_init);
    static const uint32_t index_init[4] = {4, 5, 6, 7};
    uint32x4_t index = vld1q_u32(index_init);
    uint32x4_t delta = vdupq_n_u32(4);
    float32x4_t max_value = vld1q_f32(v);
    for (const float32_t *vi = &v[4], *v_end = &v[*consumed]; vi != v_end; vi += 4) {
        float32x4_t current_value = vld1q_f32(vi);
        uint32x4_t select_flags = vcgtq_f32(current_value, max_value);
        max_value = vbslq_f32(select_flags, current_value, max_value);
        index_max = vbslq_u32(select_flags, index, index_max);
        index = vaddq_u32(index, delta);
    }

    // Here index_max contains 4 indexes, one of them corresponds to the max value. Find out which.
    uint32_t ix_max[4];
    vst1q_u32(ix_max, index_max);
    int ixmax01 = v[ix_max[1]] > v[ix_max[0]] ? ix_max[1] : ix_max[0];
    int ixmax23 = v[ix_max[3]] > v[ix_max[2]] ? ix_max[3] : ix_max[2];
    return v[ixmax01] > v[ixmax23] ? ixmax01 : ixmax23;
}
#endif


/// Get index of max value in a vector
/// 
/// @param v: vector
/// @param size: number of elements in v[]
/// @return index of max item, or -1 if size is 0
inline int get_index_max(const float* v, size_t size)
{
    int32_t i = 0;
#if SYNAP_NB_NEON
    // Process as many items as we can using vectorized processing
    int index_max = get_index_max_neon(v, size, &i);
    float max_value = index_max >= 0 ? v[index_max] : numeric_limits<float>::min();
    v += i;
#else
    int index_max = -1;
    float max_value = numeric_limits<float>::min();
#endif
    for (; i < size; i++, v++) {
        if (*v > max_value) {
            max_value = *v;
            index_max = i;
        }
    }
    return index_max;
}


bool Detector::Impl::init(const Tensors& tensors)
{
    if (tensors.size() == 0) {
        LOGE << "Error, no tensors";
        return false;
    }

    // Get base class index to be used.
    // This allows to normalize the output of a classifier network which has been trained
    // with a subset or with additional background/unrecognized class in confidence[0].
    // This parameter can be sepecified in any tensor
    for (const auto& t: tensors) {
        _index_base = format_parse::get_int(t.format(), "class_index_base", _index_base);
    }

    // Get number of landmarks detected
    _landmarks_count = format_parse::get_int(tensors[0].format(), "landmarks", 0);
    // Get visibility detected
    _visibility = format_parse::get_int(tensors[0].format(), "visibility", 0);

    LOGI << "Detector class_index_base = " << _index_base;
    LOGI << "Detector landmarks = " << _landmarks_count;
    LOGI << "Detector visibility = " << _visibility;
    return true;
}


bool DetectorBoxesScores::init(const Tensors& tensors)
{
    if (tensors.size() != 2) {
        LOGE << "Error, expected tensors: 2, got: " << tensors.size();
        return false;
    }

    if (!Detector::Impl::init(tensors)) {
        return false;
    }
    const string& format = tensors[0].format();
    _scale.y = format_parse::get_float(format, "y_scale", _scale.y);
    _scale.x = format_parse::get_float(format, "x_scale", _scale.x);
    _scale.h = format_parse::get_float(format, "h_scale", _scale.h);
    _scale.w = format_parse::get_float(format, "w_scale", _scale.w);
    _anchors = format_parse::get_floats(format, "anchors");

    LOGI << "Detector scale = " <<  _scale.x << "," << _scale.y << "," << _scale.h << "," << _scale.w;
    LOGI << "Detector anchors size = " << _anchors.size();
    if (_anchors.empty()) {
        LOGE << "Error parsing anchor information";
        return false;
    }
    return true;
}


vector<Detection> DetectorBoxesScores::get_detections(float min_score, const Tensors& tensors, Dim2d in_dim)
{
    const Tensor& regression_tensor = tensors[0];
    auto num_boxes = regression_tensor.shape().at(1);
    const Tensor& classification_tensor = tensors[1];
    auto num_classes = classification_tensor.shape().at(2);
    LOGV << "Detector boxes: " << num_boxes << " classes: " << num_classes;

    // Create a detection for each box with max score above threshold
    const float* scores = classification_tensor.as_float();
    const float* deltas = regression_tensor.as_float();

    vector<Detection> dv;
    for (int32_t i = 0; i < num_boxes; i++, scores += num_classes) {
        // Find the class with the highest score
        int c = get_index_max(scores, num_classes);

        // Create a Detection for this box if score above threshold
        if (c >= 0 && scores[c] >= min_score) {
            dv.push_back({scores[c], c, get_box(&deltas[i * 4], &_anchors[i * 4], in_dim)});
        }
    }
    
    return dv;
}


/// Apply regressed deltas to anchors to produce the actual bounding box.
/// Deltas and anchors expected to be according to retinanet convention.
///
/// The boxes that are spilling out of the image are clipped.
///  @param deltas: regressed deltas in (x1, y1, x2, y2) format for this box
///  @param anchors: anchor boxes in (x1, y1, x2, y2) format for this box
///  @param dim: input image dimensions
///  @return bounding box in image coordinates
Box DetectorRetinanet::get_box(const float* deltas, const float* anchors, const Dim2d& dim)
{
    constexpr float mean = 0.0f;
    constexpr float std = 0.2f;
    const float h_scale = _scale.h ? dim.y / _scale.h : 1;
    const float w_scale = _scale.w ? dim.x / _scale.w : 1;
    // Get current anchor width and height.
    float w = anchors[2] - anchors[0];
    float h = anchors[3] - anchors[1];

    // Denormalize deltas, add them to anchors
    Box box;
    box.tl.x = h_scale * (anchors[0] + (deltas[0] * std + mean) * w);
    box.tl.y = w_scale * (anchors[1] + (deltas[1] * std + mean) * h);
    box.br.x = h_scale * (anchors[2] + (deltas[2] * std + mean) * w);
    box.br.y = w_scale * (anchors[3] + (deltas[3] * std + mean) * h);
    return box;
}


/// Apply regressed deltas to anchors to produce the actual bounding box.
/// Deltas and anchors expected to be according to TFLite_Detection_PostProcess convention.
/// @see https://github.com/tensorflow/tensorflow/blob/master/tensorflow/lite/kernels/detection_postprocess.cc
/// The boxes that are spilling out of the image are clipped.
/// 
///  @param deltas: regressed deltas in (y-center, x-center, h, w) format for this box
///  @param anchors: anchor boxes in (y-center, x-center, h, w) format for this box
///  @param dim: input image dimensions
///  @return bounding box in image coordinates
Box DetectorTfliteODPostprocessIn::get_box(const float* deltas, const float* anchors, const Dim2d& dim)
{
    struct CenterSizeEncoding {
        float y, x, h, w;
    };
    const CenterSizeEncoding& box_centersize = *reinterpret_cast<const CenterSizeEncoding*>(deltas);
    const CenterSizeEncoding& anchor = *reinterpret_cast<const CenterSizeEncoding*>(anchors);

    float ycenter = box_centersize.y / _scale.y * anchor.h + anchor.y;
    float xcenter = box_centersize.x / _scale.x * anchor.w + anchor.x;
    float half_h = 0.5 * std::exp(box_centersize.h / _scale.h) * anchor.h;
    float half_w = 0.5 * std::exp(box_centersize.w / _scale.w) * anchor.w;

    Box box;
    box.tl.y = (ycenter - half_h) * dim.y;
    box.tl.x = (xcenter - half_w) * dim.x;
    box.br.y = (ycenter + half_h) * dim.y;
    box.br.x = (xcenter + half_w) * dim.x;
    return box;
}


bool DetectorTfliteODPostprocessOut::get_outputs(const Tensors& tensors, Outputs& outs)
{
    if (tensors.size() != 4) {
        LOGE << "Error, expected tensors: 4, got: " << tensors.size();
        return false;
    }
    outs.boxes = &tensors[0];
    outs.classes = &tensors[1];
    outs.scores = &tensors[2];
    outs.num_detections = &tensors[3];
    
    if (outs.boxes->shape().size() != 3 ) {
        LOGE << "Invalid tensor shape for outputs[0] (boxes): " << outs.boxes->shape() << ". Expected 3 dimensions";
        return false;
    }
    if (outs.classes->shape().size() != 2 ) {
        LOGE << "Invalid tensor shape for outputs[1] (classes): " << outs.classes->shape() << ". Expected 2 dimensions";
        return false;
    }
    if (outs.scores->shape().size() != 2 ) {
        LOGE << "Invalid tensor shape for outputs[2] (scores): " << outs.scores->shape() << ". Expected 2 dimensions";
        return false;
    }
    if (!outs.num_detections->is_scalar()) {
        LOGE << "Error, outputs[3] must be a scalar, got shape: " << outs.num_detections->shape();
        return false;
    }
    const int detection_count_max = outs.boxes->shape()[1];
    if (outs.classes->shape()[1] != detection_count_max || outs.scores->shape()[1] != detection_count_max) {
        LOGE << "Incongruent tensor shapes";
        return false;
    }

    return true;
}

bool DetectorTfliteODPostprocessOut::init(const Tensors& tensors)
{
    Outputs outs;
    if (!get_outputs(tensors, outs)) {
        return false;
    }
    if (!Detector::Impl::init(tensors)) {
        return false;
    }

    const string& format = tensors[0].format();
    _in_tensor_dim.x = format_parse::get_float(format, "w_scale", 0);
    _in_tensor_dim.y = format_parse::get_float(format, "h_scale", 0);
    if (!_in_tensor_dim.x) {
        LOGW << "Detector w_scale not specified, boxes will not be rescaled to image size";
    }
    if (!_in_tensor_dim.y) {
        LOGW << "Detector h_scale not specified, boxes will not be rescaled to image size";
    }
    LOGI << "Detector scale = " <<  _in_tensor_dim.x << "," << _in_tensor_dim.y;
    return true;
}


vector<Detection> DetectorTfliteODPostprocessOut::get_detections(float min_score, const Tensors& tensors, Dim2d in_dim)
{
    // For more info:
    // https://github.com/tensorflow/tensorflow/blob/master/tensorflow/lite/kernels/detection_postprocess.cc 
    // https://github.com/tensorflow/tensorflow/issues/53713
    const bool _out_bb_norm = true;
    struct RawDetection {
        float y, x, y1, x1;
    };
    Pt scale {_in_tensor_dim.x ? (float)in_dim.x / _in_tensor_dim.x : 1.0f,
             _in_tensor_dim.y ? (float)in_dim.y / _in_tensor_dim.y : 1.0f};
    Pt relative_scale {_out_bb_norm ? (float)in_dim.x : scale.x,
                      _out_bb_norm ? (float)in_dim.y : scale.y};
    
    Outputs outs;
    if (!get_outputs(tensors, outs)) {
        return {};
    }
    
    const int detection_count_max = outs.boxes->shape()[1];
    const int detection_count = static_cast<int>(outs.num_detections->as_float()[0]);
    if (detection_count < 0 || detection_count > detection_count_max) {
        LOGE << "Invalid detection_count: " << detection_count;
        return {};
    }

    vector<Detection> dv;
    
    for (int i = 0; i < detection_count; i++) {
        float class_score = outs.scores->as_float()[i];
        if (class_score < min_score) continue;
        int c = static_cast<int>(outs.classes->as_float()[i]);
        const RawDetection* detection = reinterpret_cast<const RawDetection*>(&outs.boxes->as_float()[i * 4]);
        Box box;
        box.tl.x = (detection->x /* - detection->w / 2 */) * relative_scale.x;
        box.tl.y = (detection->y /* - detection->h / 2 */) * relative_scale.y;
        box.br.x = (detection->x1 /* + detection->w / 2 */) * relative_scale.x;
        box.br.y = (detection->y1 /* + detection->h / 2 */) * relative_scale.y;
        vector<Landmark> lms;
        dv.push_back({class_score, c, box, lms});
    }

    return dv;
}



bool DetectorYoloBase::init(const Tensors& tensors)
{
    if (!Detector::Impl::init(tensors)) {
        return false;
    }
    const string& format = tensors[0].format();
    _in_tensor_dim.x = format_parse::get_float(format, "w_scale", 0);
    _in_tensor_dim.y = format_parse::get_float(format, "h_scale", 0);
    _out_bb_norm = format_parse::get_bool(format, "bb_normalized", 0);
    if (!_in_tensor_dim.x && !_out_bb_norm) {
        LOGW << "Detector w_scale not specified, boxes will not be rescaled to image size";
    }
    if (!_in_tensor_dim.y && !_out_bb_norm) {
        LOGW << "Detector h_scale not specified, boxes will not be rescaled to image size";
    }
    LOGI << "Detector scale = " <<  _in_tensor_dim.x << "," << _in_tensor_dim.y;
    return true;
}

vector<Detection> DetectorYolov5::get_detections(float min_score, const Tensors& tensors, Dim2d in_dim)
{
    struct RawDetection {
        float x, y, w, h, confidence, lm_class_confidence[0];
    };
    Pt scale {_in_tensor_dim.x ? (float)in_dim.x / _in_tensor_dim.x : 1.0f,
              _in_tensor_dim.y ? (float)in_dim.y / _in_tensor_dim.y : 1.0f};
    constexpr int classes_base_index = 5;
    const Tensor& t0 = tensors[0];
    if ((t0.shape().size() != 3 || t0.shape()[0] != 1) /* TODO: && t0.shape().size() != 2 */) {
        LOGE << "Invalid tensor shape: " << t0.shape();
        return {};
    }
    auto num_classes = t0.shape().at(2) - classes_base_index - landmarks_count() * 2;
    LOGV << "Detector classes: " << num_classes;
    if (num_classes <= 0) {
        LOGE << "Invalid tensor shape: " << t0.shape();
        return {};
    }

    vector<Detection> dv;
    // Loop over all output tensors (in case the final concat layer is missing)
    for(const Tensor& tensor: tensors) {
        if (tensor.shape().size() != 3 || tensor.shape()[2] != t0.shape()[2]) {
            LOGE << "Invalid tensor shape: " << tensor.shape();
            return {};
        }
        // Create a detection for each box with max score above threshold
        const float* detections = tensor.as_float();
        auto num_boxes = tensor.shape().at(1);
        size_t det_sz = sizeof(RawDetection) / sizeof(float) + landmarks_count() * 2 + num_classes;
        LOGV << "Detector boxes: " << num_boxes;
        for (int32_t i = 0; i < num_boxes; i++, detections += det_sz) {
            const RawDetection* detection = reinterpret_cast<const RawDetection*>(detections);
            if (detection->confidence < min_score) {
                // Overall confidence is too low
                continue;
            }
    
            // Find the class with the highest score
            int c = get_index_max(&detection->lm_class_confidence[landmarks_count() * 2], num_classes);
    
            // Create a Detection for this box if score above threshold
            float class_score = detection->confidence * detection->lm_class_confidence[landmarks_count() * 2 + c];
            if (class_score < min_score) continue;

            Box box;
            box.tl.x = (detection->x - detection->w / 2) * scale.x;
            box.tl.y = (detection->y - detection->h / 2) * scale.y;
            box.br.x = (detection->x + detection->w / 2) * scale.x;
            box.br.y = (detection->y + detection->h / 2) * scale.y;

            vector<Landmark> lms;
            if (landmarks_count()) {
                lms.reserve(landmarks_count());
                const float* landmark = detection->lm_class_confidence;
                for (int l = 0; l < landmarks_count(); l++, landmark += 2) {
                    Landmark lm;
                    lm.x = *landmark * in_dim.x;
                    lm.y = *landmark * in_dim.y;
                    lms.push_back(lm);
                }
            }
            dv.push_back({class_score, c, box, lms});
        }
    }
    return dv;
}

vector<Detection> DetectorYolov8::get_detections(float min_score, const Tensors& tensors, Dim2d in_dim)
{

    struct RawDetection {
        float x, y, w, h, lm_class_confidence[0];
    };
    Pt scale {_in_tensor_dim.x ? (float)in_dim.x / _in_tensor_dim.x : 1.0f,
              _in_tensor_dim.y ? (float)in_dim.y / _in_tensor_dim.y : 1.0f};
    Pt relative_scale {_out_bb_norm ? (float)in_dim.x : scale.x,
              _out_bb_norm ? (float)in_dim.y : scale.y};

    constexpr int classes_base_index = 4;
    const Tensor& t0 = tensors[0];
    const int visibility_base_index = 2;
    const int num_landmark_points = visibility_base_index + visibility();
    if ((t0.shape().size() != 3 || t0.shape()[0] != 1) /* TODO: && t0.shape().size() != 2 */) {
        LOGE << "Invalid tensor shape: " << t0.shape() << endl;
        return {};
    }

    auto num_classes = t0.shape().at(1) - classes_base_index - landmarks_count() * num_landmark_points;
    LOGV << "Detector classes: " << num_classes << endl;
    if (num_classes <= 0) {
        LOGE << "Invalid tensor shape: " << t0.shape();
        return {};
    }

    int raw_size = t0.shape().at(1);
    vector<float> detection_raw(raw_size);
    vector<Detection> dv;

    // Loop over all output tensors (in case the final concat layer is missing)
    for(const Tensor& tensor: tensors) {
        if (tensor.shape().size() != 3 || tensor.shape()[2] != t0.shape()[2]) {
            LOGE << "Invalid tensor shape: " << tensor.shape();
            return {};
        }

        // Create a detection for each box with max score above threshold
        const float* data_pr = tensor.as_float();
        auto num_boxes = tensor.shape().at(2);
        LOGV << "Detector boxes: " << num_boxes;
        for (int32_t i = 0; i < num_boxes; i++) {
            for (int32_t k = 0; k < raw_size; k++) {
                detection_raw[k] = data_pr[num_boxes*k + i];
            }
            const RawDetection* detection = reinterpret_cast<const RawDetection*>(detection_raw.data());
            int c = get_index_max(detection->lm_class_confidence, num_classes);
            if (c == -1) continue;

            float class_score = detection->lm_class_confidence[c];
            // Overall confidence is too low
            if (class_score < min_score) continue;
            Box box;
            box.tl.x = (detection->x - detection->w / 2) * relative_scale.x;
            box.tl.y = (detection->y - detection->h / 2) * relative_scale.y;
            box.br.x = (detection->x + detection->w / 2) * relative_scale.x;
            box.br.y = (detection->y + detection->h / 2) * relative_scale.y;

            vector<Landmark> lms;
            if (landmarks_count()) {
                lms.reserve(landmarks_count());
                const float* landmark = detection->lm_class_confidence + 1;
                for (int l = 0; l < landmarks_count(); l++, landmark += num_landmark_points) {
                    Landmark lm;
                    lm.x = (landmark[0]) * scale.x;
                    lm.y = (landmark[1]) * scale.y;
                    if(visibility()){
                        lm.visibility = landmark[visibility_base_index];
                    }
                    lms.push_back(lm);
                }
            }
            dv.push_back({class_score, c, box, lms});
        }
    }
    return dv;
}

bool DetectorYolov5Pyramid::init(const Tensors& tensors)
{
    if (!Detector::Impl::init(tensors)) {
        return false;
    }

    const Tensor& t0 = tensors[0];
    const auto& shape = t0.shape();
    if (shape.size() != 4 && shape.size() != 5) {
        LOGE << "Error, expected tensor rank: 4 or 5, got: " << shape.size();
        return false;
    }

    const string& format = t0.format();
    bool transposed = format_parse::get_bool(format, "transposed");
    _anchors = format_parse::get_ints2(format, "anchors");
    if (_anchors.empty()) {
        LOGE << "Error parsing anchor information";
        return false;
    }

#if YOLOV5_ANCHOR_DEBUG
    // Print anchors
    for (const auto& aa: _anchors) {
        for (const auto& a: aa) cout << a << " ";
        cout << endl;
    }
#endif

    // Anchors array must contain one entry for each pyramid element from P0
    // Each pyramid element must contain the same number of anchors
    const size_t anchors_count = _anchors[_anchors.size() - 1].size();
    if (anchors_count < 2 || anchors_count % 2) {
        LOGE << "Anchors must contain x,y pairs, got: " << anchors_count << " items";
        return {};
    }
    _anchors_count = anchors_count / 2;  // For each anchor we have x and y
    for (const auto& a: _anchors) {
        if (a.size() && a.size() != anchors_count) {
            LOGE << "Anchor count mismatch, expected " << anchors_count << ", got: " << a.size();
            return {};
        }
    }

    // Count number of empty anchor lists to determine the actual elements in the pyramid
    _pyramid_base = 0;
    for (const auto& a: _anchors) {
        if (a.size()) break;
        _pyramid_base++;
    }
    int pyramid_size = _anchors.size() - _pyramid_base;
    if (pyramid_size != tensors.size()) {
        LOGE << "Anchor lists specified: " << pyramid_size << ", expected: " << tensors.size();
        return {};
    }

    // Get info about the layout of data in the output tensors
    size_t detection_axis = shape.size() - 1;
    if (transposed) {
         _ol = OutLayout::adhw;
         _y_axis = shape.size() - 2;
         _x_axis = shape.size() - 1;
         detection_axis = 1 + (shape.size() == 5);
    }
    else if (shape.size() == 5 && shape[1] == _anchors_count) {
        _ol = OutLayout::ahwd;
        _y_axis = 2;
        _x_axis = 3;
    }
    else {
        _ol = OutLayout::hwad;
        _y_axis = 1;
        _x_axis = 2;
    }
    
    LOGI << "Detector anchors: " << _anchors_count;
    LOGI << "Detector pyramid: P" << _pyramid_base << "..P" << _pyramid_base + pyramid_size;
    LOGI << "Detector layout: " << (_ol == OutLayout::adhw ? "NADHW" : _ol == OutLayout::ahwd? "NAHWD" : "NHWAD");
    LOGV << "Detector D axis: " << detection_axis;

    int det_sz = shape[detection_axis];
    if (shape.size() == 4) {
        if (det_sz % _anchors_count) {
            LOGE << "shape[" << detection_axis << "]: " << shape[detection_axis] << " not multiple of anchors:" << _anchors_count ;
            return {};
        }
        det_sz /= _anchors_count;
    }

    det_sz -= sizeof(RawDetection) / sizeof(float) + landmarks_count() * 2;
    if (det_sz <= 0) {
        LOGE << "Invalid deduced class count: << " << det_sz;
        return {};
    }
    _class_count = det_sz;

    LOGV << "Detector X axis: " << _x_axis;
    LOGV << "Detector Y axis: " << _y_axis;
    LOGI << "Detector classes: " << _class_count;

    return true;
}


static inline float sigmoid(float x)
{
    // "fast" version: return 0.5 * (x / (1 + abs(x)) + 1)
    return 1 / (1 + exp(-x));
}

// This is the inverse of the sigmoid function
static inline float logit(float x)
{
    return log(x / (1 - x));
}


vector<Detection> DetectorYolov5Pyramid::get_detections(float min_score, const Tensors& tensors, Dim2d in_dim)
{
    size_t det_sz = sizeof(RawDetection) / sizeof(float) + landmarks_count() * 2 + _class_count;
    LOGV << "Detection size (floats): " << det_sz;
    const float logit_min_score = logit(min_score);

    // Compute size of P0 image to determine scaling factors for x and y
    const Shape& shape0 = tensors[0].shape();
    const float y_scale = static_cast<float>(in_dim.y) / (shape0[_y_axis] * (1 << _pyramid_base));
    const float x_scale = static_cast<float>(in_dim.x) / (shape0[_x_axis] * (1 << _pyramid_base));

    vector<float> detection(_ol == OutLayout::adhw? det_sz : 0);
    vector<Detection> dv;
    // Loop over all output tensors in the pyramid
    size_t pyramid_ix = _pyramid_base;
    for(const Tensor& tensor: tensors) {
        const Shape& shape = tensor.shape();
        int height = shape[_y_axis];
        int width = shape[_x_axis];
        int stripe = _ol == OutLayout::adhw? height * width : 1;
        const float* out_data = tensor.as_float();

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int a = 0; a < _anchors_count; a++) {
                    const float* dptr{};
                    switch(_ol) {
                    case OutLayout::hwad:
                        dptr = &out_data[((y * width + x) * _anchors_count + a) * det_sz];
                        break;
                    case OutLayout::ahwd:
                        dptr = &out_data[((a * height + y) * width + x) * det_sz];
                        break;
                    case OutLayout::adhw:
                        dptr = &out_data[(a * det_sz * height + y) * width + x];
                        break;
                    }
                    float confidence = dptr[stripe * offsetof(RawDetection, confidence) / sizeof(float)];
                    if (confidence < logit_min_score) {
                        // Overall confidence too low, just skip this detection.
                        continue;
                    }

                    auto d = reinterpret_cast<const RawDetection*>(dptr);
                    if (stripe > 1) {
                        // "De-stripe" the detection
                        for (int i = 0; i < det_sz; i++) {
                            detection[i] = dptr[stripe * i];
                        }
                        d = reinterpret_cast<const RawDetection*>(detection.data());
                    }

                    // Find the class with the highest score
                    int c = _class_count > 1 ? get_index_max(&d->lm_class_confidence[landmarks_count() * 2], _class_count) : 0;
                    float class_score = sigmoid(d->confidence) * sigmoid(d->lm_class_confidence[landmarks_count() * 2 + c]);
                    if (class_score < min_score) continue;

                    const int* anchor = &_anchors[pyramid_ix][a * 2];
                    float pyramid_scale = 1 << pyramid_ix;
                    float cx = (sigmoid(d->x) * 2 - 0.5 + x) * pyramid_scale;
                    float cy = (sigmoid(d->y) * 2 - 0.5 + y) * pyramid_scale;
                    float w = pow(sigmoid(d->w) * 2, 2) * anchor[0];
                    float h = pow(sigmoid(d->h) * 2, 2) * anchor[1];

                    Box box;
                    box.tl.x = (cx - 0.5 * w) * x_scale;
                    box.br.x = (cx + 0.5 * w) * x_scale;
                    box.tl.y = (cy - 0.5 * h) * y_scale;
                    box.br.y = (cy + 0.5 * h) * y_scale;

                    vector<Landmark> lms;
                    if (landmarks_count()) {
                        lms.reserve(landmarks_count());
                        const float* landmark = d->lm_class_confidence;
                        for (int l = 0; l < landmarks_count(); l++, landmark += 2) {
                            Landmark lm;
                            lm.x = (landmark[0] * anchor[0] + x * pyramid_scale) * x_scale;
                            lm.y = (landmark[1] * anchor[1]+ y * pyramid_scale) * y_scale;
                            lms.push_back(lm);
                        }
                    }
                    dv.push_back({class_score, c, box, lms});
                }
            }
        }
        ++pyramid_ix;
    }

    return dv;
}


//
// Detector
//


Detector::Detector(float score_threshold, int n_max, bool nms, float iou_threshold, bool iou_with_min)
  : _score_threshold{score_threshold},
    _max_detections{n_max},
    _nms{nms},
    _iou_threshold{iou_threshold},
    _iou_with_min{iou_with_min}
{
}


Detector::~Detector()
{
}


// Limit the given value to the range [lo, hi]
static inline void clamp(Dim2d& val, const Dim2d& lo, const Dim2d& hi)
{
    val.x = max(lo.x, min(val.x, hi.x));
    val.y = max(lo.y, min(val.y, hi.y));
}

bool Detector::init(const Tensors& tensors)
{
    const string od_type = format_parse::get_type(tensors[0].format());
    const size_t shape_rank = tensors[0].shape().size();
    LOGI << "Detector format: " << od_type;
    if (od_type == "retinanet_boxes") {
        d.reset(new DetectorRetinanet);
    }
    else if (od_type == "tflite_detection_input" || od_type == "tflite_detection_boxes") {
        d.reset(new DetectorTfliteODPostprocessIn);
    }
    else if (od_type == "tflite_detection") {
        d.reset(new DetectorTfliteODPostprocessOut);
    }
    else if (od_type == "yolov5") {
        if (shape_rank <= 3) {
            d.reset(new DetectorYolov5);
        }
        else {
            d.reset(new DetectorYolov5Pyramid);
        }
    }
    else if (od_type == "yolov8"){
        d.reset(new DetectorYolov8);
    }
    else {
        LOGE << "Unknown object detection format: " << od_type;
        return false;
    }

    return d->init(tensors) && d->validate();
}


Detector::Result Detector::process(const Tensors& tensors, const Rect& input_rect)
{
    if (!d) {
        // Self-init detector if not yet done
        init(tensors);
    }
    if (!d || !d->valid()) {
        LOGE << "Detector not correctly intialized";
        return {};
    }

    // Get detections and select them according to score and IoU
    Timer tmr;
    vector<Detection> dv = d->get_detections(_score_threshold, tensors, input_rect.size);
    vector<int32_t> selected = select(_max_detections, dv, _nms, _iou_threshold, _iou_with_min);

    // Create result with selected detections (ensure the bounding box is inside the image)
    Detector::Result res;
    const Dim2d zero{0, 0};
    for (auto idx : selected) {
        const Detection& d = dv[idx];
        Detector::Result::Item item;
        item.bounding_box.origin = {int(round(d.box.tl.x)), int(round(d.box.tl.y))};
        clamp(item.bounding_box.origin, zero, input_rect.size);
        Dim2d br{int(round(d.box.br.x)), int(round(d.box.br.y))};
        clamp(br, zero, input_rect.size);
        item.bounding_box.size = {br.x - item.bounding_box.origin.x, br.y - item.bounding_box.origin.y};
        item.bounding_box.origin = item.bounding_box.origin + input_rect.origin;
        item.confidence = d.score;
        item.class_index = d.class_index + this->d->index_base();
        if (!d.lm.empty()) {
            item.landmarks.reserve(d.lm.size());
            for (const auto& lm: d.lm) {
                Dim2d l{int(round(lm.x)), int(round(lm.y))};
                clamp(l, zero, input_rect.size);
                l = l + input_rect.origin;
                // TODO: clamp z
                item.landmarks.push_back(Landmark{l.x, l.y, lm.z, lm.visibility});
            }
        }
        res.items.push_back(item);
    }
    res.success = true;

    LOGV << "Post-processing time: " << tmr;
    LOGV << "Objects detected: " << res.items.size();
    return res;
}


}  // namespace synap
}  // namespace synaptics
