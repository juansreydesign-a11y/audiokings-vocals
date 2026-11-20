#include "PluginEditor.h"

using namespace akui;

namespace
{
const char* const kGainIds[4] = { "bass", "mud", "treble", "air" };
const char* const kFreqIds[4] = { "bassf", "mudf", "trebf", "airf" };
const char* const kNodeNames[4] = { "BASS", "LOW-MID", "TREBLE", "AIR" };
const float kFreqLo[4] = { 40.f, 150.f, 1500.f, 6000.f };
const float kFreqHi[4] = { 400.f, 1000.f, 8000.f, 18000.f };
const juce::uint32 kNodeCol[4] = { 0xff38e1ff, 0xff9b5cff, 0xffff4fd8, 0xfff5b83d };
}

//==============================================================================
// Look and feel
//==============================================================================
AKLookAndFeel::AKLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff161a2e));
    setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.12f));
    setColour(juce::ComboBox::textColourId, txt);
    setColour(juce::ComboBox::arrowColourId, accA);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff111427));
    setColour(juce::PopupMenu::textColourId, txt);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accB.withAlpha(0.45f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId, gold);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a2e));
    setColour(juce::TextButton::textColourOffId, accA);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Label::textColourId, dim);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff111427));
    setColour(juce::TooltipWindow::textColourId, txt);
    setColour(juce::TooltipWindow::outlineColourId, accB);
}

void AKLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                         bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.f);
    if (b.getToggleState())
    {
        g.setColour(accA.withAlpha(0.22f));
        g.fillRoundedRectangle(r.expanded(2.f), 9.f);
        juce::ColourGradient gr(accB, r.getX(), r.getY(), accA, r.getRight(), r.getBottom(), false);
        g.setGradientFill(gr);
        g.fillRoundedRectangle(r, 7.f);
    }
    else
    {
        g.setColour(juce::Colour(0xff161a2e).brighter(down ? 0.3f : (highlighted ? 0.15f : 0.f)));
        g.fillRoundedRectangle(r, 7.f);
        g.setColour(juce::Colours::white.withAlpha(highlighted ? 0.25f : 0.12f));
        g.drawRoundedRectangle(r, 7.f, 1.f);
    }
}

void AKLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                     float sa, float ea, juce::Slider& s)
{
    auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float hov = (float) s.getProperties().getWithDefault("hover", 0.0f);
    const bool big = (bool) s.getProperties().getWithDefault("big", false);
    const float arcR = r - 4.f;
    const float inner = arcR - (big ? 12.f : 9.f);
    const float ang = sa + pos * (ea - sa);

    // origin of the value arc (centre for bipolar knobs)
    float originPos = 0.f;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        originPos = (float) s.valueToProportionOfLength(0.0);
    else if (s.getProperties().contains("origin"))
        originPos = (float) s.valueToProportionOfLength((double) s.getProperties()["origin"]);
    const float angO = sa + originPos * (ea - sa);

    // soft glow behind the knob
    {
        juce::ColourGradient glow(accA.withAlpha(0.08f + 0.20f * hov + (big ? 0.06f : 0.f)), c.x, c.y,
                                  accA.withAlpha(0.f), c.x, c.y - r - 12.f, true);
        g.setGradientFill(glow);
        g.fillEllipse(c.x - r - 12.f, c.y - r - 12.f, (r + 12.f) * 2.f, (r + 12.f) * 2.f);
    }

    const float thick = big ? 6.f : 4.f;

    // tick marks around the big knob
    if (big)
    {
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        for (int k = 0; k <= 10; ++k)
        {
            const float a = sa + (float) k / 10.f * (ea - sa);
            const float s0 = std::sin(a), c0 = std::cos(a);
            g.drawLine(c.x + s0 * (arcR + 6.f), c.y - c0 * (arcR + 6.f),
                       c.x + s0 * (arcR + 10.f), c.y - c0 * (arcR + 10.f), k % 5 == 0 ? 2.f : 1.f);
        }
    }

    // track
    juce::Path track;
    track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, sa, ea, true);
    g.setColour(juce::Colour(0xff222640));
    g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    if (std::abs(ang - angO) > 0.01f)
    {
        juce::Path v;
        v.addCentredArc(c.x, c.y, arcR, arcR, 0.f, juce::jmin(angO, ang), juce::jmax(angO, ang), true);
        juce::ColourGradient gr(accB, c.x - arcR, c.y, accA, c.x + arcR, c.y, false);

        auto glowG = gr;
        glowG.multiplyOpacity(0.25f + 0.2f * hov);
        g.setGradientFill(glowG);
        g.strokePath(v, juce::PathStrokeType(thick + 5.f + 3.f * hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setGradientFill(gr);
        g.strokePath(v, juce::PathStrokeType(thick + hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // knob body
    {
        juce::ColourGradient body(juce::Colour(0xff343a5a), c.x, c.y - inner, juce::Colour(0xff101322), c.x, c.y + inner, false);
        g.setGradientFill(body);
        g.fillEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f);
        g.setColour(juce::Colours::white.withAlpha(0.08f + 0.10f * hov));
        g.drawEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f, 1.f);
    }

    // pointer
    {
        juce::Path p;
        const float pw = big ? 4.f : 3.f;
        p.addRoundedRectangle(-pw * 0.5f, -inner * 0.88f, pw, inner * 0.42f, pw * 0.5f);
        p.applyTransform(juce::AffineTransform::rotation(ang).translated(c.x, c.y));
        g.setColour(juce::Colours::white.interpolatedWith(accA, 0.5f * hov));
        g.fillPath(p);
    }
}

//==============================================================================
// Knob
//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
           const juce::String& title, const juce::String& tooltip, double origin, bool big)
    : isBig(big), attachment(apvts, paramId, slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 18);
    slider.setColour(juce::Slider::textBoxTextColourId, txt);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTooltip(tooltip);
    if (! std::isnan(origin))
        slider.getProperties().set("origin", origin);
    if (big)
        slider.getProperties().set("big", true);
    addAndMakeVisible(slider);

    label.setText(title, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(mkFont(big ? 14.f : 11.5f, true).withExtraKerningFactor(0.12f));
    label.setColour(juce::Label::textColourId, big ? txt : dim);
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromTop(isBig ? 22 : 16));
    slider.setBounds(b);
}

void Knob::tick()
{
    float h = (float) slider.getProperties().getWithDefault("hover", 0.0f);
    const float target = slider.isMouseOverOrDragging(true) ? 1.f : 0.f;
    h += (target - h) * 0.2f;
    slider.getProperties().set("hover", h);
}

//==============================================================================
// Editor
//==============================================================================
AudiokingsVocalsEditor::AudiokingsVocalsEditor(AudiokingsVocalsProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);

    // ---- preset selector with sections ----
    int id = 1;
    for (const auto& sec : ak::sections())
    {
        presetBox.addSectionHeading(sec.name);
        for (const auto& pr : sec.presets)
            presetBox.addItem(pr.name, id++);
    }
    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setSelectedId(proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram(presetBox.getSelectedId() - 1); };
    addAndMakeVisible(presetBox);

    prevBtn.onClick = [this] { stepPreset(-1); };
    nextBtn.onClick = [this] { stepPreset(1); };
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);

    // ---- reverb type buttons ----
    const char* revNames[3] = { "ROOM", "PLATE", "HALL" };
    const char* revTips[3] = { "Small, tight space", "Smooth, bright vocal plate", "Large, long hall" };
    for (int i = 0; i < 3; ++i)
    {
        revBtns[i].setButtonText(revNames[i]);
        revBtns[i].setTooltip(revTips[i]);
        revBtns[i].onClick = [this, i] { setRevType(i); };
        addAndMakeVisible(revBtns[i]);
    }

    // ---- knobs ----
    struct Spec { const char* id; const char* title; const char* tip; double origin; bool big; };
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Spec specs[13] = {
        { "sens",      "LEVEL / SENS", "Input level and preamp sensitivity. Aim the loudest peaks at the OPTIMAL zone of the meter.", nan, true },
        { "compress",  "COMPRESS",     "Amount of compression (more = denser and more even vocal)", nan, false },
        { "bass",      "BASS",         "Low-end body (also movable in the EQ display)", nan, false },
        { "treble",    "TREBLE",       "Presence and clarity (also movable in the EQ display)", nan, false },
        { "air",       "AIR",          "High-frequency shelf (also movable in the EQ display)", nan, false },
        { "fx",        "FX",           "Macro: reverb + delay + harmonic saturation + shine", nan, false },
        { "revtone",   "REVERB TONE",  "Dark <-> bright reverb tail", nan, false },
        { "decay",     "DECAY",        "Length of the reverb tail", nan, false },
        { "delay",     "DELAY",        "Amount of echo (tempo-synced dotted 1/8)", nan, false },
        { "delaytone", "DELAY TONE",   "Low: dark echoes that bloom into the reverb. High: bright, crisp echoes", nan, false },
        { "airlow",    "AIR LOW",      "Harmonic exciter on the upper mids (presence and bite)", nan, false },
        { "airhigh",   "AIR HIGH",     "Harmonic exciter on the highs (silky shine)", nan, false },
        { "output",    "OUTPUT",       "Output level", nan, false }
    };
    for (const auto& s : specs)
    {
        knobs.push_back(std::make_unique<Knob>(proc.apvts, s.id, s.title, s.tip, s.origin, s.big));
        addAndMakeVisible(*knobs.back());
        knobs.back()->setAlpha(0.f);
    }

    scopeImg = juce::Image(juce::Image::RGB, 150, 150, true);

    // setSize last: resized() needs the knobs to exist
    setSize(1100, 764);
    setResizable(false, false);
    startTimerHz(60);
}

AudiokingsVocalsEditor::~AudiokingsVocalsEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void AudiokingsVocalsEditor::stepPreset(int delta)
{
    const int n = ak::numPresets();
    const int i = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram(i);
    presetBox.setSelectedId(i + 1, juce::dontSendNotification);
}

void AudiokingsVocalsEditor::setRevType(int type)
{
    if (auto* p = proc.apvts.getParameter("revtype"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1((float) type));
        p->endChangeGesture();
    }
}

void AudiokingsVocalsEditor::resized()
{
    // header controls
    juce::Rectangle<int> h(0, 16, getWidth() - 22, 32);
    nextBtn.setBounds(h.removeFromRight(32));
    h.removeFromRight(6);
    presetBox.setBounds(h.removeFromRight(280));
    h.removeFromRight(6);
    prevBtn.setBounds(h.removeFromRight(32));

    eqPanel = { 16.f, 72.f, 560.f, 300.f };
    meterPanel = { 588.f, 72.f, 496.f, 300.f };
    dynPanel = { 16.f, 384.f, 1068.f, 184.f };
    tonePanel = { 16.f, 580.f, 1068.f, 168.f };
    eqPlot = eqPanel.reduced(18.f, 0.f).withTrimmedTop(40.f).withTrimmedBottom(28.f);
    ledRect = { 238.f, 404.f, 74.f, 164.f };

    if (knobs.size() < 13)
        return;

    // Dynamics: big Level/Sens + 5 knobs
    knobs[0]->setBounds(28, 398, 200, 178);
    for (int k = 0; k < 5; ++k)
        knobs[(size_t) (1 + k)]->setBounds(326 + k * 148, 410, 148, 140);

    // Tone & Space
    const int xs[7] = { 176, 288, 424, 536, 684, 796, 944 };
    for (int k = 0; k < 7; ++k)
        knobs[(size_t) (6 + k)]->setBounds(xs[k], 604, 112, 140);

    for (int i = 0; i < 3; ++i)
        revBtns[i].setBounds(30, 620 + i * 34, 130, 28);
}

//==============================================================================
float AudiokingsVocalsEditor::freqToX(float f) const
{
    return eqPlot.getX() + eqPlot.getWidth() * std::log10(f / 20.f) / 3.f;
}

float AudiokingsVocalsEditor::getP(const char* id) const
{
    return proc.apvts.getRawParameterValue(id)->load();
}

void AudiokingsVocalsEditor::setP(const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(v));
}

juce::Point<float> AudiokingsVocalsEditor::nodePos(int i) const
{
    const float f = getP(kFreqIds[i]);
    const float g = juce::jlimit(-15.f, 15.f, getP(kGainIds[i]));
    return { freqToX(f), eqPlot.getCentreY() - g / 15.f * eqPlot.getHeight() * 0.5f };
}

int AudiokingsVocalsEditor::hitNode(juce::Point<float> p) const
{
    int best = -1;
    float bestD = 16.f * 16.f;
    for (int i = 0; i < 4; ++i)
    {
        const auto np = nodePos(i);
        const float dx = np.x - p.x, dy = np.y - p.y;
        const float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void AudiokingsVocalsEditor::setNodeFromPos(int i, juce::Point<float> p)
{
    const float xn = juce::jlimit(0.f, 1.f, (p.x - eqPlot.getX()) / eqPlot.getWidth());
    const float f = juce::jlimit(kFreqLo[i], kFreqHi[i], 20.f * std::pow(1000.f, xn));
    float g = (eqPlot.getCentreY() - p.y) / (eqPlot.getHeight() * 0.5f) * 15.f;
    g = juce::jlimit(-12.f, 12.f, g);
    if (std::abs(g) < 0.25f) g = 0.f;
    g = std::round(g * 10.f) / 10.f;
    setP(kFreqIds[i], std::round(f));
    setP(kGainIds[i], g);
}

void AudiokingsVocalsEditor::mouseMove(const juce::MouseEvent& e)
{
    hoverNode = hitNode(e.position);
    setMouseCursor(hoverNode >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void AudiokingsVocalsEditor::mouseExit(const juce::MouseEvent&)
{
    if (dragNode < 0) hoverNode = -1;
}

void AudiokingsVocalsEditor::mouseDown(const juce::MouseEvent& e)
{
    dragNode = hitNode(e.position);
    if (dragNode >= 0)
    {
        hoverNode = dragNode;
        for (auto* id : { kGainIds[dragNode], kFreqIds[dragNode] })
            if (auto* p = proc.apvts.getParameter(id)) p->beginChangeGesture();
        setNodeFromPos(dragNode, e.position);
    }
}

void AudiokingsVocalsEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragNode >= 0)
        setNodeFromPos(dragNode, e.position);
}

void AudiokingsVocalsEditor::mouseUp(const juce::MouseEvent& e)
{
    if (dragNode >= 0)
    {
        for (auto* id : { kGainIds[dragNode], kFreqIds[dragNode] })
            if (auto* p = proc.apvts.getParameter(id)) p->endChangeGesture();
        dragNode = -1;
        hoverNode = hitNode(e.position);
    }
}

void AudiokingsVocalsEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int n = hitNode(e.position);
    if (n < 0) return;
    // reset the node: flat gain and its default frequency
    const float defF[4] = { 120.f, 350.f, 3500.f, 10000.f };
    setP(kGainIds[n], 0.f);
    setP(kFreqIds[n], defF[n]);
}

//==============================================================================
void AudiokingsVocalsEditor::timerCallback()
{
    phase += 1.f / 60.f;
    intro += (1.f - intro) * 0.06f;

    // meters (the processor stores the peak since our last read)
    inL.update(juce::Decibels::gainToDecibels(proc.meterInL.exchange(0.f), -100.f));
    inR.update(juce::Decibels::gainToDecibels(proc.meterInR.exchange(0.f), -100.f));
    outL.update(juce::Decibels::gainToDecibels(proc.meterOutL.exchange(0.f), -100.f));
    outR.update(juce::Decibels::gainToDecibels(proc.meterOutR.exchange(0.f), -100.f));

    const float gr = proc.meterGR.exchange(0.f);
    grDb = gr > grDb ? gr : std::max(0.f, grDb - 0.5f);
    grHist[(size_t) histPos] = grDb;
    histPos = (histPos + 1) % histN;

    corrSmooth += (proc.correlation.load() - corrSmooth) * 0.15f;

    proc.copyScope(scopeL.data(), scopeR.data());
    updateSpectrum();
    updateCurve();
    updateVectorscope();

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i]->tick();
        knobs[i]->setAlpha(juce::jlimit(0.f, 1.f, intro * 1.6f - (float) i * 0.04f));
    }

    // keep preset box and reverb buttons in sync with the host / state
    const int cur = proc.getCurrentProgram();
    if (presetBox.getSelectedId() != cur + 1)
        presetBox.setSelectedId(cur + 1, juce::dontSendNotification);

    if (auto* rp = proc.apvts.getParameter("revtype"))
    {
        const int t = (int) std::lround(rp->convertFrom0to1(rp->getValue()));
        for (int i = 0; i < 3; ++i)
            revBtns[i].setToggleState(i == t, juce::dontSendNotification);
    }

    repaint();
}

void AudiokingsVocalsEditor::updateSpectrum()
{
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = 0.5f * (scopeL[(size_t) i] + scopeR[(size_t) i]);
    window.multiplyWithWindowingTable(fftData.data(), (size_t) fftSize);
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.f);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;
    const float binsPerHz = (float) fftSize / (float) sr;

    for (int i = 0; i < numPts; ++i)
    {
        const float f0 = 20.f * std::pow(1000.f, (float) i / (float) (numPts - 1));
        const float f1 = 20.f * std::pow(1000.f, (float) (i + 1) / (float) (numPts - 1));
        const int b0 = juce::jlimit(0, fftSize / 2 - 1, (int) (f0 * binsPerHz));
        const int b1 = juce::jlimit(b0 + 1, fftSize / 2, (int) (f1 * binsPerHz) + 1);

        float m = 0.f;
        for (int b = b0; b < b1; ++b)
            m = std::max(m, fftData[(size_t) b]);

        float db = juce::Decibels::gainToDecibels(m / ((float) fftSize * 0.25f), -100.f);
        db += 4.5f * std::log2(f0 / 1000.f);   // gentle tilt so the display looks balanced
        specDb[(size_t) i] = db > specDb[(size_t) i] ? db : specDb[(size_t) i] + (db - specDb[(size_t) i]) * 0.15f;
    }
}

void AudiokingsVocalsEditor::updateCurve()
{
    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;
    const ak::EqParams ep { getP("hpf"), getP("bass"), getP("bassf"), getP("mud"), getP("mudf"),
                            getP("treble"), getP("trebf"), getP("air"), getP("airf") };
    const auto e = ak::makeEq(sr, ep);

    for (int i = 0; i < numPts; ++i)
    {
        const double f = 20.0 * std::pow(1000.0, (double) i / (double) (numPts - 1));
        const double mag = ak::magnitudeAt(e.hp, f, sr) * ak::magnitudeAt(e.bass, f, sr)
                         * ak::magnitudeAt(e.mud, f, sr) * ak::magnitudeAt(e.treb, f, sr)
                         * ak::magnitudeAt(e.air, f, sr);
        curveDb[(size_t) i] = (float) (20.0 * std::log10(std::max(mag, 1.0e-6)));
    }
}

void AudiokingsVocalsEditor::updateVectorscope()
{
    juce::Graphics ig(scopeImg);
    const float s = (float) scopeImg.getWidth();
    const float cx = s * 0.5f, cy = s * 0.5f, rad = cx - 4.f;

    // fade the previous frame (phosphor persistence)
    ig.setColour(juce::Colour(0xff080a14).withAlpha(0.22f));
    ig.fillAll();

    juce::Path path;
    const int N = 600;
    for (int k = 0; k < N; ++k)
    {
        const size_t idx = (size_t) (AudiokingsVocalsProcessor::scopeSize - N + k);
        const float L = scopeL[idx], R = scopeR[idx];
        const float x = juce::jlimit(-1.f, 1.f, (R - L) * 0.7071f * 2.5f);   // L goes up-left, R up-right
        const float y = juce::jlimit(-1.f, 1.f, (L + R) * 0.7071f * 2.5f);   // mono is vertical
        const float px = cx + x * rad, py = cy - y * rad;
        if (k == 0) path.startNewSubPath(px, py); else path.lineTo(px, py);
    }
    ig.setColour(accA.withAlpha(0.85f));
    ig.strokePath(path, juce::PathStrokeType(1.1f));
}

//==============================================================================
// Painting
//==============================================================================
void AudiokingsVocalsEditor::paintBackground(juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    juce::ColourGradient bg(bg1, 0.f, 0.f, bg2, w, h, false);
    g.setGradientFill(bg);
    g.fillAll();

    auto blob = [&](juce::Colour c, float cx, float cy, float r, float a)
    {
        juce::ColourGradient gr(c.withAlpha(a), cx, cy, c.withAlpha(0.f), cx + r, cy, true);
        g.setGradientFill(gr);
        g.fillEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
    };
    blob(accB, w * 0.20f + 120.f * std::sin(phase * 0.35f), h * 0.30f + 60.f * std::cos(phase * 0.27f), 400.f, 0.22f);
    blob(accA, w * 0.80f + 100.f * std::cos(phase * 0.30f), h * 0.25f + 50.f * std::sin(phase * 0.40f), 360.f, 0.14f);
    blob(accC, w * 0.50f + 160.f * std::sin(phase * 0.20f), h * 0.95f, 420.f, 0.10f);
}

void AudiokingsVocalsEditor::paintPanel(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff0d1020).withAlpha(0.78f));
    g.fillRoundedRectangle(r, 14.f);
    juce::ColourGradient edge(juce::Colours::white.withAlpha(0.16f), r.getX(), r.getY(),
                              juce::Colours::white.withAlpha(0.03f), r.getX(), r.getBottom(), false);
    g.setGradientFill(edge);
    g.drawRoundedRectangle(r.reduced(0.5f), 14.f, 1.f);
}

void AudiokingsVocalsEditor::paintHeader(juce::Graphics& g)
{
    // crown logo
    {
        const float x = 22.f, y = 14.f, w = 36.f, h = 30.f;
        juce::Path crown;
        crown.startNewSubPath(x, y + h);
        crown.lineTo(x, y + h * 0.25f);
        crown.lineTo(x + w * 0.25f, y + h * 0.58f);
        crown.lineTo(x + w * 0.5f, y);
        crown.lineTo(x + w * 0.75f, y + h * 0.58f);
        crown.lineTo(x + w, y + h * 0.25f);
        crown.lineTo(x + w, y + h);
        crown.closeSubPath();

        juce::ColourGradient cg(gold.brighter(0.3f), x, y, gold.darker(0.4f), x, y + h, false);
        g.setColour(gold.withAlpha(0.18f + 0.08f * std::sin(phase * 2.f)));
        g.fillEllipse(x - 10.f, y - 8.f, w + 20.f, h + 16.f);
        g.setGradientFill(cg);
        g.fillPath(crown);
        g.fillEllipse(x - 2.f, y + h * 0.25f - 3.f, 6.f, 6.f);
        g.fillEllipse(x + w * 0.5f - 3.f, y - 3.f, 6.f, 6.f);
        g.fillEllipse(x + w - 4.f, y + h * 0.25f - 3.f, 6.f, 6.f);
    }

    // title with a moving shine
    {
        const float t = std::fmod(phase * 0.3f, 1.8f) - 0.4f;
        const float tx = 70.f, tw = 190.f;
        juce::ColourGradient tg(gold, tx, 0.f, gold, tx + tw, 0.f, false);
        tg.addColour(juce::jlimit(0.0, 1.0, (double) t - 0.12), gold);
        tg.addColour(juce::jlimit(0.0, 1.0, (double) t), juce::Colours::white);
        tg.addColour(juce::jlimit(0.0, 1.0, (double) t + 0.12), gold);
        g.setGradientFill(tg);
        g.setFont(mkFont(23.f, true).withExtraKerningFactor(0.12f));
        g.drawText("AUDIOKINGS", juce::Rectangle<int>((int) tx, 8, (int) tw, 28), juce::Justification::centredLeft);

        g.setColour(dim);
        g.setFont(mkFont(10.5f, true).withExtraKerningFactor(0.3f));
        g.drawText("VOCALS  \xc2\xb7  CHANNEL STRIP", juce::Rectangle<int>((int) tx, 36, 260, 14), juce::Justification::centredLeft);
    }
}

void AudiokingsVocalsEditor::paintEq(juce::Graphics& g)
{
    paintPanel(g, eqPanel);

    g.setColour(dim);
    g.setFont(mkFont(11.f, true).withExtraKerningFactor(0.25f));
    g.drawText("EQ  \xc2\xb7  LIVE RESPONSE", juce::Rectangle<int>((int) eqPanel.getX() + 18, (int) eqPanel.getY() + 10, 260, 16),
               juce::Justification::centredLeft);
    g.setFont(mkFont(10.f));
    g.drawText("drag the nodes  \xc2\xb7  double-click to reset",
               juce::Rectangle<int>((int) eqPanel.getRight() - 290, (int) eqPanel.getY() + 10, 272, 16),
               juce::Justification::centredRight);

    const auto plot = eqPlot;

    // grid
    const float gridF[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    for (float f : gridF)
        g.drawVerticalLine((int) freqToX(f), plot.getY(), plot.getBottom());
    const float dbY[] = { -12.f, -6.f, 6.f, 12.f };
    for (float d : dbY)
        g.drawHorizontalLine((int) (plot.getCentreY() - d / 15.f * plot.getHeight() * 0.5f), plot.getX(), plot.getRight());
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawHorizontalLine((int) plot.getCentreY(), plot.getX(), plot.getRight());

    g.setColour(dim.withAlpha(0.8f));
    g.setFont(mkFont(9.5f));
    const std::pair<float, const char*> labels[] = { { 100.f, "100" }, { 1000.f, "1k" }, { 10000.f, "10k" } };
    for (const auto& l : labels)
        g.drawText(l.second, juce::Rectangle<int>((int) freqToX(l.first) - 20, (int) plot.getBottom() + 4, 40, 12), juce::Justification::centred);
    g.drawText("0 dB", juce::Rectangle<int>((int) eqPanel.getX() + 2, (int) plot.getCentreY() - 6, 34, 12), juce::Justification::centredRight);

    g.saveState();
    g.reduceClipRegion(plot.expanded(0.f, 10.f).toNearestInt());

    // spectrum
    {
        juce::Path line;
        for (int i = 0; i < numPts; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (numPts - 1);
            const float y = juce::jmap(juce::jlimit(-85.f, -20.f, specDb[(size_t) i]), -85.f, -20.f, plot.getBottom(), plot.getY());
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(plot.getRight(), plot.getBottom());
        area.lineTo(plot.getX(), plot.getBottom());
        area.closeSubPath();

        juce::ColourGradient ag(accB.withAlpha(0.38f), 0.f, plot.getY(), accB.withAlpha(0.f), 0.f, plot.getBottom(), false);
        g.setGradientFill(ag);
        g.fillPath(area);
        g.setColour(accB.withAlpha(0.6f));
        g.strokePath(line, juce::PathStrokeType(1.2f));
    }

    // EQ curve
    juce::Path curve;
    auto yOf = [&](float db) { return plot.getCentreY() - juce::jlimit(-15.f, 15.f, db) / 15.f * plot.getHeight() * 0.5f; };
    for (int i = 0; i < numPts; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (numPts - 1);
        const float y = yOf(curveDb[(size_t) i]);
        if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }
    {
        juce::Path area(curve);
        area.lineTo(plot.getRight(), plot.getCentreY());
        area.lineTo(plot.getX(), plot.getCentreY());
        area.closeSubPath();
        g.setColour(accA.withAlpha(0.10f));
        g.fillPath(area);

        juce::ColourGradient cg(accA, plot.getX(), 0.f, accC, plot.getRight(), 0.f, false);
        auto glow = cg;
        glow.multiplyOpacity(0.25f);
        g.setGradientFill(glow);
        g.strokePath(curve, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(cg);
        g.strokePath(curve, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.restoreState();

    // draggable nodes
    for (int i = 0; i < 4; ++i)
    {
        const auto np = nodePos(i);
        const juce::Colour col(kNodeCol[i]);
        const bool active = (i == hoverNode || i == dragNode);
        const float pulse = 6.f + 1.6f * std::sin(phase * 3.f + (float) i) + (active ? 3.f : 0.f);

        // stem to the 0 dB line
        g.setColour(col.withAlpha(0.25f));
        g.drawLine(np.x, np.y, np.x, plot.getCentreY(), 1.f);

        g.setColour(col.withAlpha(active ? 0.32f : 0.18f));
        g.fillEllipse(np.x - pulse * 1.7f, np.y - pulse * 1.7f, pulse * 3.4f, pulse * 3.4f);
        g.setColour(col);
        g.fillEllipse(np.x - 7.f, np.y - 7.f, 14.f, 14.f);
        g.setColour(juce::Colours::white.withAlpha(active ? 1.f : 0.85f));
        g.drawEllipse(np.x - 7.f, np.y - 7.f, 14.f, 14.f, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(mkFont(9.f, true));
        g.drawText(juce::String(kNodeNames[i]),
                   juce::Rectangle<int>((int) np.x - 30, (int) np.y - 26, 60, 12), juce::Justification::centred);
    }

    // value read-out for the node under the mouse
    const int shown = dragNode >= 0 ? dragNode : hoverNode;
    if (shown >= 0)
    {
        const float f = getP(kFreqIds[shown]), gdb = getP(kGainIds[shown]);
        const juce::String ftxt = f >= 1000.f ? juce::String(f / 1000.f, 2) + " kHz" : juce::String((int) f) + " Hz";
        const juce::String text = juce::String(kNodeNames[shown]) + "   " + ftxt + "   " + (gdb >= 0.f ? "+" : "") + juce::String(gdb, 1) + " dB";
        auto pill = juce::Rectangle<float>(plot.getCentreX() - 110.f, plot.getY() + 2.f, 220.f, 20.f);
        g.setColour(juce::Colour(0xff05060d).withAlpha(0.85f));
        g.fillRoundedRectangle(pill, 10.f);
        g.setColour(juce::Colour(kNodeCol[shown]));
        g.drawRoundedRectangle(pill, 10.f, 1.f);
        g.setColour(txt);
        g.setFont(mkFont(11.f, true));
        g.drawText(text, pill.toNearestInt(), juce::Justification::centred);
    }
}

void AudiokingsVocalsEditor::drawLevelBar(juce::Graphics& g, juce::Rectangle<float> bar, float db, float hold)
{
    g.setColour(juce::Colour(0xff0a0c16));
    g.fillRoundedRectangle(bar, 3.f);

    const float yv = juce::jmap(juce::jlimit(-60.f, 3.f, db), -60.f, 3.f, bar.getBottom(), bar.getY());
    juce::ColourGradient gr(juce::Colour(0xffff4d6d), 0.f, bar.getY(), juce::Colour(0xff38e1ff), 0.f, bar.getBottom(), false);
    gr.addColour(0.05, juce::Colour(0xffff4d6d));
    gr.addColour(0.16, juce::Colour(0xffffd25a));
    gr.addColour(0.40, juce::Colour(0xff2be3b0));
    g.setGradientFill(gr);
    g.fillRect(juce::Rectangle<float>(bar.getX(), yv, bar.getWidth(), bar.getBottom() - yv));

    // LED segment mask
    g.setColour(juce::Colour(0xff0d1020));
    for (float yy = bar.getY(); yy < bar.getBottom(); yy += 3.f)
        g.fillRect(bar.getX(), yy, bar.getWidth(), 1.f);

    const float yh = juce::jmap(juce::jlimit(-60.f, 3.f, hold), -60.f, 3.f, bar.getBottom(), bar.getY());
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillRect(bar.getX(), yh - 1.f, bar.getWidth(), 2.f);
}

void AudiokingsVocalsEditor::drawGRBar(juce::Graphics& g, juce::Rectangle<float> bar, float gr)
{
    g.setColour(juce::Colour(0xff0a0c16));
    g.fillRoundedRectangle(bar, 3.f);

    const float hgt = juce::jmap(juce::jlimit(0.f, 18.f, gr), 0.f, 18.f, 0.f, bar.getHeight());
    juce::ColourGradient grad(accC, 0.f, bar.getY(), accB, 0.f, bar.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRect(juce::Rectangle<float>(bar.getX(), bar.getY(), bar.getWidth(), hgt));

    g.setColour(juce::Colour(0xff0d1020));
    for (float yy = bar.getY(); yy < bar.getBottom(); yy += 3.f)
        g.fillRect(bar.getX(), yy, bar.getWidth(), 1.f);

    // scale ticks
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    for (int d : { 3, 6, 9, 12, 15 })
    {
        const float y = bar.getY() + (float) d / 18.f * bar.getHeight();
        g.fillRect(bar.getX(), y, 5.f, 1.f);
        g.fillRect(bar.getRight() - 5.f, y, 5.f, 1.f);
    }
}

void AudiokingsVocalsEditor::drawGroupLabel(juce::Graphics& g, juce::Rectangle<float> r,
                                            const juce::String& name, const juce::String& value)
{
    g.setColour(txt);
    g.setFont(mkFont(10.5f));
    g.drawText(value, r.removeFromTop(14.f).toNearestInt(), juce::Justification::centred);
    g.setColour(dim);
    g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.2f));
    g.drawText(name, r.removeFromTop(14.f).toNearestInt(), juce::Justification::centred);
}

void AudiokingsVocalsEditor::paintMeters(juce::Graphics& g)
{
    paintPanel(g, meterPanel);

    auto inner = meterPanel.reduced(18.f, 12.f);
    g.setColour(dim);
    g.setFont(mkFont(11.f, true).withExtraKerningFactor(0.25f));
    g.drawText("METERS  \xc2\xb7  COMPRESSION  \xc2\xb7  STEREO", inner.removeFromTop(18.f).toNearestInt(), juce::Justification::centredLeft);
    inner.removeFromTop(4.f);

    auto strip = inner.removeFromBottom(56.f);
    inner.removeFromBottom(8.f);
    auto leftA = inner.removeFromLeft(210.f);
    inner.removeFromLeft(14.f);
    auto rightA = inner;

    // ---- level / gain-reduction bars ----
    const float groupW = leftA.getWidth() / 3.f;
    const float labelH = 28.f;
    auto makeGroup = [&](int idx)
    {
        auto grp = leftA.withX(leftA.getX() + groupW * (float) idx).withWidth(groupW);
        auto lab = grp.removeFromBottom(labelH);
        return std::make_pair(grp.reduced(0.f, 2.f), lab);
    };
    {
        auto [bars, lab] = makeGroup(0);
        const float bw = 20.f, gap = 5.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, inL.db, inL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, inR.db, inR.hold);
        drawGroupLabel(g, lab, "INPUT", juce::String(std::max(inL.hold, inR.hold), 1));
    }
    {
        auto [bars, lab] = makeGroup(1);
        const float bw = 32.f;
        drawGRBar(g, { bars.getCentreX() - bw * 0.5f, bars.getY(), bw, bars.getHeight() }, grDb);
        drawGroupLabel(g, lab, "COMPRESSION", "-" + juce::String(grDb, 1) + " dB");
    }
    {
        auto [bars, lab] = makeGroup(2);
        const float bw = 20.f, gap = 5.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, outL.db, outL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, outR.db, outR.hold);
        drawGroupLabel(g, lab, "OUTPUT", juce::String(std::max(outL.hold, outR.hold), 1));
    }

    // ---- vectorscope ----
    {
        const float side = 150.f;
        juce::Rectangle<float> scope(rightA.getCentreX() - side * 0.5f, rightA.getY(), side, side);

        g.saveState();
        juce::Path clip;
        clip.addEllipse(scope);
        g.reduceClipRegion(clip, juce::AffineTransform());
        g.drawImage(scopeImg, scope);
        g.restoreState();

        g.setColour(juce::Colours::white.withAlpha(0.16f));
        g.drawEllipse(scope, 1.f);
        g.drawEllipse(scope.reduced(side * 0.25f), 0.6f);
        const auto c = scope.getCentre();
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawLine(c.x, scope.getY(), c.x, scope.getBottom(), 0.8f);
        g.drawLine(scope.getX(), c.y, scope.getRight(), c.y, 0.8f);
        const float d = side * 0.5f * 0.7071f;
        g.drawLine(c.x - d, c.y - d, c.x + d, c.y + d, 0.6f);
        g.drawLine(c.x + d, c.y - d, c.x - d, c.y + d, 0.6f);

        g.setColour(dim);
        g.setFont(mkFont(9.5f, true));
        g.drawText("L", juce::Rectangle<int>((int) (c.x - d - 12.f), (int) (c.y - d - 12.f), 14, 12), juce::Justification::centred);
        g.drawText("R", juce::Rectangle<int>((int) (c.x + d - 2.f), (int) (c.y - d - 12.f), 14, 12), juce::Justification::centred);
        g.drawText("M", juce::Rectangle<int>((int) c.x - 7, (int) scope.getY() - 12, 14, 12), juce::Justification::centred);

        // correlation bar
        auto corr = rightA.withTop(scope.getBottom() + 6.f);
        auto top = corr.removeFromTop(14.f);
        g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.2f));
        g.drawText("STEREO", top.toNearestInt(), juce::Justification::centredLeft);
        g.drawText("CORR " + juce::String(corrSmooth, 2), top.toNearestInt(), juce::Justification::centredRight);

        auto track = corr.removeFromTop(12.f).reduced(0.f, 2.f);
        g.setColour(juce::Colour(0xff0a0c16));
        g.fillRoundedRectangle(track, 4.f);
        const float cx = track.getCentreX();
        const float vx = cx + juce::jlimit(-1.f, 1.f, corrSmooth) * track.getWidth() * 0.5f;
        const auto col = corrSmooth >= 0.f ? juce::Colour(0xff2be3b0) : juce::Colour(0xffff4d6d);
        juce::ColourGradient cg(col.withAlpha(0.35f), cx, 0.f, col, vx, 0.f, false);
        g.setGradientFill(cg);
        g.fillRoundedRectangle(juce::Rectangle<float>(std::min(cx, vx), track.getY(), std::abs(vx - cx), track.getHeight()), 4.f);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(cx - 0.5f, track.getY() - 2.f, 1.f, track.getHeight() + 4.f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(vx - 3.f, track.getCentreY() - 3.f, 6.f, 6.f);
    }

    // ---- gain-reduction read-out + history graph ----
    {
        auto num = strip.removeFromLeft(124.f);
        strip.removeFromLeft(8.f);

        const bool active = grDb > 0.2f;
        const juce::Colour grCol = grDb > 9.f ? juce::Colour(0xffff4d6d) : (grDb > 4.f ? accC : accB);
        g.setColour(active ? grCol : dim);
        g.setFont(mkFont(24.f, true));
        g.drawText((active ? "-" : "") + juce::String(grDb, 1) + " dB", num.removeFromTop(32.f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour(dim);
        g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.2f));
        g.drawText(active ? "GAIN REDUCTION" : "NO COMPRESSION", num.toNearestInt(), juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xff0a0c16));
        g.fillRoundedRectangle(strip, 6.f);

        g.setColour(juce::Colours::white.withAlpha(0.06f));
        for (int d : { 6, 12 })
            g.drawHorizontalLine((int) (strip.getY() + (float) d / 18.f * strip.getHeight()), strip.getX(), strip.getRight());

        juce::Path line;
        for (int i = 0; i < histN; ++i)
        {
            const float v = grHist[(size_t) ((histPos + i) % histN)];
            const float x = strip.getX() + strip.getWidth() * (float) i / (float) (histN - 1);
            const float y = strip.getY() + juce::jlimit(0.f, 18.f, v) / 18.f * strip.getHeight();
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(strip.getRight(), strip.getY());
        area.lineTo(strip.getX(), strip.getY());
        area.closeSubPath();
        juce::ColourGradient ag(accC.withAlpha(0.05f), 0.f, strip.getY(), accC.withAlpha(0.55f), 0.f, strip.getBottom(), false);
        g.setGradientFill(ag);
        g.fillPath(area);
        g.setColour(accC);
        g.strokePath(line, juce::PathStrokeType(1.4f));
    }
}

void AudiokingsVocalsEditor::paintDynamics(juce::Graphics& g)
{
    paintPanel(g, dynPanel);

    g.setColour(gold.withAlpha(0.85f));
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.3f));
    g.drawText("DYNAMICS", juce::Rectangle<int>((int) dynPanel.getX() + 16, (int) dynPanel.getY() + 6, 140, 12), juce::Justification::centredLeft);

    // vertical divider after the input section
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawVerticalLine(314, dynPanel.getY() + 16.f, dynPanel.getBottom() - 16.f);

    // ---- input level LEDs (green / yellow / red) with the optimal zone ----
    auto r = ledRect;
    const float lvl = std::max(inL.db, inR.db), hold = std::max(inL.hold, inR.hold);

    g.setColour(dim);
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.2f));
    g.drawText("INPUT LEVEL", r.removeFromTop(14.f).toNearestInt(), juce::Justification::centred);
    auto statusR = r.removeFromBottom(16.f);

    const int nSeg = 15;
    const float segGap = 2.f;
    const float segH = (r.getHeight() - (float) (nSeg - 1) * segGap) / (float) nSeg;
    const float barX = r.getX() + 4.f, barW = 28.f;

    // optimal band: segments with threshold -18 .. -9 dB (i = 8..11)
    const float optBottom = r.getBottom() - 8.f * (segH + segGap) + segGap * 0.5f;
    const float optTop = r.getBottom() - 12.f * (segH + segGap) + segGap * 0.5f;
    g.setColour(juce::Colour(0xff2be3b0).withAlpha(0.10f + 0.04f * std::sin(phase * 3.f)));
    g.fillRoundedRectangle(barX - 3.f, optTop, barW + 6.f, optBottom - optTop, 4.f);

    for (int i = 0; i < nSeg; ++i)
    {
        const float thr = -42.f + 3.f * (float) i;
        const float y = r.getBottom() - (float) (i + 1) * segH - (float) i * segGap;
        const juce::Colour col = thr < -9.f ? juce::Colour(0xff2be3b0)
                               : (thr < -3.f ? juce::Colour(0xffffd25a) : juce::Colour(0xffff4d6d));
        const bool lit = lvl >= thr;
        const bool holdSeg = hold >= thr && hold < thr + 3.f;
        if (lit)
        {
            g.setColour(col.withAlpha(0.25f));
            g.fillRoundedRectangle(barX - 2.f, y - 1.f, barW + 4.f, segH + 2.f, 3.f);
        }
        g.setColour(col.withAlpha(lit ? 1.f : (holdSeg ? 0.9f : 0.13f)));
        g.fillRoundedRectangle(barX, y, barW, segH, 2.f);
    }

    // optimal bracket and label
    {
        const float bx = barX + barW + 6.f;
        g.setColour(juce::Colour(0xff2be3b0));
        g.drawLine(bx, optTop, bx, optBottom, 1.5f);
        g.drawLine(bx - 3.f, optTop, bx, optTop, 1.5f);
        g.drawLine(bx - 3.f, optBottom, bx, optBottom, 1.5f);
        g.setFont(mkFont(8.5f, true));
        g.drawText("OPT", juce::Rectangle<int>((int) bx + 3, (int) ((optTop + optBottom) * 0.5f) - 6, 26, 12), juce::Justification::centredLeft);
    }

    // status
    juce::String status = "-";
    juce::Colour sc = dim;
    if (hold > -3.f)       { status = "TOO HOT"; sc = juce::Colour(0xffff4d6d); }
    else if (hold > -9.f)  { status = "HOT";     sc = juce::Colour(0xffffd25a); }
    else if (hold >= -18.f){ status = "OPTIMAL"; sc = juce::Colour(0xff2be3b0); }
    else if (hold > -42.f) { status = "LOW";     sc = dim; }
    g.setColour(sc);
    g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.15f));
    g.drawText(status, statusR.toNearestInt(), juce::Justification::centred);
}

void AudiokingsVocalsEditor::paintToneSpace(juce::Graphics& g)
{
    paintPanel(g, tonePanel);

    g.setColour(gold.withAlpha(0.85f));
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.3f));
    const float ty = tonePanel.getY() + 8.f;
    g.drawText("REVERB TYPE", juce::Rectangle<int>(30, (int) ty, 140, 12), juce::Justification::centredLeft);
    g.drawText("REVERB",      juce::Rectangle<int>(184, (int) ty, 140, 12), juce::Justification::centredLeft);
    g.drawText("DELAY",       juce::Rectangle<int>(432, (int) ty, 140, 12), juce::Justification::centredLeft);
    g.drawText("AIR  \xc2\xb7  HARMONIC EXCITER", juce::Rectangle<int>(692, (int) ty, 220, 12), juce::Justification::centredLeft);
    g.drawText("OUT",         juce::Rectangle<int>(952, (int) ty, 100, 12), juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    for (int x : { 168, 416, 668, 930 })
        g.drawVerticalLine(x, tonePanel.getY() + 16.f, tonePanel.getBottom() - 16.f);
}

void AudiokingsVocalsEditor::paint(juce::Graphics& g)
{
    paintBackground(g);

    const float e = intro * intro * (3.f - 2.f * intro);
    const bool fading = intro < 0.995f;
    if (fading) g.beginTransparencyLayer(e);

    paintHeader(g);
    paintEq(g);
    paintMeters(g);
    paintDynamics(g);
    paintToneSpace(g);

    if (fading) g.endTransparencyLayer();
}
