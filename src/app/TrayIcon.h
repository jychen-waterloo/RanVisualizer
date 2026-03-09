#pragma once

#include "AppCommands.h"
#include "Settings.h"

#include <optional>
#include <windows.h>

namespace rv::app {

class TrayIcon {
public:
    static constexpr UINT kTrayMessage = WM_APP + 41;

    enum MenuId : UINT {
        MenuShowHide = 3001,
        MenuToggleClickThrough = 3002,
        MenuEnableInteractive = 3003,
        MenuExit = 3004,

        MenuSizeSmall = 3101,
        MenuSizeMedium = 3102,
        MenuSizeLarge = 3103,

        MenuBars24 = 3201,
        MenuBars40 = 3202,
        MenuBars64 = 3203,
        MenuBars96 = 3204,

        MenuMotionCalm = 3301,
        MenuMotionBalanced = 3302,
        MenuMotionEnergetic = 3303,

        MenuThemeMinimalCyan = 3401,
        MenuThemeNeonPurple = 3402,
        MenuThemeWarmAmber = 3403,
        MenuThemeSoftGreen = 3404,
        MenuThemeMonochromeIce = 3405,

        MenuBarColorCyan = 3501,
        MenuBarColorMagenta = 3502,
        MenuBarColorAmber = 3503,
        MenuBarColorMint = 3504,

        MenuBgColorBlue = 3601,
        MenuBgColorViolet = 3602,
        MenuBgColorCharcoal = 3603,
        MenuBgColorForest = 3604,

        MenuOpacity25 = 3701,
        MenuOpacity45 = 3702,
        MenuOpacity65 = 3703,
        MenuOpacity85 = 3704,
    };

    bool Initialize(HWND hwnd);
    void Shutdown();
    void ShowMenu(const SettingsData& settings, bool overlayVisible, bool clickThrough);
    bool IsTrayCommand(WPARAM command) const;
    AppCommand CommandFromMenu(WPARAM command) const;
    bool TryApplySettingsCommand(WPARAM command, SettingsData& settings) const;

private:
    HWND hwnd_{nullptr};
    UINT iconId_{1};
    bool installed_{false};
};

} // namespace rv::app
