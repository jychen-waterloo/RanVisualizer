#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace rv::dsp {

class Fft {
public:
    bool Configure(size_t size);
    [[nodiscard]] size_t Size() const { return size_; }

    void ForwardReal(const std::vector<float>& input, std::vector<float>& outMagnitudes);

private:
    static bool IsPowerOfTwo(size_t n);

    size_t size_{0};
    std::vector<size_t> bitReverse_;
    std::vector<std::complex<float>> twiddles_;
    std::vector<std::complex<float>> buffer_;
};

} // namespace rv::dsp
