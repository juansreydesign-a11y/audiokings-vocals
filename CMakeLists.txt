cmake_minimum_required(VERSION 3.22)

# macOS: universal binary (Apple Silicon + Intel), macOS 10.13 or newer
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

# Static MSVC runtime: the plugin runs on any Windows PC without installing the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(AudiokingsVocals VERSION 3.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# Windows: VST3.  macOS: VST3 + Audio Unit (Logic Pro, GarageBand, Live, FL Studio, Reaper...)
if(APPLE)
    set(AK_FORMATS VST3 AU)
else()
    set(AK_FORMATS VST3)
endif()

juce_add_plugin(AudiokingsVocals
    COMPANY_NAME "Audiokings"
    PLUGIN_MANUFACTURER_CODE Akng
    PLUGIN_CODE Akvc
    FORMATS ${AK_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "Audiokings Vocals"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Dynamics EQ
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AudiokingsVocals PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditorPages.cpp
    Source/Dynamics.h
    Source/PluginEditor.h
    Source/Presets.h
    Source/Params.h
    Source/Biquad.h)

target_compile_definitions(AudiokingsVocals PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AudiokingsVocals
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
