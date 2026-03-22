#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace rv::render {

namespace {
constexpr float kTopHeadroomRatio = 0.08f;
constexpr float kPeakLineGapPx = 1.0f;
}

Renderer::Renderer()
    : theme_(MakeTheme(config_.theme)) {
}

Renderer::~Renderer() {
    DiscardDeviceResources();
}

bool Renderer::Initialize(HWND hwnd) {
    hwnd_ = hwnd;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    width_ = static_cast<UINT>(rc.right - rc.left);
    height_ = static_cast<UINT>(rc.bottom - rc.top);

    if (!CreateDeviceIndependentResources()) {
        return false;
    }

    controls_.OnResize(static_cast<float>(width_), static_cast<float>(height_));
    triangleTunnel_.OnResize(static_cast<float>(width_), static_cast<float>(height_));
    controls_.SetForceVisible(interactive_);
    return CreateDeviceResources();
}

bool Renderer::CreateDeviceIndependentResources() {
    const HRESULT d2dHr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
    if (FAILED(d2dHr)) {
        return false;
    }

    const HRESULT writeHr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf()));
    if (FAILED(writeHr)) {
        return false;
    }

    return true;
}

bool Renderer::CreateDeviceResources() {
    if (renderTarget_) {
        return true;
    }

    const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    HRESULT hr = d2dFactory_->CreateDCRenderTarget(&props, &renderTarget_);
    if (FAILED(hr)) {
        return false;
    }

    renderTarget_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    renderTarget_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    if (!CreateBitmapResources(width_, height_)) {
        return false;
    }

    hr = renderTarget_->CreateSolidColorBrush(theme_.barBase, &barBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(theme_.peak, &peakBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(theme_.glow, &glowBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(theme_.debugText, &textBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(theme_.backdrop, &backdropBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &controlBgBrush_);
    if (FAILED(hr)) {
        return false;
    }
    hr = renderTarget_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &controlIconBrush_);
    if (FAILED(hr)) {
        return false;
    }

    hr = dwriteFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &textFormat_);
    return SUCCEEDED(hr);
}

bool Renderer::CreateBitmapResources(const UINT width, const UINT height) {
    if (!hwnd_ || width == 0 || height == 0) {
        return false;
    }

    if (!screenDc_) {
        screenDc_ = GetDC(nullptr);
        if (!screenDc_) {
            return false;
        }
    }

    if (!memoryDc_) {
        memoryDc_ = CreateCompatibleDC(screenDc_);
        if (!memoryDc_) {
            return false;
        }
    }

    if (dib_) {
        SelectObject(memoryDc_, oldBitmap_);
        DeleteObject(dib_);
        dib_ = nullptr;
    }

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = static_cast<LONG>(width);
    bi.bmiHeader.biHeight = -static_cast<LONG>(height);
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    dib_ = CreateDIBSection(memoryDc_, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib_) {
        return false;
    }

    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memoryDc_, dib_));

    RECT rc{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    return SUCCEEDED(renderTarget_->BindDC(memoryDc_, &rc));
}

void Renderer::DiscardDeviceResources() {
    textFormat_.Reset();
    backdropBrush_.Reset();
    controlIconBrush_.Reset();
    controlBgBrush_.Reset();
    textBrush_.Reset();
    glowBrush_.Reset();
    peakBrush_.Reset();
    barBrush_.Reset();
    renderTarget_.Reset();

    if (memoryDc_) {
        if (dib_) {
            SelectObject(memoryDc_, oldBitmap_);
            DeleteObject(dib_);
            dib_ = nullptr;
        }
        DeleteDC(memoryDc_);
        memoryDc_ = nullptr;
        oldBitmap_ = nullptr;
    }

    if (screenDc_) {
        ReleaseDC(nullptr, screenDc_);
        screenDc_ = nullptr;
    }
}

void Renderer::OnResize(const UINT width, const UINT height) {
    width_ = width;
    height_ = height;
    controls_.OnResize(static_cast<float>(width_), static_cast<float>(height_));
    triangleTunnel_.OnResize(static_cast<float>(width_), static_cast<float>(height_));
    if (renderTarget_) {
        CreateBitmapResources(width_, height_);
    }
}

void Renderer::SetInteractionState(const bool interactive, const bool pointerHovering) {
    interactive_ = interactive;
    controls_.SetForceVisible(interactive);
    controls_.SetPointerHover(pointerHovering);
}

bool Renderer::HitTestCloseButton(const float x, const float y) const {
    return controls_.HitTestCloseButton(x, y);
}

void Renderer::SetQualityMode(const RenderQualityMode mode) {
    qualityMode_ = mode;
}

void Renderer::SetConfig(const RenderConfig& config) {
    config_ = config;
    theme_ = MakeTheme(config_.theme);
    if (barBrush_) {
        barBrush_->SetColor(theme_.barBase);
    }
    if (peakBrush_) {
        peakBrush_->SetColor(theme_.peak);
    }
    if (glowBrush_) {
        glowBrush_->SetColor(theme_.glow);
    }
    if (backdropBrush_) {
        backdropBrush_->SetColor(theme_.backdrop);
    }
}

void Renderer::DrawBars(const BarLayout& layout) {
    const auto& bars = animation_.DisplayedBands();
    const auto& peaks = animation_.DisplayedPeaks();

    for (size_t i = 0; i < bars.size() && i < layout.barRects.size(); ++i) {
        const auto baseRect = layout.barRects[i];
        const float contentHeight = baseRect.bottom - baseRect.top;
        const float drawableHeight = contentHeight * (1.0f - kTopHeadroomRatio);

        const float normalizedBarValue = std::clamp(bars[i], 0.0f, 1.0f);
        const float normalizedPeakValue = std::clamp(i < peaks.size() ? peaks[i] : normalizedBarValue, 0.0f, 1.0f);

        const float barHeightPx = normalizedBarValue * drawableHeight;
        const float peakHeightPx = normalizedPeakValue * drawableHeight;

        const float barTopY = baseRect.bottom - barHeightPx;
        float peakTopY = baseRect.bottom - peakHeightPx;
        if (normalizedPeakValue > normalizedBarValue + 0.0005f) {
            peakTopY = std::min(peakTopY, barTopY - kPeakLineGapPx);
        }
        peakTopY = std::max(peakTopY, baseRect.top + layout.peakHeight);

        const D2D1_ROUNDED_RECT rr{D2D1::RectF(baseRect.left, barTopY, baseRect.right, baseRect.bottom), layout.barRadius,
            layout.barRadius};
        renderTarget_->FillRoundedRectangle(rr, barBrush_.Get());

        const float glowTop = std::max(layout.topY, barTopY - 12.0f);
        renderTarget_->FillRectangle(D2D1::RectF(baseRect.left, glowTop, baseRect.right, barTopY + 4.0f), glowBrush_.Get());

        renderTarget_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(baseRect.left, peakTopY - layout.peakHeight, baseRect.right, peakTopY), 1.4f, 1.4f),
            peakBrush_.Get());

#if defined(_DEBUG)
        assert(drawableHeight <= contentHeight + 0.001f);
        assert(normalizedBarValue >= 0.0f && normalizedBarValue <= 1.0f);
        assert(normalizedPeakValue >= 0.0f && normalizedPeakValue <= 1.0f);
        assert(std::abs((baseRect.bottom - barTopY) - (normalizedBarValue * drawableHeight)) < 0.01f);
        assert(std::abs((baseRect.bottom - (baseRect.bottom - peakHeightPx)) - (normalizedPeakValue * drawableHeight)) < 0.01f);
#endif
    }
}

void Renderer::DrawBarsFast(const BarLayout& layout) {
    const auto& bars = animation_.DisplayedBands();

    for (size_t i = 0; i < bars.size() && i < layout.barRects.size(); ++i) {
        const auto baseRect = layout.barRects[i];
        const float contentHeight = baseRect.bottom - baseRect.top;
        const float drawableHeight = contentHeight * (1.0f - kTopHeadroomRatio);
        const float barHeightPx = drawableHeight * std::clamp(bars[i], 0.0f, 1.0f);
        const float top = baseRect.bottom - barHeightPx;
        renderTarget_->FillRectangle(D2D1::RectF(baseRect.left, top, baseRect.right, baseRect.bottom), barBrush_.Get());
    }
}


void Renderer::DrawTriangleTunnel(const RenderSnapshot& snapshot, const FrameTiming& timing) {
    triangleTunnel_.Render(renderTarget_.Get(), snapshot, timing, config_, theme_,
        barBrush_.Get(), peakBrush_.Get(), glowBrush_.Get(), backdropBrush_.Get());
}

void Renderer::DrawOverlayControls() {
    const float opacity = controls_.Opacity();
    if (opacity < 0.02f) {
        return;
    }

    const D2D1_ROUNDED_RECT closeButton = controls_.CloseButton();
    controlBgBrush_->SetColor(D2D1::ColorF(0.06f, 0.06f, 0.07f, 0.60f * opacity));
    controlIconBrush_->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.88f * opacity));

    renderTarget_->FillRoundedRectangle(closeButton, controlBgBrush_.Get());

    const D2D1_RECT_F rc = closeButton.rect;
    const float pad = 7.0f;
    const D2D1_POINT_2F a1{rc.left + pad, rc.top + pad};
    const D2D1_POINT_2F a2{rc.right - pad, rc.bottom - pad};
    const D2D1_POINT_2F b1{rc.right - pad, rc.top + pad};
    const D2D1_POINT_2F b2{rc.left + pad, rc.bottom - pad};

    renderTarget_->DrawLine(a1, a2, controlIconBrush_.Get(), 1.5f);
    renderTarget_->DrawLine(b1, b2, controlIconBrush_.Get(), 1.5f);
}

void Renderer::DrawDebugText(const RenderSnapshot& snapshot, const FrameTiming& timing, const size_t bandCount) {
    std::wostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(1);
    oss << L"FPS " << timing.fps;
    oss.precision(3);
    oss << L"  loud " << snapshot.loudness << L"  silent " << (snapshot.isSilentLike ? L"yes" : L"no")
        << L"  bands " << bandCount;

    const auto text = oss.str();
    renderTarget_->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), textFormat_.Get(),
        D2D1::RectF(14.0f, 8.0f, 560.0f, 32.0f), textBrush_.Get());
}

void Renderer::PresentLayered() {
    POINT ptDst{};
    RECT rect{};
    GetWindowRect(hwnd_, &rect);
    ptDst.x = rect.left;
    ptDst.y = rect.top;

    SIZE size{static_cast<LONG>(width_), static_cast<LONG>(height_)};
    POINT ptSrc{};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hwnd_, screenDc_, &ptDst, &size, memoryDc_, &ptSrc, 0, &blend, ULW_ALPHA);
}

void Renderer::Render(const RenderSnapshot& snapshot, const FrameTiming& timing, const bool showDebug) {
    if (width_ == 0 || height_ == 0 || !CreateDeviceResources()) {
        return;
    }

    MotionProfile profile{};
    const float intensity = std::clamp(config_.motionIntensity, 0.0f, 1.0f);
    profile.attackHz = 14.0f + intensity * 18.0f;
    profile.releaseHz = 6.0f + intensity * 12.0f;
    profile.peakRiseHz = 18.0f + intensity * 30.0f;
    profile.peakDecayHz = 1.8f + intensity * 5.4f;
    profile.visualGain = 0.76f + intensity * 0.14f;
    profile.lowEndBoost = intensity * 0.16f;
    profile.accentHz = 4.0f + intensity * 9.0f;

    animation_.Update(snapshot, timing.deltaSeconds, profile, config_.barCount);
    const auto size = D2D1::SizeF(static_cast<float>(width_), static_cast<float>(height_));
    const auto layout = BuildBarLayout(size, animation_.DisplayedBands().size());

    if (showDebug) {
        const auto& bars = animation_.DisplayedBands();
        const auto& peaks = animation_.DisplayedPeaks();
        if (!bars.empty()) {
            float maxBar = 0.0f;
            float maxPeak = 0.0f;
            float avgBar = 0.0f;
            size_t nearMainCeiling = 0;
            for (size_t i = 0; i < bars.size(); ++i) {
                const float b = std::clamp(bars[i], 0.0f, 1.0f);
                const float p = std::clamp(i < peaks.size() ? peaks[i] : b, 0.0f, 1.0f);
                maxBar = std::max(maxBar, b);
                maxPeak = std::max(maxPeak, p);
                avgBar += b;
                if (b > 0.98f) {
                    ++nearMainCeiling;
                }
            }
            avgBar /= static_cast<float>(bars.size());
            const float avgMainOccupancy = avgBar * (1.0f - kTopHeadroomRatio);
            if (maxBar > 1.02f || maxPeak > 1.02f || (avgMainOccupancy > 0.74f && nearMainCeiling > bars.size() / 3)) {
                OutputDebugStringW(L"[RanVisualizer] render-range warning: bars/peaks nearing ceiling too often\n");
            }
        }
    }

    const float accent = animation_.Accent();
    barBrush_->SetColor(D2D1::ColorF(
        theme_.barBase.r + (theme_.barAccent.r - theme_.barBase.r) * accent,
        theme_.barBase.g + (theme_.barAccent.g - theme_.barBase.g) * accent,
        theme_.barBase.b + (theme_.barAccent.b - theme_.barBase.b) * accent,
        theme_.barBase.a));

    peakBrush_->SetColor(theme_.peak);
    backdropBrush_->SetColor(theme_.backdrop);

    renderTarget_->BeginDraw();
    renderTarget_->Clear(D2D1::ColorF(0, 0, 0, 0));

    if (qualityMode_ == RenderQualityMode::InteractiveResize) {
        const D2D1_RECT_F panel = D2D1::RectF(0.0f, 0.0f, size.width, size.height);
        backdropBrush_->SetColor(D2D1::ColorF(0.02f, 0.02f, 0.03f, 0.92f));
        renderTarget_->FillRectangle(panel, backdropBrush_.Get());
        barBrush_->SetColor(D2D1::ColorF(theme_.barBase.r, theme_.barBase.g, theme_.barBase.b, 0.78f));
        DrawBarsFast(layout);
        peakBrush_->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.28f));
        renderTarget_->DrawRectangle(D2D1::RectF(1.0f, 1.0f, size.width - 1.0f, size.height - 1.0f), peakBrush_.Get(), 1.0f);
    } else {
        const D2D1_ROUNDED_RECT plate = D2D1::RoundedRect(D2D1::RectF(6.0f, 6.0f, size.width - 6.0f, size.height - 6.0f), 16.0f, 16.0f);
        renderTarget_->FillRoundedRectangle(plate, backdropBrush_.Get());

        if (config_.mode == VisualizerMode::TriangleTunnel) {
            DrawTriangleTunnel(snapshot, timing);
        } else {
            DrawBars(layout);
        }
        controls_.Update(timing.deltaSeconds);
        DrawOverlayControls();
        if (showDebug) {
            DrawDebugText(snapshot, timing, animation_.DisplayedBands().size());
        }
    }

    const HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
        return;
    }

    PresentLayered();
}

} // namespace rv::render
