cmake_minimum_required(VERSION 3.20)

project(RanVisualizerCaptureValidation VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RanVisualizerCapture WIN32
    src/main.cpp
    src/app/App.h
    src/app/App.cpp
    src/app/OverlayWindow.h
    src/app/OverlayWindow.cpp
    src/app/AppCommands.h
    src/app/Hotkeys.h
    src/app/Hotkeys.cpp
    src/app/TrayIcon.h
    src/app/TrayIcon.cpp
    src/app/Settings.h
    src/app/Settings.cpp
    src/render/Renderer.h
    src/render/Renderer.cpp
    src/render/RenderTypes.h
    src/render/RenderConfig.h
    src/render/RenderConfig.cpp
    src/render/Layout.h
    src/render/OverlayControls.h
    src/render/OverlayControls.cpp
    src/render/Layout.cpp
    src/render/Theme.h
    src/render/Theme.cpp
    src/render/AnimationState.h
    src/render/AnimationState.cpp
    src/platform/WindowStyles.h
    src/platform/WindowStyles.cpp
    src/audio/LoopbackCapture.cpp
    src/audio/LoopbackCapture.h
    src/audio/DeviceNotifier.cpp
    src/audio/DeviceNotifier.h
    src/audio/WaveFormatUtil.cpp
    src/audio/WaveFormatUtil.h
    src/dsp/AnalysisFrame.h
    src/dsp/Window.cpp
    src/dsp/Window.h
    src/dsp/Fft.cpp
    src/dsp/Fft.h
    src/dsp/BandMapper.cpp
    src/dsp/BandMapper.h
    src/dsp/Smoothing.cpp
    src/dsp/Smoothing.h
    src/dsp/SpectrumAnalyzer.cpp
    src/dsp/SpectrumAnalyzer.h
    src/util/ComUtil.cpp
    src/util/ComUtil.h
    src/util/Logger.cpp
    src/util/Logger.h
)

target_compile_definitions(RanVisualizerCapture PRIVATE UNICODE _UNICODE NOMINMAX)

target_link_libraries(RanVisualizerCapture PRIVATE
    ole32
    avrt
    uuid
    d2d1
    dwrite
    shcore
    shell32
)

if(MSVC)
    target_compile_options(RanVisualizerCapture PRIVATE /W4 /permissive-)
else()
    target_compile_options(RanVisualizerCapture PRIVATE -Wall -Wextra -Wpedantic)
endif()
