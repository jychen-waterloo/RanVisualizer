#include "VisualizerMode.h"

namespace rv::render {

const char* VisualizerModeToString(const VisualizerMode mode) {
    switch (mode) {
    case VisualizerMode::TriangleTunnel:
        return "TriangleTunnel";
    case VisualizerMode::ClassicBars:
    default:
        return "ClassicBars";
    }
}

VisualizerMode VisualizerModeFromString(const std::string_view value) {
    if (value == "TriangleTunnel" || value == "SynthwaveTriangleTunnel") {
        return VisualizerMode::TriangleTunnel;
    }
    return VisualizerMode::ClassicBars;
}

const wchar_t* VisualizerModeDisplayName(const VisualizerMode mode) {
    switch (mode) {
    case VisualizerMode::TriangleTunnel:
        return L"Synthwave Triangle Tunnel";
    case VisualizerMode::ClassicBars:
    default:
        return L"Classic Bars";
    }
}

} // namespace rv::render
