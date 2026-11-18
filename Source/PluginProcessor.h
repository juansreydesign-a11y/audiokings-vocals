#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include "Presets.h"
#include "Biquad.h"

class AudiokingsVocalsProcessor : public juce::AudioProcessor
{
public:
    AudiokingsVocalsProcessor();
    ~AudiokingsVocalsProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Audiokings Vocals"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // Presets are exposed to the host as "programs"
    int getNumPrograms() override { return ak::numPresets(); }
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- Meter data (written by audio thread, read by the editor) ----
    std::atomic<float> meterInL { 0.f }, meterInR { 0.f };
    std::atomic<float> meterOutL { 0.f }, meterOutR { 0.f };
    std::atomic<float> meterGR { 0.f };        // gain reduction in dB (positive number)
    std::atomic<float> correlation { 1.f };    // -1 .. +1

    static constexpr int scopeSize = 2048;
    void copyScope(float* dest) const;

private:
    void updateFilters(float hp, float lo, float lm, float pr, float ai);

    std::array<std::atomic<float>*, ak::kNumParams> paramPtr {};

    ak::Biquad hpf[2], lowShelf[2], lowMid[2], pres[2], air[2], deessHp[2];
    juce::Reverb reverb;
    juce::AudioBuffer<float> reverbBuf;

    double sr = 44100.0;
    float grState = 0.f, makeupCur = 0.f, deessEnv = 0.f;
    float lastInGain = 1.f, lastOutGain = 1.f;

    bool eqValid = false;
    std::array<float, 5> lastEq {};

    std::array<float, scopeSize> scope {};
    std::atomic<int> scopePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudiokingsVocalsProcessor)
};
