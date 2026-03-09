#pragma once

#include <windows.h>

#include <filesystem>

namespace rv::app {

struct SettingsData {
    int x{0};
    int y{0};
    int width{460};
    int height{170};
    bool hasPosition{false};
    bool clickThrough{false};
    bool overlayVisible{true};
    bool startInteractive{true};
};

class Settings {
public:
    Settings();

    bool Load(SettingsData& data) const;
    bool Save(const SettingsData& data) const;

    std::filesystem::path Path() const { return path_; }

private:
    static std::filesystem::path ResolvePath();

    std::filesystem::path path_;
};

} // namespace rv::app
