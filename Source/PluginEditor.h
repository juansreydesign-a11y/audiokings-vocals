#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

namespace akui
{
constexpr int kBaseW = 1520;
constexpr int kBaseH = 880;

//==============================================================================
// Skins: 0 = Digital (neon), 1 = Technical (sober), 2 = Analog (console)
struct Skin
{
    const char* name;
    int style;
    juce::Colour bgTop, bgBottom, panel, accA, accB, accC, gold, text, dim;
    juce::Colour ok, warn, hot, meterBg, knobTop, knobBot, pointer, track;
};

inline const Skin& getSkin(int id)
{
    static const Skin skins[3] = {
        { "DIGITAL", 0,
          juce::Colour(0xff070912), juce::Colour(0xff140f2a), juce::Colour(0xff0d1020),
          juce::Colour(0xff38e1ff), juce::Colour(0xff9b5cff), juce::Colour(0xffff4fd8), juce::Colour(0xfff5b83d),
          juce::Colour(0xffe8ecff), juce::Colour(0xff8b92b8),
          juce::Colour(0xff2be3b0), juce::Colour(0xffffd25a), juce::Colour(0xffff4d6d), juce::Colour(0xff0a0c16),
          juce::Colour(0xff343a5a), juce::Colour(0xff101322), juce::Colour(0xffffffff), juce::Colour(0xff222640) },
        { "TECHNICAL", 1,
          juce::Colour(0xff17191c), juce::Colour(0xff1e2125), juce::Colour(0xff23272c),
          juce::Colour(0xff5ad1a4), juce::Colour(0xff7f8da0), juce::Colour(0xffe0a458), juce::Colour(0xffd9dee5),
          juce::Colour(0xffdfe3e8), juce::Colour(0xff7d8591),
          juce::Colour(0xff5ad1a4), juce::Colour(0xffe0c35a), juce::Colour(0xffe06c5a), juce::Colour(0xff121417),
          juce::Colour(0xff3a4048), juce::Colour(0xff24282d), juce::Colour(0xffffffff), juce::Colour(0xff2e3339) },
        { "ANALOG", 2,
          juce::Colour(0xff241b15), juce::Colour(0xff120d0a), juce::Colour(0xff2b221b),
          juce::Colour(0xffffb347), juce::Colour(0xffe07b39), juce::Colour(0xffe0533d), juce::Colour(0xffe9c46a),
          juce::Colour(0xfff2e8d5), juce::Colour(0xffa39380),
          juce::Colour(0xff8fbf5a), juce::Colour(0xfff2c14e), juce::Colour(0xffe0533d), juce::Colour(0xff150f0b),
          juce::Colour(0xffefe2c4), juce::Colour(0xffb8a77f), juce::Colour(0xff1b1410), juce::Colour(0xff3a2e25) }
    };
    return skins[juce::jlimit(0, 2, id)];
}

inline juce::Font mkFont(float h, bool bold = false)
{
    return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
class AKLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AKLookAndFeel() { setSkin(getSkin(0)); }
    void setSkin(const Skin& s);
    const Skin& skin() const { return *sk; }

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                          float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;

private:
    const Skin* sk = nullptr;
};

//==============================================================================
// A labelled rotary knob bound to a parameter, with animated hover glow.
class Knob : public juce::Component
{
public:
    enum Size { Small, Medium, Big };

    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
         const juce::String& title, const juce::String& tooltip, Size size,
         double origin = std::numeric_limits<double>::quiet_NaN());

    void resized() override;
    void tick();                       // animation timer
    void setLabelColour(juce::Colour c) { label.setColour(juce::Label::textColourId, c); }
    void setValueColour(juce::Colour c) { slider.setColour(juce::Slider::textBoxTextColourId, c); }

private:
    Size size;
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

//==============================================================================
// Row/column of small buttons bound to a choice parameter (reverb type, 1176 ratio, ...)
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                  const juce::StringArray& labels, bool vertical, const juce::String& tooltip);
    void resized() override;
    void sync();

private:
    juce::RangedAudioParameter* param = nullptr;
    bool vertical;
    juce::OwnedArray<juce::TextButton> buttons;
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

//==============================================================================
// Everything the user sees. It always lays out in "base" coordinates (kBaseW x kBaseH);
// the editor scales it to the window size.
class MainView : public juce::Component, private juce::Timer
{
public:
    explicit MainView(AudiokingsVocalsProcessor&);
    ~MainView() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    // presets / skins
    void rebuildPresetBox();
    void stepPreset(int delta);
    void syncPresetBox();
    void promptSavePreset();
    void applySkin(int id);

    // data updates
    void updateSpectrum();
    void updateCurve();
    void updateVectorscope();

    // painting
    const akui::Skin& S() const { return akui::getSkin(skinId); }
    void paintBackground(juce::Graphics&);
    void paintHeader(juce::Graphics&);
    void paintPanel(juce::Graphics&, juce::Rectangle<float>, const juce::String& label);
    void paintEq(juce::Graphics&);
    void paintMeters(juce::Graphics&);
    void paintConsole(juce::Graphics&);
    void paintInputLeds(juce::Graphics&);
    void drawLevelBar(juce::Graphics&, juce::Rectangle<float>, float db, float hold);
    void drawGRBar(juce::Graphics&, juce::Rectangle<float>, float grDb, juce::Colour c1, juce::Colour c2);
    void drawGroupLabel(juce::Graphics&, juce::Rectangle<float>, const juce::String& name, const juce::String& value);

    // EQ nodes (0..4) + low-pass handle (5)
    float freqToX(float f) const;
    float getP(const char* id) const;
    void setP(const char* id, float v);
    juce::Point<float> nodePos(int i) const;
    int hitNode(juce::Point<float> p) const;
    void setNodeFromPos(int i, juce::Point<float> p);
    void setQ(int i, float q);

    AudiokingsVocalsProcessor& proc;
    akui::AKLookAndFeel laf;                       // declared first: destroyed last
    juce::TooltipWindow tooltipWindow { this, 700 };
    int skinId = 0;

    // header
    juce::ComboBox presetBox, skinBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, delBtn { "DEL" },
                     abBtn { "A" }, copyBtn { "A>B" }, lpBtn { "LOW-PASS" }, listenBtn { "LISTEN" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lpAttach, listenAttach;
    juce::StringArray userNames;

    // controls
    std::vector<std::unique_ptr<akui::Knob>> knobs;
    std::unique_ptr<akui::ChoiceButtons> ratioBtns, tcBtns, revBtns, dlyBtns, deMode;

    struct Section { juce::Rectangle<float> r; juce::String label; };
    std::vector<Section> sections;
    juce::Rectangle<float> eqPanel, meterPanel, eqPlot, ledRect, fxBar, deBar;

    // animation / metering state
    float phase = 0.f, intro = 0.f;
    akui::Meter inL, inR, outL, outR;
    float gr1176Db = 0.f, gr670Db = 0.f, grDb = 0.f, grDeEssDb = 0.f, corrSmooth = 1.f;
    static constexpr int histN = 170;
    std::array<float, histN> grHist {};
    int histPos = 0;

    // spectrum, EQ curves, vectorscope
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numPts = 320;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftData = std::vector<float>(2 * fftSize, 0.f);
    std::vector<float> specDb = std::vector<float>(numPts, -100.f);
    std::vector<float> curveDb = std::vector<float>(numPts, 0.f);
    std::vector<float> pultecDb = std::vector<float>(numPts, 0.f);
    std::vector<float> scopeL = std::vector<float>(AudiokingsVocalsProcessor::scopeSize, 0.f);
    std::vector<float> scopeR = std::vector<float>(AudiokingsVocalsProcessor::scopeSize, 0.f);
    juce::Image scopeImg;

    float specTop = -30.f;            // auto-scaled top of the spectrum display (dB)
    int hoverNode = -1, dragNode = -1;
    bool dragIsQ = false;
    float qStart = 1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

} // namespace akui

//==============================================================================
// Resizable plug-in window: hosts the MainView and scales it to the window size.
class AudiokingsVocalsEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudiokingsVocalsEditor(AudiokingsVocalsProcessor&);
    ~AudiokingsVocalsEditor() override = default;

    void paint(juce::Graphics&) override {}
    void resized() override;

private:
    AudiokingsVocalsProcessor& proc;
    akui::MainView view;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudiokingsVocalsEditor)
};
