#include "Layout.h"

#include <algorithm>
#include <cmath>

namespace rv::render {

BarLayout BuildBarLayout(const D2D1_SIZE_F size, const size_t bandCount) {
    BarLayout layout;
    if (bandCount == 0 || size.width <= 0.0f || size.height <= 0.0f) {
        return layout;
    }

    const float marginX = std::max(18.0f, size.width * 0.04f);
    const float marginTop = std::max(14.0f, size.height * 0.08f);
    const float marginBottom = std::max(20.0f, size.height * 0.12f);
    const float drawableWidth = std::max(1.0f, size.width - marginX * 2.0f);
    const float drawableHeight = std::max(1.0f, size.height - marginTop - marginBottom);
    const float gap = std::clamp(drawableWidth / (bandCount * 18.0f), 1.5f, 6.0f);
    const float barWidth = (drawableWidth - gap * static_cast<float>(bandCount - 1)) / static_cast<float>(bandCount);

    layout.barRadius = std::clamp(barWidth * 0.35f, 2.0f, 8.0f);
    layout.peakHeight = std::clamp(size.height * 0.006f, 1.5f, 3.5f);
    layout.floorY = size.height - marginBottom;
    layout.topY = marginTop;

    layout.barRects.reserve(bandCount);
    float x = marginX;
    for (size_t i = 0; i < bandCount; ++i) {
        layout.barRects.push_back(D2D1::RectF(x, layout.topY, x + barWidth, layout.floorY));
        x += barWidth + gap;
    }

    return layout;
}

} // namespace rv::render
