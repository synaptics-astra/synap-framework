///
/// Network metadata support
///

#pragma once

#include "synap/types.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <iostream>

namespace synaptics {
namespace synap {

struct QuantizationInfo {
    QuantizationScheme scheme{};
    int zero_point{};
    float scale_factor{};
    int fractional_length{};
};

struct TensorAttributes {
    std::string name{};
    DataType dtype{};
    Layout layout{};
    Security security{};
    Shape shape{};
    std::string format{};
    QuantizationInfo qi{};
    
    // The scale and mean values below are those to be used to rescale the input data when it
    // is assigned to the tensor. They should not be confused with scale and zero_point values
    // in the QuantizationInfo which represent how data is quantized when asymmetric affine if used.
    // In practice when the input and quantization data types are the same, the two pair of values
    // should [essentially] match (scale ~= 1 / qi.scale_factor, average(mean) ~= qi.mean)
    // so the double affine transformation on assigment can be avoided).
    float scale{};
    std::vector<float> mean{};
};


struct NetworkMetadata {
    bool valid{};
    bool secure{};
    std::string delegate;
    std::vector<TensorAttributes> inputs;
    std::vector<TensorAttributes> outputs;
};

std::ostream& operator<<(std::ostream& os, const TensorAttributes& attr);

NetworkMetadata load_metadata(const char* metadata);

}
}  // namespace synaptics
