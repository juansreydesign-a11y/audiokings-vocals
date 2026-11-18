#include "PluginEditor.h"

using namespace akui;

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
    setColour(juce::TextButton::textColourOnId, accA);
    setColour(juce::Label::textColourId, dim);
}

void AKLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                     float sa, float ea, juce::Slider& s)
{
    auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float hov = (float) s.getProperties().getWithDefault("hover", 0.0f);
    const float arcR = r - 4.f;
    const float inner = arcR - 9.f;
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
        juce::ColourGradient glow(accA.withAlpha(0.08f + 0.20f * hov), c.x, c.y,
                                  accA.withAlpha(0.f), c.x, c.y - r - 10.f, true);
        g.setGradientFill(glow);
        g.fillEllipse(c.x - r - 10.f, c.y - r - 10.f, (r + 10.f) * 2.f, (r + 10.f) * 2.f);
    }

    // track
    juce::Path track;
    track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, sa, ea, true);
    g.setColour(juce::Colour(0xff222640));
    g.strokePath(track, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc
    if (std::abs(ang - angO) > 0.01f)
    {
        juce::Path v;
        v.addCentredArc(c.x, c.y, arcR, arcR, 0.f, juce::jmin(angO, ang), juce::jmax(angO, ang), true);
        juce::ColourGradient gr(accB, c.x - arcR, c.y, accA, c.x + arcR, c.y, false);

        auto glowG = gr;
        glowG.multiplyOpacity(0.25f + 0.2f * hov);
        g.setGradientFill(glowG);
        g.strokePath(v, juce::PathStrokeType(9.f + 3.f * hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setGradientFill(gr);
        g.strokePath(v, juce::PathStrokeType(4.f + hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        p.addRoundedRectangle(-1.5f, -inner * 0.88f, 3.f, inner * 0.42f, 1.5f);
        p.applyTransform(juce::AffineTransform::rotation(ang).translated(c.x, c.y));
        g.setColour(juce::Colours::white.interpolatedWith(accA, 0.5f * hov));
        g.fillPath(p);
    }
}

//==============================================================================
// Knob
//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
           const juce::String& title, double origin)
    : attachment(apvts, paramId, slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 18);
    slider.setColour(juce::Slider::textBoxTextColourId, txt);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
    if (! std::isnan(origin))
        slider.getProperties().set("origin", origin);
    addAndMakeVisible(slider);

    label.setText(title, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(mkFont(11.5f, true).withExtraKerningFactor(0.12f));
    label.setColour(juce::Label::textColourId, dim);
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromTop(16));
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
    setSize(960, 606);
    setResizable(false, false);

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

    // ---- knobs: row 1 (dynamics) then row 2 (tone + space) ----
    struct Spec { const char* id; const char* title; double origin; };
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Spec specs[16] = {
        { "input",    "INPUT",       nan }, { "hpf",      "HIGH-PASS", nan },
        { "comp",     "COMPRESSION", nan }, { "attack",   "ATTACK",    nan },
        { "release",  "RELEASE",     nan }, { "deess",    "DE-ESSER",  nan },
        { "drive",    "DRIVE",       nan }, { "output",   "OUTPUT",    nan },
        { "low",      "LOW",         nan }, { "lowmid",   "LOW-MID",   nan },
        { "presence", "PRESENCE",    nan }, { "air",      "AIR",       nan },
        { "revmix",   "REVERB",      nan }, { "revsize",  "SIZE",      nan },
        { "revdamp",  "DAMP",        nan }, { "width",    "WIDTH",     100.0 }
    };
    for (const auto& s : specs)
    {
        knobs.push_back(std::make_unique<Knob>(proc.apvts, s.id, s.title, s.origin));
        addAndMakeVisible(*knobs.back());
        knobs.back()->setAlpha(0.f);
    }

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

void AudiokingsVocalsEditor::resized()
{
    // header controls
    juce::Rectangle<int> h(0, 16, getWidth() - 22, 32);
    nextBtn.setBounds(h.removeFromRight(32));
    h.removeFromRight(6);
    presetBox.setBounds(h.removeFromRight(260));
    h.removeFromRight(6);
    prevBtn.setBounds(h.removeFromRight(32));

    eqPanel = { 16.f, 72.f, 468.f, 222.f };
    meterPanel = { 496.f, 72.f, 448.f, 222.f };
    rowPanel1 = { 16.f, 304.f, 928.f, 146.f };
    rowPanel2 = { 16.f, 452.f, 928.f, 146.f };

    eqPlot = eqPanel.reduced(16.f, 0.f).withTrimmedTop(34.f).withTrimmedBottom(26.f);

    const int cellW = 116;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / 8, col = (int) i % 8;
        knobs[i]->setBounds(16 + col * cellW, (row == 0 ? 304 : 452) + 14, cellW, 130);
    }
}

//==============================================================================
float AudiokingsVocalsEditor::freqToX(float f) const
{
    return eqPlot.getX() + eqPlot.getWidth() * std::log10(f / 20.f) / 3.f;
}

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

    corrSmooth += (proc.correlation.load() - corrSmooth) * 0.15f;

    updateSpectrum();
    updateCurve();

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i]->tick();
        knobs[i]->setAlpha(juce::jlimit(0.f, 1.f, intro * 1.6f - (float) i * 0.04f));
    }

    // keep the preset box in sync with the host / state
    const int cur = proc.getCurrentProgram();
    if (presetBox.getSelectedId() != cur + 1)
        presetBox.setSelectedId(cur + 1, juce::dontSendNotification);

    repaint();
}

void AudiokingsVocalsEditor::updateSpectrum()
{
    proc.copyScope(fftData.data());
    window.multiplyWithWindowingTable(fftData.data(), (size_t) fftSize);
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.f);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
    const float binsPerHz = (float) fftSize / (float) sr;   // = (N/2) / (sr/2)

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
    auto get = [this](const char* id) { return proc.apvts.getRawParameterValue(id)->load(); };
    const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;

    bandGain[0] = get("low");
    bandGain[1] = get("lowmid");
    bandGain[2] = get("presence");
    bandGain[3] = get("air");

    const auto e = ak::makeEq(sr, get("hpf"), bandGain[0], bandGain[1], bandGain[2], bandGain[3]);

    for (int i = 0; i < numPts; ++i)
    {
        const double f = 20.0 * std::pow(1000.0, (double) i / (double) (numPts - 1));
        const double mag = ak::magnitudeAt(e.hp, f, sr) * ak::magnitudeAt(e.lo, f, sr)
                         * ak::magnitudeAt(e.lm, f, sr) * ak::magnitudeAt(e.pr, f, sr)
                         * ak::magnitudeAt(e.ai, f, sr);
        curveDb[(size_t) i] = (float) (20.0 * std::log10(std::max(mag, 1.0e-6)));
    }
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
    blob(accB, w * 0.20f + 120.f * std::sin(phase * 0.35f), h * 0.30f + 60.f * std::cos(phase * 0.27f), 380.f, 0.22f);
    blob(accA, w * 0.80f + 100.f * std::cos(phase * 0.30f), h * 0.25f + 50.f * std::sin(phase * 0.40f), 340.f, 0.14f);
    blob(accC, w * 0.50f + 160.f * std::sin(phase * 0.20f), h * 0.95f, 380.f, 0.10f);
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
    g.reduceClipRegion(plot.expanded(0.f, 6.f).toNearestInt());

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

    // pulsing band markers
    const float bandF[4] = { 120.f, 350.f, 3500.f, 10000.f };
    const char* bandN[4] = { "LOW", "MID", "PRES", "AIR" };
    for (int b = 0; b < 4; ++b)
    {
        const float x = freqToX(bandF[b]);
        const float y = yOf(curveDb[(size_t) juce::jlimit(0, numPts - 1,
                              (int) std::round(std::log10(bandF[b] / 20.f) / 3.f * (float) (numPts - 1)))]);
        const float pulse = 4.f + 1.6f * std::sin(phase * 3.f + (float) b);
        g.setColour(accA.withAlpha(0.18f));
        g.fillEllipse(x - pulse * 1.8f, y - pulse * 1.8f, pulse * 3.6f, pulse * 3.6f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 3.f, y - 3.f, 6.f, 6.f);
        g.setColour(dim);
        g.setFont(mkFont(9.f, true));
        g.drawText(bandN[b], juce::Rectangle<int>((int) x - 20, (int) y - 20, 40, 12), juce::Justification::centred);
    }
    g.restoreState();
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

    const float hgt = juce::jmap(juce::jlimit(0.f, 24.f, gr), 0.f, 24.f, 0.f, bar.getHeight());
    juce::ColourGradient grad(accC, 0.f, bar.getY(), accB, 0.f, bar.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRect(juce::Rectangle<float>(bar.getX(), bar.getY(), bar.getWidth(), hgt));

    g.setColour(juce::Colour(0xff0d1020));
    for (float yy = bar.getY(); yy < bar.getBottom(); yy += 3.f)
        g.fillRect(bar.getX(), yy, bar.getWidth(), 1.f);
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
    g.drawText("LEVELS", inner.removeFromTop(18.f).toNearestInt(), juce::Justification::centredLeft);
    inner.removeFromTop(4.f);

    auto stereoArea = inner.removeFromBottom(48.f);
    const float groupW = inner.getWidth() / 3.f;

    auto labelH = 28.f;
    auto makeGroups = [&](int idx)
    {
        auto grp = inner.withX(inner.getX() + groupW * (float) idx).withWidth(groupW);
        auto bars = grp.withTrimmedBottom(labelH).reduced(0.f, 2.f);
        auto lab = grp.removeFromBottom(labelH);
        return std::make_pair(bars, lab);
    };

    // INPUT
    {
        auto [bars, lab] = makeGroups(0);
        const float bw = 22.f, gap = 6.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, inL.db, inL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, inR.db, inR.hold);
        drawGroupLabel(g, lab, "INPUT", juce::String(std::max(inL.hold, inR.hold), 1) + " dB");
    }
    // COMPRESSION (gain reduction)
    {
        auto [bars, lab] = makeGroups(1);
        const float bw = 34.f;
        drawGRBar(g, { bars.getCentreX() - bw * 0.5f, bars.getY(), bw, bars.getHeight() }, grDb);
        drawGroupLabel(g, lab, "COMPRESSION", "-" + juce::String(grDb, 1) + " dB");
    }
    // OUTPUT
    {
        auto [bars, lab] = makeGroups(2);
        const float bw = 22.f, gap = 6.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, outL.db, outL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, outR.db, outR.hold);
        drawGroupLabel(g, lab, "OUTPUT", juce::String(std::max(outL.hold, outR.hold), 1) + " dB");
    }

    // STEREO field: correlation bar
    {
        g.setColour(dim);
        g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.2f));
        auto top = stereoArea.removeFromTop(16.f);
        g.drawText("STEREO", top.toNearestInt(), juce::Justification::centredLeft);
        const float width = proc.apvts.getRawParameterValue("width")->load();
        g.drawText("WIDTH " + juce::String((int) width) + "%    CORR " + juce::String(corrSmooth, 2),
                   top.toNearestInt(), juce::Justification::centredRight);

        auto track = stereoArea.removeFromTop(12.f).reduced(0.f, 2.f);
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

        g.setColour(dim.withAlpha(0.8f));
        g.setFont(mkFont(9.f));
        g.drawText("-1", stereoArea.withWidth(30.f).toNearestInt(), juce::Justification::centredLeft);
        g.drawText("0", stereoArea.withX(cx - 15.f).withWidth(30.f).toNearestInt(), juce::Justification::centred);
        g.drawText("+1", stereoArea.withX(track.getRight() - 30.f).withWidth(30.f).toNearestInt(), juce::Justification::centredRight);
    }
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
    paintPanel(g, rowPanel1);
    paintPanel(g, rowPanel2);

    g.setColour(gold.withAlpha(0.8f));
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.3f));
    g.drawText("DYNAMICS", juce::Rectangle<int>((int) rowPanel1.getX() + 14, (int) rowPanel1.getY() + 5, 140, 12), juce::Justification::centredLeft);
    g.drawText("TONE & SPACE", juce::Rectangle<int>((int) rowPanel2.getX() + 14, (int) rowPanel2.getY() + 5, 140, 12), juce::Justification::centredLeft);

    if (fading) g.endTransparencyLayer();
}
