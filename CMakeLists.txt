cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime: the plugin runs on any Windows PC without installing the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(AudiokingsVocals VERSION 3.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AudiokingsVocals
    COMPANY_NAME "Audiokings"
    PLUGIN_MANUFACTURER_CODE Akng
    PLUGIN_CODE Akvc
    FORMATS VST3
    PRODUCT_NAME "Audiokings Vocals"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AudiokingsVocals PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
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
