#include "TriangleTunnelRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rv::render {

namespace {
constexpr float kTau = 6.283185307179586f;
}

void TriangleTunnelRenderer::OnResize(const float width, const float height) {
    width_ = width;
    height_ = height;
}

float TriangleTunnelRenderer::NextRand01() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return static_cast<float>((rng_ >> 8) & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}

void TriangleTunnelRenderer::EnsureInitialized() {
    if (frameDepths_[0] > 0.0f) {
        return;
    }

    for (size_t i = 0; i < kFrameCount; ++i) {
        frameDepths_[i] = 0.06f + (static_cast<float>(i) / static_cast<float>(kFrameCount)) * 0.94f;
    }

    for (auto& streak : streaks_) {
        streak.lane = NextRand01() * 2.0f - 1.0f;
        streak.depth = 0.05f + NextRand01() * 0.95f;
        streak.velocityBias = 0.75f + NextRand01() * 0.6f;
        streak.length = 0.05f + NextRand01() * 0.12f;
        streak.phase = NextRand01() * kTau;
    }
}

D2D1_POINT_2F TriangleTunnelRenderer::ProjectPoint(
    const float cx, const float cy, const float radius, const float aspect, const float nx, const float ny, const float depth) const {
    const float z = std::max(0.01f, depth);
    const float perspective = 1.0f / (0.16f + z * 1.55f);
    return D2D1::Point2F(cx + nx * radius * perspective * aspect, cy + ny * radius * perspective);
}

void TriangleTunnelRenderer::Render(ID2D1DCRenderTarget* target,
    const RenderSnapshot& snapshot,
    const FrameTiming& timing,
    const RenderConfig& config,
    const Theme& theme,
    ID2D1SolidColorBrush* primaryBrush,
    ID2D1SolidColorBrush* accentBrush,
    ID2D1SolidColorBrush* glowBrush,
    ID2D1SolidColorBrush* backdropBrush) {
    if (!target || width_ <= 1.0f || height_ <= 1.0f || !primaryBrush || !accentBrush || !glowBrush || !backdropBrush) {
        return;
    }

    EnsureInitialized();
    const float dt = std::clamp(timing.deltaSeconds, 0.001f, 0.05f);
    time_ += dt;

    const float bass = std::clamp(snapshot.bassEnergy, 0.0f, 1.0f);
    const float mid = std::clamp(snapshot.midEnergy, 0.0f, 1.0f);
    const float treble = std::clamp(snapshot.trebleEnergy, 0.0f, 1.0f);
    const float loud = std::clamp(snapshot.loudness, 0.0f, 1.0f);
    const float motion = std::clamp(config.motionIntensity, 0.0f, 1.0f);

    const float speed = 0.20f + motion * 0.26f + bass * 0.62f + loud * 0.22f;
    const float pulse = 1.0f + bass * (0.08f + 0.14f * motion);
    const float glowBoost = 0.35f + loud * 0.75f;

    const float cx = width_ * 0.5f + std::sin(time_ * (0.45f + mid * 0.7f)) * 7.0f * mid;
    const float cy = height_ * 0.5f + std::cos(time_ * 0.38f) * 4.0f * bass;
    const float aspect = width_ / std::max(height_, 1.0f);
    const float radius = std::min(width_, height_) * 0.24f * pulse;

    backdropBrush->SetColor(D2D1::ColorF(theme.backdrop.r * 0.45f, theme.backdrop.g * 0.35f, std::min(1.0f, theme.backdrop.b + 0.03f), theme.backdrop.a));
    target->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(6.0f, 6.0f, width_ - 6.0f, height_ - 6.0f), 16.0f, 16.0f),
        backdropBrush);

    travel_ += speed * dt;
    if (travel_ >= 1.0f) {
        travel_ -= std::floor(travel_);
    }

    for (size_t i = 0; i < frameDepths_.size(); ++i) {
        float depth = frameDepths_[i] - travel_;
        if (depth <= 0.02f) {
            depth += 1.0f;
        }

        const float p = 1.0f - depth;
        const float thick = 0.8f + p * 2.2f + loud * 0.9f;
        const float alpha = std::clamp(0.08f + p * 0.58f + loud * 0.12f, 0.0f, 0.92f);
        const float rot = time_ * (0.28f + 0.16f * mid) + depth * 0.8f;

        std::array<D2D1_POINT_2F, 3> pts{};
        for (size_t v = 0; v < pts.size(); ++v) {
            const float a = rot + static_cast<float>(v) * (kTau / 3.0f) - 1.5707963f;
            const float nx = std::cos(a);
            const float ny = std::sin(a);
            pts[v] = ProjectPoint(cx, cy, radius, aspect, nx, ny, depth);
        }

        glowBrush->SetColor(D2D1::ColorF(0.09f, 0.95f, 1.0f, alpha * 0.32f * glowBoost));
        primaryBrush->SetColor(D2D1::ColorF(theme.barBase.r, theme.barBase.g, theme.barBase.b, alpha));
        accentBrush->SetColor(D2D1::ColorF(1.0f, 0.18f + treble * 0.55f, 0.78f + treble * 0.2f, alpha * 0.78f));

        for (int e = 0; e < 3; ++e) {
            const D2D1_POINT_2F a = pts[e];
            const D2D1_POINT_2F b = pts[(e + 1) % 3];
            target->DrawLine(a, b, glowBrush, thick + 2.6f);
            target->DrawLine(a, b, primaryBrush, thick);
            target->DrawLine(a, b, accentBrush, std::max(0.8f, thick * 0.33f));
        }
    }

    const size_t streakLimit = static_cast<size_t>(18 + mid * 22.0f + loud * 8.0f);
    for (size_t i = 0; i < streakLimit && i < streaks_.size(); ++i) {
        auto& s = streaks_[i];
        s.depth -= dt * speed * (0.7f + s.velocityBias * 0.8f);
        if (s.depth < 0.03f) {
            s.depth += 1.0f;
            s.lane = NextRand01() * 2.0f - 1.0f;
            s.phase = NextRand01() * kTau;
            s.length = 0.05f + NextRand01() * (0.08f + 0.06f * mid);
        }

        const float shimmer = 0.5f + 0.5f * std::sin(time_ * (12.0f + treble * 36.0f) + s.phase);
        const float yWave = std::sin(time_ * 1.3f + s.phase) * 0.16f;
        const float z0 = s.depth;
        const float z1 = std::min(1.0f, s.depth + s.length);
        const D2D1_POINT_2F p0 = ProjectPoint(cx, cy, radius, aspect, s.lane * 0.9f, yWave, z0);
        const D2D1_POINT_2F p1 = ProjectPoint(cx, cy, radius, aspect, s.lane * 0.9f, yWave, z1);

        const float alpha = std::clamp((0.18f + treble * 0.45f) * shimmer, 0.05f, 0.85f);
        accentBrush->SetColor(D2D1::ColorF(0.35f + treble * 0.55f, 0.82f, 1.0f, alpha));
        target->DrawLine(p0, p1, accentBrush, 0.9f + treble * 1.1f);
    }
}

} // namespace rv::render
