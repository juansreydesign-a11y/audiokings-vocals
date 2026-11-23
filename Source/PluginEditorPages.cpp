#include "PluginEditor.h"

namespace akui
{
namespace
{
float logPos(float f, float f0, float f1) { return std::log(f / f0) / std::log(f1 / f0); }

void gridFreq(juce::Graphics& g, juce::Rectangle<float> r, float f0, float f1, const Skin& k, bool labels)
{
    static const float fs[] = { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
    g.setFont(mkFont(9.f));
    for (float f : fs)
    {
        if (f < f0 || f > f1) continue;
        const float x = r.getX() + r.getWidth() * logPos(f, f0, f1);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawVerticalLine((int) x, r.getY(), r.getBottom());
        if (labels)
        {
            g.setColour(k.dim.withAlpha(0.8f));
            const juce::String t = f >= 1000.f ? juce::String((int) (f / 1000.f)) + "k" : juce::String((int) f);
            g.drawText(t, juce::Rectangle<int>((int) x - 18, (int) r.getBottom() + 2, 36, 12), juce::Justification::centred);
        }
    }
}

double toDb(double mag) { return 20.0 * std::log10(std::max(mag, 1.0e-6)); }
} // namespace

//==============================================================================
// Build / layout / navigation
//==============================================================================
void MainView::buildPages()
{
    const char* names[kNumPages] = { "EQ", "COMP", "PULTEC", "DE-ESS", "AIR", "FX", "SPACE" };
    const char* tips[kNumPages] = {
        "Graphic EQ with the effect of the Air exciter and the de-esser",
        "Compressors: transfer curves, gain reduction, attack / release, make-up",
        "Pultec EQ: detailed curve, tube warmth and treble bandwidth",
        "De-esser: sibilance band, threshold and reduction",
        "Air exciter: source band, generated harmonics and frequencies",
        "FX macro: the six effects and their individual trims",
        "Reverb and delay: room / particle sphere and detailed parameters" };
    for (int i = 0; i < kNumPages; ++i)
    {
        tabBtns[i].setButtonText(names[i]);
        tabBtns[i].setTooltip(tips[i]);
        tabBtns[i].onClick = [this, i] { setPage(i); };
        addAndMakeVisible(tabBtns[i]);
    }

    auto addK = [this](int pg, const char* id, const char* title, const char* tip, int col, int row)
    {
        PageKnob pk;
        pk.knob = std::make_unique<Knob>(proc.apvts, id, title, tip, Knob::Small);
        pk.page = pg; pk.col = col; pk.row = row;
        addAndMakeVisible(*pk.knob);
        pageComps[pg].push_back(pk.knob.get());
        pageKnobs.push_back(std::move(pk));
    };

    // COMPRESSION
    addK(PgComp, "compress", "COMPRESS",    "Master compression amount", 0, 0);
    addK(PgComp, "w1176",    "1176",        "How much of the compression is done by the 1176", 1, 0);
    addK(PgComp, "attack",   "ATTACK",      "1176 attack time (20 - 800 us)", 2, 0);
    addK(PgComp, "release",  "RELEASE",     "1176 release time (the real release is program dependent)", 3, 0);
    addK(PgComp, "w670",     "670",         "How much of the compression is done by the Fairchild 670", 0, 1);
    addK(PgComp, "mk1176",   "1176 MAKEUP", "Make-up gain after the 1176 (added to the automatic compensation)", 1, 1);
    addK(PgComp, "mk670",    "670 MAKEUP",  "Make-up gain after the 670 (added to the automatic compensation)", 2, 1);
    // PULTEC
    addK(PgPultec, "meqlm",   "LM BOOST", "Low-mid peak boost (200-1000 Hz)", 4, 0);
    addK(PgPultec, "meqlmf",  "LM FREQ",  "Low-mid peak frequency", 5, 0);
    addK(PgPultec, "mequm",   "MID DIP",  "Mid-range attenuation (200-1500 Hz)", 4, 1);
    addK(PgPultec, "mequmf",  "UM FREQ",  "Upper-mid dip frequency", 5, 1);
    addK(PgPultec, "meqhm",   "HM BOOST", "High-mid peak boost (1500-5000 Hz)", 6, 0);
    addK(PgPultec, "meqhmf",  "HM FREQ",  "High-mid peak frequency", 6, 1);
    addK(PgPultec, "pbass",    "BASS",      "Low boost", 0, 0);
    addK(PgPultec, "pbassatt", "ATTEN",     "Low attenuation", 1, 0);
    addK(PgPultec, "pbassf",   "FREQ",      "Bass frequency, 25 - 220 Hz", 2, 0);
    addK(PgPultec, "ptube",    "TUBE",      "Valve warmth of the passive EQ", 3, 0);
    addK(PgPultec, "ptreb",    "TREBLE",    "High boost", 0, 1);
    addK(PgPultec, "ptrebatt", "ATTEN",     "High attenuation", 1, 1);
    addK(PgPultec, "ptrebf",   "FREQ",      "Treble frequency, 4 - 20 kHz", 2, 1);
    addK(PgPultec, "ptrebbw",  "BANDWIDTH", "Width of the treble boost", 3, 1);
    // DE-ESSER
    addK(PgDeEss, "deessglo",  "GUARD LO", "High-pass before de-esser: protects bass from being detected", 4, 0);
    addK(PgDeEss, "deessguhi", "GUARD HI", "Low-pass before de-esser: protects highs above the sibilance zone", 5, 0);
    addK(PgDeEss, "deess",    "AMOUNT",    "Sensitivity of the de-esser", 0, 0);
    addK(PgDeEss, "deessf",   "FREQ",      "Centre frequency of the sibilance", 1, 0);
    addK(PgDeEss, "deessr",   "RANGE",     "Maximum reduction", 2, 0);
    addK(PgDeEss, "deessq",   "BANDWIDTH", "Width of the sibilance band (lower = wider)", 3, 0);
    addK(PgDeEss, "deessatk", "ATTACK",    "How fast the de-esser reacts", 0, 1);
    addK(PgDeEss, "deessrel", "RELEASE",   "How fast the de-esser lets go", 1, 1);
    // AIR
    addK(PgAir, "airlow",  "AIR LOW",   "Harmonics + presence boost on the upper mids", 0, 0);
    addK(PgAir, "airhigh", "AIR HIGH",  "Harmonics + shelf boost on the highs", 1, 0);
    addK(PgAir, "airlf",   "LOW FREQ",  "Centre of the Air Low band", 2, 0);
    addK(PgAir, "airhf",   "HIGH FREQ", "Corner of the Air High band", 3, 0);
    // FX
    addK(PgFx, "fx",     "FX",         "Macro: how much of every effect is applied", 0, 0);
    addK(PgFx, "fxsat",  "SATURATION", "Trim of the warm saturation", 1, 0);
    addK(PgFx, "fxpc",   "PARALLEL",   "Trim of the parallel compression", 2, 0);
    addK(PgFx, "fxch",   "CHORUS",     "Trim of the chorus", 3, 0);
    addK(PgFx, "fxdist", "DISTORTION", "Trim of the distortion (second half of the FX knob)", 0, 1);
    addK(PgFx, "fxfl",   "FLANGER",    "Trim of the flanger (second half of the FX knob)", 1, 1);
    addK(PgFx, "fxcham", "CHAMBER",    "Trim of the chamber reverb (second half of the FX knob)", 2, 1);
    // SPACE
    addK(PgSpace, "revtone",   "TONE",    "Reverb tone: dark <-> bright", 0, 0);
    addK(PgSpace, "decay",     "DECAY",   "Reverb decay", 1, 0);
    addK(PgSpace, "revmix",    "MIX",     "Reverb level (parallel)", 2, 0);
    addK(PgSpace, "revpre",    "PRE-DLY", "Extra reverb pre-delay (ms)", 3, 0);
    addK(PgSpace, "revwidth",  "WIDTH",   "Stereo width of the reverb", 4, 0);
    addK(PgSpace, "delay",     "DELAY",   "Amount of echo (repeats)", 0, 1);
    addK(PgSpace, "delaytone", "TONE",    "Dark echoes that bloom into the reverb <-> bright and crisp", 1, 1);
    addK(PgSpace, "delaymix",  "MIX",     "Delay level (parallel)", 2, 1);
    addK(PgSpace, "dlywidth",  "WIDTH",   "Ping-pong amount: 0 = dual mono, 100 = full ping-pong", 3, 1);
    addK(PgSpace, "dlyhp",     "HP",      "High-pass on the repeats", 4, 1);

    // choice buttons and toggles
    pRatio  = std::make_unique<ChoiceButtons>(proc.apvts, "ratio1176", juce::StringArray { "4", "8", "12", "20", "ALL" }, false, "1176 ratio");
    pTc     = std::make_unique<ChoiceButtons>(proc.apvts, "tc670", juce::StringArray { "1", "2", "3", "4", "5", "6" }, false, "Fairchild 670 time constant");
    pRev    = std::make_unique<ChoiceButtons>(proc.apvts, "revtype", juce::StringArray { "ROOM", "PLATE", "HALL", "STUDIO" }, false, "Reverb type");
    pDly    = std::make_unique<ChoiceButtons>(proc.apvts, "delaydiv", juce::StringArray { "1/2", "1/4", "1/8 DOT", "1/8", "1/16" }, false, "Delay time (tempo-synced)");
    pMeq5On.setClickingTogglesState(true);
    pMeq5On.setTooltip("Enable the Pultec MEQ-5 Mid-Range Equalizer");
    aMeq5On = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "meq5on", pMeq5On);
    addAndMakeVisible(pMeq5On);
    pageComps[PgPultec].push_back(&pMeq5On);
    pDeMode = std::make_unique<ChoiceButtons>(proc.apvts, "deessmode", juce::StringArray { "SPLIT", "WIDE" }, true, "Split = only the sibilant band is reduced. Wide = whole signal");
    for (auto* c : { pRatio.get(), pTc.get() })              { addAndMakeVisible(c); pageComps[PgComp].push_back(c); }
    for (auto* c : { pRev.get(), pDly.get() })               { addAndMakeVisible(c); pageComps[PgSpace].push_back(c); }
    addAndMakeVisible(*pDeMode);  pageComps[PgDeEss].push_back(pDeMode.get());

    for (auto* b : { &pAuto1176, &pAuto670, &pListen, &autoBtn1176, &autoBtn670 })
        b->setClickingTogglesState(true);
    pAuto1176.setTooltip("Automatic gain compensation of the 1176 (on/off)");
    pAuto670.setTooltip("Automatic gain compensation of the 670 (on/off)");
    autoBtn1176.setTooltip("Automatic gain compensation after the 1176 (on/off)");
    autoBtn670.setTooltip("Automatic gain compensation after the 670 (on/off)");
    pListen.setTooltip("Listen to the sibilance band");
    aPAuto1176 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "auto1176", pAuto1176);
    aPAuto670  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "auto670", pAuto670);
    aPListen   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "deesslisten", pListen);
    aCAuto1176 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "auto1176", autoBtn1176);
    aCAuto670  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "auto670", autoBtn670);
    addAndMakeVisible(pAuto1176); addAndMakeVisible(pAuto670); addAndMakeVisible(pListen);
    addAndMakeVisible(autoBtn1176); addAndMakeVisible(autoBtn670);
    pageComps[PgComp].push_back(&pAuto1176);
    pageComps[PgComp].push_back(&pAuto670);
    pageComps[PgDeEss].push_back(&pListen);

    // vectorscope source: input or output of the whole chain
    scopeInBtn.setTooltip("Vectorscope shows the signal BEFORE the chain");
    scopeOutBtn.setTooltip("Vectorscope shows the signal AFTER the whole chain");
    scopeInBtn.onClick  = [this] { scopeSrc = 0; proc.apvts.state.setProperty("scopesrc", 0, nullptr); };
    scopeOutBtn.onClick = [this] { scopeSrc = 1; proc.apvts.state.setProperty("scopesrc", 1, nullptr); };
    addAndMakeVisible(scopeInBtn);
    addAndMakeVisible(scopeOutBtn);
    scopeSrc = juce::jlimit(0, 1, (int) proc.apvts.state.getProperty("scopesrc", 1));

    setPage((int) proc.apvts.state.getProperty("page", 0));
}

void MainView::setPage(int p)
{
    page = juce::jlimit(0, kNumPages - 1, p);
    for (int i = 0; i < kNumPages; ++i)
        for (auto* c : pageComps[i])
            c->setVisible(i == page);
    lpBtn.setVisible(page == PgEq);
    for (int i = 0; i < kNumPages; ++i)
        tabBtns[i].setToggleState(i == page, juce::dontSendNotification);
    hoverNode = dragNode = -1;
    proc.apvts.state.setProperty("page", page, nullptr);
    repaint();
}

void MainView::layoutPages()
{
    int x = (int) eqPanel.getX() + 14;
    for (int i = 0; i < kNumPages; ++i)
    {
        tabBtns[i].setBounds(x, (int) eqPanel.getY() + 8, 76, 24);
        x += 80;
    }

    pageGraph = { (int) eqPanel.getX() + 14, (int) eqPanel.getY() + 42, 520, 262 };
    pageGrid  = { pageGraph.getRight() + 12, pageGraph.getY(), 360, 262 };

    for (auto& pk : pageKnobs)
    {
        const bool space = pk.page == PgSpace;
        const int cw = space ? 72 : 90, chh = space ? 116 : 104;
        pk.knob->setBounds(pageGrid.getX() + pk.col * cw, pageGrid.getY() + pk.row * (chh + 4), cw, chh);
    }

    // COMP page extras
    const int row1y = pageGrid.getY() + 108;
    pMeq5On.setBounds(pageGrid.getX() + 4 * 72, pageGrid.getY() - 2, 100, 22);
    pAuto1176.setBounds(pageGrid.getX() + 3 * 90 + 2, row1y + 8, 86, 28);
    pAuto670.setBounds(pageGrid.getX() + 3 * 90 + 2, row1y + 44, 86, 28);
    pRatio->setBounds(pageGrid.getX() + 64, pageGrid.getY() + 220, 296, 22);
    pTc->setBounds(pageGrid.getX() + 64, pageGrid.getY() + 246, 296, 16);
    // DE-ESS page extras
    pDeMode->setBounds(pageGrid.getX() + 2 * 90 + 4, row1y + 14, 82, 60);
    pListen.setBounds(pageGrid.getX() + 3 * 90 + 2, row1y + 30, 86, 30);
    // SPACE page extras
    pRev->setBounds(pageGraph.getX() + 64, pageGraph.getY() + 210, 456, 22);
    pDly->setBounds(pageGraph.getX() + 64, pageGraph.getY() + 238, 456, 22);
}

//==============================================================================
// Dispatcher
//==============================================================================
void MainView::paintEq(juce::Graphics& g)
{
    paintPanel(g, eqPanel, "");
    switch (page)
    {
        case PgEq:     paintEqPage(g);     break;
        case PgComp:   paintCompPage(g);   break;
        case PgPultec: paintPultecPage(g); break;
        case PgDeEss:  paintDeEssPage(g);  break;
        case PgAir:    paintAirPage(g);    break;
        case PgFx:     paintFxPage(g);     break;
        case PgSpace:  paintSpacePage(g);  break;
        default: break;
    }
}

//==============================================================================
// COMPRESSION page
//==============================================================================
void MainView::paintCompPage(juce::Graphics& g)
{
    const auto& k = S();
    const auto gr = pageGraph.toFloat();

    const float comp = getP("compress");
    const float e1176 = juce::jlimit(0.f, 100.f, comp * getP("w1176") * 0.01f);
    const float e670 = juce::jlimit(0.f, 100.f, comp * getP("w670") * 0.01f);
    const int ri = juce::jlimit(0, 4, (int) std::lround(getP("ratio1176")));
    static const float kRatio[5] = { 4.f, 8.f, 12.f, 20.f, 30.f };
    const float ratio = kRatio[ri], knee = ri == 4 ? 0.5f : 3.f;
    const float thrA = -4.f - 0.30f * e1176, thrB = -4.f - 0.25f * e670;
    const bool on1176 = e1176 > 0.5f, on670 = e670 > 0.5f;
    const bool a1 = getP("auto1176") > 0.5f, a2 = getP("auto670") > 0.5f;
    const int tc = juce::jlimit(0, 5, (int) std::lround(getP("tc670")));

    auto f1176 = [&](float x) { return on1176 ? x - ak::comp1176Reduction(x, thrA, ratio, knee) : x; };
    auto f670 = [&](float x) { return on670 ? x - ak::comp670Reduction(x, thrB) : x; };
    auto ftot = [&](float x) { return f670(f1176(x)); };

    // ---- transfer curves ----
    juce::Rectangle<float> tp(gr.getX() + 36.f, gr.getY() + 8.f, 226.f, 190.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(tp.expanded(4.f), 6.f);
    auto X = [&](float db) { return tp.getX() + (juce::jlimit(-48.f, 0.f, db) + 48.f) / 48.f * tp.getWidth(); };
    auto Y = [&](float db) { return tp.getBottom() - (juce::jlimit(-48.f, 0.f, db) + 48.f) / 48.f * tp.getHeight(); };

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    for (float d : { -36.f, -24.f, -12.f })
    {
        g.drawVerticalLine((int) X(d), tp.getY(), tp.getBottom());
        g.drawHorizontalLine((int) Y(d), tp.getX(), tp.getRight());
    }
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawLine(X(-48.f), Y(-48.f), X(0.f), Y(0.f), 1.f);        // unity

    auto curve = [&](auto fn, juce::Colour c, float w)
    {
        juce::Path p;
        for (int i = 0; i <= 96; ++i)
        {
            const float xin = -48.f + 48.f * (float) i / 96.f;
            const float yy = fn(xin);
            if (i == 0) p.startNewSubPath(X(xin), Y(yy)); else p.lineTo(X(xin), Y(yy));
        }
        g.setColour(c);
        g.strokePath(p, juce::PathStrokeType(w));
    };
    curve(f1176, k.accC, 1.6f);
    curve(f670, k.accA, 1.6f);
    curve(ftot, k.text, 2.6f);

    // threshold markers
    g.setColour(k.accC.withAlpha(0.6f));
    if (on1176) g.drawVerticalLine((int) X(thrA), tp.getY(), tp.getBottom());
    g.setColour(k.accA.withAlpha(0.6f));
    if (on670) g.drawVerticalLine((int) X(thrB), tp.getY(), tp.getBottom());

    // live operating point
    const float lvl = std::max(inL.db, inR.db);
    if (lvl > -48.f)
    {
        const float px = X(lvl), py = Y(ftot(lvl));
        const float pr = 4.f + 1.5f * std::sin(phase * 6.f);
        g.setColour(k.text.withAlpha(0.25f));
        g.fillEllipse(px - pr * 1.8f, py - pr * 1.8f, pr * 3.6f, pr * 3.6f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(px - 3.f, py - 3.f, 6.f, 6.f);
    }

    g.setFont(mkFont(9.f));
    g.setColour(k.dim);
    g.drawText("IN dBFS", juce::Rectangle<int>((int) tp.getX(), (int) tp.getBottom() + 3, (int) tp.getWidth(), 12), juce::Justification::centred);
    g.drawText("-48", juce::Rectangle<int>((int) tp.getX() - 8, (int) tp.getBottom() + 3, 30, 12), juce::Justification::centredLeft);
    g.drawText("0", juce::Rectangle<int>((int) tp.getRight() - 22, (int) tp.getBottom() + 3, 30, 12), juce::Justification::centredRight);
    g.drawText("OUT", juce::Rectangle<int>((int) gr.getX(), (int) tp.getY(), 34, 12), juce::Justification::centredRight);
    g.setFont(mkFont(9.f, true));
    g.setColour(k.accC);  g.drawText("1176",  juce::Rectangle<int>((int) tp.getX() + 6, (int) tp.getY() + 4, 50, 12),  juce::Justification::centredLeft);
    g.setColour(k.accA);  g.drawText("670",   juce::Rectangle<int>((int) tp.getX() + 6, (int) tp.getY() + 16, 50, 12), juce::Justification::centredLeft);
    g.setColour(k.text);  g.drawText("TOTAL", juce::Rectangle<int>((int) tp.getX() + 6, (int) tp.getY() + 28, 50, 12), juce::Justification::centredLeft);

    // ---- gain-reduction history of each compressor ----
    auto strip = [&](juce::Rectangle<float> r, const std::array<float, histN>& hist, juce::Colour c, const juce::String& name, float cur)
    {
        g.setColour(k.meterBg);
        g.fillRoundedRectangle(r, 6.f);
        juce::Path line;
        for (int i = 0; i < histN; ++i)
        {
            const float v = hist[(size_t) ((histPos + i) % histN)];
            const float x = r.getX() + r.getWidth() * (float) i / (float) (histN - 1);
            const float y = r.getY() + juce::jlimit(0.f, 18.f, v) / 18.f * r.getHeight();
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(r.getRight(), r.getY());
        area.lineTo(r.getX(), r.getY());
        area.closeSubPath();
        g.setColour(c.withAlpha(0.28f));
        g.fillPath(area);
        g.setColour(c);
        g.strokePath(line, juce::PathStrokeType(1.4f));
        g.setFont(mkFont(10.f, true));
        g.drawText(name, juce::Rectangle<int>((int) r.getX() + 8, (int) r.getY() + 4, 120, 14), juce::Justification::centredLeft);
        g.setFont(mkFont(20.f, true));
        g.setColour(cur > 0.2f ? c : k.dim);
        g.drawText((cur > 0.2f ? "-" : "") + juce::String(cur, 1) + " dB", juce::Rectangle<int>((int) r.getRight() - 120, (int) r.getBottom() - 30, 112, 24), juce::Justification::centredRight);
    };
    strip({ gr.getX() + 284.f, gr.getY() + 8.f, 236.f, 88.f }, gr1176Hist, k.accC, "1176  GAIN REDUCTION", gr1176Db);
    strip({ gr.getX() + 284.f, gr.getY() + 104.f, 236.f, 94.f }, gr670Hist, k.accA, "FAIRCHILD 670  GAIN REDUCTION", gr670Db);

    // ---- numeric summary ----
    const float auto1 = on1176 && a1 ? 0.30f * (-thrA) * (1.f - 1.f / ratio) : 0.f;
    const float auto2 = on670 && a2 ? 0.85f * ak::vmReduction(-14.f - thrB + 6.f) : 0.f;
    g.setFont(mkFont(10.5f));
    g.setColour(k.text);
    g.drawText("1176   threshold " + juce::String(thrA, 1) + " dB   ratio " + (ri == 4 ? juce::String("ALL") : juce::String((int) ratio) + ":1")
               + "   make-up " + (a1 ? "AUTO " : "MANUAL ") + juce::String(auto1 + getP("mk1176"), 1) + " dB",
               juce::Rectangle<int>((int) gr.getX() + 4, (int) gr.getY() + 212, 520, 14), juce::Justification::centredLeft);
    g.drawText("670    threshold " + juce::String(thrB, 1) + " dB   time constant " + juce::String(tc + 1)
               + "   make-up " + (a2 ? "AUTO " : "MANUAL ") + juce::String(auto2 + getP("mk670"), 1) + " dB",
               juce::Rectangle<int>((int) gr.getX() + 4, (int) gr.getY() + 230, 520, 14), juce::Justification::centredLeft);
    g.setColour(k.dim);
    g.setFont(mkFont(9.5f));
    g.drawText("Analog colour: FET harmonics grow with the reduction (1176)  -  transformer and core saturation (670)",
               juce::Rectangle<int>((int) gr.getX() + 4, (int) gr.getY() + 248, 520, 13), juce::Justification::centredLeft);

    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.15f));
    g.drawText("1176 RATIO", juce::Rectangle<int>(pageGrid.getX(), pageGrid.getY() + 220, 62, 22), juce::Justification::centredLeft);
    g.drawText("670 TIME",   juce::Rectangle<int>(pageGrid.getX(), pageGrid.getY() + 244, 62, 18), juce::Justification::centredLeft);
}

//==============================================================================
// PULTEC page
//==============================================================================
void MainView::paintPultecPage(juce::Graphics& g)
{
    const auto& k = S();
    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;
    const ak::PultecParams pp { getP("pbass"), getP("pbassatt"), getP("pbassf"), getP("ptreb"), getP("ptrebatt"), getP("ptrebf"),
                                ak::pultecQ(getP("ptrebbw")) };
    const auto ps = ak::makePultec(sr, pp);

    auto pl = pageGraph.toFloat().reduced(30.f, 6.f).withTrimmedBottom(16.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(pl.expanded(4.f), 6.f);
    gridFreq(g, pl, 20.f, 20000.f, k, true);

    auto Y = [&](float db) { return pl.getCentreY() - juce::jlimit(-16.f, 16.f, db) / 16.f * pl.getHeight() * 0.5f; };
    g.setFont(mkFont(9.f));
    for (int d : { -12, -6, 0, 6, 12 })
    {
        g.setColour(juce::Colours::white.withAlpha(d == 0 ? 0.16f : 0.06f));
        g.drawHorizontalLine((int) Y((float) d), pl.getX(), pl.getRight());
        g.setColour(k.dim.withAlpha(0.8f));
        g.drawText(juce::String(d), juce::Rectangle<int>((int) pl.getX() - 30, (int) Y((float) d) - 6, 26, 12), juce::Justification::centredRight);
    }

    auto path = [&](auto dbFn)
    {
        juce::Path p;
        for (int i = 0; i <= 240; ++i)
        {
            const float f = 20.f * std::pow(1000.f, (float) i / 240.f);
            const float x = pl.getX() + pl.getWidth() * (float) i / 240.f;
            const float y = Y(dbFn(f));
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        return p;
    };
    auto comp = [&](const ak::BiquadCoeffs& c, juce::Colour col)
    {
        g.setColour(col.withAlpha(0.8f));
        g.strokePath(path([&](float f) { return (float) toDb(ak::magnitudeAt(c, f, sr)); }), juce::PathStrokeType(1.1f));
    };
    comp(ps.bBoost, k.accA);
    comp(ps.bAtt, k.accB);
    comp(ps.tBoost, k.accC);
    comp(ps.tAtt, k.ok);

    auto total = path([&](float f)
    {
        return (float) toDb(ak::magnitudeAt(ps.bBoost, f, sr) * ak::magnitudeAt(ps.bAtt, f, sr)
                          * ak::magnitudeAt(ps.tBoost, f, sr) * ak::magnitudeAt(ps.tAtt, f, sr));
    });
    juce::Path fill(total);
    fill.lineTo(pl.getRight(), Y(0.f));
    fill.lineTo(pl.getX(), Y(0.f));
    fill.closeSubPath();
    g.setColour(k.gold.withAlpha(0.12f));
    g.fillPath(fill);
    if (k.style == 0)
    {
        g.setColour(k.gold.withAlpha(0.25f));
        g.strokePath(total, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour(k.gold);
    g.strokePath(total, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // frequency markers
    g.setFont(mkFont(9.5f, true));
    for (int m = 0; m < 2; ++m)
    {
        const float f = m == 0 ? pp.bassF : pp.trebF;
        const float x = pl.getX() + pl.getWidth() * logPos(f, 20.f, 20000.f);
        g.setColour((m == 0 ? k.accA : k.accC).withAlpha(0.6f));
        g.drawVerticalLine((int) x, pl.getY(), pl.getBottom());
        const juce::String t = juce::String(m == 0 ? "BASS " : "TREBLE ") + (f >= 1000.f ? juce::String(f / 1000.f, 1) + " kHz" : juce::String((int) f) + " Hz");
        g.drawText(t, juce::Rectangle<int>((int) x - 50, (int) pl.getY() + 3, 100, 12), juce::Justification::centred);
    }
    g.setColour(k.dim);
    g.setFont(mkFont(9.f));
    g.drawText("boost  +  attenuation at the same frequency = the classic Pultec curve", juce::Rectangle<int>((int) pl.getX(), (int) pl.getBottom() + 14, (int) pl.getWidth(), 12),
               juce::Justification::centred);
}

//==============================================================================
// DE-ESSER page
//==============================================================================
void MainView::paintDeEssPage(juce::Graphics& g)
{
    const auto& k = S();
    const auto gr = pageGraph.toFloat();
    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;
    const float f0 = getP("deessf"), amount = getP("deess"), range = getP("deessr");
    const bool wide = getP("deessmode") > 0.5f;
    const float deMax = 3.f + 0.15f * range, thr = -14.f - 0.28f * amount;
    const auto bp = ak::makeBiquad(ak::BQ::BandPass, sr, (double) f0, (double) getP("deessq"), 0.0);
    const float dg = juce::Decibels::decibelsToGain(-grDeEssDb), dgMax = juce::Decibels::decibelsToGain(-deMax);

    // 1) zoomed spectrum 2 - 16 kHz with the detection band
    juce::Rectangle<float> sp(gr.getX() + 10.f, gr.getY() + 4.f, gr.getWidth() - 20.f, 104.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(sp.expanded(2.f), 6.f);
    gridFreq(g, sp, 2000.f, 16000.f, k, true);

    juce::Path band;
    for (int i = 0; i <= 120; ++i)
    {
        const float f = 2000.f * std::pow(8.f, (float) i / 120.f);
        const float x = sp.getX() + sp.getWidth() * (float) i / 120.f;
        const float y = sp.getBottom() - (float) ak::magnitudeAt(bp, f, sr) * sp.getHeight() * 0.9f;
        if (i == 0) band.startNewSubPath(x, y); else band.lineTo(x, y);
    }
    juce::Path bandFill(band);
    bandFill.lineTo(sp.getRight(), sp.getBottom());
    bandFill.lineTo(sp.getX(), sp.getBottom());
    bandFill.closeSubPath();
    g.setColour(k.hot.withAlpha(0.18f));
    g.fillPath(bandFill);

    {
        const float lo = specTop - 66.f;
        juce::Path line;
        for (int i = 0; i <= 240; ++i)
        {
            const float f = 2000.f * std::pow(8.f, (float) i / 240.f);
            const float fi = (float) (numPts - 1) * std::log10(f / 20.f) / 3.f;
            const int i0 = juce::jlimit(0, numPts - 2, (int) fi);
            const float fr = fi - (float) i0;
            const float db = specDb[(size_t) i0] * (1.f - fr) + specDb[(size_t) i0 + 1] * fr;
            const float x = sp.getX() + sp.getWidth() * (float) i / 240.f;
            const float y = juce::jmap(juce::jlimit(lo, specTop, db), lo, specTop, sp.getBottom(), sp.getY() + 4.f);
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(sp.getRight(), sp.getBottom());
        area.lineTo(sp.getX(), sp.getBottom());
        area.closeSubPath();
        g.setColour(k.accB.withAlpha(0.30f));
        g.fillPath(area);
        g.setColour(k.accB.withAlpha(0.8f));
        g.strokePath(line, juce::PathStrokeType(1.2f));
    }
    const float fx0 = sp.getX() + sp.getWidth() * logPos(juce::jlimit(2000.f, 16000.f, f0), 2000.f, 16000.f);
    g.setColour(k.hot);
    g.drawVerticalLine((int) fx0, sp.getY(), sp.getBottom());
    g.setFont(mkFont(9.f, true));
    g.drawText(juce::String(f0 / 1000.f, 1) + " kHz", juce::Rectangle<int>((int) fx0 - 30, (int) sp.getY() + 2, 60, 12), juce::Justification::centred);

    // 2) reduction curve (live) and its maximum
    juce::Rectangle<float> re(sp.getX(), sp.getBottom() + 16.f, sp.getWidth(), 44.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(re.expanded(2.f), 6.f);
    auto dipPath = [&](float gain)
    {
        juce::Path p;
        for (int i = 0; i <= 120; ++i)
        {
            const float f = 2000.f * std::pow(8.f, (float) i / 120.f);
            const double dm = wide ? (double) gain : 1.0 - (1.0 - (double) gain) * ak::magnitudeAt(bp, f, sr);
            const float x = re.getX() + re.getWidth() * (float) i / 120.f;
            const float y = re.getY() + juce::jlimit(0.f, 18.f, (float) -toDb(dm)) / 18.f * re.getHeight();
            if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
        }
        return p;
    };
    auto mx = dipPath(dgMax);
    g.setColour(k.hot.withAlpha(0.35f));
    g.strokePath(mx, juce::PathStrokeType(1.f));
    auto cur = dipPath(dg);
    juce::Path curFill(cur);
    curFill.lineTo(re.getRight(), re.getY());
    curFill.lineTo(re.getX(), re.getY());
    curFill.closeSubPath();
    g.setColour(k.hot.withAlpha(0.35f));
    g.fillPath(curFill);
    g.setColour(k.hot);
    g.strokePath(cur, juce::PathStrokeType(1.6f));
    g.setColour(k.dim);
    g.setFont(mkFont(9.f));
    g.drawText("REDUCTION  0 .. -18 dB", juce::Rectangle<int>((int) re.getX() + 6, (int) re.getBottom() - 13, 160, 12), juce::Justification::centredLeft);

    // 3) sibilance level versus threshold
    juce::Rectangle<float> bar(sp.getX(), re.getBottom() + 10.f, sp.getWidth(), 16.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(bar, 4.f);
    auto bx = [&](float db) { return bar.getX() + juce::jmap(juce::jlimit(-60.f, 0.f, db), -60.f, 0.f, 0.f, bar.getWidth()); };
    const bool over = deessEnvDb > thr;
    g.setColour((over ? k.hot : k.ok).withAlpha(0.9f));
    g.fillRoundedRectangle(bar.withRight(std::max(bar.getX() + 2.f, bx(deessEnvDb))), 4.f);
    g.setColour(juce::Colours::white);
    g.fillRect(bx(thr) - 1.f, bar.getY() - 3.f, 2.f, bar.getHeight() + 6.f);
    g.setFont(mkFont(9.f, true));
    g.setColour(k.text);
    g.drawText("SIBILANCE LEVEL", juce::Rectangle<int>((int) bar.getX() + 6, (int) bar.getY() + 2, 120, 12), juce::Justification::centredLeft);
    g.drawText("THRESHOLD " + juce::String(thr, 0) + " dB", juce::Rectangle<int>((int) bar.getRight() - 140, (int) bar.getY() + 2, 134, 12), juce::Justification::centredRight);

    // 4) de-esser gain-reduction history
    juce::Rectangle<float> hr(sp.getX(), bar.getBottom() + 8.f, sp.getWidth(), 50.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(hr, 6.f);
    juce::Path line;
    for (int i = 0; i < histN; ++i)
    {
        const float v = deessHist[(size_t) ((histPos + i) % histN)];
        const float x = hr.getX() + hr.getWidth() * (float) i / (float) (histN - 1);
        const float y = hr.getY() + juce::jlimit(0.f, 18.f, v) / 18.f * hr.getHeight();
        if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
    }
    juce::Path area(line);
    area.lineTo(hr.getRight(), hr.getY());
    area.lineTo(hr.getX(), hr.getY());
    area.closeSubPath();
    g.setColour(k.hot.withAlpha(0.3f));
    g.fillPath(area);
    g.setColour(k.hot);
    g.strokePath(line, juce::PathStrokeType(1.4f));
    g.setFont(mkFont(18.f, true));
    g.setColour(grDeEssDb > 0.2f ? k.hot : k.dim);
    g.drawText((grDeEssDb > 0.2f ? "-" : "") + juce::String(grDeEssDb, 1) + " dB", juce::Rectangle<int>((int) hr.getRight() - 120, (int) hr.getBottom() - 26, 112, 22), juce::Justification::centredRight);

    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.15f));
    g.setColour(k.dim);
    g.drawText("MODE", juce::Rectangle<int>(pageGrid.getX() + 2 * 90 + 4, pageGrid.getY() + 108, 82, 14), juce::Justification::centredLeft);
}

//==============================================================================
// AIR page
//==============================================================================
void MainView::paintAirPage(juce::Graphics& g)
{
    const auto& k = S();
    const auto gr = pageGraph.toFloat();
    const double sr = proc.getCurrentSampleRate() > 0.0 ? proc.getCurrentSampleRate() : 44100.0;
    const float lf = getP("airlf"), hf = getP("airhf");
    const float aLo = getP("airlow") * 0.01f, aHi = getP("airhigh") * 0.01f;
    const auto pk = ak::makeBiquad(ak::BQ::Peak, sr, (double) lf, 0.8, (double) (aLo * 5.f));
    const auto sh = ak::makeBiquad(ak::BQ::HighShelf, sr, (double) hf, 0.7, (double) (aHi * 8.f));

    juce::Rectangle<float> pl(gr.getX() + 30.f, gr.getY() + 8.f, gr.getWidth() - 40.f, 172.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(pl.expanded(4.f), 6.f);
    gridFreq(g, pl, 500.f, 20000.f, k, true);

    auto X = [&](float f) { return pl.getX() + pl.getWidth() * logPos(juce::jlimit(500.f, 20000.f, f), 500.f, 20000.f); };
    auto Yd = [&](float db) { return pl.getBottom() - (juce::jlimit(-3.f, 14.f, db) + 3.f) / 17.f * pl.getHeight(); };
    g.setFont(mkFont(9.f));
    for (int d : { 0, 5, 10 })
    {
        g.setColour(juce::Colours::white.withAlpha(d == 0 ? 0.16f : 0.06f));
        g.drawHorizontalLine((int) Yd((float) d), pl.getX(), pl.getRight());
        g.setColour(k.dim.withAlpha(0.8f));
        g.drawText("+" + juce::String(d), juce::Rectangle<int>((int) pl.getX() - 30, (int) Yd((float) d) - 6, 26, 12), juce::Justification::centredRight);
    }

    // live output spectrum (faint)
    {
        const float lo = specTop - 66.f;
        juce::Path line;
        for (int i = 0; i <= 240; ++i)
        {
            const float f = 500.f * std::pow(40.f, (float) i / 240.f);
            const float fi = (float) (numPts - 1) * std::log10(f / 20.f) / 3.f;
            const int i0 = juce::jlimit(0, numPts - 2, (int) fi);
            const float fr = fi - (float) i0;
            const float db = specDb[(size_t) i0] * (1.f - fr) + specDb[(size_t) i0 + 1] * fr;
            const float x = pl.getX() + pl.getWidth() * (float) i / 240.f;
            const float y = juce::jmap(juce::jlimit(lo, specTop, db), lo, specTop, pl.getBottom(), pl.getY() + 4.f);
            if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
        }
        juce::Path area(line);
        area.lineTo(pl.getRight(), pl.getBottom());
        area.lineTo(pl.getX(), pl.getBottom());
        area.closeSubPath();
        g.setColour(k.accB.withAlpha(0.18f));
        g.fillPath(area);
    }

    // source band -> generated harmonics, for both exciters
    struct Zone { float srcA, srcB, harm; float amt; const char* name; };
    const Zone zones[2] = { { lf * 0.375f, lf * 0.69f, lf * 1.06f, aLo, "LOW" },
                            { hf * 0.34f, hf * 0.58f, hf * 0.95f, aHi, "HIGH" } };
    for (int z = 0; z < 2; ++z)
    {
        const auto& zn = zones[z];
        const float y = pl.getY() + 6.f + (float) z * 26.f;
        const float a = 0.25f + 0.6f * zn.amt;
        g.setColour(k.accB.withAlpha(a));
        g.fillRoundedRectangle(X(zn.srcA), y, X(zn.srcB) - X(zn.srcA), 12.f, 3.f);
        g.setColour(k.ok.withAlpha(a));
        g.fillRoundedRectangle(X(zn.harm), y, X(20000.f) - X(zn.harm), 12.f, 3.f);
        g.setColour(k.text.withAlpha(0.8f));
        g.setFont(mkFont(8.5f, true));
        g.drawText(juce::String(zn.name) + " SOURCE", juce::Rectangle<int>((int) X(zn.srcA) - 10, (int) y + 12, 90, 11), juce::Justification::centredLeft);
        g.drawText("HARMONICS", juce::Rectangle<int>((int) X(zn.harm), (int) y + 12, 90, 11), juce::Justification::centredLeft);
        const float sx = (X(zn.srcA) + X(zn.srcB)) * 0.5f;
        g.setColour(k.text.withAlpha(0.35f + 0.5f * zn.amt));
        g.drawArrow(juce::Line<float>(sx + 14.f, y + 6.f, X(zn.harm) - 6.f, y + 6.f), 1.4f, 7.f, 7.f);
    }

    // resulting boost (shelf + presence peak)
    juce::Path ap;
    for (int i = 0; i <= 240; ++i)
    {
        const float f = 500.f * std::pow(40.f, (float) i / 240.f);
        const float x = pl.getX() + pl.getWidth() * (float) i / 240.f;
        const float y = Yd((float) toDb(ak::magnitudeAt(pk, f, sr) * ak::magnitudeAt(sh, f, sr)));
        if (i == 0) ap.startNewSubPath(x, y); else ap.lineTo(x, y);
    }
    juce::Path af(ap);
    af.lineTo(pl.getRight(), Yd(0.f));
    af.lineTo(pl.getX(), Yd(0.f));
    af.closeSubPath();
    g.setColour(k.ok.withAlpha(0.18f));
    g.fillPath(af);
    g.setColour(k.ok);
    g.strokePath(ap, juce::PathStrokeType(2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont(mkFont(10.f));
    g.setColour(k.text);
    g.drawText("Harmonics are generated from a band that never overlaps the output band, so the exciter only adds - it never cuts.",
               juce::Rectangle<int>((int) gr.getX() + 4, (int) pl.getBottom() + 22, 520, 14), juce::Justification::centredLeft);
    g.setColour(k.dim);
    g.setFont(mkFont(9.5f));
    g.drawText("LOW: harmonics above " + juce::String(lf * 1.06f / 1000.f, 1) + " kHz + presence peak   -   HIGH: harmonics above "
               + juce::String(hf * 0.95f / 1000.f, 1) + " kHz + shelf up to +8 dB",
               juce::Rectangle<int>((int) gr.getX() + 4, (int) pl.getBottom() + 40, 520, 14), juce::Justification::centredLeft);
}

//==============================================================================
// FX page
//==============================================================================
void MainView::paintFxPage(juce::Graphics& g)
{
    const auto& k = S();
    const float fx = getP("fx");
    const float t = ak::fxT(fx), u = ak::fxU(fx);
    const float amts[6] = { t * getP("fxsat") * 0.01f, t * getP("fxpc") * 0.01f, t * getP("fxch") * 0.01f,
                            u * getP("fxdist") * 0.01f, u * getP("fxfl") * 0.01f, u * getP("fxcham") * 0.01f };
    const char* names[6] = { "SATURATION", "PARALLEL COMP", "CHORUS", "DISTORTION", "FLANGER", "CHAMBER" };
    const juce::Colour cols[6] = { k.warn, k.accA, k.accB, k.hot, k.accC, k.ok };

    const auto gr = pageGraph.toFloat();
    const float cw = (gr.getWidth() - 20.f) / 3.f, ch = (gr.getHeight() - 10.f) / 2.f;
    const float ph = phase * 3.f;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < 6; ++i)
    {
        const float a = amts[i];
        juce::Rectangle<float> card(gr.getX() + (float) (i % 3) * (cw + 10.f), gr.getY() + (float) (i / 3) * (ch + 10.f), cw, ch);
        g.setColour(k.meterBg);
        g.fillRoundedRectangle(card, 8.f);
        g.setColour(cols[i].withAlpha(0.15f + 0.6f * a));
        g.drawRoundedRectangle(card.reduced(0.5f), 8.f, 1.2f);

        g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.12f));
        g.setColour(cols[i]);
        g.drawText(names[i], juce::Rectangle<int>((int) card.getX() + 10, (int) card.getY() + 6, (int) cw - 60, 14), juce::Justification::centredLeft);
        g.setColour(a > 0.01f ? k.text : k.dim);
        g.drawText(juce::String((int) std::round(a * 100.f)) + "%", juce::Rectangle<int>((int) card.getRight() - 50, (int) card.getY() + 6, 40, 14), juce::Justification::centredRight);

        auto in = card.reduced(12.f, 0.f).withTrimmedTop(26.f).withTrimmedBottom(18.f);
        const float cy = in.getCentreY(), hh = in.getHeight() * 0.42f;
        auto waveform = [&](auto fn, juce::Colour c, float alpha, float w)
        {
            juce::Path p;
            for (int s = 0; s <= 80; ++s)
            {
                const float xn = (float) s / 80.f;
                const float y = cy - fn(xn) * hh;
                if (s == 0) p.startNewSubPath(in.getX(), y); else p.lineTo(in.getX() + in.getWidth() * xn, y);
            }
            g.setColour(c.withAlpha(alpha));
            g.strokePath(p, juce::PathStrokeType(w));
        };
        auto sine = [&](float xn, float cycles, float off) { return std::sin(twoPi * cycles * xn + ph + off); };

        const float vis = 0.35f + 0.65f * a;
        switch (i)
        {
            case 0:   // soft saturation
            {
                const float d = 1.f + 5.f * a;
                waveform([&](float xn) { return sine(xn, 2.f, 0.f); }, k.dim, 0.5f, 1.f);
                waveform([&](float xn) { return std::tanh(d * sine(xn, 2.f, 0.f)) / std::tanh(d); }, cols[i], vis, 2.f);
                break;
            }
            case 1:   // parallel compression
            {
                auto src = [&](float xn) { return sine(xn, 2.5f, 0.f) * (0.35f + 0.65f * (0.5f + 0.5f * std::sin(twoPi * xn * 0.7f - ph * 0.3f))); };
                waveform(src, k.dim, 0.5f, 1.f);
                waveform([&](float xn) { const float s = src(xn); return s / (1.f + 4.f * a * std::abs(s)); }, cols[i], vis, 2.f);
                break;
            }
            case 2:   // chorus
                waveform([&](float xn) { return 0.7f * sine(xn, 2.f, 0.f); }, cols[i], 0.35f + 0.4f * a, 1.6f);
                waveform([&](float xn) { return 0.7f * sine(xn, 2.f, 0.4f + 0.8f * a * std::sin(ph * 0.7f)); }, cols[i], vis, 1.6f);
                break;
            case 3:   // distortion
                waveform([&](float xn) { return sine(xn, 2.f, 0.f); }, k.dim, 0.5f, 1.f);
                waveform([&](float xn) { return juce::jlimit(-1.f, 1.f, (1.f + 12.f * a) * sine(xn, 2.f, 0.f)); }, cols[i], vis, 2.f);
                break;
            case 4:   // flanger: swept comb
            {
                for (int j = 0; j < 14; ++j)
                {
                    const float x = in.getX() + in.getWidth() * (((float) j + 0.5f + 0.35f * std::sin(ph * 0.8f + (float) j * 0.35f)) / 14.f);
                    const float hgt = hh * (0.35f + 0.65f * std::abs(std::sin((float) j * 0.9f + ph * 0.5f)));
                    g.setColour(cols[i].withAlpha(0.2f + 0.7f * a));
                    g.drawLine(x, cy - hgt, x, cy + hgt, 1.6f);
                }
                break;
            }
            default:  // chamber: ripples
            {
                for (int j = 0; j < 3; ++j)
                {
                    const float r = std::fmod(ph * 12.f + (float) j * 26.f, 78.f);
                    g.setColour(cols[i].withAlpha((1.f - r / 78.f) * (0.2f + 0.8f * a)));
                    const float rr = r * (0.55f + 0.45f * a);
                    g.drawEllipse(in.getCentreX() - rr, cy - rr * 0.6f, rr * 2.f, rr * 1.2f, 1.5f);
                }
                break;
            }
        }

        juce::Rectangle<float> tr(card.getX() + 10.f, card.getBottom() - 12.f, card.getWidth() - 20.f, 5.f);
        g.setColour(k.panel);
        g.fillRoundedRectangle(tr, 2.5f);
        g.setColour(cols[i]);
        g.fillRoundedRectangle(tr.withWidth(tr.getWidth() * a), 2.5f);
    }
}

//==============================================================================
// SPACE page: reverb room / particle sphere + delay echoes
//==============================================================================
void MainView::paintSpacePage(juce::Graphics& g)
{
    const auto& k = S();
    auto cv = pageGraph.toFloat().withHeight(204.f);
    g.setColour(k.meterBg);
    g.fillRoundedRectangle(cv, 8.f);
    g.saveState();
    g.reduceClipRegion(cv.toNearestInt());

    const int type = juce::jlimit(0, 3, (int) std::lround(getP("revtype")));
    const float decay = getP("decay") * 0.01f, mix = getP("revmix") * 0.01f, tone = getP("revtone") * 0.01f;
    const float dMix = getP("delaymix") * 0.01f, dRep = getP("delay") * 0.01f, dTone = getP("delaytone") * 0.01f;
    const float dW = getP("dlywidth") * 0.01f;

    static const float dims[4][3] = { { 0.55f, 0.35f, 0.55f }, { 0.90f, 0.12f, 0.70f }, { 1.00f, 0.55f, 1.00f }, { 0.70f, 0.40f, 0.65f } };
    const float sc = 0.55f + 0.9f * decay;
    for (int i = 0; i < 3; ++i)
        sv[i] += (dims[type][i] * sc - sv[i]) * 0.08f;       // the room grows / shrinks smoothly
    sv[3] += (mix - sv[3]) * 0.10f;
    sv[4] += (tone - sv[4]) * 0.10f;
    sv[5] += (decay - sv[5]) * 0.08f;

    const float yaw = phase * 0.22f + 0.6f, pitch = 0.38f;
    const auto c = cv.getCentre() + juce::Point<float>(0.f, 6.f);
    const float unit = 78.f;
    auto proj = [&](float x, float y, float z, float& depth)
    {
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        const float xr = x * cy - z * sy, zr = x * sy + z * cy;
        const float cp = std::cos(pitch), sp = std::sin(pitch);
        const float yr = y * cp - zr * sp, zr2 = y * sp + zr * cp;
        const float persp = 3.2f / (3.2f + zr2 * 0.6f);
        depth = zr2;
        return juce::Point<float>(c.x + xr * unit * persp, c.y - yr * unit * persp);
    };
    float dp = 0.f;
    const juce::Colour roomCol = k.accB.interpolatedWith(k.accA, sv[4]);

    // room wireframe
    {
        const float hx = sv[0], hy = sv[1], hz = sv[2];
        juce::Point<float> v[8];
        for (int i = 0; i < 8; ++i)
            v[i] = proj((i & 1 ? hx : -hx), (i & 2 ? hy : -hy), (i & 4 ? hz : -hz), dp);
        g.setColour(roomCol.withAlpha(0.45f));
        const int edges[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
        for (auto& e : edges)
            g.drawLine(v[e[0]].x, v[e[0]].y, v[e[1]].x, v[e[1]].y, 1.2f);
        g.setColour(roomCol.withAlpha(0.14f));
        for (int s = -3; s <= 3; ++s)   // floor grid
        {
            const auto a1 = proj((float) s / 3.f * hx, -hy, -hz, dp), a2 = proj((float) s / 3.f * hx, -hy, hz, dp);
            const auto b1 = proj(-hx, -hy, (float) s / 3.f * hz, dp), b2 = proj(hx, -hy, (float) s / 3.f * hz, dp);
            g.drawLine(a1.x, a1.y, a2.x, a2.y, 0.8f);
            g.drawLine(b1.x, b1.y, b2.x, b2.y, 0.8f);
        }
    }

    // particle sphere: grows with the decay, glows with the mix, changes colour with the tone
    {
        static std::vector<std::array<float, 4>> pts;
        if (pts.empty())
        {
            const int N = 170;
            for (int i = 0; i < N; ++i)
            {
                const float yy = 1.f - 2.f * ((float) i + 0.5f) / (float) N;
                const float rr = std::sqrt(std::max(0.f, 1.f - yy * yy));
                const float phi = (float) i * 2.39996323f;
                const float h = std::fmod((float) i * 0.6180339f, 1.f);
                pts.push_back({ std::cos(phi) * rr, yy, std::sin(phi) * rr, 0.4f + 0.6f * h });
            }
        }
        const float R = (0.28f + 0.72f * sv[5]) * std::min(sv[0], sv[2]) * 1.15f;
        const float breathe = 1.f + 0.05f * std::sin(phase * 1.3f);
        const float wob = 0.03f + 0.10f * sv[3];
        const juce::Colour pc = k.accB.interpolatedWith(juce::Colours::white, 0.15f).interpolatedWith(k.accA, sv[4]);
        for (size_t i = 0; i < pts.size(); ++i)
        {
            const auto& p = pts[i];
            const float rr = R * breathe * (0.55f + 0.45f * p[3]);
            const float w = wob * std::sin(phase * (1.f + 2.f * p[3]) + (float) i);
            const auto pr = proj(p[0] * rr + w, p[1] * rr + w * 0.6f, p[2] * rr - w, dp);
            const float depth = juce::jlimit(0.f, 1.f, 0.5f - dp * 0.35f);
            const float a = (0.12f + 0.75f * sv[3]) * (0.35f + 0.65f * depth);
            const float sz = 1.2f + 1.8f * p[3] * (0.4f + sv[3]);
            g.setColour(pc.withAlpha(a));
            g.fillEllipse(pr.x - sz * 0.5f, pr.y - sz * 0.5f, sz, sz);
        }
    }

    // delay: echoes bouncing left / right, brightness follows the repeats and the mix
    {
        const float fb = 0.10f + 0.55f * dRep;
        const int nEcho = 2 + (int) std::round(fb * 6.f);
        static const float mult[5] = { 2.f, 1.f, 0.75f, 0.5f, 0.25f };
        const int div = juce::jlimit(0, 4, (int) std::lround(getP("delaydiv")));
        const float bpm = std::max(30.f, proc.hostBpm.load());
        const float T = juce::jlimit(0.12f, 2.f, 60.f / bpm * mult[div]);
        const float tpos = std::fmod(phase / T, (float) nEcho);
        const juce::Colour ec = k.accC.interpolatedWith(k.accA, dTone);
        const auto src = proj(0.f, 0.f, 0.f, dp);
        for (int e = 1; e <= nEcho; ++e)
        {
            const float side = (e % 2 == 1) ? 1.f : -1.f;
            const float ex = side * dW * (0.32f + 0.17f * (float) e) * 1.2f;
            const auto pr = proj(ex, 0.05f * (float) e, 0.12f * (float) e, dp);
            const float level = dMix * std::pow(fb, (float) (e - 1)) + 0.03f;
            const float pulse = std::max(0.f, 1.f - std::abs(tpos - (float) (e - 1)));
            const float a = juce::jlimit(0.f, 1.f, level * 1.4f);
            g.setColour(ec.withAlpha(a * 0.25f));
            g.drawLine(src.x, src.y, pr.x, pr.y, 1.f);
            const float rad = std::max(3.f, 11.f - (float) e * 1.4f);
            g.setColour(ec.withAlpha(a * (0.35f + 0.5f * pulse)));
            g.fillEllipse(pr.x - rad, pr.y - rad, rad * 2.f, rad * 2.f);
            if (pulse > 0.f)
            {
                const float rr = rad + (1.f - pulse) * 16.f;
                g.setColour(ec.withAlpha(a * pulse * 0.8f));
                g.drawEllipse(pr.x - rr, pr.y - rr, rr * 2.f, rr * 2.f, 1.4f);
            }
        }
        // the source
        g.setColour(k.gold.withAlpha(0.35f + 0.25f * std::sin(phase * 4.f)));
        g.fillEllipse(src.x - 9.f, src.y - 9.f, 18.f, 18.f);
        g.setColour(k.gold);
        g.fillEllipse(src.x - 4.f, src.y - 4.f, 8.f, 8.f);
    }
    g.restoreState();

    // read-outs
    static const char* tn[4] = { "ROOM", "PLATE", "HALL", "STUDIO" };
    static const float rtk[4] = { 0.6f, 1.0f, 1.3f, 0.8f };
    static const float preBase[4] = { 0.f, 12.f, 28.f, 8.f };
    const float rt60 = 0.25f + 4.0f * decay * rtk[type];
    g.setFont(mkFont(10.f, true));
    g.setColour(k.text);
    g.drawText(juce::String(tn[type]) + "   RT60 ~ " + juce::String(rt60, 1) + " s   PRE-DELAY " + juce::String((int) (preBase[type] + getP("revpre"))) + " ms",
               juce::Rectangle<int>((int) cv.getX() + 10, (int) cv.getY() + 6, 400, 14), juce::Justification::centredLeft);
    static const char* dn[4] = { "1/4", "1/8 dotted", "1/8", "1/16" };
    static const float mult2[4] = { 1.f, 0.75f, 0.5f, 0.25f };
    const int div2 = juce::jlimit(0, 3, (int) std::lround(getP("delaydiv")));
    const float ms = 60000.f / std::max(30.f, proc.hostBpm.load()) * mult2[div2];
    g.setColour(k.dim);
    g.drawText("ECHO " + juce::String(dn[div2]) + " = " + juce::String((int) ms) + " ms   REPEATS " + juce::String((int) std::round(getP("delay"))) + "%",
               juce::Rectangle<int>((int) cv.getX() + 10, (int) cv.getBottom() - 20, 400, 14), juce::Justification::centredLeft);

    g.setFont(mkFont(9.5f, true).withExtraKerningFactor(0.15f));
    g.drawText("REVERB", juce::Rectangle<int>(pageGraph.getX(), pageGraph.getY() + 210, 62, 22), juce::Justification::centredLeft);
    g.drawText("DELAY",  juce::Rectangle<int>(pageGraph.getX(), pageGraph.getY() + 238, 62, 22), juce::Justification::centredLeft);
}

} // namespace akui
