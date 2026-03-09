#pragma once

#include "AnalysisFrame.h"
#include "BandMapper.h"
#include "Fft.h"
#include "Smoothing.h"
#include "Window.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rv::dsp {

class SpectrumAnalyzer {
public:
    struct Config {
        size_t fftSize{2048};
        size_t hopSize{1024};
        size_t bandCount{64};
        float minFrequencyHz{30.0f};
        float maxFrequencyHz{16000.0f};
        float noiseFloorDb{-72.0f};
        float compressionGamma{0.7f};
        float adaptiveGainTarget{0.35f};
        float adaptiveGainMin{0.6f};
        float adaptiveGainMax{2.5f};
        float adaptiveGainRiseSeconds{1.8f};
        float adaptiveGainFallSeconds{2.8f};
        Smoothing::Config smoothing{};
    };

    bool Configure(uint32_t sampleRate, const Config& config = Config{});
    void PushSamples(const float* monoSamples, size_t sampleCount);
    bool ConsumeLatestFrame(AnalysisFrame& outFrame);
    void Reset();

private:
    void AnalyzeFrame();
    void ConditionBands(float dtSeconds);
    void ComputeSummaryMetrics();

    uint32_t sampleRate_{0};
    Config config_{};

    Window window_;
    Fft fft_;
    BandMapper bandMapper_;
    Smoothing smoothing_;

    std::vector<float> rollingSamples_;
    std::vector<float> windowedFrame_;
    std::vector<float> fftMagnitudes_;
    std::vector<float> conditionedBands_;

    size_t consumedOffset_{0};
    float adaptiveGain_{1.0f};

    AnalysisFrame latestFrame_{};
    bool hasFrame_{false};
    std::chrono::steady_clock::time_point lastFrameTime_{};
};

} // namespace rv::dsp
