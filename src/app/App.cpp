#include "App.h"

#include "../util/ComUtil.h"

#include <Mmdeviceapi.h>
#include <shellscalingapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <thread>

namespace rv::app {

bool App::InitializeAudio() {
    if (!capture_.Start()) {
        return false;
    }

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    const HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) {
        return false;
    }

    notifier_.reset(new audio::DeviceNotifier([this](const std::wstring& endpointId) {
        capture_.RequestReinitialize(endpointId);
    }));

    if (FAILED(notifier_->Register(enumerator.Get()))) {
        notifier_.reset();
        return false;
    }

    return true;
}

void App::ShutdownAudio() {
    if (notifier_) {
        notifier_->Unregister();
        notifier_.reset();
    }
    capture_.Stop();
}

int App::Run(HINSTANCE instance, const int showCmd) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    util::ScopedCoInitialize com;
    if (FAILED(com.Result()) && com.Result() != RPC_E_CHANGED_MODE) {
        return 1;
    }

    if (!InitializeAudio()) {
        ShutdownAudio();
        return 1;
    }

    (void)showCmd;
    if (!window_.Create(instance)) {
        ShutdownAudio();
        return 1;
    }

    MSG msg{};
    auto prev = std::chrono::steady_clock::now();
    auto fpsWindowStart = prev;
    size_t fpsFrames = 0;
    float fps = 60.0f;

    while (true) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                ShutdownAudio();
                return static_cast<int>(msg.wParam);
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - prev).count();
        prev = now;
        dt = (std::clamp)(dt, 0.001f, 0.05f);

        ++fpsFrames;
        const float elapsed = std::chrono::duration<float>(now - fpsWindowStart).count();
        if (elapsed >= 0.5f) {
            fps = static_cast<float>(fpsFrames) / elapsed;
            fpsFrames = 0;
            fpsWindowStart = now;
        }

        window_.Tick(render::FrameTiming{now, dt, fps});
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace rv::app
