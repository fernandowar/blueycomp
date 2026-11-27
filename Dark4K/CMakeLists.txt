cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
endif()

project(Dark4K VERSION 3.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.7
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3)
endif()

juce_add_plugin(Dark4K
    COMPANY_NAME "Dark Shape Studios"
    PLUGIN_MANUFACTURER_CODE Dksh
    PLUGIN_CODE D4kc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Dark 4K"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx EQ Dynamics
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(Dark4kFonts
    HEADER_NAME "Dark4kFonts.h"
    NAMESPACE Dark4kFontData
    SOURCES
        Fonts/Michroma.ttf
        Fonts/BarlowSemiBold.ttf
        Fonts/BarlowMedium.ttf
        Fonts/LiberationSans-Regular.ttf
        Fonts/LiberationSans-Bold.ttf)

target_sources(Dark4K PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Dark4K PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Dark4K
    PRIVATE
        Dark4kFonts
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
