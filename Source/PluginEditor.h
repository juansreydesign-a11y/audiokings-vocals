#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

namespace akui
{
inline const juce::Colour bg1  { 0xff070912 };
inline const juce::Colour bg2  { 0xff140f2a };
inline const juce::Colour accA { 0xff38e1ff };   // cyan
inline const juce::Colour accB { 0xff9b5cff };   // violet
inline const juce::Colour accC { 0xffff4fd8 };   // magenta
inline const juce::Colour gold { 0xfff5b83d };
inline const juce::Colour txt  { 0xffe8ecff };
inline const juce::Colour dim  { 0xff8b92b8 };

inline juce::Font mkFont(float h, bool bold = false)
{
    return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
class AKLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AKLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                          float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
// A labelled rotary knob bound to a parameter, with animated hover glow.
class Knob : public juce::Component
{
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
         const juce::String& title, double origin = std::numeric_limits<double>::quiet_NaN());

    void resized() override;
    void tick();   // called from the editor's animation timer

private:
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

struct Meter
{
    float db = -100.f, hold = -100.f;
    int holdCount = 0;

    void update(float target)
    {
        db = target > db ? target : std::max(target, db - 1.0f);
        if (target >= hold) { hold = target; holdCount = 50; }
        else if (holdCount > 0) --holdCount;
        else hold = std::max(target, hold - 0.8f);
    }
};
} // namespace akui

//==============================================================================
class AudiokingsVocalsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudiokingsVocalsEditor(AudiokingsVocalsProcessor&);
    ~AudiokingsVocalsEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepPreset(int delta);
    void updateSpectrum();
    void updateCurve();

    void paintBackground(juce::Graphics&);
    void paintHeader(juce::Graphics&);
    void paintPanel(juce::Graphics&, juce::Rectangle<float>);
    void paintEq(juce::Graphics&);
    void paintMeters(juce::Graphics&);
    void drawLevelBar(juce::Graphics&, juce::Rectangle<float>, float db, float hold);
    void drawGRBar(juce::Graphics&, juce::Rectangle<float>, float grDb);
    void drawGroupLabel(juce::Graphics&, juce::Rectangle<float>, const juce::String& name, const juce::String& value);

    float freqToX(float f) const;

    AudiokingsVocalsProcessor& proc;
    akui::AKLookAndFeel laf;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    std::vector<std::unique_ptr<akui::Knob>> knobs;

    juce::Rectangle<float> eqPanel, meterPanel, rowPanel1, rowPanel2, eqPlot;

    // animation / metering state
    float phase = 0.f, intro = 0.f;
    akui::Meter inL, inR, outL, outR;
    float grDb = 0.f, corrSmooth = 1.f;

    // spectrum + EQ curve
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numPts = 256;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftData = std::vector<float>(2 * fftSize, 0.f);
    std::vector<float> specDb = std::vector<float>(numPts, -100.f);
    std::vector<float> curveDb = std::vector<float>(numPts, 0.f);
    float bandGain[4] = { 0, 0, 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudiokingsVocalsEditor)
};
