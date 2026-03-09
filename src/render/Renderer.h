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
    ~Renderer();

    bool Initialize(HWND hwnd);
    void OnResize(UINT width, UINT height);
    void Render(const RenderSnapshot& snapshot, const FrameTiming& timing, bool showDebug);

private:
    bool CreateDeviceIndependentResources();
    bool CreateDeviceResources();
    bool CreateBitmapResources(UINT width, UINT height);
    void DiscardDeviceResources();
    void DrawBars(const BarLayout& layout);
    void DrawDebugText(const RenderSnapshot& snapshot, const FrameTiming& timing, size_t bandCount);
    void PresentLayered();

    HWND hwnd_{nullptr};
    UINT width_{0};
    UINT height_{0};
    Theme theme_{};
    AnimationState animation_{};

    HDC screenDc_{nullptr};
    HDC memoryDc_{nullptr};
    HBITMAP dib_{nullptr};
    HBITMAP oldBitmap_{nullptr};

    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> renderTarget_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> barBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> peakBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> glowBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> backdropBrush_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
};

} // namespace rv::render
