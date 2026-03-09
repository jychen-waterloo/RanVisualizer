#pragma once

#include "MainWindow.h"
#include "../audio/DeviceNotifier.h"

#include <memory>

namespace rv::app {

class App {
public:
    int Run(HINSTANCE instance, int showCmd);

private:
    bool InitializeAudio();
    void ShutdownAudio();

    audio::LoopbackCapture capture_;
    MainWindow window_{capture_, true};
    std::unique_ptr<audio::DeviceNotifier, void (*)(audio::DeviceNotifier*)> notifier_{nullptr, [](audio::DeviceNotifier* p) {
        if (p) {
            p->Release();
        }
    }};
};

} // namespace rv::app
