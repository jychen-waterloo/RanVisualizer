#pragma once

#include "../render/Theme.h"
#include "../render/VisualizerMode.h"

#include <windows.h>

#include <filesystem>

namespace rv::app {

struct SettingsData {
    int x{0};
    int y{0};
    int width{460};
    int height{170};
    bool hasPosition{false};
    bool clickThrough{false};
    bool overlayVisible{true};
    bool startInteractive{true};
    size_t barCount{64};
    float backgroundOpacity{0.34f};
    float barOpacity{0.95f};
    float motionIntensity{0.6f};
    render::VisualizerMode visualizerMode{render::VisualizerMode::ClassicBars};
    render::ThemePreset themePreset{render::ThemePreset::MinimalCyan};
    D2D1_COLOR_F barColor{0.26f, 0.72f, 0.95f, 0.95f};
    D2D1_COLOR_F backgroundColor{0.03f, 0.05f, 0.08f, 0.34f};
};

class Settings {
public:
    Settings();

    bool Load(SettingsData& data) const;
    bool Save(const SettingsData& data) const;

    std::filesystem::path Path() const { return path_; }

private:
    static std::filesystem::path ResolvePath();

    std::filesystem::path path_;
};

} // namespace rv::app
