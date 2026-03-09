#include "MainWindow.h"

namespace rv::app {

MainWindow::MainWindow(audio::LoopbackCapture& capture, const bool showDebug)
    : capture_(capture), showDebug_(showDebug) {
}

bool MainWindow::Create(HINSTANCE instance, const int showCmd) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = MainWindow::WndProcStatic;
    wc.hInstance = instance;
    wc.lpszClassName = L"RanVisualizerTask3Window";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"Audio Visualizer - Task 3", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1120, 680, nullptr, nullptr, instance, this);

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, showCmd);
    UpdateWindow(hwnd_);
    return renderer_.Initialize(hwnd_);
}

void MainWindow::Tick(const render::FrameTiming& timing) {
    if (!hwnd_ || IsIconic(hwnd_)) {
        return;
    }

    dsp::AnalysisFrame frame;
    if (capture_.GetLatestAnalysisFrame(frame)) {
        lastSnapshot_ = render::RenderSnapshot::FromAnalysisFrame(frame);
    }

    renderer_.Render(lastSnapshot_, timing, showDebug_);
}

LRESULT CALLBACK MainWindow::WndProcStatic(HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->WndProc(msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::WndProc(const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        renderer_.OnResize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }
}

} // namespace rv::app
