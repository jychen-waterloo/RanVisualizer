#pragma once

#include <d2d1.h>

namespace rv::render {

class OverlayControls {
public:
    void OnResize(float width, float height);
    void SetForceVisible(bool visible);
    void SetPointerHover(bool hovering);
    void Update(float deltaSeconds);

    bool HitTestCloseButton(float x, float y) const;
    float Opacity() const { return opacity_; }
    D2D1_ROUNDED_RECT CloseButton() const;

private:
    void RebuildRect();

    float width_{0.0f};
    float height_{0.0f};
    bool forceVisible_{false};
    bool pointerHover_{false};
    float hoverTimer_{0.0f};
    float hideDelayTimer_{0.0f};
    float opacity_{0.0f};
    D2D1_RECT_F closeRect_{D2D1::RectF(0, 0, 0, 0)};
};

} // namespace rv::render
