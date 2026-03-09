#include "Renderer.h"

#include <algorithm>
#include <sstream>

namespace rv::render {

Renderer::Renderer()
    : theme_(MakeDefaultTheme()) {
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
    if (renderTarget_) {
        CreateBitmapResources(width_, height_);
    }
}

void Renderer::DrawBars(const BarLayout& layout) {
    const auto& bands = animation_.DisplayedBands();
    const auto& peaks = animation_.DisplayedPeaks();

    for (size_t i = 0; i < bands.size() && i < layout.barRects.size(); ++i) {
        const auto baseRect = layout.barRects[i];
        const float usableHeight = baseRect.bottom - baseRect.top;
        const float barHeight = usableHeight * std::clamp(bands[i], 0.0f, 1.0f);
        const float top = baseRect.bottom - barHeight;

        const D2D1_ROUNDED_RECT rr{D2D1::RectF(baseRect.left, top, baseRect.right, baseRect.bottom), layout.barRadius,
            layout.barRadius};
        renderTarget_->FillRoundedRectangle(rr, barBrush_.Get());

        const float glowTop = std::max(layout.topY, top - 12.0f);
        renderTarget_->FillRectangle(D2D1::RectF(baseRect.left, glowTop, baseRect.right, top + 4.0f), glowBrush_.Get());

        const float peakY = baseRect.bottom - usableHeight * std::clamp(peaks[i], 0.0f, 1.0f);
        renderTarget_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(baseRect.left, peakY - layout.peakHeight, baseRect.right, peakY), 1.4f, 1.4f),
            peakBrush_.Get());
    }
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

    animation_.Update(snapshot, timing.deltaSeconds);
    const auto size = D2D1::SizeF(static_cast<float>(width_), static_cast<float>(height_));
    const auto layout = BuildBarLayout(size, animation_.DisplayedBands().size());

    const float accent = animation_.Accent();
    barBrush_->SetColor(D2D1::ColorF(
        theme_.barBase.r + (theme_.barAccent.r - theme_.barBase.r) * accent,
        theme_.barBase.g + (theme_.barAccent.g - theme_.barBase.g) * accent,
        theme_.barBase.b + (theme_.barAccent.b - theme_.barBase.b) * accent,
        theme_.barBase.a));

    renderTarget_->BeginDraw();
    renderTarget_->Clear(D2D1::ColorF(0, 0, 0, 0));

    const D2D1_ROUNDED_RECT plate = D2D1::RoundedRect(D2D1::RectF(6.0f, 6.0f, size.width - 6.0f, size.height - 6.0f), 16.0f, 16.0f);
    renderTarget_->FillRoundedRectangle(plate, backdropBrush_.Get());

    DrawBars(layout);
    if (showDebug) {
        DrawDebugText(snapshot, timing, animation_.DisplayedBands().size());
    }

    const HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
        return;
    }

    PresentLayered();
}

} // namespace rv::render
