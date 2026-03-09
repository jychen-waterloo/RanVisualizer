#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace rv::dsp {

class BandMapper {
public:
    struct BandRange {
        size_t startBin{0};
        size_t endBin{0};
        float centerFreq{0.0f};
    };

    void Configure(uint32_t sampleRate, size_t fftSize, size_t bandCount, float minFrequencyHz, float maxFrequencyHz);
    void MapToBands(const std::vector<float>& fftMagnitudes, std::vector<float>& outBands) const;

    [[nodiscard]] const std::vector<BandRange>& Bands() const { return bands_; }

private:
    uint32_t sampleRate_{0};
    size_t fftSize_{0};
    std::vector<BandRange> bands_;
};

} // namespace rv::dsp
