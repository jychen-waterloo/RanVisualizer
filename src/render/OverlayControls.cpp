#include "OverlayControls.h"

#include <algorithm>

namespace rv::render {

void OverlayControls::OnResize(const float width, const float height) {
    width_ = width;
    height_ = height;
    RebuildRect();
}

void OverlayControls::SetForceVisible(const bool visible) {
    forceVisible_ = visible;
}

void OverlayControls::SetPointerHover(const bool hovering) {
    pointerHover_ = hovering;
    if (hovering) {
        hideDelayTimer_ = 0.0f;
    }
}

void OverlayControls::Update(const float deltaSeconds) {
    if (pointerHover_) {
        hoverTimer_ = std::min(hoverTimer_ + deltaSeconds, 1.0f);
    } else {
        hoverTimer_ = 0.0f;
        hideDelayTimer_ = std::min(hideDelayTimer_ + deltaSeconds, 1.0f);
    }

    const bool shouldShow = forceVisible_ || hoverTimer_ >= 0.14f || (!pointerHover_ && hideDelayTimer_ < 0.3f && opacity_ > 0.0f);
    const float targetOpacity = shouldShow ? 1.0f : 0.0f;
    const float speed = shouldShow ? 8.5f : 6.5f;
    opacity_ = std::clamp(opacity_ + (targetOpacity - opacity_) * speed * deltaSeconds, 0.0f, 1.0f);
}

bool OverlayControls::HitTestCloseButton(const float x, const float y) const {
    if (opacity_ < 0.2f) {
        return false;
    }

    return x >= closeRect_.left && x <= closeRect_.right && y >= closeRect_.top && y <= closeRect_.bottom;
}

D2D1_ROUNDED_RECT OverlayControls::CloseButton() const {
    return D2D1::RoundedRect(closeRect_, 8.0f, 8.0f);
}

void OverlayControls::RebuildRect() {
    const float size = 22.0f;
    const float margin = 12.0f;
    const float left = std::max(margin, width_ - size - margin);
    const float top = margin;
    closeRect_ = D2D1::RectF(left, top, left + size, top + size);
}

} // namespace rv::render
