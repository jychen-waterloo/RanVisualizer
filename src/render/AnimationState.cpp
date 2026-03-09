#include "AnimationState.h"

#include <algorithm>
#include <cmath>

namespace rv::render {

void AnimationState::EnsureBandCount(const size_t bandCount) {
    if (displayedBands_.size() == bandCount) {
        return;
    }

    displayedBands_.assign(bandCount, 0.0f);
    displayedPeaks_.assign(bandCount, 0.0f);
}

void AnimationState::Update(const RenderSnapshot& snapshot, const float deltaSeconds) {
    EnsureBandCount(snapshot.smoothedBands.size());

    const float riseAlpha = 1.0f - std::exp(-deltaSeconds * 34.0f);
    const float fallAlpha = 1.0f - std::exp(-deltaSeconds * 18.0f);
    const float peakDecay = std::exp(-deltaSeconds * (snapshot.isSilentLike ? 10.0f : 6.0f));

    for (size_t i = 0; i < displayedBands_.size(); ++i) {
        const float target = std::clamp(snapshot.smoothedBands[i], 0.0f, 1.0f);
        const float alpha = target > displayedBands_[i] ? riseAlpha : fallAlpha;
        displayedBands_[i] += (target - displayedBands_[i]) * alpha;

        const float peakTarget = i < snapshot.peaks.size() ? snapshot.peaks[i] : target;
        const float clampedPeak = std::clamp(peakTarget, 0.0f, 1.0f);
        if (clampedPeak > displayedPeaks_[i]) {
            displayedPeaks_[i] = clampedPeak;
        } else {
            displayedPeaks_[i] *= peakDecay;
        }
    }

    const float accentTarget = std::clamp(snapshot.loudness * 0.65f + snapshot.bassEnergy * 0.35f, 0.0f, 1.0f);
    const float accentAlpha = 1.0f - std::exp(-deltaSeconds * 8.0f);
    accent_ += (accentTarget - accent_) * accentAlpha;
}

} // namespace rv::render
