#include "TriangleTunnelRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rv::render {

namespace {
constexpr float kTau = 6.283185307179586f;

struct TunnelTuning {
    float baseSpeed;
    float motionSpeedGain;
    float bassSpeedGain;
    float loudnessSpeedGain;
    float pulseBassGain;
    float pulseMotionGain;
    float glowBase;
    float glowLoudnessGain;
    float centerDriftBasePx;
    float centerDriftMidGainPx;
    float centerThumpBassGainPx;
    float radiusFactor;
    float lineThicknessBase;
    float lineThicknessDepthGain;
    float lineThicknessLoudGain;
    float lineAlphaBase;
    float lineAlphaDepthGain;
    float lineAlphaLoudGain;
    float maxLineAlpha;
    size_t streakMin;
    float streakMidGain;
    float streakLoudGain;
};

TunnelTuning BuildTuning(const float motionIntensity) {
    const float motion = std::clamp(motionIntensity, 0.0f, 1.0f);
    return TunnelTuning{
        0.085f,
        0.085f * motion,
        0.15f,
        0.05f,
        0.02f,
        0.035f * motion,
        0.30f,
        0.32f,
        1.5f,
        2.6f,
        2.0f,
        0.225f,
        0.65f,
        1.15f,
        0.38f,
        0.06f,
        0.42f,
        0.08f,
        0.62f,
        8,
        8.0f,
        4.0f,
    };
}

} // namespace

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
        frameDepths_[i] = 0.09f + (static_cast<float>(i) / static_cast<float>(kFrameCount)) * 0.86f;
    }

    for (auto& streak : streaks_) {
        streak.lane = NextRand01() * 2.0f - 1.0f;
        streak.depth = 0.08f + NextRand01() * 0.90f;
        streak.velocityBias = 0.82f + NextRand01() * 0.36f;
        streak.length = 0.045f + NextRand01() * 0.07f;
        streak.phase = NextRand01() * kTau;
    }
}

D2D1_POINT_2F TriangleTunnelRenderer::ProjectPoint(
    const float cx, const float cy, const float radius, const float aspect, const float nx, const float ny, const float depth) const {
    const float z = std::max(0.01f, depth);
    const float perspective = 1.0f / (0.22f + z * 1.65f);
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
    const TunnelTuning tuning = BuildTuning(config.motionIntensity);

    const float speed = tuning.baseSpeed + (tuning.motionSpeedGain * std::clamp(config.motionIntensity, 0.0f, 1.0f))
        + bass * tuning.bassSpeedGain + loud * tuning.loudnessSpeedGain;
    const float pulse = 1.0f + bass * (tuning.pulseBassGain + tuning.pulseMotionGain);
    const float glowBoost = tuning.glowBase + loud * tuning.glowLoudnessGain;

    const float cx = width_ * 0.5f + std::sin(time_ * (0.32f + mid * 0.34f)) * (tuning.centerDriftBasePx + tuning.centerDriftMidGainPx * mid);
    const float cy = height_ * 0.5f + std::cos(time_ * 0.24f) * (tuning.centerDriftBasePx + tuning.centerThumpBassGainPx * bass);
    const float aspect = width_ / std::max(height_, 1.0f);
    const float radius = std::min(width_, height_) * tuning.radiusFactor * pulse;

    backdropBrush->SetColor(D2D1::ColorF(
        theme.backdrop.r * 0.42f,
        theme.backdrop.g * 0.34f,
        std::min(1.0f, theme.backdrop.b + 0.02f),
        std::min(0.88f, theme.backdrop.a)));
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
        const float thick = tuning.lineThicknessBase + p * tuning.lineThicknessDepthGain + loud * tuning.lineThicknessLoudGain;
        const float alpha = std::clamp(
            tuning.lineAlphaBase + p * tuning.lineAlphaDepthGain + loud * tuning.lineAlphaLoudGain,
            0.0f,
            tuning.maxLineAlpha);
        const float rot = time_ * (0.17f + 0.08f * mid) + depth * 0.52f;

        std::array<D2D1_POINT_2F, 3> pts{};
        for (size_t v = 0; v < pts.size(); ++v) {
            const float a = rot + static_cast<float>(v) * (kTau / 3.0f) - 1.5707963f;
            pts[v] = ProjectPoint(cx, cy, radius, aspect, std::cos(a), std::sin(a), depth);
        }

        glowBrush->SetColor(D2D1::ColorF(0.12f, 0.88f, 1.0f, alpha * 0.22f * glowBoost));
        primaryBrush->SetColor(D2D1::ColorF(theme.barBase.r, theme.barBase.g, theme.barBase.b, alpha));
        accentBrush->SetColor(D2D1::ColorF(0.92f, 0.35f + treble * 0.35f, 0.84f + treble * 0.10f, alpha * 0.45f));

        for (int e = 0; e < 3; ++e) {
            const D2D1_POINT_2F a = pts[e];
            const D2D1_POINT_2F b = pts[(e + 1) % 3];
            target->DrawLine(a, b, glowBrush, thick + 1.25f);
            target->DrawLine(a, b, primaryBrush, thick);
            target->DrawLine(a, b, accentBrush, std::max(0.5f, thick * 0.22f));
        }
    }

    const size_t streakLimit = std::min(
        streaks_.size(),
        static_cast<size_t>(tuning.streakMin + mid * tuning.streakMidGain + loud * tuning.streakLoudGain));

    for (size_t i = 0; i < streakLimit; ++i) {
        auto& s = streaks_[i];
        s.depth -= dt * speed * (0.55f + s.velocityBias * 0.42f);
        if (s.depth < 0.03f) {
            s.depth += 1.0f;
            s.lane = NextRand01() * 2.0f - 1.0f;
            s.phase = NextRand01() * kTau;
            s.length = 0.04f + NextRand01() * (0.05f + 0.03f * mid);
        }

        const float shimmer = 0.58f + 0.42f * std::sin(time_ * (8.0f + treble * 18.0f) + s.phase);
        const float yWave = std::sin(time_ * 0.78f + s.phase) * 0.09f;
        const D2D1_POINT_2F p0 = ProjectPoint(cx, cy, radius, aspect, s.lane * 0.78f, yWave, s.depth);
        const D2D1_POINT_2F p1 = ProjectPoint(cx, cy, radius, aspect, s.lane * 0.78f, yWave, std::min(1.0f, s.depth + s.length));

        const float alpha = std::clamp((0.11f + treble * 0.24f) * shimmer, 0.04f, 0.42f);
        accentBrush->SetColor(D2D1::ColorF(0.40f + treble * 0.35f, 0.80f, 1.0f, alpha));
        target->DrawLine(p0, p1, accentBrush, 0.72f + treble * 0.55f);
    }
}

} // namespace rv::render
