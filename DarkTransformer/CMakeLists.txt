cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
endif()

project(DarkTransformer VERSION 1.0.0)

# NeuralAmpModelerCore usa C++20
set(CMAKE_CXX_STANDARD 20)
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

# Núcleo de Neural Amp Modeler (licencia MIT), fijado a un commit probado.
# Trae Eigen y AudioDSPTools (resampler) como submódulos.
FetchContent_Declare(
    namcore
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG d3189ed482ded880c128bf312941266c4fa5ceec
    GIT_SUBMODULES "Dependencies/eigen" "Dependencies/AudioDSPTools"
    GIT_SUBMODULES_RECURSE FALSE
    # Solo queremos el código fuente, no su CMakeLists (herramientas y tests)
    SOURCE_SUBDIR "_no_cmake_"
)
FetchContent_MakeAvailable(JUCE namcore)

# Eigen: el del submódulo de NAM, salvo que se indique otro (DT_EIGEN_DIR)
set(DT_EIGEN_DIR "${namcore_SOURCE_DIR}/Dependencies/eigen" CACHE PATH "Carpeta de Eigen")

#==============================================================================
# Motor neural (sin los warnings estrictos de JUCE)
file(GLOB NAM_SOURCES
    "${namcore_SOURCE_DIR}/NAM/*.cpp"
    "${namcore_SOURCE_DIR}/NAM/wavenet/*.cpp")

# OBJECT (no STATIC): cada arquitectura se registra con un inicializador estático
# y el enlazador descartaría esos archivos si fuera una librería estática
add_library(namcore OBJECT ${NAM_SOURCES})
target_include_directories(namcore SYSTEM PUBLIC
    "${namcore_SOURCE_DIR}"
    "${namcore_SOURCE_DIR}/NAM"
    "${namcore_SOURCE_DIR}/Dependencies/nlohmann"
    "${namcore_SOURCE_DIR}/Dependencies/AudioDSPTools/dsp"
    "${DT_EIGEN_DIR}")
target_compile_definitions(namcore PUBLIC NAM_SAMPLE_FLOAT NAM_ENABLE_A2_FAST)
set_target_properties(namcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(WIN32)
    target_compile_definitions(namcore PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
    target_compile_options(namcore PRIVATE /O2 /bigobj /W0)
else()
    target_compile_options(namcore PRIVATE -O3 -w)
endif()

#==============================================================================
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3)
endif()

juce_add_plugin(DarkTransformer
    COMPANY_NAME "Dark Shape Studios"
    PLUGIN_MANUFACTURER_CODE Dksh
    PLUGIN_CODE Dtrf
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Dark Transformer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Recursos: tipografías + (si existe) la captura del preamp en Models/*.nam
set(DT_RESOURCES
    Fonts/Michroma.ttf
    Fonts/BarlowSemiBold.ttf
    Fonts/BarlowMedium.ttf)

file(GLOB DT_MODELS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/Models/*.nam")
list(SORT DT_MODELS)
if(DT_MODELS)
    list(GET DT_MODELS 0 DT_MODEL)
    message(STATUS "Dark Transformer: captura incluida -> ${DT_MODEL}")
    list(APPEND DT_RESOURCES ${DT_MODEL})
    get_filename_component(DT_MODEL_NAME "${DT_MODEL}" NAME_WE)
    target_compile_definitions(DarkTransformer PRIVATE
        DT_EMBEDDED_MODEL=1
        DT_EMBEDDED_MODEL_NAME="${DT_MODEL_NAME}")
else()
    message(STATUS "Dark Transformer: sin captura en Models/, se usa el modelo DSP de respaldo")
endif()

juce_add_binary_data(DarkTransformerData
    HEADER_NAME "DarkTransformerData.h"
    NAMESPACE DarkTransformerData
    SOURCES ${DT_RESOURCES})

target_sources(DarkTransformer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TransformerModel.cpp)

target_compile_definitions(DarkTransformer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DarkTransformer
    PRIVATE
        DarkTransformerData
        namcore
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
