# RanVisualizer Task 3 - Windowed Direct2D Spectrum Renderer

This repository now contains **Task 1 + Task 2 + Task 3** for the Windows audio visualizer project.

- Task 1: WASAPI loopback capture + default-output-device reinitialize handling.
- Task 2: FFT/log-band analyzer with smoothing + peaks + summary energies.
- Task 3 (this milestone): a polished **normal Win32 resizable desktop window** rendered with **Direct2D**.

> This is intentionally **not** the final transparent corner overlay yet.

## What Task 3 adds

- Standard overlapped Win32 window (`Audio Visualizer - Task 3`)
- Per-monitor DPI-aware desktop app shell
- Dedicated render loop targeting smooth frame updates (~60 FPS feel)
- Direct2D renderer with:
  - dark background
  - rounded vertical bars from Task 2 smoothed log bands
  - peak caps with soft decay
  - subtle glow underlay
  - calm silence behavior
- Optional lightweight debug overlay text (FPS/loudness/silence/band count)
- Clean modular split between app lifecycle, windowing, layout, animation state, and rendering theme

## Why this is still a normal window

Task 3 is a quality checkpoint for animation and visual design in a robust renderer path. It deliberately avoids overlay-specific behavior:

- no transparent layered window behavior
- no always-on-top
- no click-through
- no tray icon
- no hotkeys

Those are deferred to Task 4.

## Rendering architecture overview

- `src/app/App.*`
  - process DPI setup
  - COM lifetime
  - capture/notifier startup and shutdown
  - message pump + frame pacing
- `src/app/MainWindow.*`
  - Win32 class/window creation
  - resize/destroy handling
  - fetch latest analyzer snapshot each frame
- `src/render/Renderer.*`
  - Direct2D/DirectWrite resource management
  - draw background, bars, peaks, glow, debug text
  - handle `D2DERR_RECREATE_TARGET`
- `src/render/Layout.*`
  - responsive bar geometry for current client size
- `src/render/AnimationState.*`
  - frame-to-frame interpolation/easing for smooth motion
- `src/render/RenderTypes.h`
  - render snapshot/timing model

## How renderer consumes analyzer output

`LoopbackCapture` now publishes the most recent `AnalysisFrame`. The render thread reads this latest snapshot per frame and converts it into a render model:

- primary bars: `smoothedBandValues`
- secondary peak caps: `peakBandValues`
- accent response: `loudness + bassEnergy`
- silence calming: `isSilentLike` influences decay

No raw FFT bins are rendered directly.

## Resize and frame update behavior

- Render target is resized on `WM_SIZE`.
- Bar layout is rebuilt from current client size each frame (no stretched bitmap caching).
- Message pump is decoupled from analysis cadence.
- Renderer runs continuously with small sleep-based pacing and interpolated animation state.
- Minimized windows skip rendering work.

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

## Known limitations

- Renderer currently targets a normal window only (no translucent overlay behavior yet).
- No settings UI/config surface yet.
- Debug overlay is compile-time behavior in app wiring (easy to toggle in code).

## What Task 4 will add

Task 4 will reuse this renderer/app structure and introduce overlay behavior (transparent layered presentation, topmost/click-through policy, and related windowing mechanics) without replacing the Task 2 analysis pipeline.
