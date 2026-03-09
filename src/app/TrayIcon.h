#pragma once

#include "AppCommands.h"

#include <windows.h>

namespace rv::app {

class TrayIcon {
public:
    static constexpr UINT kTrayMessage = WM_APP + 41;

    enum MenuId : UINT {
        MenuShowHide = 3001,
        MenuToggleClickThrough = 3002,
        MenuEnableInteractive = 3003,
        MenuExit = 3004,
    };

    bool Initialize(HWND hwnd);
    void Shutdown();
    void ShowMenu(bool overlayVisible, bool clickThrough);
    bool IsTrayCommand(WPARAM command) const;
    AppCommand CommandFromMenu(WPARAM command) const;

private:
    HWND hwnd_{nullptr};
    UINT iconId_{1};
    bool installed_{false};
};

} // namespace rv::app
