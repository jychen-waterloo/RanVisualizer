#include "OverlayWindow.h"

#include "../platform/WindowStyles.h"

#include <algorithm>
#include <chrono>
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
            if (interactionMode_ == InteractionMode::Resizing) {
                RenderInteractiveFrame();
            }
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
    case WM_NCLBUTTONDOWN:
        if (IsResizeHit(static_cast<int>(wParam))) {
            EnterInteractionMode(InteractionMode::Resizing);
        } else if (wParam == HTCAPTION) {
            EnterInteractionMode(InteractionMode::Moving);
        }
        break;
    case WM_ENTERSIZEMOVE:
        if (interactionMode_ == InteractionMode::Idle) {
            EnterInteractionMode(IsResizeHit(lastHitTestCode_) ? InteractionMode::Resizing : InteractionMode::Moving);
        }
        return 0;
    case WM_EXITSIZEMOVE:
        UpdateTrackedWindowSize();
        ExitInteractionMode();
        NotifySettingsChanged();
        return 0;
    case WM_CAPTURECHANGED:
        if (interactionMode_ != InteractionMode::Idle) {
            ExitInteractionMode();
        }
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
            renderer_.SetQualityMode(render::RenderQualityMode::InteractiveResize);
            ApplyRenderConfig();
            RenderInteractiveFrame();
            renderer_.SetQualityMode(render::RenderQualityMode::Normal);
            NotifySettingsChanged();
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
        ExitInteractionMode();
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
    data.visualizerMode = settings_.visualizerMode;
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
    cfg.mode = settings_.visualizerMode;
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

int OverlayWindow::ScaleDipToPixels(const int dip) const {
    const UINT dpi = hwnd_ ? GetDpiForWindow(hwnd_) : 96U;
    return std::max(1, MulDiv(dip, static_cast<int>(dpi), 96));
}

bool OverlayWindow::IsInDragRegion(const int clientX, const int clientY, const int resizeEdgePx) const {
    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);

    const RECT dragRect{
        clientRect.left + resizeEdgePx,
        clientRect.top + resizeEdgePx,
        clientRect.right - resizeEdgePx,
        clientRect.bottom - resizeEdgePx};

    return clientX >= dragRect.left && clientX < dragRect.right && clientY >= dragRect.top && clientY < dragRect.bottom;
}

LRESULT OverlayWindow::HandleHitTest(const LPARAM lParam) {
    if (clickThrough_) {
        lastHitTestCode_ = HTTRANSPARENT;
        return HTTRANSPARENT;
    }

    RECT windowRect{};
    GetWindowRect(hwnd_, &windowRect);

    const int screenX = GET_X_LPARAM(lParam);
    const int screenY = GET_Y_LPARAM(lParam);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int clientX = screenX - windowRect.left;
    const int clientY = screenY - windowRect.top;

    if (clientX < 0 || clientY < 0 || clientX >= width || clientY >= height) {
        lastHitTestCode_ = HTNOWHERE;
        return HTNOWHERE;
    }

    if (renderer_.HitTestCloseButton(static_cast<float>(clientX), static_cast<float>(clientY))) {
        lastHitTestCode_ = HTCLIENT;
        return HTCLIENT;
    }

    const int edge = ScaleDipToPixels(kResizeEdgeThicknessDip);
    const int corner = std::max(edge, ScaleDipToPixels(kResizeCornerThicknessDip));

    const bool nearLeft = clientX >= 0 && clientX < edge;
    const bool nearRight = clientX >= (width - edge) && clientX < width;
    const bool nearTop = clientY >= 0 && clientY < edge;
    const bool nearBottom = clientY >= (height - edge) && clientY < height;

    const bool inTopCornerBand = clientY >= 0 && clientY < corner;
    const bool inBottomCornerBand = clientY >= (height - corner) && clientY < height;
    const bool inLeftCornerBand = clientX >= 0 && clientX < corner;
    const bool inRightCornerBand = clientX >= (width - corner) && clientX < width;

    if (inTopCornerBand && inLeftCornerBand) {
        lastHitTestCode_ = HTTOPLEFT;
        return HTTOPLEFT;
    }
    if (inTopCornerBand && inRightCornerBand) {
        lastHitTestCode_ = HTTOPRIGHT;
        return HTTOPRIGHT;
    }
    if (inBottomCornerBand && inLeftCornerBand) {
        lastHitTestCode_ = HTBOTTOMLEFT;
        return HTBOTTOMLEFT;
    }
    if (inBottomCornerBand && inRightCornerBand) {
        lastHitTestCode_ = HTBOTTOMRIGHT;
        return HTBOTTOMRIGHT;
    }

    if (nearLeft) {
        lastHitTestCode_ = HTLEFT;
        return HTLEFT;
    }
    if (nearRight) {
        lastHitTestCode_ = HTRIGHT;
        return HTRIGHT;
    }
    if (nearTop) {
        lastHitTestCode_ = HTTOP;
        return HTTOP;
    }
    if (nearBottom) {
        lastHitTestCode_ = HTBOTTOM;
        return HTBOTTOM;
    }

    if (IsInDragRegion(clientX, clientY, edge)) {
        lastHitTestCode_ = HTCAPTION;
        return HTCAPTION;
    }

    lastHitTestCode_ = HTCLIENT;
    return HTCLIENT;
}

bool OverlayWindow::IsResizeHit(const int hitTest) const {
    switch (hitTest) {
    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTBOTTOM:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        return true;
    default:
        return false;
    }
}

void OverlayWindow::EnterInteractionMode(const InteractionMode mode) {
    interactionMode_ = mode;
    if (interactionMode_ == InteractionMode::Resizing) {
        renderer_.SetQualityMode(render::RenderQualityMode::InteractiveResize);
        RenderInteractiveFrame();
    }
}

void OverlayWindow::ExitInteractionMode() {
    interactionMode_ = InteractionMode::Idle;
    renderer_.SetQualityMode(render::RenderQualityMode::Normal);
}

void OverlayWindow::RenderInteractiveFrame() {
    const auto now = std::chrono::steady_clock::now();
    renderer_.Render(lastSnapshot_, render::FrameTiming{now, 1.0f / 60.0f, 60.0f}, false);
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
