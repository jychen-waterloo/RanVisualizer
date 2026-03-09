#include "OverlayWindow.h"

#include "../platform/WindowStyles.h"

#include <utility>
#include <windowsx.h>

namespace rv::app {

OverlayWindow::OverlayWindow(audio::LoopbackCapture& capture, const bool showDebug)
    : capture_(capture), showDebug_(showDebug) {
}

bool OverlayWindow::Create(HINSTANCE instance, const SettingsData& settings) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWindow::WndProcStatic;
    wc.hInstance = instance;
    wc.lpszClassName = L"RanVisualizerOverlayWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    RECT rect = platform::ComputeBottomRightRect(platform::OverlayMetrics{settings.width, settings.height, 24});
    if (settings.hasPosition) {
        rect.left = settings.x;
        rect.top = settings.y;
        rect.right = rect.left + settings.width;
        rect.bottom = rect.top + settings.height;
    }

    hwnd_ = CreateWindowExW(platform::kOverlayExStyle, wc.lpszClassName, L"RanVisualizer Overlay",
        platform::kOverlayStyle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, instance, this);

    if (!hwnd_) {
        return false;
    }

    overlayVisible_ = settings.overlayVisible;
    ShowWindow(hwnd_, overlayVisible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
    UpdateWindow(hwnd_);

    SetClickThrough(settings.clickThrough && !settings.startInteractive);
    renderer_.SetInteractionState(!clickThrough_, false);

    if (!renderer_.Initialize(hwnd_)) {
        return false;
    }

    tray_.Initialize(hwnd_);
    hotkeys_.Register(hwnd_);
    return true;
}

void OverlayWindow::Tick(const render::FrameTiming& timing) {
    if (!hwnd_ || IsIconic(hwnd_) || !overlayVisible_) {
        return;
    }

    dsp::AnalysisFrame frame;
    if (capture_.GetLatestAnalysisFrame(frame)) {
        lastSnapshot_ = render::RenderSnapshot::FromAnalysisFrame(frame);
    }

    renderer_.SetInteractionState(!clickThrough_, pointerHovering_);
    renderer_.Render(lastSnapshot_, timing, showDebug_);
}

LRESULT CALLBACK OverlayWindow::WndProcStatic(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    OverlayWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<OverlayWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->WndProc(msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::WndProc(const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        renderer_.OnResize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_MOUSEMOVE:
        if (!clickThrough_) {
            pointerHovering_ = true;
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd_;
            TrackMouseEvent(&tme);
        }
        return 0;
    case WM_MOUSELEAVE:
        pointerHovering_ = false;
        return 0;
    case WM_NCHITTEST: {
        if (clickThrough_) {
            return HTTRANSPARENT;
        }

        POINT cursor{};
        cursor.x = GET_X_LPARAM(lParam);
        cursor.y = GET_Y_LPARAM(lParam);
        ScreenToClient(hwnd_, &cursor);
        if (renderer_.HitTestCloseButton(static_cast<float>(cursor.x), static_cast<float>(cursor.y))) {
            return HTCLIENT;
        }
        return HTCAPTION;
    }
    case WM_LBUTTONUP: {
        if (clickThrough_) {
            return 0;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (PointFromLParam(lParam, x, y) && renderer_.HitTestCloseButton(x, y)) {
            RequestCommand(AppCommand::Exit);
            return 0;
        }
        return 0;
    }
    case WM_HOTKEY:
        switch (static_cast<int>(wParam)) {
        case Hotkeys::IdToggleVisibility:
            RequestCommand(AppCommand::ToggleOverlayVisibility);
            return 0;
        case Hotkeys::IdToggleClickThrough:
            RequestCommand(AppCommand::ToggleClickThrough);
            return 0;
        case Hotkeys::IdExit:
            RequestCommand(AppCommand::Exit);
            return 0;
        default:
            break;
        }
        break;
    case TrayIcon::kTrayMessage:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            tray_.ShowMenu(overlayVisible_, clickThrough_);
            return 0;
        }
        if (lParam == WM_LBUTTONDBLCLK) {
            RequestCommand(AppCommand::ToggleOverlayVisibility);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (tray_.IsTrayCommand(wParam)) {
            RequestCommand(tray_.CommandFromMenu(wParam));
            return 0;
        }
        break;
    case WM_DESTROY:
        hotkeys_.Unregister(hwnd_);
        tray_.Shutdown();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void OverlayWindow::ToggleClickThrough() {
    SetClickThrough(!clickThrough_);
}

void OverlayWindow::SetClickThrough(const bool enabled) {
    clickThrough_ = enabled;
    platform::SetClickThrough(hwnd_, clickThrough_);
    if (clickThrough_) {
        pointerHovering_ = false;
    }
}

void OverlayWindow::EnsureInteractive() {
    SetClickThrough(false);
}

void OverlayWindow::ToggleOverlayVisibility() {
    overlayVisible_ = !overlayVisible_;
    ShowWindow(hwnd_, overlayVisible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
}

SettingsData OverlayWindow::CaptureSettings() const {
    SettingsData data{};
    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    data.x = rc.left;
    data.y = rc.top;
    data.width = rc.right - rc.left;
    data.height = rc.bottom - rc.top;
    data.hasPosition = true;
    data.clickThrough = clickThrough_;
    data.overlayVisible = overlayVisible_;
    data.startInteractive = !clickThrough_;
    return data;
}

void OverlayWindow::SetCommandCallback(CommandCallback callback) {
    onCommand_ = std::move(callback);
}

void OverlayWindow::RequestCommand(const AppCommand command) {
    if (onCommand_) {
        onCommand_(command);
    }
}

bool OverlayWindow::PointFromLParam(const LPARAM lParam, float& x, float& y) const {
    POINTS p = MAKEPOINTS(lParam);
    x = static_cast<float>(p.x);
    y = static_cast<float>(p.y);
    return true;
}

} // namespace rv::app
