#include "audio/DeviceNotifier.h"
#include "audio/LoopbackCapture.h"
#include "util/ComUtil.h"
#include "util/Logger.h"

#include <Mmdeviceapi.h>
#include <wrl/client.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

void SignalHandler(int) {
    g_running.store(false);
}

} // namespace

int main() {
    using rv::util::HResultToString;
    using rv::util::Logger;

    std::signal(SIGINT, SignalHandler);

    rv::util::ScopedCoInitialize com;
    if (FAILED(com.Result()) && com.Result() != RPC_E_CHANGED_MODE) {
        Logger::Instance().Error("Main COM initialization failed: " + HResultToString(com.Result()));
        return 1;
    }

    rv::audio::LoopbackCapture capture;
    if (!capture.Start()) {
        Logger::Instance().Error("Failed to start capture service.");
        return 1;
    }

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) {
        Logger::Instance().Error("CoCreateInstance(MMDeviceEnumerator) failed in main: " + HResultToString(hr));
        capture.Stop();
        return 1;
    }

    auto* notifier = new rv::audio::DeviceNotifier([&capture](const std::wstring& endpointId) {
        capture.RequestReinitialize(endpointId);
    });

    hr = notifier->Register(enumerator.Get());
    if (FAILED(hr)) {
        Logger::Instance().Error("RegisterEndpointNotificationCallback failed: " + HResultToString(hr));
        notifier->Release();
        capture.Stop();
        return 1;
    }

    Logger::Instance().Info("Device notification registered. Press Ctrl+C to exit.");

    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::Instance().Info("Shutdown requested.");

    notifier->Unregister();
    notifier->Release();

    capture.Stop();

    Logger::Instance().Info("Exit complete.");
    return 0;
}
