#pragma once

#include <Audioclient.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rv::audio {

enum class SampleFormat {
    Float32,
    Pcm16,
    Pcm24In32,
    Unsupported
};

struct FormatInfo {
    SampleFormat sampleFormat{SampleFormat::Unsupported};
    bool isFloat{false};
    bool isPcm{false};
    uint32_t sampleRate{0};
    uint16_t channels{0};
    uint16_t bitsPerSample{0};
    uint16_t blockAlign{0};
    uint32_t avgBytesPerSec{0};
    std::string tagOrSubtype;
};

FormatInfo ParseFormat(const WAVEFORMATEX& format);
std::string DescribeFormat(const WAVEFORMATEX& format);

void ComputeLevelsForBuffer(
    const BYTE* data,
    uint32_t frames,
    const FormatInfo& format,
    double& outSumSquares,
    float& outPeakAbs,
    uint64_t& outSampleCount);

void ConvertToNormalizedMono(
    const BYTE* data,
    uint32_t frames,
    const FormatInfo& format,
    std::vector<float>& outMonoSamples);

} // namespace rv::audio
