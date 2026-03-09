#include "Renderer.h"

#include <algorithm>
#include <sstream>

namespace rv::render {

Renderer::Renderer()
    : theme_(MakeDefaultTheme()) {
}

bool Renderer::Initialize(HWND hwnd) {
    hwnd_ = hwnd;

    const HRESULT d2dHr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
    if (FAILED(d2dHr)) {
        return false;
    }

    const HRESULT writeHr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf()));
    if (FAILED(writeHr)) {
        return false;
    }

    return CreateDeviceResources();
}

bool Renderer::CreateDeviceResources() {
    if (renderTarget_) {
        return true;
    }

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HRESULT hr = d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        &renderTarget_);
    if (FAILED(hr)) {
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

    hr = dwriteFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"en-us", &textFormat_);
    return SUCCEEDED(hr);
}

void Renderer::DiscardDeviceResources() {
    textFormat_.Reset();
    textBrush_.Reset();
    glowBrush_.Reset();
    peakBrush_.Reset();
    barBrush_.Reset();
    renderTarget_.Reset();
}

void Renderer::OnResize(const UINT width, const UINT height) {
    if (renderTarget_) {
        renderTarget_->Resize(D2D1::SizeU(width, height));
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

void Renderer::Render(const RenderSnapshot& snapshot, const FrameTiming& timing, const bool showDebug) {
    if (!CreateDeviceResources()) {
        return;
    }

    animation_.Update(snapshot, timing.deltaSeconds);
    const auto size = renderTarget_->GetSize();
    const auto layout = BuildBarLayout(size, animation_.DisplayedBands().size());

    D2D1_COLOR_F bg = theme_.background;
    bg.r += animation_.Accent() * 0.02f;
    bg.g += animation_.Accent() * 0.02f;
    bg.b += animation_.Accent() * 0.03f;

    const float accent = animation_.Accent();
    barBrush_->SetColor(D2D1::ColorF(
        theme_.barBase.r + (theme_.barAccent.r - theme_.barBase.r) * accent,
        theme_.barBase.g + (theme_.barAccent.g - theme_.barBase.g) * accent,
        theme_.barBase.b + (theme_.barAccent.b - theme_.barBase.b) * accent,
        theme_.barBase.a));

    renderTarget_->BeginDraw();
    renderTarget_->Clear(bg);
    DrawBars(layout);
    if (showDebug) {
        DrawDebugText(snapshot, timing, animation_.DisplayedBands().size());
    }

    const HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
}

} // namespace rv::render
