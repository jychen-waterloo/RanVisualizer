#include "WaveFormatUtil.h"

#include <Windows.h>
#include <Audioclient.h>

#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace {

std::string GuidToString(const GUID& guid) {
    wchar_t wide[64]{};
    std::snwprintf(
        wide,
        std::size(wide),
        L"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
        guid.Data4[2],
        guid.Data4[3],
        guid.Data4[4],
        guid.Data4[5],
        guid.Data4[6],
        guid.Data4[7]);

    const int sourceLen = static_cast<int>(std::wcslen(wide));
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide, sourceLen, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) {
        return "<guid-conversion-failed>";
    }

    std::string out(static_cast<size_t>(bytes), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wide, sourceLen, out.data(), bytes, nullptr, nullptr) <= 0) {
        return "<guid-conversion-failed>";
    }
    return out;
}

} // namespace

namespace rv::audio {

FormatInfo ParseFormat(const WAVEFORMATEX& format) {
    FormatInfo info{};
    info.sampleRate = format.nSamplesPerSec;
    info.channels = format.nChannels;
    info.bitsPerSample = format.wBitsPerSample;
    info.blockAlign = format.nBlockAlign;
    info.avgBytesPerSec = format.nAvgBytesPerSec;

    if (format.wFormatTag == WAVE_FORMAT_PCM) {
        info.isPcm = true;
        info.tagOrSubtype = "WAVE_FORMAT_PCM";
        if (format.wBitsPerSample == 16) {
            info.sampleFormat = SampleFormat::Pcm16;
        }
    } else if (format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        info.isFloat = true;
        info.tagOrSubtype = "WAVE_FORMAT_IEEE_FLOAT";
        if (format.wBitsPerSample == 32) {
            info.sampleFormat = SampleFormat::Float32;
        }
    } else if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        info.tagOrSubtype = "WAVE_FORMAT_EXTENSIBLE";
        const auto& ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE&>(format);
        if (ext.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            info.isFloat = true;
            info.tagOrSubtype += " / IEEE_FLOAT";
            if (format.wBitsPerSample == 32) {
                info.sampleFormat = SampleFormat::Float32;
            }
        } else if (ext.SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            info.isPcm = true;
            info.tagOrSubtype += " / PCM";
            if (format.wBitsPerSample == 16) {
                info.sampleFormat = SampleFormat::Pcm16;
            } else if (format.wBitsPerSample == 32 && ext.Samples.wValidBitsPerSample == 24) {
                info.sampleFormat = SampleFormat::Pcm24In32;
            }
        } else {
            info.tagOrSubtype += " / " + GuidToString(ext.SubFormat);
        }
    } else {
        std::ostringstream oss;
        oss << "tag=" << format.wFormatTag;
        info.tagOrSubtype = oss.str();
    }

    return info;
}

std::string DescribeFormat(const WAVEFORMATEX& format) {
    const auto info = ParseFormat(format);
    std::ostringstream oss;
    oss << "sampleRate=" << info.sampleRate << "Hz, channels=" << info.channels
        << ", bitsPerSample=" << info.bitsPerSample << ", blockAlign=" << info.blockAlign
        << ", avgBytesPerSec=" << info.avgBytesPerSec << ", format=" << info.tagOrSubtype
        << ", type=";

    switch (info.sampleFormat) {
    case SampleFormat::Float32:
        oss << "float32";
        break;
    case SampleFormat::Pcm16:
        oss << "pcm16";
        break;
    case SampleFormat::Pcm24In32:
        oss << "pcm24-in-32";
        break;
    default:
        oss << "unsupported";
        break;
    }

    return oss.str();
}

void ComputeLevelsForBuffer(
    const BYTE* data,
    const uint32_t frames,
    const FormatInfo& format,
    double& outSumSquares,
    float& outPeakAbs,
    uint64_t& outSampleCount) {
    outSumSquares = 0.0;
    outPeakAbs = 0.0f;
    outSampleCount = 0;

    if (data == nullptr || frames == 0 || format.channels == 0) {
        return;
    }

    const uint64_t samples = static_cast<uint64_t>(frames) * format.channels;
    outSampleCount = samples;

    switch (format.sampleFormat) {
    case SampleFormat::Float32: {
        const auto* input = reinterpret_cast<const float*>(data);
        for (uint64_t i = 0; i < samples; ++i) {
            const float s = input[i];
            outSumSquares += static_cast<double>(s) * s;
            outPeakAbs = std::max(outPeakAbs, std::abs(s));
        }
        break;
    }
    case SampleFormat::Pcm16: {
        const auto* input = reinterpret_cast<const int16_t*>(data);
        constexpr float norm = 1.0f / 32768.0f;
        for (uint64_t i = 0; i < samples; ++i) {
            const float s = static_cast<float>(input[i]) * norm;
            outSumSquares += static_cast<double>(s) * s;
            outPeakAbs = std::max(outPeakAbs, std::abs(s));
        }
        break;
    }
    case SampleFormat::Pcm24In32: {
        const auto* input = reinterpret_cast<const int32_t*>(data);
        constexpr float norm = 1.0f / 8388608.0f;
        for (uint64_t i = 0; i < samples; ++i) {
            const int32_t v = input[i] >> 8;
            const float s = static_cast<float>(v) * norm;
            outSumSquares += static_cast<double>(s) * s;
            outPeakAbs = std::max(outPeakAbs, std::abs(s));
        }
        break;
    }
    default:
        outSampleCount = 0;
        break;
    }
}

} // namespace rv::audio
