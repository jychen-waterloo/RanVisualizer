#pragma once

#include "RenderConfig.h"
#include "RenderTypes.h"
#include "Theme.h"

#include <array>
#include <cstdint>
#include <d2d1.h>

namespace rv::render {

class TriangleTunnelRenderer {
public:
    void OnResize(float width, float height);
    void Render(ID2D1DCRenderTarget* target,
        const RenderSnapshot& snapshot,
        const FrameTiming& timing,
        const RenderConfig& config,
        const Theme& theme,
        ID2D1SolidColorBrush* primaryBrush,
        ID2D1SolidColorBrush* accentBrush,
        ID2D1SolidColorBrush* glowBrush,
        ID2D1SolidColorBrush* backdropBrush);

private:
    struct Streak {
        float lane{0.0f};
        float depth{0.0f};
        float velocityBias{1.0f};
        float length{0.08f};
        float phase{0.0f};
    };

    static constexpr size_t kFrameCount = 18;
    static constexpr size_t kStreakCount = 30;

    float width_{0.0f};
    float height_{0.0f};
    float time_{0.0f};
    float travel_{0.0f};
    std::array<float, kFrameCount> frameDepths_{};
    std::array<Streak, kStreakCount> streaks_{};
    uint32_t rng_{0xA3579BDFu};

    void EnsureInitialized();
    float NextRand01();
    D2D1_POINT_2F ProjectPoint(float cx, float cy, float radius, float aspect, float nx, float ny, float depth) const;
};

} // namespace rv::render
