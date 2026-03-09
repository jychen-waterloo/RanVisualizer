# RanVisualizer - Configuration Milestone

This milestone adds practical live configuration to the existing Windows overlay visualizer while preserving:

- WASAPI loopback capture
- analyzer pipeline
- smooth layered overlay rendering
- tray integration
- hotkeys
- hover close button

## New user-facing settings

All of the following are now configurable and persisted:

- **Window size** (small / medium / large presets)
- **Theme preset**
  - Minimal Cyan
  - Neon Purple
  - Warm Amber
  - Soft Green
  - Monochrome Ice
- **Bar color** (manual palette path)
- **Background color** (manual palette path)
- **Overlay/background opacity**
- **Spectrum bar count** (24 / 40 / 64 / 96)
- **Motion intensity** (Calm / Balanced / Energetic)

## How to change settings

Use the **system tray icon** (right click) and the compact submenus:

- Window size
- Spectrum bars
- Motion feel
- Theme preset
- Bar color
- Background color
- Overlay opacity

Most options apply **live** immediately.

## Persistence and config file

Settings are persisted as JSON.

Primary path:

```text
%APPDATA%\RanVisualizer\settings.json
```

Fallback path (if AppData resolution fails):

```text
./config/settings.json
```

If the config is missing or partially invalid, safe defaults are used.

## Motion intensity mapping (real behavior, not random)

`motionIntensity` drives a runtime motion profile used by the renderer:

- higher intensity -> faster attack
- higher intensity -> faster release
- higher intensity -> slower visible peak drop
- higher intensity -> more visual gain
- higher intensity -> extra low-end emphasis
- higher intensity -> faster accent response

This makes low intensity calm/smooth and high intensity punchier without adding random jitter.

## Bar count remapping strategy

Analyzer output is remapped to the selected visible bar count using **weighted interval sampling** across source bands.

This avoids harsh drop/duplicate artifacts and keeps spacing/energy distribution intentional for both low and high bar counts.

## Build (Visual Studio 2022 + CMake)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output:

```text
build/Release/RanVisualizerCapture.exe
```

## Known limitations

- Settings UI is intentionally tray-driven (compact) rather than a full dialog window.
- Color selection uses curated palette options (plus preset themes) instead of a full color picker.
- Linux CI/build environments without Windows SDK cannot compile this target (Windows native app).
