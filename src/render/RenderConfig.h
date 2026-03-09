#pragma once

#include "Theme.h"

#include <cstddef>

namespace rv::render {

struct RenderConfig {
    size_t barCount{64};
    float motionIntensity{0.6f};
    ThemeConfig theme{};
};

} // namespace rv::render
