#include "BandMapper.h"

#include <algorithm>
#include <cmath>

namespace rv::dsp {

void BandMapper::Configure(
    const uint32_t sampleRate,
    const size_t fftSize,
    const size_t bandCount,
    const float minFrequencyHz,
    const float maxFrequencyHz) {
    sampleRate_ = sampleRate;
    fftSize_ = fftSize;
    bands_.clear();

    if (sampleRate_ == 0 || fftSize_ == 0 || bandCount == 0) {
        return;
    }

    const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
    const float minFreq = std::max(10.0f, minFrequencyHz);
    const float maxFreq = std::clamp(maxFrequencyHz, minFreq + 1.0f, nyquist);
    const float ratio = std::pow(maxFreq / minFreq, 1.0f / static_cast<float>(bandCount));

    bands_.reserve(bandCount);

    const auto toBin = [this](const float freq) {
        const float binFloat = freq * static_cast<float>(fftSize_) / static_cast<float>(sampleRate_);
        return static_cast<size_t>(std::floor(binFloat));
    };

    float low = minFreq;
    size_t prevEnd = 1;
    const size_t maxBin = fftSize_ / 2;
    for (size_t i = 0; i < bandCount; ++i) {
        const float high = (i + 1 == bandCount) ? maxFreq : (low * ratio);
        size_t startBin = std::max<size_t>(1, toBin(low));
        size_t endBin = std::max(startBin + 1, toBin(high));

        startBin = std::max(startBin, prevEnd);
        endBin = std::min(maxBin, std::max(endBin, startBin + 1));
        prevEnd = endBin;

        BandRange range{};
        range.startBin = startBin;
        range.endBin = endBin;
        range.centerFreq = std::sqrt(low * high);
        bands_.push_back(range);
        low = high;
    }
}

void BandMapper::MapToBands(const std::vector<float>& fftMagnitudes, std::vector<float>& outBands) const {
    outBands.assign(bands_.size(), 0.0f);
    if (bands_.empty()) {
        return;
    }

    for (size_t bandIndex = 0; bandIndex < bands_.size(); ++bandIndex) {
        const BandRange& range = bands_[bandIndex];
        const size_t end = std::min(range.endBin, fftMagnitudes.size());
        if (range.startBin >= end) {
            continue;
        }

        double sumPower = 0.0;
        size_t count = 0;
        for (size_t bin = range.startBin; bin < end; ++bin) {
            const float mag = fftMagnitudes[bin];
            sumPower += static_cast<double>(mag) * static_cast<double>(mag);
            ++count;
        }

        if (count > 0) {
            const double meanPower = sumPower / static_cast<double>(count);
            outBands[bandIndex] = static_cast<float>(std::sqrt(meanPower));
        }
    }
}

} // namespace rv::dsp
