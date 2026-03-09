#pragma once

#include <cstdint>
#include <vector>

namespace rv::dsp {

struct AnalysisFrame {
    uint64_t timestampMs{0};
    uint32_t sampleRate{0};
    uint32_t fftSize{0};
    uint32_t hopSize{0};

    std::vector<float> rawBandValues;
    std::vector<float> smoothedBandValues;
    std::vector<float> peakBandValues;

    float bassEnergy{0.0f};
    float midEnergy{0.0f};
    float trebleEnergy{0.0f};
    float loudness{0.0f};
    bool isSilentLike{true};
};

} // namespace rv::dsp
