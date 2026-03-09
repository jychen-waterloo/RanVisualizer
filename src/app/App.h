#pragma once

#include "AppCommands.h"
#include "OverlayWindow.h"
#include "Settings.h"
#include "../audio/DeviceNotifier.h"

#include <memory>

namespace rv::app {

class App {
public:
    int Run(HINSTANCE instance, int showCmd);

private:
    bool InitializeAudio();
    void ShutdownAudio();
    void HandleCommand(AppCommand command);
    void SaveSettings();

    audio::LoopbackCapture capture_;
    OverlayWindow window_{capture_, false};
    std::unique_ptr<audio::DeviceNotifier, void (*)(audio::DeviceNotifier*)> notifier_{nullptr, [](audio::DeviceNotifier* p) {
        if (p) {
            p->Release();
        }
    }};
    Settings settings_{};
    SettingsData settingsData_{};
};

} // namespace rv::app
