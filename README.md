# RanVisualizer Task 2 - Loopback Capture + DSP Analysis Validation

This repository now contains **Task 1 + Task 2** for the Windows audio visualizer project.

Task 1 remains the same: robust WASAPI loopback capture from the default playback endpoint with device-change recovery.

Task 2 adds a production-oriented **DSP analysis pipeline** that turns captured PCM into smooth, log-band, visualization-ready spectrum data (console-only validation, no UI/rendering yet).

## Task 2 additions

- Input normalization and channel combine to normalized mono float
- Rolling buffered framing decoupled from WASAPI packet sizes
- Hann-windowed short-time FFT analysis
- Log-spaced spectral band mapping (default: 64 bands)
- Dynamic conditioning (noise-floor gate + mild compression + conservative adaptive gain)
- Temporal smoothing with separate attack/release
- Per-band peak tracking with graceful decay
- Summary energies (bass / mid / treble / loudness)
- Compact periodic analysis diagnostics in console

## Defaults used

- FFT size: `2048`
- Hop size: `1024` (50% overlap)
- Band count: `64`
- Band range: `30 Hz` to `16 kHz` (clamped by Nyquist)
- Smoothing: fast attack + slower release
- Peak tracking: immediate rise + exponential decay
- Noise floor gate: about `-72 dB`

These defaults were chosen to balance responsiveness, stability, and CPU cost for real-time desktop loopback.

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

Press `Ctrl+C` to exit.

## Console output meaning

You will see two periodic streams:

1. Capture diagnostics (`[capture ...]`)
   - packet/frame counters
   - RMS + peak
   - silent packet indicator
   - capture throughput estimate

2. DSP diagnostics (`[analysis]`)
   - `loud`: overall loudness proxy
   - `bass/mid/treb`: summary energies from smoothed log bands
   - `silent`: silence-like state after gating/smoothing
   - `bars`: compact ASCII view of selected smoothed bands

Example:

```text
[analysis] loud=0.241 bass=0.372 mid=0.229 treb=0.104 silent=no bars=.-==++*##%%#*+=-
```

## Validation guidance

- Play bass-heavy content: bass energy and lower bars should respond more strongly.
- Play speech/mid-focused content: mid should dominate.
- Pause playback/silence: values should settle smoothly with minimal flicker.
- Change default playback device while running: capture should reinitialize and analysis should continue.

## Architecture notes

- Capture and DSP are decoupled with a queue so WASAPI packet timing does not directly define FFT frames.
- FFT frames are built from a rolling sample buffer (`fftSize`, `hopSize`) independent of packet boundaries.
- No per-frame FFT/window allocation; reusable buffers and precomputed Hann coefficients are used.

## Known limitations

- Current analyzer output is console diagnostics only; no renderer/UI yet.
- Optional onset/pulse hint is not implemented in this task.
- Format support mirrors Task 1 practical formats (float32, pcm16, 24-in-32).

## What comes in Task 3

Task 3 will consume the existing smoothed/peak band model and build the actual rendering layer (window/overlay/visualizer presentation), while reusing this DSP pipeline unchanged.
