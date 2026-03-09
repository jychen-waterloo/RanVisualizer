#pragma once

#include <cstddef>
#include <vector>

namespace rv::dsp {

class Smoothing {
public:
    struct Config {
        float attackSeconds{0.020f};
        float releaseSeconds{0.14f};
        float peakDecayPerSecond{1.8f};
    };

    void Configure(size_t bandCount, const Config& config);
    void Process(const std::vector<float>& input, float dtSeconds);

    [[nodiscard]] const std::vector<float>& Smoothed() const { return smoothed_; }
    [[nodiscard]] const std::vector<float>& Peaks() const { return peaks_; }

private:
    Config config_{};
    std::vector<float> smoothed_;
    std::vector<float> peaks_;
};

} // namespace rv::dsp
