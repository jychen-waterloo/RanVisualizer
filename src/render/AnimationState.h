#pragma once

#include "RenderTypes.h"

#include <vector>

namespace rv::render {

class AnimationState {
public:
    void EnsureBandCount(size_t bandCount);
    void Update(const RenderSnapshot& snapshot, float deltaSeconds);

    const std::vector<float>& DisplayedBands() const { return displayedBands_; }
    const std::vector<float>& DisplayedPeaks() const { return displayedPeaks_; }
    float Accent() const { return accent_; }

private:
    std::vector<float> displayedBands_;
    std::vector<float> displayedPeaks_;
    float accent_{0.0f};
};

} // namespace rv::render
