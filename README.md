# RanVisualizer Task 5 - Usable Overlay Utility

This repository now contains **Task 1 + Task 2 + Task 3 + Task 4 + Task 5** for the Windows audio visualizer project.

- Task 1: WASAPI loopback capture + endpoint reinitialize behavior
- Task 2: FFT/log-band analyzer with smoothing + peaks
- Task 3: smooth Direct2D spectrum renderer
- Task 4: semi-transparent topmost desktop overlay
- Task 5: tray control, hotkeys, persisted settings, and clean exit UX

## What Task 5 adds

- Reliable exit paths (no Task Manager requirement):
  - tray menu -> **Exit**
  - overlay hover close button -> **X**
  - global hotkey -> `Ctrl+Alt+Q`
- System tray integration with context menu actions:
  - Show/Hide overlay
  - Toggle click-through
  - Enable interaction
  - Exit
- Global hotkeys (app focus not required):
  - `Ctrl+Alt+V` toggle overlay visibility
  - `Ctrl+Alt+I` toggle click-through
  - `Ctrl+Alt+Q` exit app
- Lightweight persisted settings in `%APPDATA%\RanVisualizer\settings.ini`:
  - overlay position
  - overlay size
  - click-through state
  - overlay visibility
  - interactive-start preference (derived from current click-through)
- Hover-revealed close control integrated into the render path:
  - subtle rounded close button drawn in top-right
  - hidden by default
  - fades in when hovering in interactive mode
  - fades out after pointer leaves

## Interaction model

- **Interactive mode** (`click-through = off`)
  - overlay accepts mouse input
  - drag from visual area to reposition
  - hover reveals the close button
- **Click-through mode** (`click-through = on`)
  - overlay does not intercept mouse input
  - use tray menu or hotkey to return to interactive mode

This prevents the “stuck in click-through forever” failure mode.

## Rendering and responsiveness notes

Task 5 preserves the Task 4 layered Direct2D pipeline:

- `ID2D1DCRenderTarget` + premultiplied alpha DIB
- `UpdateLayeredWindow(..., ULW_ALPHA)` presentation
- existing analyzer-to-render cadence and smoothing behavior

The overlay control UI is very lightweight and rendered in the same pass, avoiding heavy UI framework overhead.

## Clean shutdown behavior

All exit paths converge on normal window destruction and process shutdown:

1. request exit command
2. destroy overlay window
3. unregister hotkeys and remove tray icon
4. persist settings
5. stop audio capture and unregister device notifications
6. quit process normally (no zombie background process)

## Architecture additions (Task 5)

- `src/app/TrayIcon.*` - tray icon lifecycle and menu command mapping
- `src/app/Hotkeys.*` - global hotkey registration/unregistration
- `src/app/Settings.*` - tiny persisted settings loader/saver
- `src/render/OverlayControls.*` - hover animation state and close-button hit test
- `src/app/AppCommands.h` - command enum used by overlay/app dispatch

## Build (Visual Studio 2022 + CMake)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output:

```text
build/Release/RanVisualizerCapture.exe
```

## Run

```powershell
.\build\Release\RanVisualizerCapture.exe
```

## Settings location

Primary path:

```text
%APPDATA%\RanVisualizer\settings.ini
```

Fallback path (if AppData resolution fails):

```text
./config/settings.ini
```

## Known limitations

- Tray icon currently uses the default application icon.
- No full preferences UI yet (intentionally out-of-scope for Task 5).
- Overlay resizing is persisted only if window size changes by code/path that updates window rect.

## Potential Task 6 focus

- richer in-overlay affordances (optional tiny control strip)
- custom tray/app icon assets
- optional snap-to-edge placement helpers
- optional startup-with-Windows + startup mode policy
