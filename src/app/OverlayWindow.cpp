#include "OverlayWindow.h"

#include "../platform/WindowStyles.h"

namespace rv::app {

namespace {
constexpr UINT_PTR kMenuToggleClickThrough = 1001;
}

OverlayWindow::OverlayWindow(audio::LoopbackCapture& capture, const bool showDebug)
    : capture_(capture), showDebug_(showDebug) {
}

bool OverlayWindow::Create(HINSTANCE instance) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayWindow::WndProcStatic;
    wc.hInstance = instance;
    wc.lpszClassName = L"RanVisualizerOverlayWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    const auto rect = platform::ComputeBottomRightRect(platform::OverlayMetrics{});
    hwnd_ = CreateWindowExW(platform::kOverlayExStyle, wc.lpszClassName, L"RanVisualizer Overlay",
        platform::kOverlayStyle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, instance, this);

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);

    return renderer_.Initialize(hwnd_);
}

void OverlayWindow::Tick(const render::FrameTiming& timing) {
    if (!hwnd_ || IsIconic(hwnd_)) {
        return;
    }

    const bool pressed = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (pressed && !toggleLatch_) {
        ToggleClickThrough();
    }
    toggleLatch_ = pressed;

    dsp::AnalysisFrame frame;
    if (capture_.GetLatestAnalysisFrame(frame)) {
        lastSnapshot_ = render::RenderSnapshot::FromAnalysisFrame(frame);
    }

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
    case WM_NCHITTEST:
        if (!clickThrough_) {
            return HTCAPTION;
        }
        break;
    case WM_RBUTTONUP: {
        POINT pt{};
        GetCursorPos(&pt);
        HMENU menu = CreatePopupMenu();
        if (menu) {
            AppendMenuW(menu, MF_STRING | (clickThrough_ ? MF_CHECKED : 0), kMenuToggleClickThrough,
                L"Toggle click-through (F8)");
            SetForegroundWindow(hwnd_);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
            DestroyMenu(menu);
        }
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kMenuToggleClickThrough) {
            ToggleClickThrough();
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == 'T') {
            ToggleClickThrough();
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void OverlayWindow::ToggleClickThrough() {
    clickThrough_ = !clickThrough_;
    platform::SetClickThrough(hwnd_, clickThrough_);
}

} // namespace rv::app
