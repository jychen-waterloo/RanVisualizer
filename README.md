# RanVisualizer - Resize Refinement Milestone

This milestone extends the existing Windows audio visualizer overlay with **borderless edge/corner resizing** while preserving:

- WASAPI loopback capture
- analyzer pipeline
- smooth layered overlay rendering
- tray integration
- hotkeys
- hover close button
- existing theme/config system

## What's new

- The borderless overlay can now be resized from all edges and corners in **interactive mode**.
- Width and height are persisted in the existing settings file and restored on startup.
- Renderer layout updates live while resizing (bars, backdrop, and controls relayout to current client size).

## Resize behavior

- **Interactive mode**: resize hit-testing is enabled and works from:
  - left / right / top / bottom edges
  - top-left / top-right / bottom-left / bottom-right corners
- **Click-through mode**: resize interaction is disabled (window remains non-interactive).
- Dragging the non-control body still moves the overlay.
- Hover close button keeps priority over resize hit regions near top-right control area.

## Layout and spectrum semantics

Resize changes only presentation/layout:

- configured bar count remains fixed
- analyzer output meaning remains unchanged
- normalized band values remain normalized
- final bar pixel height is `normalizedValue * currentDrawableHeight`

So a taller window gives more pixel range without changing normalized amplitude semantics.

## Size constraints

To keep the overlay practical and polished during borderless resizing:

- minimum width: **280 px**
- minimum height: **120 px**
- maximum height: **900 px**

(Width upper bounds are additionally clamped by config loading logic.)

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

Persisted size fields:

- `width`
- `height`

If the config is missing or partially invalid, safe defaults and clamped values are used.

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

- Resize interaction is intentionally disabled in click-through mode.
- No automatic bar-count scaling based on width in this milestone (bar count stays user-configured).
- Linux CI/build environments without Windows SDK cannot compile this target (Windows native app).
