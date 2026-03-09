#pragma once

#include "AnimationState.h"
#include "Layout.h"
#include "RenderTypes.h"
#include "Theme.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace rv::render {

class Renderer {
public:
    Renderer();

    bool Initialize(HWND hwnd);
    void OnResize(UINT width, UINT height);
    void Render(const RenderSnapshot& snapshot, const FrameTiming& timing, bool showDebug);

private:
    bool CreateDeviceResources();
    void DiscardDeviceResources();
    void DrawBars(const BarLayout& layout);
    void DrawDebugText(const RenderSnapshot& snapshot, const FrameTiming& timing, size_t bandCount);

    HWND hwnd_{nullptr};
    Theme theme_{};
    AnimationState animation_{};

    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> barBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> peakBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> glowBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
};

} // namespace rv::render
