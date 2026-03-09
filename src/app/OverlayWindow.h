#pragma once

#include "AppCommands.h"
#include "Hotkeys.h"
#include "Settings.h"
#include "TrayIcon.h"

#include "../audio/LoopbackCapture.h"
#include "../render/Renderer.h"

#include <functional>
#include <windows.h>

namespace rv::app {

class OverlayWindow {
public:
    using CommandCallback = std::function<void(AppCommand)>;

    OverlayWindow(audio::LoopbackCapture& capture, bool showDebug);

    bool Create(HINSTANCE instance, const SettingsData& settings);
    void Tick(const render::FrameTiming& timing);
    HWND Hwnd() const { return hwnd_; }

    bool IsClickThrough() const { return clickThrough_; }
    bool IsOverlayVisible() const { return overlayVisible_; }
    void ToggleClickThrough();
    void SetClickThrough(bool enabled);
    void EnsureInteractive();
    void ToggleOverlayVisibility();
    SettingsData CaptureSettings() const;
    void SetCommandCallback(CommandCallback callback);
    void SetSettingsChangedCallback(std::function<void()> callback);

private:
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    void RequestCommand(AppCommand command);
    bool PointFromLParam(LPARAM lParam, float& x, float& y) const;
    void ApplyRenderConfig();

    HWND hwnd_{nullptr};
    audio::LoopbackCapture& capture_;
    render::Renderer renderer_;
    render::RenderSnapshot lastSnapshot_{};
    CommandCallback onCommand_{};
    std::function<void()> onSettingsChanged_{};
    bool showDebug_{false};
    bool clickThrough_{false};
    bool overlayVisible_{true};
    bool pointerHovering_{false};
    TrayIcon tray_{};
    Hotkeys hotkeys_{};
    SettingsData settings_{};
};

} // namespace rv::app
