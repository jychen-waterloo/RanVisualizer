#pragma once

#include <cstddef>
#include <vector>

namespace rv::dsp {

class Window {
public:
    void ConfigureHann(size_t size);
    [[nodiscard]] const std::vector<float>& Coefficients() const { return coefficients_; }

private:
    std::vector<float> coefficients_;
};

} // namespace rv::dsp
