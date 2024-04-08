#pragma once
#include <cstdint>
#include <stddef.h>
#include "predictor.hpp"
namespace synaptics {
namespace synap {


/// PredictorEBG
class PredictorEBG : public Predictor{
public:
    PredictorEBG();
    ~PredictorEBG();

    static bool init();
    bool load_model(const void* model, size_t size, NetworkMetadata* meta) override;
    bool predict() override;

    BufferAttachment attach_buffer(Buffer* buffer, int32_t index, bool is_input) override;
    bool set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle) override;
    bool detach_buffer(BufferAttachment handle) override;

private:
    uint32_t _network{};
};

}  // namespace synap
}  // namespace synaptics
