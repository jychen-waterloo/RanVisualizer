#include "Smoothing.h"

#include <algorithm>
#include <cmath>

namespace rv::dsp {

void Smoothing::Configure(const size_t bandCount, const Config& config) {
    config_ = config;
    smoothed_.assign(bandCount, 0.0f);
    peaks_.assign(bandCount, 0.0f);
}

void Smoothing::Process(const std::vector<float>& input, float dtSeconds) {
    if (input.size() != smoothed_.size()) {
        return;
    }

    dtSeconds = std::max(dtSeconds, 0.001f);
    const float attackAlpha = std::exp(-dtSeconds / std::max(config_.attackSeconds, 0.001f));
    const float releaseAlpha = std::exp(-dtSeconds / std::max(config_.releaseSeconds, 0.001f));
    const float peakDecay = std::exp(-config_.peakDecayPerSecond * dtSeconds);

    for (size_t i = 0; i < input.size(); ++i) {
        const float target = std::clamp(input[i], 0.0f, 1.0f);
        const float prev = smoothed_[i];
        const float alpha = target > prev ? attackAlpha : releaseAlpha;
        const float smooth = alpha * prev + (1.0f - alpha) * target;
        smoothed_[i] = smooth;

        if (smooth >= peaks_[i]) {
            peaks_[i] = smooth;
        } else {
            peaks_[i] = std::max(smooth, peaks_[i] * peakDecay);
        }
    }
}

} // namespace rv::dsp
