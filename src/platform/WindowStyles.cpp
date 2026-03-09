#include "WindowStyles.h"

namespace rv::platform {

RECT ComputeBottomRightRect(const OverlayMetrics& metrics) {
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);

    RECT rect{};
    rect.right = work.right - metrics.margin;
    rect.left = rect.right - metrics.width;
    rect.bottom = work.bottom - metrics.margin;
    rect.top = rect.bottom - metrics.height;
    return rect;
}

void SetClickThrough(const HWND hwnd, const bool enabled) {
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (enabled) {
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

} // namespace rv::platform
