#include "AnimationState.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace rv::render {

namespace {

void RemapBands(const std::vector<float>& src, std::vector<float>& dst) {
    if (dst.empty()) {
        return;
    }
    if (src.empty()) {
        std::fill(dst.begin(), dst.end(), 0.0f);
        return;
    }

    const float srcCount = static_cast<float>(src.size());
    for (size_t i = 0; i < dst.size(); ++i) {
        const float t0 = (static_cast<float>(i) / static_cast<float>(dst.size())) * srcCount;
        const float t1 = (static_cast<float>(i + 1) / static_cast<float>(dst.size())) * srcCount;
        const int s0 = static_cast<int>(std::floor(t0));
        const int s1 = static_cast<int>(std::ceil(t1));

        float weighted = 0.0f;
        float total = 0.0f;
        for (int s = s0; s <= s1; ++s) {
            if (s < 0 || s >= static_cast<int>(src.size())) {
                continue;
            }
            const float left = std::max(t0, static_cast<float>(s));
            const float right = std::min(t1, static_cast<float>(s + 1));
            const float w = std::max(0.0f, right - left);
            weighted += src[static_cast<size_t>(s)] * w;
            total += w;
        }
        dst[i] = total > 0.0f ? (weighted / total) : src[std::min(src.size() - 1, static_cast<size_t>(std::max(s0, 0)))];
    }
}

} // namespace

void AnimationState::EnsureBandCount(const size_t bandCount) {
    if (displayedBands_.size() == bandCount) {
        return;
    }

    mappedBands_.assign(bandCount, 0.0f);
    mappedPeaks_.assign(bandCount, 0.0f);
    displayedBands_.assign(bandCount, 0.0f);
    displayedPeaks_.assign(bandCount, 0.0f);
}

void AnimationState::Update(
    const RenderSnapshot& snapshot,
    const float deltaSeconds,
    const MotionProfile& profile,
    const size_t targetBarCount) {
    // Amplitude contract:
    // - `snapshot.smoothedBands` is analyzer-normalized in [0, 1].
    // - render-side remapping must preserve that scale (no summation inflation).
    // - motion profile adjusts timing and only applies mild amplitude shaping.
    EnsureBandCount(targetBarCount);

    RemapBands(snapshot.smoothedBands, mappedBands_);
    RemapBands(snapshot.peaks, mappedPeaks_);

    const float riseAlpha = 1.0f - std::exp(-deltaSeconds * profile.attackHz);
    const float fallAlpha = 1.0f - std::exp(-deltaSeconds * profile.releaseHz);
    const float peakDecay = std::exp(-deltaSeconds * (snapshot.isSilentLike ? profile.peakDecayHz * 1.6f : profile.peakDecayHz));

    for (size_t i = 0; i < displayedBands_.size(); ++i) {
        const float x = static_cast<float>(i) / std::max(1.0f, static_cast<float>(displayedBands_.size() - 1));
        const float lowLift = 1.0f + profile.lowEndBoost * (1.0f - x) * (1.0f - x);
        // Compensate low-end emphasis so it changes shape more than absolute loudness.
        const float lowLiftComp = 1.0f / (1.0f + profile.lowEndBoost * 0.38f);
        const float shaped = mappedBands_[i] * lowLift * lowLiftComp;
        const float target = std::clamp(shaped * profile.visualGain, 0.0f, 1.0f);

        const float alpha = target > displayedBands_[i] ? riseAlpha : fallAlpha;
        displayedBands_[i] += (target - displayedBands_[i]) * alpha;

        const float peakShaped = (i < mappedPeaks_.size() ? mappedPeaks_[i] : target) * lowLift * lowLiftComp;
        const float peakTarget = std::clamp(peakShaped, 0.0f, 1.0f);
        if (peakTarget > displayedPeaks_[i]) {
            displayedPeaks_[i] = peakTarget;
        } else {
            displayedPeaks_[i] *= peakDecay;
        }

#if defined(_DEBUG)
        assert(target >= 0.0f && target <= 1.0f);
        assert(displayedBands_[i] >= -0.001f && displayedBands_[i] <= 1.001f);
#endif
    }

    const float accentTarget = std::clamp(snapshot.loudness * 0.65f + snapshot.bassEnergy * 0.35f, 0.0f, 1.0f);
    const float accentAlpha = 1.0f - std::exp(-deltaSeconds * profile.accentHz);
    accent_ += (accentTarget - accent_) * accentAlpha;
}

} // namespace rv::render
