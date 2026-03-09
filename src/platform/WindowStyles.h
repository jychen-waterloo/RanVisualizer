#pragma once

#include <windows.h>

namespace rv::platform {

struct OverlayMetrics {
    int width{460};
    int height{170};
    int margin{24};
};

constexpr DWORD kOverlayStyle = WS_POPUP;
constexpr DWORD kOverlayExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;

RECT ComputeBottomRightRect(const OverlayMetrics& metrics);
void SetClickThrough(HWND hwnd, bool enabled);

} // namespace rv::platform
