#pragma once

#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <wrl/client.h>

#include "../dsp/AnalysisFrame.h"
#include "../dsp/SpectrumAnalyzer.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rv::audio {

class LoopbackCapture {
public:
    LoopbackCapture();
    ~LoopbackCapture();

    bool Start();
    void Stop();

    void RequestReinitialize(const std::wstring& endpointIdHint);

private:
    struct RuntimeStats {
        uint64_t packets{0};
        uint64_t frames{0};
        uint64_t silentFrames{0};
        bool lastPacketSilent{false};
        double sumSquares{0.0};
        uint64_t levelSamples{0};
        float peakAbs{0.0f};
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point lastPrint;
    };

    bool InitializeForDefaultDevice();
    void TeardownCapture();
    bool ReadAvailablePackets(RuntimeStats& stats);
    void PrintPeriodicStatus(const RuntimeStats& stats) const;
    std::string GetDeviceFriendlyName(IMMDevice* device) const;
    void CaptureThreadMain();
    void AnalysisThreadMain();

    std::atomic<bool> running_{false};
    std::atomic<bool> reinitializeRequested_{false};
    std::thread captureThread_;
    std::thread analysisThread_;

    mutable std::mutex stateMutex_;
    std::wstring currentEndpointId_;

    std::mutex analysisMutex_;
    std::condition_variable analysisCv_;
    std::deque<float> analysisQueue_;
    uint32_t analysisSampleRate_{0};
    bool analyzerReconfigureRequested_{false};
    dsp::SpectrumAnalyzer analyzer_;
    std::vector<float> conversionScratch_;

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator_;
    Microsoft::WRL::ComPtr<IMMDevice> currentDevice_;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient_;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient_;
    WAVEFORMATEX* mixFormat_{nullptr};
    UINT32 bufferFrames_{0};
    REFERENCE_TIME defaultPeriod_{0};
    REFERENCE_TIME minPeriod_{0};
};

} // namespace rv::audio
