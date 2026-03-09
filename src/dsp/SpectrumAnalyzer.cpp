#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace rv::dsp {

bool SpectrumAnalyzer::Configure(const uint32_t sampleRate, const Config& config) {
    if (sampleRate == 0 || config.fftSize == 0 || config.hopSize == 0 || config.hopSize > config.fftSize) {
        return false;
    }

    sampleRate_ = sampleRate;
    config_ = config;

    if (!fft_.Configure(config_.fftSize)) {
        return false;
    }

    window_.ConfigureHann(config_.fftSize);
    bandMapper_.Configure(sampleRate_, config_.fftSize, config_.bandCount, config_.minFrequencyHz, config_.maxFrequencyHz);
    smoothing_.Configure(config_.bandCount, config_.smoothing);

    rollingSamples_.clear();
    rollingSamples_.reserve(config_.fftSize * 4);
    windowedFrame_.assign(config_.fftSize, 0.0f);
    conditionedBands_.assign(config_.bandCount, 0.0f);
    consumedOffset_ = 0;
    adaptiveGain_ = 1.0f;
    hasFrame_ = false;
    lastFrameTime_ = std::chrono::steady_clock::time_point{};
    return true;
}

void SpectrumAnalyzer::PushSamples(const float* monoSamples, const size_t sampleCount) {
    if (sampleRate_ == 0 || monoSamples == nullptr || sampleCount == 0) {
        return;
    }

    rollingSamples_.insert(rollingSamples_.end(), monoSamples, monoSamples + sampleCount);

    while (rollingSamples_.size() >= consumedOffset_ + config_.fftSize) {
        AnalyzeFrame();
        consumedOffset_ += config_.hopSize;
    }

    if (consumedOffset_ > config_.fftSize * 4 && consumedOffset_ < rollingSamples_.size()) {
        rollingSamples_.erase(rollingSamples_.begin(), rollingSamples_.begin() + static_cast<std::ptrdiff_t>(consumedOffset_));
        consumedOffset_ = 0;
    }
}

void SpectrumAnalyzer::AnalyzeFrame() {
    const auto& window = window_.Coefficients();
    for (size_t i = 0; i < config_.fftSize; ++i) {
        windowedFrame_[i] = rollingSamples_[consumedOffset_ + i] * window[i];
    }

    fft_.ForwardReal(windowedFrame_, fftMagnitudes_);
    bandMapper_.MapToBands(fftMagnitudes_, conditionedBands_);

    const auto now = std::chrono::steady_clock::now();
    float dtSeconds = static_cast<float>(config_.hopSize) / static_cast<float>(sampleRate_);
    if (lastFrameTime_.time_since_epoch().count() != 0) {
        dtSeconds = std::chrono::duration<float>(now - lastFrameTime_).count();
    }
    lastFrameTime_ = now;

    ConditionBands(dtSeconds);
    smoothing_.Process(conditionedBands_, dtSeconds);
    ComputeSummaryMetrics();

    latestFrame_.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    latestFrame_.sampleRate = sampleRate_;
    latestFrame_.fftSize = static_cast<uint32_t>(config_.fftSize);
    latestFrame_.hopSize = static_cast<uint32_t>(config_.hopSize);
    latestFrame_.rawBandValues = conditionedBands_;
    latestFrame_.smoothedBandValues = smoothing_.Smoothed();
    latestFrame_.peakBandValues = smoothing_.Peaks();
    hasFrame_ = true;
}

void SpectrumAnalyzer::ConditionBands(const float dtSeconds) {
    float mean = 0.0f;
    for (float& v : conditionedBands_) {
        const float db = 20.0f * std::log10(std::max(v, 1e-8f));
        float normalized = (db - config_.noiseFloorDb) / -config_.noiseFloorDb;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        normalized = std::pow(normalized, config_.compressionGamma);
        v = normalized;
        mean += v;
    }

    if (!conditionedBands_.empty()) {
        mean /= static_cast<float>(conditionedBands_.size());
    }

    float desiredGain = 1.0f;
    if (mean > 0.0005f) {
        desiredGain = std::clamp(
            config_.adaptiveGainTarget / mean,
            config_.adaptiveGainMin,
            config_.adaptiveGainMax);
    }

    const float riseAlpha = std::exp(-dtSeconds / std::max(config_.adaptiveGainRiseSeconds, 0.01f));
    const float fallAlpha = std::exp(-dtSeconds / std::max(config_.adaptiveGainFallSeconds, 0.01f));
    const float alpha = desiredGain > adaptiveGain_ ? riseAlpha : fallAlpha;
    adaptiveGain_ = alpha * adaptiveGain_ + (1.0f - alpha) * desiredGain;

    for (float& v : conditionedBands_) {
        v = std::clamp(v * adaptiveGain_, 0.0f, 1.0f);
    }
}

void SpectrumAnalyzer::ComputeSummaryMetrics() {
    const auto& bands = bandMapper_.Bands();
    const auto& smoothed = smoothing_.Smoothed();

    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float loudness = 0.0f;
    size_t bassCount = 0;
    size_t midCount = 0;
    size_t trebleCount = 0;

    for (size_t i = 0; i < smoothed.size() && i < bands.size(); ++i) {
        const float v = smoothed[i];
        const float f = bands[i].centerFreq;
        loudness += v;
        if (f < 250.0f) {
            bass += v;
            ++bassCount;
        } else if (f < 4000.0f) {
            mid += v;
            ++midCount;
        } else {
            treble += v;
            ++trebleCount;
        }
    }

    latestFrame_.bassEnergy = bassCount ? bass / static_cast<float>(bassCount) : 0.0f;
    latestFrame_.midEnergy = midCount ? mid / static_cast<float>(midCount) : 0.0f;
    latestFrame_.trebleEnergy = trebleCount ? treble / static_cast<float>(trebleCount) : 0.0f;
    latestFrame_.loudness = smoothed.empty() ? 0.0f : loudness / static_cast<float>(smoothed.size());
    latestFrame_.isSilentLike = latestFrame_.loudness < 0.03f;
}

bool SpectrumAnalyzer::ConsumeLatestFrame(AnalysisFrame& outFrame) {
    if (!hasFrame_) {
        return false;
    }

    outFrame = latestFrame_;
    return true;
}

void SpectrumAnalyzer::Reset() {
    rollingSamples_.clear();
    consumedOffset_ = 0;
    adaptiveGain_ = 1.0f;
    hasFrame_ = false;
    lastFrameTime_ = std::chrono::steady_clock::time_point{};
}

} // namespace rv::dsp
