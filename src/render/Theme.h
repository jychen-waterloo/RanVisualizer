#pragma once

#include <d2d1.h>

namespace rv::render {

struct Theme {
    D2D1_COLOR_F background{0.04f, 0.05f, 0.08f, 0.0f};
    D2D1_COLOR_F backdrop{0.03f, 0.05f, 0.08f, 0.34f};
    D2D1_COLOR_F glow{0.18f, 0.47f, 0.88f, 0.22f};
    D2D1_COLOR_F barBase{0.26f, 0.72f, 0.95f, 0.95f};
    D2D1_COLOR_F barAccent{0.59f, 0.86f, 1.0f, 1.0f};
    D2D1_COLOR_F peak{0.96f, 0.98f, 1.0f, 0.9f};
    D2D1_COLOR_F debugText{0.78f, 0.84f, 0.93f, 0.85f};
};

Theme MakeDefaultTheme();

} // namespace rv::render
