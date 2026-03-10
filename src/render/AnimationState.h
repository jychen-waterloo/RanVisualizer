#pragma once

#include "RenderTypes.h"

#include <vector>

namespace rv::render {

struct MotionProfile {
    float attackHz{34.0f};
    float releaseHz{18.0f};
    float peakRiseHz{36.0f};
    float peakDecayHz{6.0f};
    float visualGain{1.0f};
    float lowEndBoost{0.0f};
    float accentHz{8.0f};
};

class AnimationState {
public:
    void EnsureBandCount(size_t bandCount);
    void Update(const RenderSnapshot& snapshot, float deltaSeconds, const MotionProfile& profile, size_t targetBarCount);

    const std::vector<float>& DisplayedBands() const { return smoothedBands_; }
    const std::vector<float>& DisplayedPeaks() const { return peakBands_; }
    float Accent() const { return accent_; }

private:
    std::vector<float> normalizedBands_;
    std::vector<float> normalizedPeaks_;
    std::vector<float> smoothedBands_;
    std::vector<float> peakBands_;
    float accent_{0.0f};
};

} // namespace rv::render
