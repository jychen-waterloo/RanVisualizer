# RanVisualizer - Triangle Tunnel Mode Milestone

This milestone extends the existing Windows audio visualizer overlay with a **new selectable render mode**:

- **Classic Bars** (existing mode, unchanged)
- **Synthwave Triangle Tunnel** (new mode)

The app still uses the same WASAPI + analyzer pipeline, tray menu, overlay controls, and settings persistence.

## What's new

- Added a new render mode: **Synthwave Triangle Tunnel**.
- Added tray menu mode switching under **Visualizer mode**.
- Added settings persistence for selected visualizer mode (`visualizerMode`).
- Kept the existing classic bars path fully intact.

## Triangle Tunnel render approach

The new mode is implemented as a **lightweight fake-3D perspective scene** in the existing Direct2D rendering stack:

1. Repeating triangle frames are distributed across virtual depth.
2. Depth is advanced each frame to create forward tunnel movement.
3. Frames are projected with simple perspective scaling (no heavy 3D engine).
4. Interior streak lines are projected and recycled for motion texture.
5. Neon color layering is achieved with multiple line passes (glow + core accent).

No webview/browser rendering and no external 3D runtime were introduced.

## Audio-reactive mapping

The tunnel mode maps analyzer outputs as follows:

- **Bass energy**
  - boosts tunnel pulse/scale (frame expansion)
  - increases forward movement speed
  - adds slight center thump drift

- **Mid energy**
  - increases tunnel drift activity
  - increases streak count/density
  - influences rotational motion feel

- **Treble energy**
  - increases streak shimmer/flicker
  - brightens thin accent highlights

- **Overall loudness**
  - increases overall glow/alpha intensity
  - increases general scene energy

This keeps the scene coherent and musically driven rather than random.

## How to switch modes

1. Right-click the tray icon.
2. Open **Visualizer mode**.
3. Select either:
   - **Classic Bars**
   - **Synthwave Triangle Tunnel**

Switching is immediate and does not require app restart.

## Settings persistence

Settings are persisted as JSON at:

```text
%APPDATA%\RanVisualizer\settings.json
```

(With existing fallback to `./config/settings.json` if needed.)

The selected mode is saved in:

- `visualizerMode` (`"ClassicBars"` or `"TriangleTunnel"`)

## Overlay behavior and compatibility

The new mode preserves existing overlay behavior:

- layered transparent overlay rendering
- click-through / interactive switching
- tray + hotkeys
- hover close button
- resize behavior and persisted size/position

It adapts to different aspect ratios and remains suitable for resizable overlay windows.

## First-version limitations

This first implementation intentionally focuses on a lightweight core scene:

- repeating triangle frames with perspective
- reactive speed/pulse/glow
- interior streak lines

Not yet included in this milestone:

- advanced particle systems
- complex camera paths
- multi-pass bloom post-processing

## Build (Visual Studio 2022 + CMake)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output:

```text
build/Release/RanVisualizerCapture.exe
```
