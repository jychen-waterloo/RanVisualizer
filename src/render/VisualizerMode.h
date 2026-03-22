#pragma once

#include <string_view>

namespace rv::render {

enum class VisualizerMode {
    ClassicBars,
    TriangleTunnel,
};

const char* VisualizerModeToString(VisualizerMode mode);
VisualizerMode VisualizerModeFromString(std::string_view value);
const wchar_t* VisualizerModeDisplayName(VisualizerMode mode);

} // namespace rv::render
