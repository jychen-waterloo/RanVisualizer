#include "TrayIcon.h"

#include <shellapi.h>

namespace rv::app {

namespace {

UINT Checked(const bool enabled) {
    return MF_STRING | (enabled ? MF_CHECKED : 0U);
}

} // namespace

bool TrayIcon::Initialize(HWND hwnd) {
    hwnd_ = hwnd;

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd_;
    nid.uID = iconId_;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = kTrayMessage;
    nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(nid.szTip, L"RanVisualizer", ARRAYSIZE(nid.szTip));

    installed_ = Shell_NotifyIconW(NIM_ADD, &nid) != FALSE;
    return installed_;
}

void TrayIcon::Shutdown() {
    if (!installed_ || !hwnd_) {
        return;
    }

    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd_;
    nid.uID = iconId_;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    installed_ = false;
}

void TrayIcon::ShowMenu(const SettingsData& settings, const bool overlayVisible, const bool clickThrough) {
    if (!installed_ || !hwnd_) {
        return;
    }

    POINT pt{};
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    HMENU sizeMenu = CreatePopupMenu();
    HMENU barMenu = CreatePopupMenu();
    HMENU motionMenu = CreatePopupMenu();
    HMENU themeMenu = CreatePopupMenu();
    HMENU barColorMenu = CreatePopupMenu();
    HMENU bgColorMenu = CreatePopupMenu();
    HMENU opacityMenu = CreatePopupMenu();
    if (!menu || !sizeMenu || !barMenu || !motionMenu || !themeMenu || !barColorMenu || !bgColorMenu || !opacityMenu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, MenuShowHide, overlayVisible ? L"Hide overlay" : L"Show overlay");
    AppendMenuW(menu, Checked(clickThrough), MenuToggleClickThrough, L"Toggle click-through");
    AppendMenuW(menu, Checked(!clickThrough), MenuEnableInteractive, L"Enable interaction");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(sizeMenu, Checked(settings.width == 420), MenuSizeSmall, L"Small 420x150");
    AppendMenuW(sizeMenu, Checked(settings.width == 460), MenuSizeMedium, L"Medium 460x170");
    AppendMenuW(sizeMenu, Checked(settings.width == 560), MenuSizeLarge, L"Large 560x210");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(sizeMenu), L"Window size");

    AppendMenuW(barMenu, Checked(settings.barCount == 24), MenuBars24, L"24");
    AppendMenuW(barMenu, Checked(settings.barCount == 40), MenuBars40, L"40");
    AppendMenuW(barMenu, Checked(settings.barCount == 64), MenuBars64, L"64");
    AppendMenuW(barMenu, Checked(settings.barCount == 96), MenuBars96, L"96");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(barMenu), L"Spectrum bars");

    AppendMenuW(motionMenu, Checked(settings.motionIntensity < 0.33f), MenuMotionCalm, L"Calm");
    AppendMenuW(motionMenu, Checked(settings.motionIntensity >= 0.33f && settings.motionIntensity < 0.75f), MenuMotionBalanced, L"Balanced");
    AppendMenuW(motionMenu, Checked(settings.motionIntensity >= 0.75f), MenuMotionEnergetic, L"Energetic");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(motionMenu), L"Motion feel");

    AppendMenuW(themeMenu, Checked(settings.themePreset == render::ThemePreset::MinimalCyan), MenuThemeMinimalCyan, L"Minimal Cyan");
    AppendMenuW(themeMenu, Checked(settings.themePreset == render::ThemePreset::NeonPurple), MenuThemeNeonPurple, L"Neon Purple");
    AppendMenuW(themeMenu, Checked(settings.themePreset == render::ThemePreset::WarmAmber), MenuThemeWarmAmber, L"Warm Amber");
    AppendMenuW(themeMenu, Checked(settings.themePreset == render::ThemePreset::SoftGreen), MenuThemeSoftGreen, L"Soft Green");
    AppendMenuW(themeMenu, Checked(settings.themePreset == render::ThemePreset::MonochromeIce), MenuThemeMonochromeIce, L"Monochrome Ice");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(themeMenu), L"Theme preset");

    AppendMenuW(barColorMenu, MF_STRING, MenuBarColorCyan, L"Cyan");
    AppendMenuW(barColorMenu, MF_STRING, MenuBarColorMagenta, L"Magenta");
    AppendMenuW(barColorMenu, MF_STRING, MenuBarColorAmber, L"Amber");
    AppendMenuW(barColorMenu, MF_STRING, MenuBarColorMint, L"Mint");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(barColorMenu), L"Bar color");

    AppendMenuW(bgColorMenu, MF_STRING, MenuBgColorBlue, L"Deep Blue");
    AppendMenuW(bgColorMenu, MF_STRING, MenuBgColorViolet, L"Violet");
    AppendMenuW(bgColorMenu, MF_STRING, MenuBgColorCharcoal, L"Charcoal");
    AppendMenuW(bgColorMenu, MF_STRING, MenuBgColorForest, L"Forest");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(bgColorMenu), L"Background color");

    AppendMenuW(opacityMenu, Checked(settings.backgroundOpacity < 0.35f), MenuOpacity25, L"25%");
    AppendMenuW(opacityMenu, Checked(settings.backgroundOpacity >= 0.35f && settings.backgroundOpacity < 0.55f), MenuOpacity45, L"45%");
    AppendMenuW(opacityMenu, Checked(settings.backgroundOpacity >= 0.55f && settings.backgroundOpacity < 0.75f), MenuOpacity65, L"65%");
    AppendMenuW(opacityMenu, Checked(settings.backgroundOpacity >= 0.75f), MenuOpacity85, L"85%");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(opacityMenu), L"Overlay opacity");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MenuExit, L"Exit");

    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

bool TrayIcon::IsTrayCommand(const WPARAM command) const {
    switch (LOWORD(command)) {
    case MenuShowHide:
    case MenuToggleClickThrough:
    case MenuEnableInteractive:
    case MenuExit:
        return true;
    default:
        return false;
    }
}

AppCommand TrayIcon::CommandFromMenu(const WPARAM command) const {
    switch (LOWORD(command)) {
    case MenuShowHide:
        return AppCommand::ToggleOverlayVisibility;
    case MenuToggleClickThrough:
        return AppCommand::ToggleClickThrough;
    case MenuEnableInteractive:
        return AppCommand::EnableInteractive;
    case MenuExit:
    default:
        return AppCommand::Exit;
    }
}

bool TrayIcon::TryApplySettingsCommand(const WPARAM command, SettingsData& settings) const {
    switch (LOWORD(command)) {
    case MenuSizeSmall: settings.width = 420; settings.height = 150; return true;
    case MenuSizeMedium: settings.width = 460; settings.height = 170; return true;
    case MenuSizeLarge: settings.width = 560; settings.height = 210; return true;

    case MenuBars24: settings.barCount = 24; return true;
    case MenuBars40: settings.barCount = 40; return true;
    case MenuBars64: settings.barCount = 64; return true;
    case MenuBars96: settings.barCount = 96; return true;

    case MenuMotionCalm: settings.motionIntensity = 0.2f; return true;
    case MenuMotionBalanced: settings.motionIntensity = 0.55f; return true;
    case MenuMotionEnergetic: settings.motionIntensity = 0.95f; return true;

    case MenuThemeMinimalCyan: settings.themePreset = render::ThemePreset::MinimalCyan; return true;
    case MenuThemeNeonPurple: settings.themePreset = render::ThemePreset::NeonPurple; return true;
    case MenuThemeWarmAmber: settings.themePreset = render::ThemePreset::WarmAmber; return true;
    case MenuThemeSoftGreen: settings.themePreset = render::ThemePreset::SoftGreen; return true;
    case MenuThemeMonochromeIce: settings.themePreset = render::ThemePreset::MonochromeIce; return true;

    case MenuBarColorCyan: settings.barColor = D2D1::ColorF(0.26f, 0.72f, 0.95f, settings.barOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBarColorMagenta: settings.barColor = D2D1::ColorF(0.78f, 0.35f, 0.95f, settings.barOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBarColorAmber: settings.barColor = D2D1::ColorF(0.96f, 0.66f, 0.23f, settings.barOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBarColorMint: settings.barColor = D2D1::ColorF(0.35f, 0.89f, 0.64f, settings.barOpacity); settings.themePreset = render::ThemePreset::Custom; return true;

    case MenuBgColorBlue: settings.backgroundColor = D2D1::ColorF(0.03f, 0.05f, 0.08f, settings.backgroundOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBgColorViolet: settings.backgroundColor = D2D1::ColorF(0.09f, 0.05f, 0.12f, settings.backgroundOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBgColorCharcoal: settings.backgroundColor = D2D1::ColorF(0.09f, 0.10f, 0.11f, settings.backgroundOpacity); settings.themePreset = render::ThemePreset::Custom; return true;
    case MenuBgColorForest: settings.backgroundColor = D2D1::ColorF(0.04f, 0.10f, 0.08f, settings.backgroundOpacity); settings.themePreset = render::ThemePreset::Custom; return true;

    case MenuOpacity25: settings.backgroundOpacity = 0.25f; return true;
    case MenuOpacity45: settings.backgroundOpacity = 0.45f; return true;
    case MenuOpacity65: settings.backgroundOpacity = 0.65f; return true;
    case MenuOpacity85: settings.backgroundOpacity = 0.85f; return true;
    default:
        return false;
    }
}

} // namespace rv::app
