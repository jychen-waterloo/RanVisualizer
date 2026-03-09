cmake_minimum_required(VERSION 3.20)

project(RanVisualizerCaptureValidation VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RanVisualizerCapture
    src/main.cpp
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
)

if(MSVC)
    target_compile_options(RanVisualizerCapture PRIVATE /W4 /permissive-)
else()
    target_compile_options(RanVisualizerCapture PRIVATE -Wall -Wextra -Wpedantic)
endif()
