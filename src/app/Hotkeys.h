#pragma once

#include <windows.h>

namespace rv::app {

class Hotkeys {
public:
    enum : int {
        IdToggleVisibility = 1,
        IdToggleClickThrough = 2,
        IdExit = 3,
    };

    bool Register(HWND hwnd);
    void Unregister(HWND hwnd);
};

} // namespace rv::app
