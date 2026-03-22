#pragma once

#include "Theme.h"
#include "VisualizerMode.h"

#include <cstddef>

namespace rv::render {

struct RenderConfig {
    size_t barCount{64};
    float motionIntensity{0.6f};
    VisualizerMode mode{VisualizerMode::ClassicBars};
    ThemeConfig theme{};
};

} // namespace rv::render
