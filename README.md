# RanVisualizer Task 1 - WASAPI Loopback Capture Validation

This repository contains **Task 1 only** for a Windows desktop audio visualizer project.

## What this task validates

This console application validates the **audio capture layer** by capturing system output audio from the current default playback endpoint using WASAPI loopback.

It validates:
- opening the default render endpoint (not microphone capture)
- reading packetized loopback audio continuously
- handling silent packets correctly
- computing simple runtime levels (RMS + peak)
- reacting to default playback device changes at runtime
- clean shutdown and robust logging

It intentionally does **not** include UI, FFT, rendering, overlays, tray icon, hotkeys, or config UI.

## Prerequisites

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 with C++ desktop workload
- CMake 3.20+

## Build (CMake + Visual Studio 2022)

From a Developer PowerShell / Command Prompt:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output (default):

```text
build/Release/RanVisualizerCapture.exe
```

## Run

```powershell
.\build\Release\RanVisualizerCapture.exe
```

Press `Ctrl+C` to stop.

## Expected console output

Startup logs include:
- `Opened default playback device`
- device friendly name
- endpoint id
- format summary (sample rate, channels, bit depth, block align, avg bytes/sec, subtype)
- buffer size and device period

Runtime logs every ~500 ms include counters and levels:

```text
[t=12.5s] packets=1234 frames=543210 rms=0.0321 peak=0.1842 silent=no fps_audio=48000
```

If system audio is silent, logs continue with low/zero RMS/peak and `silent=yes` where appropriate.

## Manual test plan

1. Start the app.
2. Start system playback (music/video/browser audio).
3. Verify RMS/peak rise and vary with audio content.
4. Pause playback; verify RMS/peak drop and silence is still reported as healthy capture.
5. Change the Windows default playback device (e.g. speakers -> headset).
6. Verify logs show default device change and successful reinitialization without app restart.

## Known limitations

- Timer-driven polling is used for reliability/simplicity in this validation task (no event-driven mode yet).
- The level estimator currently handles common loopback mix formats:
  - IEEE float 32-bit
  - PCM 16-bit
  - PCM 24-bit in 32-bit container
- Other uncommon formats are logged and skipped for level estimation.
- This is capture-layer validation only; no visualization pipeline exists yet.
