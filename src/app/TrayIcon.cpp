#include "TrayIcon.h"

#include <shellapi.h>

namespace rv::app {

bool TrayIcon::Initialize(HWND hwnd) {
    hwnd_ = hwnd;

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd_;
    nid.uID = iconId_;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = kTrayMessage;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(nid.szTip, L"RanVisualizer", ARRAYSIZE(nid.szTip));

    installed_ = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
    return installed_;
}

void TrayIcon::Shutdown() {
    if (!installed_ || !hwnd_) {
        return;
    }

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd_;
    nid.uID = iconId_;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    installed_ = false;
}

void TrayIcon::ShowMenu(const bool overlayVisible, const bool clickThrough) {
    if (!installed_ || !hwnd_) {
        return;
    }

    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, MenuShowHide, overlayVisible ? L"Hide overlay" : L"Show overlay");
    AppendMenuW(menu, MF_STRING | (clickThrough ? MF_CHECKED : 0), MenuToggleClickThrough, L"Toggle click-through");
    AppendMenuW(menu, MF_STRING | (!clickThrough ? MF_CHECKED : 0), MenuEnableInteractive, L"Enable interaction");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MenuExit, L"Exit");

    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

bool TrayIcon::IsTrayCommand(const WPARAM command) const {
    switch (LOWORD(command)) {
    case MenuShowHide:
    case MenuToggleClickThrough:
    case MenuEnableInteractive:
    case MenuExit:
        return true;
    default:
        return false;
    }
}

AppCommand TrayIcon::CommandFromMenu(const WPARAM command) const {
    switch (LOWORD(command)) {
    case MenuShowHide:
        return AppCommand::ToggleOverlayVisibility;
    case MenuToggleClickThrough:
        return AppCommand::ToggleClickThrough;
    case MenuEnableInteractive:
        return AppCommand::EnableInteractive;
    case MenuExit:
    default:
        return AppCommand::Exit;
    }
}

} // namespace rv::app
