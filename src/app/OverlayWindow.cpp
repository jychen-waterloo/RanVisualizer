#include "OverlayWindow.h"

#include "../platform/WindowStyles.h"

#include <algorithm>
#include <utility>
#include <windowsx.h>

namespace rv::app {

namespace {

int ClampOverlayWidth(const int width) {
    return (std::clamp)(width, 280, 2400);
}

int ClampOverlayHeight(const int height) {
    return (std::clamp)(height, 120, 900);
}

} // namespace

OverlayWindow::OverlayWindow(audio::LoopbackCapture& capture, const bool showDebug)
    : capture_(capture), showDebug_(showDebug) {
}

bool OverlayWindow::Create(HINSTANCE instance, const SettingsData& settings) {
    settings_ = settings;
    settings_.width = (std::clamp)(settings_.width, kMinOverlayWidth, 2400);
    settings_.height = (std::clamp)(settings_.height, kMinOverlayHeight, kMaxOverlayHeight);
    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWindow::WndProcStatic;
    wc.hInstance = instance;
    wc.lpszClassName = L"RanVisualizerOverlayWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    RECT rect = platform::ComputeBottomRightRect(platform::OverlayMetrics{settings_.width, settings_.height, 24});
    if (settings_.hasPosition) {
        rect.left = settings_.x;
        rect.top = settings_.y;
        rect.right = rect.left + settings_.width;
        rect.bottom = rect.top + settings_.height;
    }

    hwnd_ = CreateWindowExW(platform::kOverlayExStyle, wc.lpszClassName, L"RanVisualizer Overlay",
        platform::kOverlayStyle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, instance, this);

    if (!hwnd_) {
        return false;
    }

    overlayVisible_ = settings_.overlayVisible;
    ShowWindow(hwnd_, overlayVisible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
    UpdateWindow(hwnd_);

    SetClickThrough(settings_.clickThrough && !settings_.startInteractive);
    renderer_.SetInteractionState(!clickThrough_, false);

    if (!renderer_.Initialize(hwnd_)) {
        return false;
    }

    ApplyRenderConfig();

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
        if (wParam != SIZE_MINIMIZED) {
            UpdateTrackedWindowSize();
        }
        return 0;
    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = kMinOverlayWidth;
        mmi->ptMinTrackSize.y = kMinOverlayHeight;
        mmi->ptMaxTrackSize.y = kMaxOverlayHeight;
        return 0;
    }
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
    case WM_NCHITTEST:
        return HandleHitTest(lParam);
    case WM_ENTERSIZEMOVE:
        return 0;
    case WM_EXITSIZEMOVE:
        UpdateTrackedWindowSize();
        NotifySettingsChanged();
        return 0;
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
            tray_.ShowMenu(settings_, overlayVisible_, clickThrough_);
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
        if (tray_.TryApplySettingsCommand(wParam, settings_)) {
            ApplyRenderConfig();
            if (onSettingsChanged_) {
                onSettingsChanged_();
            }
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
    if (clickThrough_ == enabled) {
        return;
    }

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
    data.width = settings_.width;
    data.height = settings_.height;
    data.hasPosition = true;
    data.clickThrough = clickThrough_;
    data.overlayVisible = overlayVisible_;
    data.startInteractive = !clickThrough_;
    data.barCount = settings_.barCount;
    data.backgroundOpacity = settings_.backgroundOpacity;
    data.barOpacity = settings_.barOpacity;
    data.motionIntensity = settings_.motionIntensity;
    data.themePreset = settings_.themePreset;
    data.barColor = settings_.barColor;
    data.backgroundColor = settings_.backgroundColor;
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


void OverlayWindow::SetSettingsChangedCallback(std::function<void()> callback) {
    onSettingsChanged_ = std::move(callback);
}

void OverlayWindow::ApplyRenderConfig() {
    settings_.barCount = (std::clamp)(settings_.barCount, static_cast<size_t>(24), static_cast<size_t>(96));
    settings_.motionIntensity = (std::clamp)(settings_.motionIntensity, 0.0f, 1.0f);
    settings_.backgroundOpacity = (std::clamp)(settings_.backgroundOpacity, 0.05f, 1.0f);
    settings_.width = ClampOverlayWidth(settings_.width);
    settings_.height = ClampOverlayHeight(settings_.height);

    render::RenderConfig cfg{};
    cfg.barCount = settings_.barCount;
    cfg.motionIntensity = settings_.motionIntensity;
    cfg.theme.preset = settings_.themePreset;
    cfg.theme.backgroundOpacity = settings_.backgroundOpacity;
    cfg.theme.barOpacity = settings_.barOpacity;
    cfg.theme.customBarColor = settings_.barColor;
    cfg.theme.customBackgroundColor = settings_.backgroundColor;
    renderer_.SetConfig(cfg);

    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    SetWindowPos(hwnd_, nullptr, rc.left, rc.top, settings_.width, settings_.height, SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT OverlayWindow::HandleHitTest(const LPARAM lParam) const {
    if (clickThrough_) {
        return HTTRANSPARENT;
    }

    RECT windowRect{};
    GetWindowRect(hwnd_, &windowRect);

    const int x = GET_X_LPARAM(lParam);
    const int y = GET_Y_LPARAM(lParam);

    POINT cursor{x - windowRect.left, y - windowRect.top};
    if (renderer_.HitTestCloseButton(static_cast<float>(cursor.x), static_cast<float>(cursor.y))) {
        return HTCLIENT;
    }

    const bool left = x >= windowRect.left && x < windowRect.left + kResizeBorderPx;
    const bool right = x < windowRect.right && x >= windowRect.right - kResizeBorderPx;
    const bool top = y >= windowRect.top && y < windowRect.top + kResizeBorderPx;
    const bool bottom = y < windowRect.bottom && y >= windowRect.bottom - kResizeBorderPx;

    if (top && left) return HTTOPLEFT;
    if (top && right) return HTTOPRIGHT;
    if (bottom && left) return HTBOTTOMLEFT;
    if (bottom && right) return HTBOTTOMRIGHT;
    if (left) return HTLEFT;
    if (right) return HTRIGHT;
    if (top) return HTTOP;
    if (bottom) return HTBOTTOM;

    return HTCAPTION;
}

void OverlayWindow::UpdateTrackedWindowSize() {
    if (!hwnd_) {
        return;
    }

    RECT rc{};
    GetWindowRect(hwnd_, &rc);
    settings_.width = ClampOverlayWidth(rc.right - rc.left);
    settings_.height = ClampOverlayHeight(rc.bottom - rc.top);
}

void OverlayWindow::NotifySettingsChanged() {
    if (onSettingsChanged_) {
        onSettingsChanged_();
    }
}

} // namespace rv::app
