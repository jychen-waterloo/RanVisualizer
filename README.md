# RanVisualizer Task 4 - Desktop Overlay Visualizer

This repository now contains **Task 1 + Task 2 + Task 3 + Task 4** for the Windows audio visualizer project.

- Task 1: WASAPI loopback capture + endpoint reinitialize behavior
- Task 2: FFT/log-band analyzer with smoothing + peaks
- Task 3: smooth Direct2D spectrum renderer
- Task 4 (this milestone): borderless topmost semi-transparent desktop overlay

## What Task 4 adds

- Replaces the old normal resizable window with a compact overlay shell.
- Overlay window is borderless (`WS_POPUP`), topmost, and tool-window styled.
- Default placement is bottom-right with margin from work area edges.
- Renderer now targets a premultiplied-alpha layered window path.
- Transparent canvas + subtle rounded backdrop plate to keep readability on mixed wallpapers.
- Drag-to-reposition behavior while interactive.
- Minimal click-through toggle for testing:
  - `F8` global poll toggle (works even when click-through is active)
  - right-click context menu toggle when interactive

## Rendering/composition path used

Task 4 keeps the Task 3 bar/peak animation logic and moves presentation to a **layered-window composition path**:

- Direct2D `ID2D1DCRenderTarget` draws into a 32-bit premultiplied DIB.
- The frame is presented with `UpdateLayeredWindow(..., ULW_ALPHA)`.
- Fully transparent clear color is used each frame to avoid opaque black artifacts.

This is stable on Windows 10/11 and preserves smooth animation while providing true alpha-composited overlay behavior.

## Interaction model (Task 4 scope)

- Starts in **interactive mode** (not click-through).
- Drag overlay from anywhere in the visual area.
- Toggle click-through for testing with `F8`.
- Toggle is reversible without killing the app.

No tray icon/global hotkeys/settings UI are introduced in this task.

## Architecture overview

- `src/app/App.*` - process lifetime, COM/audio setup, frame loop
- `src/app/OverlayWindow.*` - overlay window creation, drag/hit testing, click-through state
- `src/render/Renderer.*` - layered render target creation, frame draw, alpha presentation
- `src/platform/WindowStyles.*` - overlay style constants and click-through style switching

Task 1 capture and Task 2 analyzer integration remain unchanged and continue feeding the renderer via latest-frame snapshots.

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

## Current limitations (before Task 5)

- No tray icon yet
- No global hotkey registration system yet
- No formal config/settings persistence yet

## What Task 5 will add next

Task 5 will build on this overlay foundation with tray controls, polished toggle UX, and structured persistence/configuration.
