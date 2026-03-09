#include "Settings.h"

#include <ShlObj.h>

#include <cstdlib>
#include <fstream>
#include <string>

namespace rv::app {

namespace {

bool TryParseInt(const std::string& value, int& out) {
    char* end = nullptr;
    const long v = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != "\0"[0]) {
        return false;
    }

    out = static_cast<int>(v);
    return true;
}

bool ParseBool(const std::string& value, const bool fallback) {
    if (value == "1" || value == "true") {
        return true;
    }
    if (value == "0" || value == "false") {
        return false;
    }
    return fallback;
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

    SettingsData parsed = data;
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);

        if (key == "x") {
            int parsedValue = 0;
            if (TryParseInt(value, parsedValue)) {
                parsed.x = parsedValue;
                parsed.hasPosition = true;
            }
        } else if (key == "y") {
            int parsedValue = 0;
            if (TryParseInt(value, parsedValue)) {
                parsed.y = parsedValue;
                parsed.hasPosition = true;
            }
        } else if (key == "width") {
            int parsedValue = 0;
            if (TryParseInt(value, parsedValue)) {
                parsed.width = parsedValue;
            }
        } else if (key == "height") {
            int parsedValue = 0;
            if (TryParseInt(value, parsedValue)) {
                parsed.height = parsedValue;
            }
        } else if (key == "clickThrough") {
            parsed.clickThrough = ParseBool(value, parsed.clickThrough);
        } else if (key == "overlayVisible") {
            parsed.overlayVisible = ParseBool(value, parsed.overlayVisible);
        } else if (key == "startInteractive") {
            parsed.startInteractive = ParseBool(value, parsed.startInteractive);
        }
    }

    if (parsed.width < 240 || parsed.height < 100) {
        return false;
    }

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

    out << "x=" << data.x << "\n";
    out << "y=" << data.y << "\n";
    out << "width=" << data.width << "\n";
    out << "height=" << data.height << "\n";
    out << "clickThrough=" << (data.clickThrough ? "true" : "false") << "\n";
    out << "overlayVisible=" << (data.overlayVisible ? "true" : "false") << "\n";
    out << "startInteractive=" << (data.startInteractive ? "true" : "false") << "\n";
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

    return base / "settings.ini";
}

} // namespace rv::app
