#include "PluginEditor.h"

namespace
{
const char* const kGainIds[5] = { "eqlow", "eqlowmid", "eqhimid", "eqpres", "eqair" };
const char* const kFreqIds[5] = { "eqlowf", "eqlowmidf", "eqhimidf", "eqpresf", "eqairf" };
const char* const kNodeNames[5] = { "LOW", "LOW-MID", "HIGH-MID", "PRESENCE", "AIR" };
const float kFreqLo[5] = { 40.f, 150.f, 800.f, 3000.f, 6000.f };
const float kFreqHi[5] = { 400.f, 1000.f, 4000.f, 10000.f, 18000.f };
const float kFreqDef[5] = { 100.f, 350.f, 2000.f, 5000.f, 11000.f };

juce::File skinSettingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Audiokings").getChildFile("Vocals").getChildFile("skin.txt");
}
} // namespace

namespace akui
{
//==============================================================================
// Look and feel
//==============================================================================
void AKLookAndFeel::setSkin(const Skin& s)
{
    sk = &s;
    setColour(juce::ComboBox::backgroundColourId, s.panel.brighter(0.18f));
    setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.14f));
    setColour(juce::ComboBox::textColourId, s.text);
    setColour(juce::ComboBox::arrowColourId, s.accA);
    setColour(juce::PopupMenu::backgroundColourId, s.panel.darker(0.4f));
    setColour(juce::PopupMenu::textColourId, s.text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, s.accB.withAlpha(0.5f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId, s.gold);
    setColour(juce::TextButton::buttonColourId, s.panel.brighter(0.18f));
    setColour(juce::TextButton::textColourOffId, s.style == 2 ? s.text : s.accA);
    setColour(juce::TextButton::textColourOnId, s.style == 2 ? juce::Colour(0xff1b1410) : juce::Colours::white);
    setColour(juce::Label::textColourId, s.dim);
    setColour(juce::TooltipWindow::backgroundColourId, s.panel.darker(0.4f));
    setColour(juce::TooltipWindow::textColourId, s.text);
    setColour(juce::TooltipWindow::outlineColourId, s.accB);
    setColour(juce::AlertWindow::backgroundColourId, s.panel.darker(0.3f));
    setColour(juce::AlertWindow::textColourId, s.text);
}

void AKLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                         bool highlighted, bool down)
{
    const Skin& k = *sk;
    auto r = b.getLocalBounds().toFloat().reduced(1.f);
    const float rad = k.style == 1 ? 3.f : 7.f;

    if (b.getToggleState())
    {
        if (k.style == 0)
        {
            g.setColour(k.accA.withAlpha(0.22f));
            g.fillRoundedRectangle(r.expanded(2.f), rad + 2.f);
            juce::ColourGradient gr(k.accB, r.getX(), r.getY(), k.accA, r.getRight(), r.getBottom(), false);
            g.setGradientFill(gr);
        }
        else if (k.style == 1)
            g.setColour(k.accA);
        else
        {
            g.setColour(k.accA.withAlpha(0.25f));
            g.fillRoundedRectangle(r.expanded(2.f), rad + 2.f);
            juce::ColourGradient gr(k.accA.brighter(0.2f), 0.f, r.getY(), k.accB, 0.f, r.getBottom(), false);
            g.setGradientFill(gr);
        }
        g.fillRoundedRectangle(r, rad);
    }
    else
    {
        g.setColour(k.panel.brighter(down ? 0.35f : (highlighted ? 0.22f : 0.12f)));
        g.fillRoundedRectangle(r, rad);
        g.setColour(juce::Colours::white.withAlpha(highlighted ? 0.28f : 0.12f));
        g.drawRoundedRectangle(r, rad, 1.f);
    }
}

void AKLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                     float sa, float ea, juce::Slider& s)
{
    const Skin& k = *sk;
    auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(3.f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float hov = (float) s.getProperties().getWithDefault("hover", 0.0f);
    const bool big = (bool) s.getProperties().getWithDefault("big", false);
    const float arcR = r - 3.f;
    const float inner = arcR - (big ? 11.f : 8.f);
    const float ang = sa + pos * (ea - sa);

    float originPos = 0.f;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        originPos = (float) s.valueToProportionOfLength(0.0);
    else if (s.getProperties().contains("origin"))
        originPos = (float) s.valueToProportionOfLength((double) s.getProperties()["origin"]);
    const float angO = sa + originPos * (ea - sa);
    const float lo = juce::jmin(angO, ang), hi = juce::jmax(angO, ang);

    auto pointer = [&](float len0, float len1, float width, juce::Colour col)
    {
        juce::Path p;
        p.addRoundedRectangle(-width * 0.5f, -inner * len1, width, inner * (len1 - len0), width * 0.5f);
        p.applyTransform(juce::AffineTransform::rotation(ang).translated(c.x, c.y));
        g.setColour(col);
        g.fillPath(p);
    };

    if (k.style == 0)
    {
        // ---------- DIGITAL: neon arc ----------
        {
            juce::ColourGradient glow(k.accA.withAlpha(0.08f + 0.20f * hov + (big ? 0.06f : 0.f)), c.x, c.y,
                                      k.accA.withAlpha(0.f), c.x, c.y - r - 12.f, true);
            g.setGradientFill(glow);
            g.fillEllipse(c.x - r - 12.f, c.y - r - 12.f, (r + 12.f) * 2.f, (r + 12.f) * 2.f);
        }
        const float thick = big ? 5.5f : 3.5f;
        juce::Path track;
        track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, sa, ea, true);
        g.setColour(k.track);
        g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (hi - lo > 0.01f)
        {
            juce::Path v;
            v.addCentredArc(c.x, c.y, arcR, arcR, 0.f, lo, hi, true);
            juce::ColourGradient gr(k.accB, c.x - arcR, c.y, k.accA, c.x + arcR, c.y, false);
            auto glowG = gr;
            glowG.multiplyOpacity(0.25f + 0.2f * hov);
            g.setGradientFill(glowG);
            g.strokePath(v, juce::PathStrokeType(thick + 5.f + 3.f * hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill(gr);
            g.strokePath(v, juce::PathStrokeType(thick + hov, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        juce::ColourGradient body(k.knobTop, c.x, c.y - inner, k.knobBot, c.x, c.y + inner, false);
        g.setGradientFill(body);
        g.fillEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f);
        g.setColour(juce::Colours::white.withAlpha(0.08f + 0.10f * hov));
        g.drawEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f, 1.f);
        pointer(0.46f, 0.88f, big ? 4.f : 3.f, k.pointer.interpolatedWith(k.accA, 0.5f * hov));
    }
    else if (k.style == 1)
    {
        // ---------- TECHNICAL: flat, thin, precise ----------
        juce::Path track;
        track.addCentredArc(c.x, c.y, arcR, arcR, 0.f, sa, ea, true);
        g.setColour(k.track);
        g.strokePath(track, juce::PathStrokeType(2.f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        if (hi - lo > 0.01f)
        {
            juce::Path v;
            v.addCentredArc(c.x, c.y, arcR, arcR, 0.f, lo, hi, true);
            g.setColour(k.accA.withAlpha(0.75f + 0.25f * hov));
            g.strokePath(v, juce::PathStrokeType(2.5f + hov, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }
        g.setColour(k.knobBot);
        g.fillEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f);
        g.setColour(k.knobTop);
        g.drawEllipse(c.x - inner, c.y - inner, inner * 2.f, inner * 2.f, 1.5f);
        pointer(0.15f, 0.9f, 2.f, k.text.interpolatedWith(k.accA, 0.6f * hov));
    }
    else
    {
        // ---------- ANALOG: console knob with silk-screened ticks and a knurled cap ----------
        for (int t = 0; t <= 10; ++t)
        {
            const float a = sa + (float) t / 10.f * (ea - sa);
            const bool lit = a >= lo - 0.001f && a <= hi + 0.001f && (hi - lo) > 0.01f;
            const float s0 = std::sin(a), c0 = std::cos(a);
            g.setColour(lit ? k.accA : k.dim.withAlpha(0.7f));
            g.drawLine(c.x + s0 * (arcR - 1.f), c.y - c0 * (arcR - 1.f),
                       c.x + s0 * (arcR + 3.f), c.y - c0 * (arcR + 3.f), (t % 5 == 0) ? 2.2f : 1.4f);
        }
        // shadow + skirt
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(c.x - inner - 3.f, c.y - inner - 1.f, (inner + 3.f) * 2.f, (inner + 3.f) * 2.f);
        juce::ColourGradient skirt(juce::Colour(0xff2a211b), c.x, c.y - inner, juce::Colour(0xff0c0907), c.x, c.y + inner, false);
        g.setGradientFill(skirt);
        g.fillEllipse(c.x - inner - 2.f, c.y - inner - 2.f, (inner + 2.f) * 2.f, (inner + 2.f) * 2.f);
        // cap
        const float capR = inner - 2.f;
        juce::ColourGradient cap(k.knobTop.brighter(0.15f), c.x - capR * 0.5f, c.y - capR, k.knobBot, c.x + capR * 0.4f, c.y + capR, false);
        g.setGradientFill(cap);
        g.fillEllipse(c.x - capR, c.y - capR, capR * 2.f, capR * 2.f);
        // knurling
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        for (int j = 0; j < 40; ++j)
        {
            const float a = (float) j / 40.f * juce::MathConstants<float>::twoPi;
            const float s0 = std::sin(a), c0 = std::cos(a);
            g.drawLine(c.x + s0 * (capR - 3.f), c.y - c0 * (capR - 3.f), c.x + s0 * capR, c.y - c0 * capR, 1.f);
        }
        g.setColour(juce::Colours::white.withAlpha(0.18f + 0.15f * hov));
        g.drawEllipse(c.x - capR, c.y - capR, capR * 2.f, capR * 2.f, 1.f);
        pointer(0.25f, 0.9f, big ? 4.f : 3.f, k.pointer);
    }
}

//==============================================================================
// Knob
//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
           const juce::String& title, const juce::String& tooltip, Size sz, double origin)
    : size(sz), attachment(apvts, paramId, slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, sz == Small ? 76 : 90, sz == Small ? 16 : 18);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTooltip(tooltip);
    if (! std::isnan(origin))
        slider.getProperties().set("origin", origin);
    if (sz == Big)
        slider.getProperties().set("big", true);
    addAndMakeVisible(slider);

    label.setText(title, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(mkFont(sz == Big ? 14.f : (sz == Medium ? 11.5f : 10.5f), true).withExtraKerningFactor(0.12f));
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromTop(size == Big ? 22 : (size == Medium ? 16 : 14)));
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
// ChoiceButtons
//==============================================================================
ChoiceButtons::ChoiceButtons(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                             const juce::StringArray& labels, bool vert, const juce::String& tooltip)
    : param(apvts.getParameter(paramId)), vertical(vert)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add(new juce::TextButton(labels[i]));
        b->setTooltip(tooltip);
        b->onClick = [this, i]
        {
            if (param == nullptr) return;
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1((float) i));
            param->endChangeGesture();
        };
        addAndMakeVisible(b);
    }
}

void ChoiceButtons::resized()
{
    auto b = getLocalBounds();
    const int n = buttons.size();
    if (n == 0) return;
    const int gap = 3;
    if (vertical)
    {
        const int h = (b.getHeight() - gap * (n - 1)) / n;
        for (auto* btn : buttons) { btn->setBounds(b.removeFromTop(h)); b.removeFromTop(gap); }
    }
    else
    {
        const int w = (b.getWidth() - gap * (n - 1)) / n;
        for (auto* btn : buttons) { btn->setBounds(b.removeFromLeft(w)); b.removeFromLeft(gap); }
    }
}

void ChoiceButtons::sync()
{
    if (param == nullptr) return;
    const int cur = (int) std::lround(param->convertFrom0to1(param->getValue()));
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState(i == cur, juce::dontSendNotification);
}

//==============================================================================
// MainView
//==============================================================================
MainView::MainView(AudiokingsVocalsProcessor& p) : proc(p)
{
    setLookAndFeel(&laf);

    // ---- skin (per instance, falling back to the last one used) ----
    {
        int id = (int) proc.apvts.state.getProperty("skin", -1);
        if (id < 0)
        {
            auto f = skinSettingsFile();
            id = f.existsAsFile() ? f.loadFileAsString().trim().getIntValue() : 0;
        }
        skinId = juce::jlimit(0, 2, id);
        laf.setSkin(S());
    }

    // ---- header ----
    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1001 && id - 1001 < userNames.size()) proc.loadUserPreset(userNames[id - 1001]);
        else if (id >= 1) proc.setCurrentProgram(id - 1);
    };
    addAndMakeVisible(presetBox);

    prevBtn.onClick = [this] { stepPreset(-1); };
    nextBtn.onClick = [this] { stepPreset(1); };
    saveBtn.setTooltip("Save the current settings as your own preset");
    saveBtn.onClick = [this] { promptSavePreset(); };
    delBtn.setTooltip("Delete the selected user preset");
    delBtn.onClick = [this]
    {
        const auto name = proc.getCurrentUserPreset();
        if (name.isNotEmpty() && proc.deleteUserPreset(name))
            rebuildPresetBox();
    };
    abBtn.setTooltip("A/B compare: switch between two settings");
    abBtn.onClick = [this] { proc.toggleAB(); };
    copyBtn.setTooltip("Copy the current settings to the other slot");
    copyBtn.onClick = [this] { proc.copyToOtherAB(); };
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn, &abBtn, &copyBtn })
        addAndMakeVisible(b);

    skinBox.addItem("DIGITAL", 1);
    skinBox.addItem("TECHNICAL", 2);
    skinBox.addItem("ANALOG", 3);
    skinBox.setJustificationType(juce::Justification::centred);
    skinBox.setTooltip("Interface skin");
    skinBox.setSelectedId(skinId + 1, juce::dontSendNotification);
    skinBox.onChange = [this] { applySkin(skinBox.getSelectedId() - 1); };
    addAndMakeVisible(skinBox);

    lpBtn.setClickingTogglesState(true);
    lpBtn.setTooltip("Low-pass filter on/off (drag the handle in the EQ to set the frequency)");
    lpAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "lpon", lpBtn);
    addAndMakeVisible(lpBtn);

    rebuildPresetBox();

    // ---- knobs ----
    struct Spec { const char* id; const char* title; const char* tip; Knob::Size size; };
    const Spec specs[21] = {
        { "sens",      "LEVEL / SENS", "Input level + preamp sensitivity. Aim the loudest peaks at the OPTIMAL zone.", Knob::Big },
        { "noise",     "NOISE",        "Noise reduction (downward expander) between phrases", Knob::Small },
        { "compress",  "COMPRESS",     "Master compression amount (feeds the 1176 and the 670)", Knob::Medium },
        { "w1176",     "1176",         "How much of the compression is done by the 1176 (fast, punchy)", Knob::Small },
        { "w670",      "670",          "How much of the compression is done by the Fairchild 670 (smooth glue)", Knob::Small },
        { "pbass",     "BASS",         "Pultec low-end boost", Knob::Medium },
        { "pbassatt",  "ATTEN",        "Pultec low-end attenuation (use with BASS for the classic Pultec curve)", Knob::Small },
        { "pbassf",    "FREQ",         "Pultec bass frequency, 25 - 220 Hz", Knob::Small },
        { "ptreb",     "TREBLE",       "Pultec high boost", Knob::Medium },
        { "ptrebatt",  "ATTEN",        "Pultec high attenuation (use with TREBLE for the classic Pultec curve)", Knob::Small },
        { "ptrebf",    "FREQ",         "Pultec treble frequency, 4 - 20 kHz", Knob::Small },
        { "fx",        "FX",           "0-50%: warm saturation, parallel compression and chorus (pop). 50-100%: distortion, flanger and chamber reverb (experimental / rock)", Knob::Medium },
        { "revtone",   "TONE",         "Reverb tone: dark <-> bright", Knob::Medium },
        { "decay",     "DECAY",        "Reverb decay / tail length", Knob::Medium },
        { "revmix",    "MIX",          "Reverb level (parallel signal)", Knob::Small },
        { "delay",     "DELAY",        "Amount of echo (repeats)", Knob::Medium },
        { "delaytone", "TONE",         "Low: dark echoes that bloom into the reverb. High: bright, crisp echoes", Knob::Medium },
        { "delaymix",  "MIX",          "Delay level (parallel signal)", Knob::Small },
        { "airlow",    "AIR LOW",      "Harmonic exciter on the upper mids (presence and bite)", Knob::Medium },
        { "airhigh",   "AIR HIGH",     "Harmonic exciter on the highs (silky commercial shine)", Knob::Medium },
        { "output",    "OUTPUT",       "Output level", Knob::Medium }
    };
    for (const auto& s : specs)
    {
        knobs.push_back(std::make_unique<Knob>(proc.apvts, s.id, s.title, s.tip, s.size));
        addAndMakeVisible(*knobs.back());
        knobs.back()->setAlpha(0.f);
    }

    ratioBtns = std::make_unique<ChoiceButtons>(proc.apvts, "ratio1176", juce::StringArray { "4", "8", "12", "20", "ALL" }, false,
                                                "1176 ratio (ALL = all-buttons-in mode)");
    tcBtns    = std::make_unique<ChoiceButtons>(proc.apvts, "tc670", juce::StringArray { "1", "2", "3", "4", "5", "6" }, false,
                                                "Fairchild 670 time constant (1 = fast ... 6 = slow, program dependent)");
    revBtns   = std::make_unique<ChoiceButtons>(proc.apvts, "revtype", juce::StringArray { "ROOM", "PLATE", "HALL" }, true,
                                                "Reverb type");
    dlyBtns   = std::make_unique<ChoiceButtons>(proc.apvts, "delaydiv", juce::StringArray { "1/4", "1/8 DOT", "1/8", "1/16" }, true,
                                                "Delay time (tempo-synced)");
    for (auto* c : { ratioBtns.get(), tcBtns.get(), revBtns.get(), dlyBtns.get() })
    {
        addAndMakeVisible(c);
        c->setAlpha(0.f);
    }

    scopeImg = juce::Image(juce::Image::RGB, 150, 150, true);
    applySkin(skinId);

    setSize(kBaseW, kBaseH);     // last: resized() needs every child to exist
    startTimerHz(60);
}

MainView::~MainView()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainView::applySkin(int id)
{
    skinId = juce::jlimit(0, 2, id);
    laf.setSkin(S());
    proc.apvts.state.setProperty("skin", skinId, nullptr);

    auto f = skinSettingsFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText(juce::String(skinId));

    skinBox.setSelectedId(skinId + 1, juce::dontSendNotification);
    for (auto& k : knobs)
    {
        k->setLabelColour(S().dim);
        k->setValueColour(S().text);
    }
    sendLookAndFeelChange();
    repaint();
}

void MainView::rebuildPresetBox()
{
    userNames = proc.getUserPresetNames();
    presetBox.clear(juce::dontSendNotification);

    int id = 1;
    for (const auto& sec : ak::sections())
    {
        presetBox.addSectionHeading(sec.name);
        for (const auto& pr : sec.presets)
            presetBox.addItem(pr.name, id++);
    }
    if (userNames.size() > 0)
    {
        presetBox.addSectionHeading("USER PRESETS");
        for (int i = 0; i < userNames.size(); ++i)
            presetBox.addItem(userNames[i], 1001 + i);
    }
    syncPresetBox();
}

void MainView::syncPresetBox()
{
    int id = 0;
    const int cur = proc.getCurrentProgram();
    if (cur >= 0)
        id = cur + 1;
    else
    {
        const int idx = userNames.indexOf(proc.getCurrentUserPreset());
        if (idx >= 0) id = 1001 + idx;
    }
    if (presetBox.getSelectedId() != id)
        presetBox.setSelectedId(id, juce::dontSendNotification);
    delBtn.setEnabled(id >= 1001);
}

void MainView::stepPreset(int delta)
{
    std::vector<int> ids;
    for (int i = 1; i <= ak::numPresets(); ++i) ids.push_back(i);
    for (int i = 0; i < userNames.size(); ++i) ids.push_back(1001 + i);
    if (ids.empty()) return;

    int pos = 0;
    for (size_t i = 0; i < ids.size(); ++i)
        if (ids[i] == presetBox.getSelectedId()) { pos = (int) i; break; }

    const int n = (int) ids.size();
    pos = (pos + delta + n) % n;
    presetBox.setSelectedId(ids[(size_t) pos], juce::sendNotificationSync);
}

void MainView::promptSavePreset()
{
    auto* aw = new juce::AlertWindow("Save preset", "Name for your preset:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", proc.getCurrentUserPreset(), "");
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<MainView> safe(this);
    aw->enterModalState(true, juce::ModalCallbackFunction::create([safe, aw](int result)
    {
        if (result != 1 || safe == nullptr) return;
        const auto name = aw->getTextEditorContents("name");
        if (safe->proc.saveUserPreset(name))
            safe->rebuildPresetBox();
    }), true);
}

//==============================================================================
void MainView::resized()
{
    // ---- header controls ----
    juce::Rectangle<int> h(0, 16, getWidth() - 22, 32);
    skinBox.setBounds(h.removeFromRight(120));  h.removeFromRight(6);
    copyBtn.setBounds(h.removeFromRight(54));   h.removeFromRight(6);
    abBtn.setBounds(h.removeFromRight(46));     h.removeFromRight(6);
    delBtn.setBounds(h.removeFromRight(46));    h.removeFromRight(6);
    saveBtn.setBounds(h.removeFromRight(56));   h.removeFromRight(6);
    nextBtn.setBounds(h.removeFromRight(32));   h.removeFromRight(6);
    presetBox.setBounds(h.removeFromRight(260)); h.removeFromRight(6);
    prevBtn.setBounds(h.removeFromRight(32));

    // ---- top zone ----
    eqPanel = { 16.f, 72.f, 830.f, 322.f };
    meterPanel = { 858.f, 72.f, 486.f, 322.f };
    eqPlot = eqPanel.reduced(18.f, 0.f).withTrimmedTop(46.f).withTrimmedBottom(30.f);
    lpBtn.setBounds((int) eqPanel.getRight() - 18 - 96, (int) eqPanel.getY() + 10, 96, 22);

    // ---- console sections ----
    sections.clear();
    sections.push_back({ { 16.f,   402.f, 300.f, 230.f }, "INPUT" });
    sections.push_back({ { 322.f,  402.f, 330.f, 230.f }, "COMPRESSION  -  1176 + FAIRCHILD 670" });
    sections.push_back({ { 658.f,  402.f, 520.f, 230.f }, "PULTEC EQ" });
    sections.push_back({ { 1184.f, 402.f, 160.f, 230.f }, "FX" });
    sections.push_back({ { 16.f,   640.f, 430.f, 224.f }, "REVERB  -  PARALLEL" });
    sections.push_back({ { 452.f,  640.f, 430.f, 224.f }, "DELAY  -  PARALLEL" });
    sections.push_back({ { 888.f,  640.f, 270.f, 224.f }, "AIR  -  HARMONIC EXCITER" });
    sections.push_back({ { 1164.f, 640.f, 180.f, 224.f }, "OUT" });

    if (knobs.size() < 21)
        return;

    auto put = [this](size_t i, int x, int y, int w, int hh) { knobs[i]->setBounds(x, y, w, hh); };

    // INPUT
    put(0, 24, 428, 150, 196);
    ledRect = { 182.f, 440.f, 52.f, 178.f };
    put(1, 236, 468, 76, 112);
    // COMPRESSION
    put(2, 330, 436, 110, 140);
    put(3, 446, 452, 84, 110);
    put(4, 534, 452, 84, 110);
    ratioBtns->setBounds(398, 586, 244, 20);
    tcBtns->setBounds(398, 608, 244, 20);
    // PULTEC
    put(5, 664, 440, 100, 150);  put(6, 766, 458, 70, 112);  put(7, 838, 458, 70, 112);
    put(8, 924, 440, 100, 150);  put(9, 1026, 458, 70, 112); put(10, 1098, 458, 70, 112);
    // FX
    put(11, 1198, 440, 130, 160);
    fxBar = { 1196.f, 606.f, 134.f, 14.f };
    // REVERB
    revBtns->setBounds(26, 686, 100, 102);
    put(12, 140, 672, 96, 150);  put(13, 240, 672, 96, 150);  put(14, 344, 690, 84, 120);
    // DELAY
    dlyBtns->setBounds(462, 686, 100, 116);
    put(15, 572, 672, 96, 150);  put(16, 672, 672, 96, 150);  put(17, 776, 690, 84, 120);
    // AIR + OUT
    put(18, 900, 672, 120, 170);  put(19, 1028, 672, 120, 170);
    put(20, 1190, 672, 120, 170);
}

//==============================================================================
float MainView::freqToX(float f) const
{
    return eqPlot.getX() + eqPlot.getWidth() * std::log10(f / 20.f) / 3.f;
}

float MainView::getP(const char* id) const
{
    return proc.apvts.getRawParameterValue(id)->load();
}

void MainView::setP(const char* id, float v)
{
    if (auto* p = proc.apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(v));
}

juce::Point<float> MainView::nodePos(int i) const
{
    if (i == 5)
        return { freqToX(getP("lpf")), eqPlot.getY() + 14.f };
    const float f = getP(kFreqIds[i]);
    const float g = juce::jlimit(-15.f, 15.f, getP(kGainIds[i]));
    return { freqToX(f), eqPlot.getCentreY() - g / 15.f * eqPlot.getHeight() * 0.5f };
}

int MainView::hitNode(juce::Point<float> p) const
{
    int best = -1;
    float bestD = 16.f * 16.f;
    for (int i = 0; i < 6; ++i)
    {
        const auto np = nodePos(i);
        const float dx = np.x - p.x, dy = np.y - p.y;
        const float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void MainView::setNodeFromPos(int i, juce::Point<float> p)
{
    const float xn = juce::jlimit(0.f, 1.f, (p.x - eqPlot.getX()) / eqPlot.getWidth());
    const float f = 20.f * std::pow(1000.f, xn);
    if (i == 5)
    {
        setP("lpf", std::round(juce::jlimit(3000.f, 20000.f, f)));
        return;
    }
    float g = (eqPlot.getCentreY() - p.y) / (eqPlot.getHeight() * 0.5f) * 15.f;
    g = juce::jlimit(-12.f, 12.f, g);
    if (std::abs(g) < 0.25f) g = 0.f;
    g = std::round(g * 10.f) / 10.f;
    setP(kFreqIds[i], std::round(juce::jlimit(kFreqLo[i], kFreqHi[i], f)));
    setP(kGainIds[i], g);
}

void MainView::mouseMove(const juce::MouseEvent& e)
{
    hoverNode = hitNode(e.position);
    setMouseCursor(hoverNode >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void MainView::mouseExit(const juce::MouseEvent&)
{
    if (dragNode < 0) hoverNode = -1;
}

void MainView::mouseDown(const juce::MouseEvent& e)
{
    dragNode = hitNode(e.position);
    if (dragNode < 0) return;
    hoverNode = dragNode;

    const char* ids[2] = { dragNode == 5 ? "lpf" : kGainIds[dragNode], dragNode == 5 ? "lpon" : kFreqIds[dragNode] };
    for (auto* id : ids)
        if (auto* p = proc.apvts.getParameter(id)) p->beginChangeGesture();

    if (dragNode == 5) setP("lpon", 1.f);     // grabbing the handle switches the filter on
    setNodeFromPos(dragNode, e.position);
}

void MainView::mouseDrag(const juce::MouseEvent& e)
{
    if (dragNode >= 0)
        setNodeFromPos(dragNode, e.position);
}

void MainView::mouseUp(const juce::MouseEvent& e)
{
    if (dragNode < 0) return;
    const char* ids[2] = { dragNode == 5 ? "lpf" : kGainIds[dragNode], dragNode == 5 ? "lpon" : kFreqIds[dragNode] };
    for (auto* id : ids)
        if (auto* p = proc.apvts.getParameter(id)) p->endChangeGesture();
    dragNode = -1;
    hoverNode = hitNode(e.position);
}

void MainView::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int n = hitNode(e.position);
    if (n < 0) return;
    if (n == 5) { setP("lpon", getP("lpon") > 0.5f ? 0.f : 1.f); return; }
    setP(kGainIds[n], 0.f);
    setP(kFreqIds[n], kFreqDef[n]);
}

//==============================================================================
void MainView::timerCallback()
{
    phase += 1.f / 60.f;
    intro += (1.f - intro) * 0.06f;

    inL.update(juce::Decibels::gainToDecibels(proc.meterInL.exchange(0.f), -100.f));
    inR.update(juce::Decibels::gainToDecibels(proc.meterInR.exchange(0.f), -100.f));
    outL.update(juce::Decibels::gainToDecibels(proc.meterOutL.exchange(0.f), -100.f));
    outR.update(juce::Decibels::gainToDecibels(proc.meterOutR.exchange(0.f), -100.f));

    auto fall = [](float& cur, float target) { cur = target > cur ? target : std::max(0.f, cur - 0.5f); };
    fall(gr1176Db, proc.meterGR1176.exchange(0.f));
    fall(gr670Db, proc.meterGR670.exchange(0.f));
    fall(grDb, proc.meterGRTotal.exchange(0.f));
    grHist[(size_t) histPos] = grDb;
    histPos = (histPos + 1) % histN;

    corrSmooth += (proc.correlation.load() - corrSmooth) * 0.15f;

    proc.copyScope(scopeL.data(), scopeR.data());
    updateSpectrum();
    updateCurve();
    updateVectorscope();

    const float a0 = juce::jlimit(0.f, 1.f, intro * 1.6f);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i]->tick();
        knobs[i]->setAlpha(juce::jlimit(0.f, 1.f, intro * 1.6f - (float) i * 0.03f));
    }
    for (auto* c : { ratioBtns.get(), tcBtns.get(), revBtns.get(), dlyBtns.get() })
    {
        c->sync();
        c->setAlpha(a0);
    }

    syncPresetBox();
    const int slot = proc.getABSlot();
    abBtn.setButtonText(slot == 0 ? "A" : "B");
    copyBtn.setButtonText(slot == 0 ? "A>B" : "B>A");

    repaint();
}

void MainView::updateSpectrum()
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
        db += 4.5f * std::log2(f0 / 1000.f);
        specDb[(size_t) i] = db > specDb[(size_t) i] ? db : specDb[(size_t) i] + (db - specDb[(size_t) i]) * 0.15f;
    }
}

void MainView::updateCurve()
{
    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;

    ak::EqParams ep {};
    ep.hpf = getP("hpf");
    for (int i = 0; i < 5; ++i) { ep.g[i] = getP(kGainIds[i]); ep.f[i] = getP(kFreqIds[i]); }
    ep.lpOn = getP("lpon") > 0.5f;
    ep.lpf = getP("lpf");
    const auto e = ak::makeEq(sr, ep);

    const ak::PultecParams pp { getP("pbass"), getP("pbassatt"), getP("pbassf"),
                                getP("ptreb"), getP("ptrebatt"), getP("ptrebf") };
    const auto ps = ak::makePultec(sr, pp);

    for (int i = 0; i < numPts; ++i)
    {
        const double f = 20.0 * std::pow(1000.0, (double) i / (double) (numPts - 1));
        double mag = ak::magnitudeAt(e.hp, f, sr);
        for (int k = 0; k < 5; ++k) mag *= ak::magnitudeAt(e.n[k], f, sr);
        if (e.lpOn) mag *= ak::magnitudeAt(e.lp1, f, sr) * ak::magnitudeAt(e.lp2, f, sr);
        curveDb[(size_t) i] = (float) (20.0 * std::log10(std::max(mag, 1.0e-6)));

        const double pm = ak::magnitudeAt(ps.bBoost, f, sr) * ak::magnitudeAt(ps.bAtt, f, sr)
                        * ak::magnitudeAt(ps.tBoost, f, sr) * ak::magnitudeAt(ps.tAtt, f, sr);
        pultecDb[(size_t) i] = (float) (20.0 * std::log10(std::max(pm, 1.0e-6)));
    }
}

void MainView::updateVectorscope()
{
    juce::Graphics ig(scopeImg);
    const float s = (float) scopeImg.getWidth();
    const float cx = s * 0.5f, cy = s * 0.5f, rad = cx - 4.f;

    ig.setColour(juce::Colour(0xff080a14).withAlpha(0.22f));
    ig.fillAll();

    juce::Path path;
    const int N = 600;
    for (int k = 0; k < N; ++k)
    {
        const size_t idx = (size_t) (AudiokingsVocalsProcessor::scopeSize - N + k);
        const float L = scopeL[idx], R = scopeR[idx];
        const float x = juce::jlimit(-1.f, 1.f, (R - L) * 0.7071f * 2.5f);
        const float y = juce::jlimit(-1.f, 1.f, (L + R) * 0.7071f * 2.5f);
        const float px = cx + x * rad, py = cy - y * rad;
        if (k == 0) path.startNewSubPath(px, py); else path.lineTo(px, py);
    }
    ig.setColour(S().accA.withAlpha(0.85f));
    ig.strokePath(path, juce::PathStrokeType(1.1f));
}

//==============================================================================
// Painting
//==============================================================================
void MainView::paintBackground(juce::Graphics& g)
{
    const auto& k = S();
    const float w = (float) getWidth(), h = (float) getHeight();
    juce::ColourGradient bg(k.bgTop, 0.f, 0.f, k.bgBottom, w, h, false);
    g.setGradientFill(bg);
    g.fillAll();

    if (k.style == 0)
    {
        auto blob = [&](juce::Colour c, float cx, float cy, float r, float a)
        {
            juce::ColourGradient gr(c.withAlpha(a), cx, cy, c.withAlpha(0.f), cx + r, cy, true);
            g.setGradientFill(gr);
            g.fillEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
        };
        blob(k.accB, w * 0.20f + 120.f * std::sin(phase * 0.35f), h * 0.30f + 60.f * std::cos(phase * 0.27f), 460.f, 0.22f);
        blob(k.accA, w * 0.80f + 100.f * std::cos(phase * 0.30f), h * 0.25f + 50.f * std::sin(phase * 0.40f), 420.f, 0.14f);
        blob(k.accC, w * 0.50f + 160.f * std::sin(phase * 0.20f), h * 0.95f, 480.f, 0.10f);
    }
    else if (k.style == 2)
    {
        g.setColour(juce::Colours::white.withAlpha(0.018f));
        for (float y = 0.f; y < h; y += 4.f)
            g.fillRect(0.f, y, w, 1.f);
        juce::ColourGradient vig(juce::Colours::transparentBlack, w * 0.5f, h * 0.5f,
                                 juce::Colours::black.withAlpha(0.4f), 0.f, 0.f, true);
        g.setGradientFill(vig);
        g.fillAll();
    }
}

void MainView::paintPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label)
{
    const auto& k = S();
    if (k.style == 0)
    {
        g.setColour(k.panel.withAlpha(0.78f));
        g.fillRoundedRectangle(r, 14.f);
        juce::ColourGradient edge(juce::Colours::white.withAlpha(0.16f), r.getX(), r.getY(),
                                  juce::Colours::white.withAlpha(0.03f), r.getX(), r.getBottom(), false);
        g.setGradientFill(edge);
        g.drawRoundedRectangle(r.reduced(0.5f), 14.f, 1.f);
    }
    else if (k.style == 1)
    {
        g.setColour(k.panel);
        g.fillRoundedRectangle(r, 5.f);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawRoundedRectangle(r.reduced(0.5f), 5.f, 1.f);
    }
    else
    {
        juce::ColourGradient mt(k.panel.brighter(0.10f), 0.f, r.getY(), k.panel.darker(0.20f), 0.f, r.getBottom(), false);
        g.setGradientFill(mt);
        g.fillRoundedRectangle(r, 6.f);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawRoundedRectangle(r.reduced(1.f), 6.f, 1.f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(r, 6.f, 1.5f);
        // screws
        const float sx[2] = { r.getX() + 9.f, r.getRight() - 9.f };
        const float sy[2] = { r.getY() + 9.f, r.getBottom() - 9.f };
        for (float x : sx)
            for (float y : sy)
            {
                juce::ColourGradient sg(juce::Colour(0xffb9b0a0), x - 3.f, y - 3.f, juce::Colour(0xff4a4237), x + 3.f, y + 3.f, false);
                g.setGradientFill(sg);
                g.fillEllipse(x - 3.5f, y - 3.5f, 7.f, 7.f);
                g.setColour(juce::Colours::black.withAlpha(0.6f));
                g.drawLine(x - 2.5f, y + 1.f, x + 2.5f, y - 1.f, 1.2f);
            }
    }

    if (label.isNotEmpty())
    {
        const bool analog = k.style == 2;
        g.setColour(analog ? k.text.withAlpha(0.9f) : (k.style == 1 ? k.dim : k.gold.withAlpha(0.85f)));
        g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.28f));
        g.drawText(label, juce::Rectangle<int>((int) r.getX() + 18, (int) r.getY() + 6, (int) r.getWidth() - 36, 14),
                   juce::Justification::centredLeft);
        g.setColour(analog ? k.text.withAlpha(0.25f) : juce::Colours::white.withAlpha(0.08f));
        const float lx = r.getX() + 18.f + (float) label.length() * 7.6f + 10.f;
        if (lx < r.getRight() - 20.f)
            g.drawHorizontalLine((int) r.getY() + 13, lx, r.getRight() - 18.f);
    }
}

void MainView::paintHeader(juce::Graphics& g)
{
    const auto& k = S();
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

        if (k.style == 0)
        {
            g.setColour(k.gold.withAlpha(0.18f + 0.08f * std::sin(phase * 2.f)));
            g.fillEllipse(x - 10.f, y - 8.f, w + 20.f, h + 16.f);
        }
        juce::ColourGradient cg(k.gold.brighter(0.3f), x, y, k.gold.darker(0.4f), x, y + h, false);
        g.setGradientFill(cg);
        g.fillPath(crown);
        g.fillEllipse(x - 2.f, y + h * 0.25f - 3.f, 6.f, 6.f);
        g.fillEllipse(x + w * 0.5f - 3.f, y - 3.f, 6.f, 6.f);
        g.fillEllipse(x + w - 4.f, y + h * 0.25f - 3.f, 6.f, 6.f);
    }
    {
        const float tx = 70.f, tw = 190.f;
        if (k.style == 0)
        {
            const float t = std::fmod(phase * 0.3f, 1.8f) - 0.4f;
            juce::ColourGradient tg(k.gold, tx, 0.f, k.gold, tx + tw, 0.f, false);
            tg.addColour(juce::jlimit(0.0, 1.0, (double) t - 0.12), k.gold);
            tg.addColour(juce::jlimit(0.0, 1.0, (double) t), juce::Colours::white);
            tg.addColour(juce::jlimit(0.0, 1.0, (double) t + 0.12), k.gold);
            g.setGradientFill(tg);
        }
        else
            g.setColour(k.gold);
        g.setFont(mkFont(23.f, true).withExtraKerningFactor(0.12f));
        g.drawText("AUDIOKINGS", juce::Rectangle<int>((int) tx, 8, (int) tw, 28), juce::Justification::centredLeft);

        g.setColour(k.dim);
        g.setFont(mkFont(10.5f, true).withExtraKerningFactor(0.3f));
        g.drawText("VOCALS  -  CHANNEL STRIP", juce::Rectangle<int>((int) tx, 36, 260, 14), juce::Justification::centredLeft);
    }
}

void MainView::paintEq(juce::Graphics& g)
{
    const auto& k = S();
    paintPanel(g, eqPanel, "EQ  -  LIVE RESPONSE");

    // legend + hint
    g.setFont(mkFont(10.f));
    const float ly = eqPanel.getY() + 21.f;
    g.setColour(k.accA);
    g.drawLine(eqPanel.getX() + 20.f, ly, eqPanel.getX() + 38.f, ly, 2.f);
    g.setColour(k.dim);
    g.drawText("EQ", juce::Rectangle<int>((int) eqPanel.getX() + 42, (int) ly - 7, 30, 14), juce::Justification::centredLeft);
    g.setColour(k.gold);
    g.drawLine(eqPanel.getX() + 76.f, ly, eqPanel.getX() + 94.f, ly, 1.4f);
    g.setColour(k.dim);
    g.drawText("PULTEC", juce::Rectangle<int>((int) eqPanel.getX() + 98, (int) ly - 7, 60, 14), juce::Justification::centredLeft);
    g.drawText("drag the nodes  -  double-click to reset",
               juce::Rectangle<int>((int) eqPanel.getX() + 200, (int) ly - 7, 360, 14), juce::Justification::centredLeft);

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

    g.setColour(k.dim.withAlpha(0.8f));
    g.setFont(mkFont(9.5f));
    const std::pair<float, const char*> labels[] = { { 50.f, "50" }, { 100.f, "100" }, { 500.f, "500" }, { 1000.f, "1k" },
                                                     { 5000.f, "5k" }, { 10000.f, "10k" } };
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
        juce::ColourGradient ag(k.accB.withAlpha(0.38f), 0.f, plot.getY(), k.accB.withAlpha(0.f), 0.f, plot.getBottom(), false);
        g.setGradientFill(ag);
        g.fillPath(area);
        g.setColour(k.accB.withAlpha(0.6f));
        g.strokePath(line, juce::PathStrokeType(1.2f));
    }

    auto yOf = [&](float db) { return plot.getCentreY() - juce::jlimit(-15.f, 15.f, db) / 15.f * plot.getHeight() * 0.5f; };

    // low-pass region
    {
        const float lx = freqToX(getP("lpf"));
        const bool on = getP("lpon") > 0.5f;
        if (on)
        {
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.fillRect(lx, plot.getY(), plot.getRight() - lx, plot.getHeight());
        }
        g.setColour(k.warn.withAlpha(on ? 0.8f : 0.25f));
        g.drawVerticalLine((int) lx, plot.getY(), plot.getBottom());
    }

    // Pultec curve (thin) and EQ curve
    {
        juce::Path pc;
        for (int i = 0; i < numPts; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (numPts - 1);
            const float y = yOf(pultecDb[(size_t) i]);
            if (i == 0) pc.startNewSubPath(x, y); else pc.lineTo(x, y);
        }
        g.setColour(k.gold.withAlpha(0.9f));
        g.strokePath(pc, juce::PathStrokeType(1.4f));
    }

    juce::Path curve;
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
        g.setColour(k.accA.withAlpha(0.10f));
        g.fillPath(area);

        juce::ColourGradient cg(k.accA, plot.getX(), 0.f, k.accC, plot.getRight(), 0.f, false);
        if (k.style == 0)
        {
            auto glow = cg;
            glow.multiplyOpacity(0.25f);
            g.setGradientFill(glow);
            g.strokePath(curve, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setGradientFill(cg);
        g.strokePath(curve, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.restoreState();

    // draggable nodes
    const juce::Colour nodeCol[5] = { k.accA, k.accB, k.accC, k.gold, k.ok };
    for (int i = 0; i < 5; ++i)
    {
        const auto np = nodePos(i);
        const juce::Colour col = nodeCol[i];
        const bool active = (i == hoverNode || i == dragNode);
        const float pulse = 6.f + 1.4f * std::sin(phase * 3.f + (float) i) + (active ? 3.f : 0.f);

        g.setColour(col.withAlpha(0.25f));
        g.drawLine(np.x, np.y, np.x, plot.getCentreY(), 1.f);
        g.setColour(col.withAlpha(active ? 0.32f : 0.16f));
        g.fillEllipse(np.x - pulse * 1.7f, np.y - pulse * 1.7f, pulse * 3.4f, pulse * 3.4f);
        g.setColour(col);
        g.fillEllipse(np.x - 7.f, np.y - 7.f, 14.f, 14.f);
        g.setColour(juce::Colours::white.withAlpha(active ? 1.f : 0.85f));
        g.drawEllipse(np.x - 7.f, np.y - 7.f, 14.f, 14.f, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(mkFont(9.f, true));
        g.drawText(juce::String(kNodeNames[i]), juce::Rectangle<int>((int) np.x - 32, (int) np.y - 26, 64, 12), juce::Justification::centred);
    }
    // low-pass handle
    {
        const auto np = nodePos(5);
        const bool on = getP("lpon") > 0.5f;
        const bool active = hoverNode == 5 || dragNode == 5;
        g.setColour(k.warn.withAlpha(on ? 1.f : 0.45f));
        g.fillRoundedRectangle(np.x - 8.f, np.y - 8.f, 16.f, 16.f, 4.f);
        g.setColour(juce::Colours::white.withAlpha(active ? 1.f : 0.7f));
        g.drawRoundedRectangle(np.x - 8.f, np.y - 8.f, 16.f, 16.f, 4.f, 1.4f);
        g.setColour(juce::Colour(0xff111111));
        g.setFont(mkFont(8.f, true));
        g.drawText("LP", juce::Rectangle<int>((int) np.x - 8, (int) np.y - 6, 16, 12), juce::Justification::centred);
    }

    // read-out for the node under the mouse
    const int shown = dragNode >= 0 ? dragNode : hoverNode;
    if (shown >= 0)
    {
        juce::String text;
        juce::Colour col = k.warn;
        auto fmtF = [](float f) { return f >= 1000.f ? juce::String(f / 1000.f, 2) + " kHz" : juce::String((int) f) + " Hz"; };
        if (shown == 5)
            text = "LOW-PASS   " + fmtF(getP("lpf")) + (getP("lpon") > 0.5f ? "   ON" : "   OFF");
        else
        {
            const float gdb = getP(kGainIds[shown]);
            text = juce::String(kNodeNames[shown]) + "   " + fmtF(getP(kFreqIds[shown])) + "   " + (gdb >= 0.f ? "+" : "") + juce::String(gdb, 1) + " dB";
            col = nodeCol[shown];
        }
        auto pill = juce::Rectangle<float>(plot.getCentreX() - 130.f, plot.getY() + 2.f, 260.f, 20.f);
        g.setColour(juce::Colour(0xff05060d).withAlpha(0.85f));
        g.fillRoundedRectangle(pill, 10.f);
        g.setColour(col);
        g.drawRoundedRectangle(pill, 10.f, 1.f);
        g.setColour(k.text);
        g.setFont(mkFont(11.f, true));
        g.drawText(text, pill.toNearestInt(), juce::Justification::centred);
    }
}

void MainView::drawLevelBar(juce::Graphics& g, juce::Rectangle<float> bar, float db, float hold)
{
    const auto& k = S();
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(bar, 3.f);

    const float yv = juce::jmap(juce::jlimit(-60.f, 3.f, db), -60.f, 3.f, bar.getBottom(), bar.getY());
    juce::ColourGradient gr(k.hot, 0.f, bar.getY(), k.accA, 0.f, bar.getBottom(), false);
    gr.addColour(0.05, k.hot);
    gr.addColour(0.16, k.warn);
    gr.addColour(0.40, k.ok);
    g.setGradientFill(gr);
    g.fillRect(juce::Rectangle<float>(bar.getX(), yv, bar.getWidth(), bar.getBottom() - yv));

    g.setColour(k.panel);
    for (float yy = bar.getY(); yy < bar.getBottom(); yy += 3.f)
        g.fillRect(bar.getX(), yy, bar.getWidth(), 1.f);

    const float yh = juce::jmap(juce::jlimit(-60.f, 3.f, hold), -60.f, 3.f, bar.getBottom(), bar.getY());
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillRect(bar.getX(), yh - 1.f, bar.getWidth(), 2.f);
}

void MainView::drawGRBar(juce::Graphics& g, juce::Rectangle<float> bar, float gr, juce::Colour c1, juce::Colour c2)
{
    const auto& k = S();
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(bar, 3.f);

    const float hgt = juce::jmap(juce::jlimit(0.f, 18.f, gr), 0.f, 18.f, 0.f, bar.getHeight());
    juce::ColourGradient grad(c1, 0.f, bar.getY(), c2, 0.f, bar.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRect(juce::Rectangle<float>(bar.getX(), bar.getY(), bar.getWidth(), hgt));

    g.setColour(k.panel);
    for (float yy = bar.getY(); yy < bar.getBottom(); yy += 3.f)
        g.fillRect(bar.getX(), yy, bar.getWidth(), 1.f);

    g.setColour(juce::Colours::white.withAlpha(0.28f));
    for (int d : { 3, 6, 9, 12, 15 })
    {
        const float y = bar.getY() + (float) d / 18.f * bar.getHeight();
        g.fillRect(bar.getX(), y, 4.f, 1.f);
        g.fillRect(bar.getRight() - 4.f, y, 4.f, 1.f);
    }
}

void MainView::drawGroupLabel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, const juce::String& value)
{
    const auto& k = S();
    g.setColour(k.text);
    g.setFont(mkFont(10.f));
    g.drawText(value, r.removeFromTop(14.f).toNearestInt(), juce::Justification::centred);
    g.setColour(k.dim);
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.15f));
    g.drawText(name, r.removeFromTop(14.f).toNearestInt(), juce::Justification::centred);
}

void MainView::paintMeters(juce::Graphics& g)
{
    const auto& k = S();
    paintPanel(g, meterPanel, "METERS  -  COMPRESSION  -  STEREO");

    auto inner = meterPanel.reduced(18.f, 10.f);
    inner.removeFromTop(24.f);
    auto strip = inner.removeFromBottom(58.f);
    inner.removeFromBottom(8.f);
    auto leftA = inner.removeFromLeft(250.f);
    inner.removeFromLeft(10.f);
    auto rightA = inner;

    // ---- bars: INPUT | 1176 | 670 | OUTPUT ----
    const float groupW = leftA.getWidth() / 4.f;
    const float labelH = 28.f;
    auto makeGroup = [&](int idx)
    {
        auto grp = leftA.withX(leftA.getX() + groupW * (float) idx).withWidth(groupW);
        auto lab = grp.removeFromBottom(labelH);
        return std::make_pair(grp.reduced(0.f, 2.f), lab);
    };
    {
        auto [bars, lab] = makeGroup(0);
        const float bw = 18.f, gap = 4.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, inL.db, inL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, inR.db, inR.hold);
        drawGroupLabel(g, lab, "INPUT", juce::String(std::max(inL.hold, inR.hold), 1));
    }
    {
        auto [bars, lab] = makeGroup(1);
        const float bw = 26.f;
        drawGRBar(g, { bars.getCentreX() - bw * 0.5f, bars.getY(), bw, bars.getHeight() }, gr1176Db, k.accC, k.accB);
        drawGroupLabel(g, lab, "1176", "-" + juce::String(gr1176Db, 1));
    }
    {
        auto [bars, lab] = makeGroup(2);
        const float bw = 26.f;
        drawGRBar(g, { bars.getCentreX() - bw * 0.5f, bars.getY(), bw, bars.getHeight() }, gr670Db, k.accB, k.accA);
        drawGroupLabel(g, lab, "670", "-" + juce::String(gr670Db, 1));
    }
    {
        auto [bars, lab] = makeGroup(3);
        const float bw = 18.f, gap = 4.f;
        const float x0 = bars.getCentreX() - bw - gap * 0.5f;
        drawLevelBar(g, { x0, bars.getY(), bw, bars.getHeight() }, outL.db, outL.hold);
        drawLevelBar(g, { x0 + bw + gap, bars.getY(), bw, bars.getHeight() }, outR.db, outR.hold);
        drawGroupLabel(g, lab, "OUTPUT", juce::String(std::max(outL.hold, outR.hold), 1));
    }

    // ---- vectorscope + correlation ----
    {
        const float side = 150.f;
        juce::Rectangle<float> scope(rightA.getCentreX() - side * 0.5f, rightA.getY() + 8.f, side, side);

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

        g.setColour(k.dim);
        g.setFont(mkFont(9.5f, true));
        g.drawText("L", juce::Rectangle<int>((int) (c.x - d - 12.f), (int) (c.y - d - 12.f), 14, 12), juce::Justification::centred);
        g.drawText("R", juce::Rectangle<int>((int) (c.x + d - 2.f), (int) (c.y - d - 12.f), 14, 12), juce::Justification::centred);
        g.drawText("M", juce::Rectangle<int>((int) c.x - 7, (int) scope.getY() - 11, 14, 12), juce::Justification::centred);

        auto corr = rightA.withTop(scope.getBottom() + 8.f);
        auto top = corr.removeFromTop(14.f);
        g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.2f));
        g.drawText("STEREO", top.toNearestInt(), juce::Justification::centredLeft);
        g.drawText("CORR " + juce::String(corrSmooth, 2), top.toNearestInt(), juce::Justification::centredRight);

        auto track = corr.removeFromTop(12.f).reduced(0.f, 2.f);
        g.setColour(k.meterBg);
        g.fillRoundedRectangle(track, 4.f);
        const float cx = track.getCentreX();
        const float vx = cx + juce::jlimit(-1.f, 1.f, corrSmooth) * track.getWidth() * 0.5f;
        const auto col = corrSmooth >= 0.f ? k.ok : k.hot;
        juce::ColourGradient cg(col.withAlpha(0.35f), cx, 0.f, col, vx, 0.f, false);
        g.setGradientFill(cg);
        g.fillRoundedRectangle(juce::Rectangle<float>(std::min(cx, vx), track.getY(), std::abs(vx - cx), track.getHeight()), 4.f);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(cx - 0.5f, track.getY() - 2.f, 1.f, track.getHeight() + 4.f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(vx - 3.f, track.getCentreY() - 3.f, 6.f, 6.f);
    }

    // ---- overall gain reduction panel ----
    {
        auto num = strip.removeFromLeft(130.f);
        strip.removeFromLeft(8.f);

        const bool active = grDb > 0.2f;
        const juce::Colour grCol = grDb > 12.f ? k.hot : (grDb > 6.f ? k.accC : k.accB);
        g.setColour(active ? grCol : k.dim);
        g.setFont(mkFont(24.f, true));
        g.drawText((active ? "-" : "") + juce::String(grDb, 1) + " dB", num.removeFromTop(32.f).toNearestInt(), juce::Justification::centredLeft);
        g.setColour(k.dim);
        g.setFont(mkFont(9.f, true).withExtraKerningFactor(0.18f));
        g.drawText(active ? "GAIN REDUCTION" : "NO COMPRESSION", num.toNearestInt(), juce::Justification::centredLeft);

        g.setColour(k.meterBg);
        g.fillRoundedRectangle(strip, 6.f);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        for (int d : { 6, 12 })
            g.drawHorizontalLine((int) (strip.getY() + (float) d / 24.f * strip.getHeight()), strip.getX(), strip.getRight());

        juce::Path line;
        for (int i = 0; i < histN; ++i)
        {
            const float v = grHist[(size_t) ((histPos + i) % histN)];
            const float x = strip.getX() + strip.getWidth() * (float) i / (float) (histN - 1);
            const float y = strip.getY() + juce::jlimit(0.f, 24.f, v) / 24.f * strip.getHeight();
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(strip.getRight(), strip.getY());
        area.lineTo(strip.getX(), strip.getY());
        area.closeSubPath();
        juce::ColourGradient ag(k.accC.withAlpha(0.05f), 0.f, strip.getY(), k.accC.withAlpha(0.55f), 0.f, strip.getBottom(), false);
        g.setGradientFill(ag);
        g.fillPath(area);
        g.setColour(k.accC);
        g.strokePath(line, juce::PathStrokeType(1.4f));
    }
}

void MainView::paintInputLeds(juce::Graphics& g)
{
    const auto& k = S();
    auto r = ledRect;
    const float lvl = std::max(inL.db, inR.db), hold = std::max(inL.hold, inR.hold);

    g.setColour(k.dim);
    g.setFont(mkFont(9.f, true).withExtraKerningFactor(0.15f));
    g.drawText("INPUT LEVEL", r.removeFromTop(14.f).expanded(8.f, 0.f).toNearestInt(), juce::Justification::centred);
    auto statusR = r.removeFromBottom(16.f);

    const int nSeg = 15;
    const float segGap = 2.f;
    const float segH = (r.getHeight() - (float) (nSeg - 1) * segGap) / (float) nSeg;
    const float barX = r.getX() + 2.f, barW = 26.f;

    const float optBottom = r.getBottom() - 8.f * (segH + segGap) + segGap * 0.5f;
    const float optTop = r.getBottom() - 12.f * (segH + segGap) + segGap * 0.5f;
    g.setColour(k.ok.withAlpha(0.10f + 0.04f * std::sin(phase * 3.f)));
    g.fillRoundedRectangle(barX - 3.f, optTop, barW + 6.f, optBottom - optTop, 4.f);

    for (int i = 0; i < nSeg; ++i)
    {
        const float thr = -42.f + 3.f * (float) i;
        const float y = r.getBottom() - (float) (i + 1) * segH - (float) i * segGap;
        const juce::Colour col = thr < -9.f ? k.ok : (thr < -3.f ? k.warn : k.hot);
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

    const float bx = barX + barW + 6.f;
    g.setColour(k.ok);
    g.drawLine(bx, optTop, bx, optBottom, 1.5f);
    g.drawLine(bx - 3.f, optTop, bx, optTop, 1.5f);
    g.drawLine(bx - 3.f, optBottom, bx, optBottom, 1.5f);
    g.setFont(mkFont(8.5f, true));
    g.drawText("OPT", juce::Rectangle<int>((int) bx + 3, (int) ((optTop + optBottom) * 0.5f) - 6, 22, 12), juce::Justification::centredLeft);

    juce::String status = "-";
    juce::Colour sc = k.dim;
    if (hold > -3.f)        { status = "TOO HOT"; sc = k.hot; }
    else if (hold > -9.f)   { status = "HOT";     sc = k.warn; }
    else if (hold >= -18.f) { status = "OPTIMAL"; sc = k.ok; }
    else if (hold > -42.f)  { status = "LOW";     sc = k.dim; }
    g.setColour(sc);
    g.setFont(mkFont(10.f, true).withExtraKerningFactor(0.12f));
    g.drawText(status, statusR.expanded(8.f, 0.f).toNearestInt(), juce::Justification::centred);
}

void MainView::paintConsole(juce::Graphics& g)
{
    const auto& k = S();
    for (const auto& s : sections)
        paintPanel(g, s.r, s.label);

    paintInputLeds(g);

    g.setColour(k.dim);
    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.15f));
    g.drawText("1176 RATIO", juce::Rectangle<int>(330, 586, 66, 20), juce::Justification::centredLeft);
    g.drawText("670 TIME",   juce::Rectangle<int>(330, 608, 66, 20), juce::Justification::centredLeft);

    // Pultec sub-groups
    g.setColour(k.gold.withAlpha(0.85f));
    g.drawText("LOW  -  25 - 220 Hz",  juce::Rectangle<int>(668, 424, 220, 12), juce::Justification::centredLeft);
    g.drawText("HIGH  -  4 - 20 kHz",  juce::Rectangle<int>(928, 424, 220, 12), juce::Justification::centredLeft);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawVerticalLine(914, 430.f, 620.f);

    g.setColour(k.dim);
    g.drawText("TYPE", juce::Rectangle<int>(26, 668, 100, 14), juce::Justification::centredLeft);
    g.drawText("TIME", juce::Rectangle<int>(462, 668, 100, 14), juce::Justification::centredLeft);

    // FX bar: pop <-> experimental
    {
        const float v = getP("fx") * 0.01f;
        g.setColour(k.meterBg);
        g.fillRoundedRectangle(fxBar, 5.f);
        auto fill = fxBar.withWidth(fxBar.getWidth() * v);
        juce::ColourGradient fg(k.accA, fxBar.getX(), 0.f, k.accC, fxBar.getRight(), 0.f, false);
        g.setGradientFill(fg);
        g.fillRoundedRectangle(fill, 5.f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillRect(fxBar.getCentreX() - 0.5f, fxBar.getY() - 2.f, 1.f, fxBar.getHeight() + 4.f);
        g.setColour(k.dim);
        g.setFont(mkFont(8.5f, true).withExtraKerningFactor(0.1f));
        g.drawText("POP", juce::Rectangle<int>((int) fxBar.getX(), (int) fxBar.getBottom() + 2, 40, 12), juce::Justification::centredLeft);
        g.drawText("ROCK", juce::Rectangle<int>((int) fxBar.getRight() - 40, (int) fxBar.getBottom() + 2, 40, 12), juce::Justification::centredRight);
    }
}

void MainView::paint(juce::Graphics& g)
{
    paintBackground(g);

    const float e = intro * intro * (3.f - 2.f * intro);
    const bool fading = intro < 0.995f;
    if (fading) g.beginTransparencyLayer(e);

    paintHeader(g);
    paintEq(g);
    paintMeters(g);
    paintConsole(g);

    if (fading) g.endTransparencyLayer();
}

} // namespace akui

//==============================================================================
// Resizable editor
//==============================================================================
AudiokingsVocalsEditor::AudiokingsVocalsEditor(AudiokingsVocalsProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), view(p)
{
    addAndMakeVisible(view);

    const double ratio = (double) akui::kBaseW / (double) akui::kBaseH;
    setConstrainer(&constrainer);
    constrainer.setFixedAspectRatio(ratio);
    constrainer.setSizeLimits(680, 440, 2720, 1760);
    setResizable(true, true);

    int w = (int) proc.apvts.state.getProperty("uiw", 1160);
    w = juce::jlimit(680, 2720, w);
    setSize(w, (int) std::lround((double) w / ratio));
}

void AudiokingsVocalsEditor::resized()
{
    const float s = (float) getWidth() / (float) akui::kBaseW;
    view.setTransform(juce::AffineTransform());
    view.setBounds(0, 0, akui::kBaseW, akui::kBaseH);
    view.setTransform(juce::AffineTransform::scale(s));
    proc.apvts.state.setProperty("uiw", getWidth(), nullptr);
}
