#pragma once

#include "../audio/LoopbackCapture.h"
#include "../render/Renderer.h"

#include <windows.h>

namespace rv::app {

class OverlayWindow {
public:
    OverlayWindow(audio::LoopbackCapture& capture, bool showDebug);

    bool Create(HINSTANCE instance);
    void Tick(const render::FrameTiming& timing);
    HWND Hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    void ToggleClickThrough();

    HWND hwnd_{nullptr};
    audio::LoopbackCapture& capture_;
    render::Renderer renderer_;
    render::RenderSnapshot lastSnapshot_{};
    bool showDebug_{false};
    bool clickThrough_{false};
    bool toggleLatch_{false};
};

} // namespace rv::app
