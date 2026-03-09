#include "Hotkeys.h"

namespace rv::app {

bool Hotkeys::Register(HWND hwnd) {
    const bool ok1 = RegisterHotKey(hwnd, IdToggleVisibility, MOD_CONTROL | MOD_ALT, 'V') != FALSE;
    const bool ok2 = RegisterHotKey(hwnd, IdToggleClickThrough, MOD_CONTROL | MOD_ALT, 'I') != FALSE;
    const bool ok3 = RegisterHotKey(hwnd, IdExit, MOD_CONTROL | MOD_ALT, 'Q') != FALSE;
    return ok1 && ok2 && ok3;
}

void Hotkeys::Unregister(HWND hwnd) {
    UnregisterHotKey(hwnd, IdToggleVisibility);
    UnregisterHotKey(hwnd, IdToggleClickThrough);
    UnregisterHotKey(hwnd, IdExit);
}

} // namespace rv::app
