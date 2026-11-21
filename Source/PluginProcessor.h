#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>
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
    double getTailLengthSeconds() const override { return 5.0; }

    // Presets live inside the plug-in (not exposed as host "programs"), so Cubase / Reaper / FL / Live
    // can never re-apply a preset over the user's settings when a project is loaded.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Audiokings Vocals"; }
    void changeProgramName(int, const juce::String&) override {}

    int getCurrentPreset() const;                 // factory preset index, -1 when a user preset is active
    void loadFactoryPreset(int index);

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- preset / snapshot helpers (message thread) ----
    ak::Values getValues() const;
    void setValues(const ak::Values& v);

    static juce::File getUserPresetDir();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::String& name);
    bool deleteUserPreset(const juce::String& name);
    juce::String getCurrentUserPreset() const;     // empty when a factory preset is active

    int getABSlot() const;
    void toggleAB();
    void copyToOtherAB();

    // ---- meter data (written by the audio thread, read by the editor) ----
    std::atomic<float> meterInL { 0.f }, meterInR { 0.f };
    std::atomic<float> meterOutL { 0.f }, meterOutR { 0.f };
    std::atomic<float> meterGR1176 { 0.f }, meterGR670 { 0.f }, meterGRTotal { 0.f }, meterGRDeEss { 0.f };   // dB, positive
    std::atomic<float> correlation { 1.f };

    static constexpr int scopeSize = 2048;
    void copyScope(float* destL, float* destR) const;
    double getCurrentSampleRate() const { return sr; }

private:
    struct Ptrs
    {
        std::atomic<float> *sens, *noise, *compress, *w1176, *w670, *ratio, *tc;
        std::atomic<float> *pbass, *pbassatt, *pbassf, *ptreb, *ptrebatt, *ptrebf;
        std::atomic<float> *eqg[5], *eqf[5], *eqq[5], *lpon, *lpf;
        std::atomic<float> *airlow, *airhigh, *fx;
        std::atomic<float> *revtype, *revtone, *decay, *revmix;
        std::atomic<float> *dlydiv, *delay, *delaytone, *delaymix;
        std::atomic<float> *output, *hpf, *attack, *release, *deess, *deessf, *deessr, *deessq, *deessmode, *deesslisten;
    } P {};

    void updateFilters(const ak::EqParams& e, const ak::PultecParams& p);

    // filters
    ak::Biquad hpf[2], eq[5][2], lp1[2], lp2[2];
    ak::Biquad pBassB[2], pBassA[2], pTrebB[2], pTrebA[2];
    ak::Biquad deessBp[2];                                      // de-esser detector / split band
    // Air exciter: the harmonics are generated from a band that does NOT overlap the output band,
    // so the added signal can never cancel the original voice
    ak::Biquad aLoSrc[2], aLoLp[4][2], aLoOut[2], aLoOut2[2], aLoPeak[2];     // low
    ak::Biquad aHiSrc[2], aHiLp[4][2], aHiOut[2], aHiOut2[2], aHiShelf[2];    // high
    float aLoEnv[2] { 0.f, 0.f }, aHiEnv[2] { 0.f, 0.f };
    ak::Biquad revHp[2], wetHs[2];                              // reverb input / wet tone

    // dynamics state
    float gateDb = 0.f, gateEnv = 0.f;
    float gr1176 = 0.f, mk1176 = 0.f;
    float gr670F = 0.f, gr670S = 0.f, mk670 = 0.f;
    float deessEnv = 0.f, pcGr = 0.f;
    float lastInGain = 1.f, lastOutGain = 1.f;

    // FX (chorus / flanger / distortion lowpass)
    std::vector<float> chBuf[2], flBuf[2];
    int chWp = 0, flWp = 0;
    float chPh = 0.f, flPh = 0.f, distLp[2] { 0.f, 0.f };

    // delay + reverb pre-delay
    std::vector<float> dlyBuf[2], preBuf[2];
    int dlyWp = 0, preWp = 0;
    float dlyLp[2] { 0.f, 0.f };

    juce::Reverb reverb, chamber;
    juce::AudioBuffer<float> reverbBuf, dlyOut, chamBuf;

    double sr = 44100.0;
    bool filtersValid = false;
    std::array<float, 18> lastEq {};
    float lastDeF = -1.f, lastDeQ = -1.f, lastAirLo = -1.f, lastAirHi = -1.f;
    std::array<float, 6> lastPultec {};
    float lastRevTone = -1.f;

    std::array<float, scopeSize> scopeL {}, scopeR {};
    std::atomic<int> scopePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudiokingsVocalsProcessor)
};
