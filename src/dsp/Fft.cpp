#include "Fft.h"

#include <cmath>

namespace rv::dsp {

bool Fft::IsPowerOfTwo(const size_t n) {
    return n >= 2 && (n & (n - 1)) == 0;
}

bool Fft::Configure(const size_t size) {
    if (!IsPowerOfTwo(size)) {
        return false;
    }

    size_ = size;
    bitReverse_.assign(size_, 0);
    twiddles_.assign(size_ / 2, {});
    buffer_.assign(size_, {});

    size_t bits = 0;
    while ((1ULL << bits) < size_) {
        ++bits;
    }

    for (size_t i = 0; i < size_; ++i) {
        size_t x = i;
        size_t r = 0;
        for (size_t b = 0; b < bits; ++b) {
            r = (r << 1) | (x & 1);
            x >>= 1;
        }
        bitReverse_[i] = r;
    }

    constexpr float kTwoPi = 6.28318530717958647692f;
    for (size_t k = 0; k < size_ / 2; ++k) {
        const float phase = -kTwoPi * static_cast<float>(k) / static_cast<float>(size_);
        twiddles_[k] = std::complex<float>(std::cos(phase), std::sin(phase));
    }

    return true;
}

void Fft::ForwardReal(const std::vector<float>& input, std::vector<float>& outMagnitudes) {
    if (input.size() < size_ || size_ == 0) {
        return;
    }

    for (size_t i = 0; i < size_; ++i) {
        buffer_[bitReverse_[i]] = std::complex<float>(input[i], 0.0f);
    }

    for (size_t len = 2; len <= size_; len <<= 1) {
        const size_t half = len / 2;
        const size_t step = size_ / len;
        for (size_t i = 0; i < size_; i += len) {
            for (size_t j = 0; j < half; ++j) {
                const auto& w = twiddles_[j * step];
                const std::complex<float> u = buffer_[i + j];
                const std::complex<float> v = buffer_[i + j + half] * w;
                buffer_[i + j] = u + v;
                buffer_[i + j + half] = u - v;
            }
        }
    }

    const size_t bins = size_ / 2 + 1;
    outMagnitudes.resize(bins);
    const float scale = 1.0f / static_cast<float>(size_);
    for (size_t i = 0; i < bins; ++i) {
        outMagnitudes[i] = std::abs(buffer_[i]) * scale;
    }
}

} // namespace rv::dsp
