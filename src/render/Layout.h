#pragma once

#include <d2d1.h>

#include <vector>

namespace rv::render {

struct BarLayout {
    std::vector<D2D1_RECT_F> barRects;
    float barRadius{4.0f};
    float peakHeight{2.0f};
    float floorY{0.0f};
    float topY{0.0f};
};

BarLayout BuildBarLayout(D2D1_SIZE_F size, size_t bandCount);

} // namespace rv::render
