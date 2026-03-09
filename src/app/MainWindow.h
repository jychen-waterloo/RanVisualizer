#pragma once

#include "../audio/LoopbackCapture.h"
#include "../render/Renderer.h"

#include <windows.h>

namespace rv::app {

class MainWindow {
public:
    MainWindow(audio::LoopbackCapture& capture, bool showDebug);

    bool Create(HINSTANCE instance, int showCmd);
    HWND Hwnd() const { return hwnd_; }
    void Tick(const render::FrameTiming& timing);

private:
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_{nullptr};
    audio::LoopbackCapture& capture_;
    render::Renderer renderer_;
    render::RenderSnapshot lastSnapshot_{};
    bool showDebug_{false};
};

} // namespace rv::app
