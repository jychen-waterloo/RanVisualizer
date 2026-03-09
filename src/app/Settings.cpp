#include "Settings.h"

#include <ShlObj.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace rv::app {

namespace {

bool FindTokenValue(const std::string& text, const std::string& key, std::string& out) {
    const std::string token = "\"" + key + "\"";
    const size_t keyPos = text.find(token);
    if (keyPos == std::string::npos) {
        return false;
    }
    const size_t colonPos = text.find(':', keyPos + token.size());
    if (colonPos == std::string::npos) {
        return false;
    }

    size_t start = text.find_first_not_of(" \t\r\n", colonPos + 1);
    if (start == std::string::npos) {
        return false;
    }

    size_t end = start;
    if (text[start] == '"') {
        ++start;
        end = text.find('"', start);
    } else if (text[start] == '{') {
        end = text.find('}', start);
        if (end != std::string::npos) {
            ++end;
        }
    } else {
        end = text.find_first_of(",}\n\r", start);
    }

    if (end == std::string::npos) {
        return false;
    }

    out = text.substr(start, end - start);
    return true;
}

bool ParseFloat(const std::string& in, float& out) {
    try {
        out = std::stof(in);
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseInt(const std::string& in, int& out) {
    try {
        out = std::stoi(in);
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseBool(const std::string& value, const bool fallback) {
    if (value == "true" || value == "1") {
        return true;
    }
    if (value == "false" || value == "0") {
        return false;
    }
    return fallback;
}

render::ThemePreset ParsePreset(const std::string& s) {
    if (s == "MinimalCyan") return render::ThemePreset::MinimalCyan;
    if (s == "NeonPurple") return render::ThemePreset::NeonPurple;
    if (s == "WarmAmber") return render::ThemePreset::WarmAmber;
    if (s == "SoftGreen") return render::ThemePreset::SoftGreen;
    if (s == "MonochromeIce") return render::ThemePreset::MonochromeIce;
    if (s == "Custom") return render::ThemePreset::Custom;
    return render::ThemePreset::MinimalCyan;
}

std::string PresetToString(const render::ThemePreset p) {
    switch (p) {
    case render::ThemePreset::MinimalCyan: return "MinimalCyan";
    case render::ThemePreset::NeonPurple: return "NeonPurple";
    case render::ThemePreset::WarmAmber: return "WarmAmber";
    case render::ThemePreset::SoftGreen: return "SoftGreen";
    case render::ThemePreset::MonochromeIce: return "MonochromeIce";
    case render::ThemePreset::Custom:
    default: return "Custom";
    }
}

D2D1_COLOR_F ParseColor(const std::string& text, const D2D1_COLOR_F fallback) {
    std::string rv, gv, bv, av;
    if (!FindTokenValue(text, "r", rv) || !FindTokenValue(text, "g", gv) || !FindTokenValue(text, "b", bv)) {
        return fallback;
    }

    D2D1_COLOR_F out = fallback;
    if (!ParseFloat(rv, out.r) || !ParseFloat(gv, out.g) || !ParseFloat(bv, out.b)) {
        return fallback;
    }
    if (FindTokenValue(text, "a", av)) {
        ParseFloat(av, out.a);
    }
    out.r = std::clamp(out.r, 0.0f, 1.0f);
    out.g = std::clamp(out.g, 0.0f, 1.0f);
    out.b = std::clamp(out.b, 0.0f, 1.0f);
    out.a = std::clamp(out.a, 0.0f, 1.0f);
    return out;
}

void WriteColor(std::ofstream& out, const char* key, const D2D1_COLOR_F& c, const bool trailingComma) {
    out << "  \"" << key << "\": { \"r\": " << c.r << ", \"g\": " << c.g << ", \"b\": " << c.b << ", \"a\": " << c.a << " }";
    if (trailingComma) {
        out << ",";
    }
    out << "\n";
}

} // namespace

Settings::Settings()
    : path_(ResolvePath()) {
}

bool Settings::Load(SettingsData& data) const {
    std::ifstream in(path_);
    if (!in.is_open()) {
        return false;
    }

    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    SettingsData parsed = data;
    std::string value;

    if (FindTokenValue(text, "x", value)) {
        int v{};
        if (ParseInt(value, v)) {
            parsed.x = v;
            parsed.hasPosition = true;
        }
    }
    if (FindTokenValue(text, "y", value)) {
        int v{};
        if (ParseInt(value, v)) {
            parsed.y = v;
            parsed.hasPosition = true;
        }
    }
    if (FindTokenValue(text, "width", value)) {
        int v{};
        if (ParseInt(value, v)) parsed.width = v;
    }
    if (FindTokenValue(text, "height", value)) {
        int v{};
        if (ParseInt(value, v)) parsed.height = v;
    }
    if (FindTokenValue(text, "barCount", value)) {
        int v{};
        if (ParseInt(value, v)) parsed.barCount = static_cast<size_t>(std::clamp(v, 24, 96));
    }
    if (FindTokenValue(text, "overlayVisible", value)) parsed.overlayVisible = ParseBool(value, parsed.overlayVisible);
    if (FindTokenValue(text, "clickThrough", value)) parsed.clickThrough = ParseBool(value, parsed.clickThrough);
    if (FindTokenValue(text, "startInteractive", value)) parsed.startInteractive = ParseBool(value, parsed.startInteractive);
    if (FindTokenValue(text, "backgroundOpacity", value)) ParseFloat(value, parsed.backgroundOpacity);
    if (FindTokenValue(text, "barOpacity", value)) ParseFloat(value, parsed.barOpacity);
    if (FindTokenValue(text, "motionIntensity", value)) ParseFloat(value, parsed.motionIntensity);
    if (FindTokenValue(text, "themePreset", value)) parsed.themePreset = ParsePreset(value);

    if (FindTokenValue(text, "barColor", value)) parsed.barColor = ParseColor(value, parsed.barColor);
    if (FindTokenValue(text, "backgroundColor", value)) parsed.backgroundColor = ParseColor(value, parsed.backgroundColor);

    parsed.width = std::clamp(parsed.width, 280, 2000);
    parsed.height = std::clamp(parsed.height, 120, 900);
    parsed.backgroundOpacity = std::clamp(parsed.backgroundOpacity, 0.05f, 1.0f);
    parsed.barOpacity = std::clamp(parsed.barOpacity, 0.1f, 1.0f);
    parsed.motionIntensity = std::clamp(parsed.motionIntensity, 0.0f, 1.0f);

    data = parsed;
    return true;
}

bool Settings::Save(const SettingsData& data) const {
    std::error_code ec;
    std::filesystem::create_directories(path_.parent_path(), ec);

    std::ofstream out(path_, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"x\": " << data.x << ",\n";
    out << "  \"y\": " << data.y << ",\n";
    out << "  \"width\": " << data.width << ",\n";
    out << "  \"height\": " << data.height << ",\n";
    out << "  \"clickThrough\": " << (data.clickThrough ? "true" : "false") << ",\n";
    out << "  \"overlayVisible\": " << (data.overlayVisible ? "true" : "false") << ",\n";
    out << "  \"startInteractive\": " << (data.startInteractive ? "true" : "false") << ",\n";
    out << "  \"barCount\": " << data.barCount << ",\n";
    out << "  \"backgroundOpacity\": " << data.backgroundOpacity << ",\n";
    out << "  \"barOpacity\": " << data.barOpacity << ",\n";
    out << "  \"motionIntensity\": " << data.motionIntensity << ",\n";
    out << "  \"themePreset\": \"" << PresetToString(data.themePreset) << "\",\n";
    WriteColor(out, "barColor", data.barColor, true);
    WriteColor(out, "backgroundColor", data.backgroundColor, false);
    out << "}\n";
    return true;
}

std::filesystem::path Settings::ResolvePath() {
    PWSTR roamingPath = nullptr;
    std::filesystem::path base;

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &roamingPath)) && roamingPath) {
        base = std::filesystem::path(roamingPath) / "RanVisualizer";
        CoTaskMemFree(roamingPath);
    } else {
        base = std::filesystem::current_path() / "config";
    }

    return base / "settings.json";
}

} // namespace rv::app
