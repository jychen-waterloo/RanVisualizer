#include "LoopbackCapture.h"

#include "WaveFormatUtil.h"
#include "../util/ComUtil.h"
#include "../util/Logger.h"

#include <Functiondiscoverykeys_devpkey.h>
#include <avrt.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {

constexpr auto kStatusPrintPeriod = std::chrono::milliseconds(500);
constexpr auto kCaptureSleepPeriod = std::chrono::milliseconds(8);
constexpr auto kAnalysisPrintPeriod = std::chrono::milliseconds(300);

std::string MiniBars(const std::vector<float>& bands) {
    static constexpr const char* kLevels = " .:-=+*#%@";
    if (bands.empty()) {
        return "<no-bands>";
    }

    std::string out;
    const size_t sampleCount = std::min<size_t>(16, bands.size());
    out.reserve(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        const size_t idx = (i * bands.size()) / sampleCount;
        const float v = std::clamp(bands[idx], 0.0f, 1.0f);
        const size_t level = static_cast<size_t>(v * 9.0f);
        out.push_back(kLevels[level]);
    }
    return out;
}

} // namespace

namespace rv::audio {

LoopbackCapture::LoopbackCapture() = default;

LoopbackCapture::~LoopbackCapture() {
    Stop();
}

bool LoopbackCapture::Start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return true;
    }

    captureThread_ = std::thread(&LoopbackCapture::CaptureThreadMain, this);
    analysisThread_ = std::thread(&LoopbackCapture::AnalysisThreadMain, this);
    return true;
}

void LoopbackCapture::Stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    analysisCv_.notify_all();

    if (captureThread_.joinable()) {
        captureThread_.join();
    }
    if (analysisThread_.joinable()) {
        analysisThread_.join();
    }
}

void LoopbackCapture::RequestReinitialize(const std::wstring& endpointIdHint) {
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        currentEndpointId_ = endpointIdHint;
    }
    reinitializeRequested_.store(true);
}

bool LoopbackCapture::InitializeForDefaultDevice() {
    using util::HResultToString;
    using util::Logger;
    using util::WStringToUtf8;

    TeardownCapture();

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnumerator_));
    if (FAILED(hr)) {
        Logger::Instance().Error("CoCreateInstance(MMDeviceEnumerator) failed: " + HResultToString(hr));
        return false;
    }

    hr = deviceEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &currentDevice_);
    if (FAILED(hr)) {
        Logger::Instance().Error("GetDefaultAudioEndpoint(eRender,eConsole) failed: " + HResultToString(hr));
        return false;
    }

    LPWSTR endpointId = nullptr;
    hr = currentDevice_->GetId(&endpointId);
    if (FAILED(hr)) {
        Logger::Instance().Error("IMMDevice::GetId failed: " + HResultToString(hr));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        currentEndpointId_ = endpointId;
    }

    const std::string endpointIdUtf8 = WStringToUtf8(endpointId);
    CoTaskMemFree(endpointId);

    const std::string friendlyName = GetDeviceFriendlyName(currentDevice_.Get());

    hr = currentDevice_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(audioClient_.GetAddressOf()));
    if (FAILED(hr)) {
        Logger::Instance().Error("IMMDevice::Activate(IAudioClient) failed: " + HResultToString(hr));
        return false;
    }

    hr = audioClient_->GetMixFormat(&mixFormat_);
    if (FAILED(hr)) {
        Logger::Instance().Error("IAudioClient::GetMixFormat failed: " + HResultToString(hr));
        return false;
    }

    hr = audioClient_->GetDevicePeriod(&defaultPeriod_, &minPeriod_);
    if (FAILED(hr)) {
        Logger::Instance().Warn("IAudioClient::GetDevicePeriod failed: " + HResultToString(hr));
    }

    hr = audioClient_->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0,
        0,
        mixFormat_,
        nullptr);
    if (FAILED(hr)) {
        Logger::Instance().Error("IAudioClient::Initialize(loopback shared) failed: " + HResultToString(hr));
        return false;
    }

    hr = audioClient_->GetBufferSize(&bufferFrames_);
    if (FAILED(hr)) {
        Logger::Instance().Error("IAudioClient::GetBufferSize failed: " + HResultToString(hr));
        return false;
    }

    hr = audioClient_->GetService(IID_PPV_ARGS(&captureClient_));
    if (FAILED(hr)) {
        Logger::Instance().Error("IAudioClient::GetService(IAudioCaptureClient) failed: " + HResultToString(hr));
        return false;
    }

    const auto formatInfo = DescribeFormat(*mixFormat_);
    std::ostringstream oss;
    oss << "Opened default playback device: \"" << friendlyName << "\", endpointId=" << endpointIdUtf8
        << ", format={" << formatInfo << "}, bufferFrames=" << bufferFrames_
        << ", defaultPeriod100ns=" << defaultPeriod_ << ", minPeriod100ns=" << minPeriod_;
    Logger::Instance().Info(oss.str());

    hr = audioClient_->Start();
    if (FAILED(hr)) {
        Logger::Instance().Error("IAudioClient::Start failed: " + HResultToString(hr));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(analysisMutex_);
        analysisSampleRate_ = mixFormat_->nSamplesPerSec;
        analysisQueue_.clear();
        analyzerReconfigureRequested_ = true;
    }
    analysisCv_.notify_one();

    Logger::Instance().Info("Loopback capture stream started.");
    return true;
}

void LoopbackCapture::TeardownCapture() {
    if (audioClient_) {
        audioClient_->Stop();
    }

    captureClient_.Reset();
    audioClient_.Reset();
    currentDevice_.Reset();
    deviceEnumerator_.Reset();

    if (mixFormat_ != nullptr) {
        CoTaskMemFree(mixFormat_);
        mixFormat_ = nullptr;
    }

    bufferFrames_ = 0;
    defaultPeriod_ = 0;
    minPeriod_ = 0;
}

bool LoopbackCapture::ReadAvailablePackets(RuntimeStats& stats) {
    if (!captureClient_ || !mixFormat_) {
        return false;
    }

    const auto formatInfo = ParseFormat(*mixFormat_);

    UINT32 nextPacketFrames = 0;
    HRESULT hr = captureClient_->GetNextPacketSize(&nextPacketFrames);
    if (FAILED(hr)) {
        util::Logger::Instance().Error("IAudioCaptureClient::GetNextPacketSize failed: " + util::HResultToString(hr));
        return false;
    }

    while (nextPacketFrames > 0) {
        BYTE* data = nullptr;
        UINT32 frames = 0;
        DWORD flags = 0;

        hr = captureClient_->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) {
            util::Logger::Instance().Error("IAudioCaptureClient::GetBuffer failed: " + util::HResultToString(hr));
            return false;
        }

        const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;

        stats.packets++;
        stats.frames += frames;
        stats.lastPacketSilent = silent;
        if (silent) {
            stats.silentFrames += frames;
        }

        if (!silent) {
            double sumSquares = 0.0;
            float peak = 0.0f;
            uint64_t sampleCount = 0;
            ComputeLevelsForBuffer(data, frames, formatInfo, sumSquares, peak, sampleCount);
            stats.sumSquares += sumSquares;
            stats.peakAbs = std::max(stats.peakAbs, peak);
            stats.levelSamples += sampleCount;
        }

        if (formatInfo.sampleFormat != SampleFormat::Unsupported) {
            ConvertToNormalizedMono(data, frames, formatInfo, conversionScratch_);
            if (silent) {
                std::fill(conversionScratch_.begin(), conversionScratch_.end(), 0.0f);
            }
            {
                std::lock_guard<std::mutex> lock(analysisMutex_);
                for (float s : conversionScratch_) {
                    analysisQueue_.push_back(s);
                }
                if (analysisQueue_.size() > formatInfo.sampleRate * 2) {
                    analysisQueue_.erase(analysisQueue_.begin(), analysisQueue_.begin() + static_cast<std::ptrdiff_t>(formatInfo.sampleRate));
                }
            }
            analysisCv_.notify_one();
        }

        hr = captureClient_->ReleaseBuffer(frames);
        if (FAILED(hr)) {
            util::Logger::Instance().Error("IAudioCaptureClient::ReleaseBuffer failed: " + util::HResultToString(hr));
            return false;
        }

        hr = captureClient_->GetNextPacketSize(&nextPacketFrames);
        if (FAILED(hr)) {
            util::Logger::Instance().Error("IAudioCaptureClient::GetNextPacketSize failed: " + util::HResultToString(hr));
            return false;
        }
    }

    return true;
}

void LoopbackCapture::PrintPeriodicStatus(const RuntimeStats& stats) const {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double>(now - stats.startTime).count();

    float rms = 0.0f;
    if (stats.levelSamples > 0) {
        rms = static_cast<float>(std::sqrt(stats.sumSquares / static_cast<double>(stats.levelSamples)));
    }

    const double packetsPerSec = elapsed > 0.0 ? static_cast<double>(stats.packets) / elapsed : 0.0;
    const double framesPerSec = elapsed > 0.0 ? static_cast<double>(stats.frames) / elapsed : 0.0;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "[capture t=" << elapsed << "s] packets=" << stats.packets << " frames=" << stats.frames
        << " rms=";
    oss << std::setprecision(4) << rms << " peak=" << stats.peakAbs
        << " silent=" << (stats.lastPacketSilent ? "yes" : "no")
        << " pps=" << std::setprecision(1) << packetsPerSec
        << " fps_audio=" << framesPerSec;

    util::Logger::Instance().Info(oss.str());
}

std::string LoopbackCapture::GetDeviceFriendlyName(IMMDevice* device) const {
    if (device == nullptr) {
        return "<null-device>";
    }

    Microsoft::WRL::ComPtr<IPropertyStore> store;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &store);
    if (FAILED(hr)) {
        return "<friendly-name-unavailable>";
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    hr = store->GetValue(PKEY_Device_FriendlyName, &value);
    if (FAILED(hr)) {
        PropVariantClear(&value);
        return "<friendly-name-unavailable>";
    }

    std::string out = util::WStringToUtf8(value.pwszVal);
    PropVariantClear(&value);
    return out;
}

void LoopbackCapture::AnalysisThreadMain() {
    std::vector<float> localSamples;
    localSamples.reserve(8192);

    auto lastPrint = std::chrono::steady_clock::now();

    while (running_.load()) {
        {
            std::unique_lock<std::mutex> lock(analysisMutex_);
            analysisCv_.wait_for(lock, std::chrono::milliseconds(50), [&] {
                return !running_.load() || !analysisQueue_.empty() || analyzerReconfigureRequested_;
            });

            if (!running_.load()) {
                break;
            }

            if (analyzerReconfigureRequested_ && analysisSampleRate_ > 0) {
                dsp::SpectrumAnalyzer::Config config;
                analyzer_.Configure(analysisSampleRate_, config);
                analyzer_.Reset();
                analyzerReconfigureRequested_ = false;
            }

            const size_t take = std::min<size_t>(analysisQueue_.size(), 4096);
            localSamples.clear();
            localSamples.reserve(take);
            for (size_t i = 0; i < take; ++i) {
                localSamples.push_back(analysisQueue_.front());
                analysisQueue_.pop_front();
            }
        }

        if (!localSamples.empty()) {
            analyzer_.PushSamples(localSamples.data(), localSamples.size());
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - lastPrint >= kAnalysisPrintPeriod) {
            dsp::AnalysisFrame frame;
            if (analyzer_.ConsumeLatestFrame(frame)) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(3);
                oss << "[analysis] loud=" << frame.loudness
                    << " bass=" << frame.bassEnergy
                    << " mid=" << frame.midEnergy
                    << " treb=" << frame.trebleEnergy
                    << " silent=" << (frame.isSilentLike ? "yes" : "no")
                    << " bars=" << MiniBars(frame.smoothedBandValues);
                util::Logger::Instance().Info(oss.str());
            }
            lastPrint = now;
        }
    }
}

void LoopbackCapture::CaptureThreadMain() {
    util::ScopedCoInitialize com;
    if (FAILED(com.Result()) && com.Result() != RPC_E_CHANGED_MODE) {
        util::Logger::Instance().Error("Capture thread COM initialization failed: " + util::HResultToString(com.Result()));
        return;
    }

    RuntimeStats stats{};
    stats.startTime = std::chrono::steady_clock::now();
    stats.lastPrint = stats.startTime;

    while (running_.load()) {
        if (!audioClient_ || reinitializeRequested_.load()) {
            reinitializeRequested_.store(false);
            if (!InitializeForDefaultDevice()) {
                util::Logger::Instance().Warn("Capture initialization failed; retrying in 1 second.");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            stats = RuntimeStats{};
            stats.startTime = std::chrono::steady_clock::now();
            stats.lastPrint = stats.startTime;
        }

        if (!ReadAvailablePackets(stats)) {
            util::Logger::Instance().Warn("Read loop failed; forcing reinitialization.");
            reinitializeRequested_.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - stats.lastPrint >= kStatusPrintPeriod) {
            PrintPeriodicStatus(stats);
            stats.lastPrint = now;
        }

        std::this_thread::sleep_for(kCaptureSleepPeriod);
    }

    TeardownCapture();
    analysisCv_.notify_all();
    util::Logger::Instance().Info("Loopback capture stopped.");
}

} // namespace rv::audio
