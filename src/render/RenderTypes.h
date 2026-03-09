#pragma once

#include "../dsp/AnalysisFrame.h"

#include <chrono>
#include <vector>

namespace rv::render {

struct RenderSnapshot {
    std::vector<float> smoothedBands;
    std::vector<float> peaks;
    float bassEnergy{0.0f};
    float midEnergy{0.0f};
    float trebleEnergy{0.0f};
    float loudness{0.0f};
    bool isSilentLike{true};
    uint64_t timestampMs{0};

    static RenderSnapshot FromAnalysisFrame(const dsp::AnalysisFrame& frame) {
        RenderSnapshot snapshot;
        snapshot.smoothedBands = frame.smoothedBandValues;
        snapshot.peaks = frame.peakBandValues;
        snapshot.bassEnergy = frame.bassEnergy;
        snapshot.midEnergy = frame.midEnergy;
        snapshot.trebleEnergy = frame.trebleEnergy;
        snapshot.loudness = frame.loudness;
        snapshot.isSilentLike = frame.isSilentLike;
        snapshot.timestampMs = frame.timestampMs;
        return snapshot;
    }
};

struct FrameTiming {
    std::chrono::steady_clock::time_point now{};
    float deltaSeconds{0.016f};
    float fps{0.0f};
};

} // namespace rv::render
