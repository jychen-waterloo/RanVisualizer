#include "Window.h"

#include <cmath>

namespace rv::dsp {

void Window::ConfigureHann(const size_t size) {
    coefficients_.assign(size, 0.0f);
    if (size <= 1) {
        return;
    }

    constexpr float kTwoPi = 6.28318530717958647692f;
    const float denom = static_cast<float>(size - 1);
    for (size_t i = 0; i < size; ++i) {
        const float phase = kTwoPi * static_cast<float>(i) / denom;
        coefficients_[i] = 0.5f * (1.0f - std::cos(phase));
    }
}

} // namespace rv::dsp
