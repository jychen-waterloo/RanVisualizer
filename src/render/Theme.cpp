#include "Theme.h"

#include <algorithm>

namespace rv::render {

namespace {

D2D1_COLOR_F ClampColor(const D2D1_COLOR_F c) {
    return D2D1::ColorF(std::clamp(c.r, 0.0f, 1.0f), std::clamp(c.g, 0.0f, 1.0f), std::clamp(c.b, 0.0f, 1.0f), std::clamp(c.a, 0.0f, 1.0f));
}

D2D1_COLOR_F Lift(const D2D1_COLOR_F c, const float amount, const float alpha = -1.0f) {
    return D2D1::ColorF(
        std::clamp(c.r + amount, 0.0f, 1.0f),
        std::clamp(c.g + amount, 0.0f, 1.0f),
        std::clamp(c.b + amount, 0.0f, 1.0f),
        alpha >= 0.0f ? std::clamp(alpha, 0.0f, 1.0f) : c.a);
}

Theme PresetTheme(const ThemePreset preset) {
    Theme t{};
    switch (preset) {
    case ThemePreset::MinimalCyan:
        return t;
    case ThemePreset::NeonPurple:
        t.backdrop = D2D1::ColorF(0.07f, 0.03f, 0.12f, 0.36f);
        t.glow = D2D1::ColorF(0.64f, 0.24f, 0.95f, 0.28f);
        t.barBase = D2D1::ColorF(0.71f, 0.34f, 0.98f, 0.95f);
        t.barAccent = D2D1::ColorF(0.92f, 0.76f, 1.0f, 1.0f);
        t.peak = D2D1::ColorF(1.0f, 0.90f, 1.0f, 0.95f);
        return t;
    case ThemePreset::WarmAmber:
        t.backdrop = D2D1::ColorF(0.12f, 0.08f, 0.03f, 0.36f);
        t.glow = D2D1::ColorF(0.95f, 0.52f, 0.10f, 0.25f);
        t.barBase = D2D1::ColorF(0.95f, 0.64f, 0.22f, 0.95f);
        t.barAccent = D2D1::ColorF(1.0f, 0.86f, 0.48f, 1.0f);
        t.peak = D2D1::ColorF(1.0f, 0.95f, 0.74f, 0.95f);
        return t;
    case ThemePreset::SoftGreen:
        t.backdrop = D2D1::ColorF(0.03f, 0.10f, 0.06f, 0.34f);
        t.glow = D2D1::ColorF(0.22f, 0.78f, 0.49f, 0.22f);
        t.barBase = D2D1::ColorF(0.39f, 0.86f, 0.58f, 0.95f);
        t.barAccent = D2D1::ColorF(0.73f, 0.98f, 0.82f, 1.0f);
        t.peak = D2D1::ColorF(0.92f, 1.0f, 0.95f, 0.95f);
        return t;
    case ThemePreset::MonochromeIce:
        t.backdrop = D2D1::ColorF(0.06f, 0.08f, 0.11f, 0.38f);
        t.glow = D2D1::ColorF(0.60f, 0.72f, 0.82f, 0.20f);
        t.barBase = D2D1::ColorF(0.73f, 0.82f, 0.92f, 0.92f);
        t.barAccent = D2D1::ColorF(0.92f, 0.96f, 1.0f, 1.0f);
        t.peak = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f);
        return t;
    case ThemePreset::Custom:
    default:
        return t;
    }
}

} // namespace

Theme MakeTheme(const ThemeConfig& config) {
    Theme t = PresetTheme(config.preset == ThemePreset::Custom ? ThemePreset::MinimalCyan : config.preset);
    if (config.preset == ThemePreset::Custom) {
        t.barBase = ClampColor(config.customBarColor);
        t.barAccent = Lift(t.barBase, 0.22f, std::clamp(config.barOpacity + 0.05f, 0.0f, 1.0f));
        t.peak = Lift(t.barBase, 0.34f, std::clamp(config.barOpacity + 0.08f, 0.0f, 1.0f));
        t.glow = Lift(t.barBase, 0.08f, std::clamp(config.barOpacity * 0.25f, 0.0f, 1.0f));
        t.backdrop = ClampColor(config.customBackgroundColor);
    }

    t.backdrop.a = std::clamp(config.backgroundOpacity, 0.05f, 1.0f);
    t.barBase.a = std::clamp(config.barOpacity, 0.1f, 1.0f);
    return t;
}

const wchar_t* ThemePresetName(const ThemePreset preset) {
    switch (preset) {
    case ThemePreset::MinimalCyan:
        return L"Minimal Cyan";
    case ThemePreset::NeonPurple:
        return L"Neon Purple";
    case ThemePreset::WarmAmber:
        return L"Warm Amber";
    case ThemePreset::SoftGreen:
        return L"Soft Green";
    case ThemePreset::MonochromeIce:
        return L"Monochrome Ice";
    case ThemePreset::Custom:
    default:
        return L"Custom";
    }
}

} // namespace rv::render
