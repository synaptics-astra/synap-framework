#include "synap/metadata.hpp"

#include "synap/logging.hpp"
#include "synap/types.hpp"
#include "synap/enum_utils.hpp"
#include "json.hpp"

#include <algorithm>
#include <sstream>


using namespace std;

namespace synaptics {
namespace synap {

using json = nlohmann::ordered_json;

NLOHMANN_JSON_SERIALIZE_ENUM( QuantizationScheme, {
                                  {QuantizationScheme::none, "none"},
                                  {QuantizationScheme::affine_asymmetric, "asymmetric_affine"},
                                  {QuantizationScheme::dynamic_fixed_point, "dynamic_fixed_point"}
                              })

NLOHMANN_JSON_SERIALIZE_ENUM( DataType, {
                                  {DataType::invalid, "invalid"},
                                  {DataType::byte, "byte"},
                                  {DataType::int8, "int8"},
                                  {DataType::uint8, "uint8"},
                                  {DataType::int16, "int16"},
                                  {DataType::uint16, "uint16"},
                                  {DataType::int32, "int32"},
                                  {DataType::uint32, "uint32"},
                                  {DataType::float16, "float16"},
                                  {DataType::float32, "float32"},
                                  // Alternative tags used in "quantize" node
                                  {DataType::uint8, "u8"},
                                  {DataType::int8, "i8"},
                                  {DataType::uint16, "u16"},
                                  {DataType::int16, "i16"}
                              })

NLOHMANN_JSON_SERIALIZE_ENUM( Layout, {
                                  {Layout::none, "none"},
                                  {Layout::nhwc, "nhwc"},
                                  {Layout::nchw, "nchw"}
                              })

NLOHMANN_JSON_SERIALIZE_ENUM( Security, {
                                  {Security::none, "none"},
                                  {Security::any, "any"},
                                  {Security::secure, "secure"},
                                  {Security::non_secure, "non-secure"},
                                  {Security::secure_if_input_secure, "secure-if-input-secure"}
                              })



inline std::ostream& operator<<(std::ostream& os, const TensorAttributes& attr)
{
    os << attr.name << std::endl;
    os << "  dtype:      " << attr.dtype << std::endl;
    os << "  layout:     " << attr.layout << std::endl;
    os << "  shape:      " << attr.shape << std::endl;
    os << "  format:     " << attr.format << std::endl;
    os << "  mean:       ";
    for (const auto mean_item: attr.mean) os << mean_item << " ";
    os << std::endl;
    os << "  scale:      " << attr.scale << std::endl;
    os << "  qi.scheme:  " << attr.qi.scheme << std::endl;
    os << "  qi.zero_pt: " << attr.qi.zero_point << std::endl;
    os << "  qi.scale_f: " << attr.qi.scale_factor << std::endl;
    os << "  qi.fr_len:  " << attr.qi.fractional_length << std::endl;
    return os;
}


void to_json(json& j, const TensorAttributes& p)
{
    j = json{
    {"name", p.name},
    {"format", p.layout},
    {"dtype", p.dtype},
    {"security", p.security},
    {"data_format", p.format},
    {"shape", p.shape},
    {"mean", p.mean},
    {"scale", p.scale},
    {"quantizer", p.qi.scheme},
    {"quantize", {
        {"zero_point", p.qi.zero_point},
        {"scale", p.qi.scale_factor},
        {"fl", p.qi.fractional_length}
    }}
    };
}

void from_json(const json& j, TensorAttributes& p) {
    if (j.contains("name"))
        j.at("name").get_to(p.name);
    if (j.contains("dtype"))
        j.at("dtype").get_to(p.dtype);
    if (j.contains("format"))
        j.at("format").get_to(p.layout);
    if (j.contains("security"))
        j.at("security").get_to(p.security);
    if (j.contains("data_format"))
        j.at("data_format").get_to(p.format);
    if (j.contains("scale"))
        j.at("scale").get_to(p.scale);
    if (j.contains("mean"))
        j.at("mean").get_to(p.mean);
    if (j.contains("quantizer"))
        j.at("quantizer").get_to(p.qi.scheme);
    if (j.contains("quantize")) {
        const json & jq = j.at("quantize");
        if (p.dtype != DataType::float32 && jq.contains("qtype")) {
            // Ignore quantization datatype if the tensor is already dequantized
            // in acuity 6.3.1 these fields are both present.
            jq.at("qtype").get_to(p.dtype);
        }

        auto get_scalar = [](const json & js, const char * field, auto & scalar) {
            if (js.contains(field)) {
                const json & jss = js.at(field);
                if (jss.is_array())
                    jss[0].get_to(scalar);
                else
                    jss.get_to(scalar);
            }
        };
        get_scalar(jq, "zero_point", p.qi.zero_point);
        get_scalar(jq, "scale", p.qi.scale_factor);
        get_scalar(jq, "fl", p.qi.fractional_length);
    }

    const json & shape = j.at("shape");
    p.shape.clear();
    if (shape.is_number()) {
        p.shape.push_back(shape.get<int>());
    } else {
        shape.get_to(p.shape);
    }

    if (j.contains("type") && j.at("type") != "scalar") {
        LOGE << "Unexpected value for 'type' field: " << j.at("type");
    }

    if (p.layout == Layout::nhwc && p.shape.size() == 4 && p.shape[1] <= 4 && p.shape[3] > 4) {
        // Check layout matches shape.
        // This was happening when the transpose layer is removed, see:
        // https://support.verisilicon.com/projects/SVIPSW/issues/SVIPSW-59
        // but this case has been fixed in converter.py.
        LOGW << "Tensor " << p.name << " has layout nhwc bus shape seems to be nchw";
    }
}


void to_json(json& j, const NetworkMetadata& p)
{
    j = json{
    {"delegate", p.delegate},
    {"secure", p.secure},
    {"Inputs", p.inputs},
    {"Outputs", p.outputs}
};
}


void from_json(const json& j, NetworkMetadata& p)
{
    if (j.contains("secure"))
        j["secure"].get_to(p.secure);
    if (j.contains("delegate"))
        j["delegate"].get_to(p.delegate);
    else {
        // For backward compatibility use NPU EBG delegate if not specified
        p.delegate = "npu";
    }
    p.valid = j.contains("Inputs") && j.contains("Outputs");
    for (auto & element : j.at("Inputs"))
        p.inputs.push_back(element.get<TensorAttributes>());
    for (auto & element : j.at("Outputs"))
        p.outputs.push_back(element.get<TensorAttributes>());
    
    // Check that input and output shapes are valid
    for (auto& attr: p.inputs) {
        if (!attr.shape.valid()) {
            LOGE << "Invalid input shape: " << attr.shape;
            p.valid = false;
        }
    }
    for (auto& attr: p.outputs) {
        if (!attr.shape.valid()) {
            LOGE << "Invalid output shape: " << attr.shape;
            p.valid = false;
        }
    }
    
    // Explicitly set mean & scale if unspecified so normalization of 8-bits images work as expected
    // This allows to import models prequantized with affine without specifying scale and mean
    // since the scale and mean normally simplify out with scale_factor and zero_point.
    // So specifying a scale=1 and means=0 is not the same as not specifying them at all.
    for (auto& attr: p.inputs) {
        if (!attr.scale) {
            attr.scale = attr.qi.scale_factor ? 1 / attr.qi.scale_factor : 1;
        }
        if (attr.mean.empty()) {
            attr.mean = { static_cast<float>(attr.qi.zero_point) };
        }
    }
}


NetworkMetadata load_metadata(const char* metadata)
{
    NetworkMetadata meta;
    if (!metadata) {
        meta.valid = false;
        LOGE << "Metadata is null";
        return meta;
    }

    try {
        json metajson = json::parse(metadata);
        metajson.get_to(meta);
    }
    catch (json::parse_error& e)
    {
        meta.valid = false;
        LOGE << e.what();
        return meta;
    }

    // For debug
    //json j = meta;
    //cout << std::setw(4) << j << endl;

    return meta;
}

}  // namespace synap
}  // namespace synaptics
