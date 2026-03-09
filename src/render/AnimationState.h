#pragma once

#include "RenderTypes.h"

#include <vector>

namespace rv::render {

struct MotionProfile {
    float attackHz{34.0f};
    float releaseHz{18.0f};
    float peakDecayHz{6.0f};
    float visualGain{1.0f};
    float lowEndBoost{0.0f};
    float accentHz{8.0f};
};

class AnimationState {
public:
    void EnsureBandCount(size_t bandCount);
    void Update(const RenderSnapshot& snapshot, float deltaSeconds, const MotionProfile& profile, size_t targetBarCount);

    const std::vector<float>& DisplayedBands() const { return displayedBands_; }
    const std::vector<float>& DisplayedPeaks() const { return displayedPeaks_; }
    float Accent() const { return accent_; }

private:
    std::vector<float> mappedBands_;
    std::vector<float> mappedPeaks_;
    std::vector<float> displayedBands_;
    std::vector<float> displayedPeaks_;
    float accent_{0.0f};
};

} // namespace rv::render
