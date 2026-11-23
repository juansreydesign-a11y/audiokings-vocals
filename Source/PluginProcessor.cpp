#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    for (const auto& d : ak::kParams)
    {
        if (d.kind == ak::KFloat)
        {
            juce::NormalisableRange<float> range(d.lo, d.hi, d.step);
            if (d.centre > 0.f)
                range.setSkewForCentre(d.centre);

            const juce::String unit(d.unit);
            const int decimals = d.decimals;
            const bool isSens = std::strcmp(d.id, "sens") == 0;

            auto attrs = juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction([unit, decimals, isSens](float v, int)
                {
                    if (isSens)   // Level/Sens is displayed as input gain in dB
                        return juce::String((v - 50.f) * 0.36f, 1) + " dB";
                    return juce::String(v, decimals) + unit;
                });

            l.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { d.id, 1 }, d.name, range, d.def, attrs));
        }
        else if (d.kind == ak::KChoice)
        {
            l.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID { d.id, 1 }, d.name,
                juce::StringArray::fromTokens(d.choices, "|", ""), (int) d.def));
        }
        else
        {
            l.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID { d.id, 1 }, d.name, d.def > 0.5f));
        }
    }
    return l;
}

inline void upMax(std::atomic<float>& a, float v)
{
    if (v > a.load(std::memory_order_relaxed))
        a.store(v, std::memory_order_relaxed);
}

inline float smoothStep(float x)
{
    x = juce::jlimit(0.f, 1.f, x);
    return x * x * (3.f - 2.f * x);
}

struct RevType { float sizeLo, sizeHi, preMs, damp; };
constexpr RevType kRev[4] = {
    { 0.15f, 0.55f,  0.f, 0.55f },   // Room
    { 0.40f, 0.80f, 12.f, 0.25f },   // Plate
    { 0.60f, 0.97f, 28.f, 0.40f },   // Hall
    { 0.30f, 0.70f,  8.f, 0.35f }    // Studio (with subtle saturation)
};

// fractional read from a circular buffer, `delay` samples behind index `wp`
inline float readFrac(const std::vector<float>& b, int wp, float delay)
{
    const int size = (int) b.size();
    float rp = (float) wp - delay;
    while (rp < 0.f) rp += (float) size;
    int i0 = (int) rp;
    const float fr = rp - (float) i0;
    if (i0 >= size) i0 -= size;
    int i1 = i0 + 1;
    if (i1 >= size) i1 -= size;
    return b[(size_t) i0] * (1.f - fr) + b[(size_t) i1] * fr;
}

juce::String valuesToString(const ak::Values& v)
{
    juce::String s;
    for (float x : v) s += juce::String(x, 5) + ",";
    return s;
}
} // namespace

//==============================================================================
AudiokingsVocalsProcessor::AudiokingsVocalsProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    auto get = [this](const char* id) { return apvts.getRawParameterValue(id); };
    P.sens = get("sens");       P.noise = get("noise");     P.compress = get("compress");
    P.w1176 = get("w1176");     P.w670 = get("w670");       P.ratio = get("ratio1176");  P.tc = get("tc670");
    P.pbass = get("pbass");     P.pbassatt = get("pbassatt"); P.pbassf = get("pbassf");
    P.ptreb = get("ptreb");     P.ptrebatt = get("ptrebatt"); P.ptrebf = get("ptrebf");

    const char* gi[5] = { "eqlow", "eqlowmid", "eqhimid", "eqpres", "eqair" };
    const char* fi[5] = { "eqlowf", "eqlowmidf", "eqhimidf", "eqpresf", "eqairf" };
    const char* qi[5] = { "eqlowq", "eqlowmidq", "eqhimidq", "eqpresq", "eqairq" };
    for (int i = 0; i < 5; ++i) { P.eqg[i] = get(gi[i]); P.eqf[i] = get(fi[i]); P.eqq[i] = get(qi[i]); }
    P.lpon = get("lpon");       P.lpf = get("lpf");
    P.airlow = get("airlow");   P.airhigh = get("airhigh"); P.fx = get("fx");
    P.revtype = get("revtype"); P.revtone = get("revtone"); P.decay = get("decay"); P.revmix = get("revmix");
    P.dlydiv = get("delaydiv"); P.delay = get("delay");     P.delaytone = get("delaytone"); P.delaymix = get("delaymix");
    P.output = get("output");   P.hpf = get("hpf");         P.attack = get("attack");
    P.release = get("release"); P.deess = get("deess");
    P.deessf = get("deessf");   P.deessr = get("deessr");   P.deessq = get("deessq");
    P.deessglo = get("deessglo"); P.deessguhi = get("deessguhi");
    P.deessmode = get("deessmode"); P.deesslisten = get("deesslisten");
    P.deessatk = get("deessatk"); P.deessrel = get("deessrel");
    P.mk1176 = get("mk1176"); P.auto1176 = get("auto1176"); P.mk670 = get("mk670"); P.auto670 = get("auto670");
    P.ptube = get("ptube"); P.ptrebbw = get("ptrebbw"); P.airlf = get("airlf"); P.airhf = get("airhf");
    P.fxsat = get("fxsat"); P.fxpc = get("fxpc"); P.fxch = get("fxch"); P.fxdist = get("fxdist");
    P.fxfl = get("fxfl"); P.fxcham = get("fxcham"); P.fxnocham = get("fxnocham");
    P.meq5on = get("meq5on"); P.meqlm = get("meqlm"); P.meqlmf = get("meqlmf");
    P.mequm = get("mequm"); P.mequmf = get("mequmf"); P.meqhm = get("meqhm"); P.meqhmf = get("meqhmf");
    P.revpre = get("revpre"); P.revwidth = get("revwidth"); P.dlywidth = get("dlywidth"); P.dlyhp = get("dlyhp");

    loadFactoryPreset(0);   // start on "Pop Voice"
}

bool AudiokingsVocalsProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void AudiokingsVocalsProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int bs = juce::jmax(samplesPerBlock, 512);

    reverb.setSampleRate(sr);  reverb.reset();
    chamber.setSampleRate(sr); chamber.reset();
    reverbBuf.setSize(2, bs);
    dlyOut.setSize(2, bs);
    chamBuf.setSize(2, bs);

    const int dlySize = (int) (sr * 2.0) + 8;
    const int preSize = (int) (sr * 0.25) + 8;
    const int chSize = (int) (sr * 0.05) + 8;
    const int flSize = (int) (sr * 0.02) + 8;
    for (int c = 0; c < 2; ++c)
    {
        dlyBuf[c].assign((size_t) dlySize, 0.f);
        preBuf[c].assign((size_t) preSize, 0.f);
        chBuf[c].assign((size_t) chSize, 0.f);
        flBuf[c].assign((size_t) flSize, 0.f);

        hpf[c].reset(); lp1[c].reset(); lp2[c].reset();
        for (int k = 0; k < 5; ++k) eq[k][c].reset();
        pBassB[c].reset(); pBassA[c].reset(); pTrebB[c].reset(); pTrebA[c].reset();
        meqLm[c].reset(); meqUm[c].reset(); meqHm[c].reset();
        deessGlo[c].reset(); deessGuHi[c].reset();
        deessBp[c].reset(); aLoPeak[c].reset(); aHiShelf[c].reset();
        aLoSrc[c].reset(); aLoOut[c].reset(); aLoOut2[c].reset();
        aHiSrc[c].reset(); aHiOut[c].reset(); aHiOut2[c].reset();
        for (int k = 0; k < 4; ++k) { aLoLp[k][c].reset(); aHiLp[k][c].reset(); }
        aLoEnv[c] = aHiEnv[c] = 0.f;
        revHp[c].reset(); wetHs[c].reset(); detHp[c].reset(); lfLp[c].reset();
        detHp[c].c = ak::makeBiquad(ak::BQ::HighPass, sr, 90.0, 0.707, 0.0);
        lfLp[c].c  = ak::makeBiquad(ak::BQ::LowPass,  sr, 150.0, 0.707, 0.0);

        // The source band is cut by an 8th-order Butterworth low-pass (4 sections) so almost nothing of it
        // reaches the output band: harmonics are added only where the source has no content.
        static const double q8[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
        // Air Low : source 1.2 - 2.2 kHz  ->  harmonics kept above 3.4 kHz
        aLoSrc[c].c  = ak::makeBiquad(ak::BQ::HighPass, sr, 1200.0, 0.707,  0.0);
        aLoOut[c].c  = ak::makeBiquad(ak::BQ::HighPass, sr, 3400.0, 0.5412, 0.0);
        aLoOut2[c].c = ak::makeBiquad(ak::BQ::HighPass, sr, 3400.0, 1.3066, 0.0);
        // Air High: source 3.2 - 5.5 kHz  ->  harmonics kept above 9 kHz
        aHiSrc[c].c  = ak::makeBiquad(ak::BQ::HighPass, sr, 3200.0, 0.707,  0.0);
        aHiOut[c].c  = ak::makeBiquad(ak::BQ::HighPass, sr, 9000.0, 0.5412, 0.0);
        aHiOut2[c].c = ak::makeBiquad(ak::BQ::HighPass, sr, 9000.0, 1.3066, 0.0);
        for (int k = 0; k < 4; ++k)
        {
            aLoLp[k][c].c = ak::makeBiquad(ak::BQ::LowPass, sr, 2200.0, q8[k], 0.0);
            aHiLp[k][c].c = ak::makeBiquad(ak::BQ::LowPass, sr, 5500.0, q8[k], 0.0);
        }
        revHp[c].c   = ak::makeBiquad(ak::BQ::HighPass, sr, 180.0, 0.707, 0.0);
    }
    dlyWp = preWp = chWp = flWp = 0;
    dlyLp[0] = dlyLp[1] = 0.f;
    dlyHp[0] = dlyHp[1] = 0.f;
    distLp[0] = distLp[1] = 0.f;
    chPh = flPh = 0.f;

    filtersValid = false;
    lastDeF = lastDeQ = lastDeGlo = lastDeGuHi = lastAirLo = lastAirHi = lastAirLF = lastAirHF = -1.f;
    lastRevTone = -1.f;
    gateDb = gateEnv = 0.f;
    gr1176 = gr1176F = mk1176 = gr670F = gr670S = mk670 = deessEnv = pcGr = 0.f;
    lastInGain  = juce::Decibels::decibelsToGain((P.sens->load() - 50.f) * 0.36f);
    lastOutGain = juce::Decibels::decibelsToGain(P.output->load());
}

void AudiokingsVocalsProcessor::updateFilters(const ak::EqParams& e, const ak::PultecParams& p)
{
    const std::array<float, 18> nowEq { e.hpf, e.g[0], e.g[1], e.g[2], e.g[3], e.g[4],
                                        e.f[0], e.f[1], e.f[2], e.f[3], e.f[4],
                                        e.q[0], e.q[1], e.q[2], e.q[3], e.q[4], e.lpOn ? 1.f : 0.f, e.lpf };
    const std::array<float, 7> nowP { p.bass, p.bassAtt, p.bassF, p.treb, p.trebAtt, p.trebF, p.trebQ };

    if (filtersValid && nowEq == lastEq && nowP == lastPultec)
        return;

    filtersValid = true;
    lastEq = nowEq;
    lastPultec = nowP;

    const auto es = ak::makeEq(sr, e);
    const auto ps = ak::makePultec(sr, p);
    for (int c = 0; c < 2; ++c)
    {
        hpf[c].c = es.hp;
        for (int k = 0; k < 5; ++k) eq[k][c].c = es.n[k];
        lp1[c].c = es.lp1;
        lp2[c].c = es.lp2;
        pBassB[c].c = ps.bBoost;
        pBassA[c].c = ps.bAtt;
        pTrebB[c].c = ps.tBoost;
        pTrebA[c].c = ps.tAtt;
    }
}

void AudiokingsVocalsProcessor::updateAirLow(float lf, float amt)
{
    static const double q8[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
    const auto pk = ak::makeBiquad(ak::BQ::Peak,     sr, (double) lf,          0.8,    (double) (amt * 5.f));
    const auto hp = ak::makeBiquad(ak::BQ::HighPass, sr, (double) lf * 0.375,  0.707,  0.0);
    const auto o1 = ak::makeBiquad(ak::BQ::HighPass, sr, (double) lf * 1.06,   0.5412, 0.0);
    const auto o2 = ak::makeBiquad(ak::BQ::HighPass, sr, (double) lf * 1.06,   1.3066, 0.0);
    for (int c = 0; c < 2; ++c)
    {
        aLoPeak[c].c = pk; aLoSrc[c].c = hp; aLoOut[c].c = o1; aLoOut2[c].c = o2;
        for (int k = 0; k < 4; ++k)
            aLoLp[k][c].c = ak::makeBiquad(ak::BQ::LowPass, sr, (double) lf * 0.69, q8[k], 0.0);
    }
}

void AudiokingsVocalsProcessor::updateAirHigh(float hf, float amt)
{
    static const double q8[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
    const auto sh = ak::makeBiquad(ak::BQ::HighShelf, sr, (double) hf,         0.7,    (double) (amt * 8.f));
    const auto hp = ak::makeBiquad(ak::BQ::HighPass,  sr, (double) hf * 0.34,  0.707,  0.0);
    const auto o1 = ak::makeBiquad(ak::BQ::HighPass,  sr, (double) hf * 0.95,  0.5412, 0.0);
    const auto o2 = ak::makeBiquad(ak::BQ::HighPass,  sr, (double) hf * 0.95,  1.3066, 0.0);
    for (int c = 0; c < 2; ++c)
    {
        aHiShelf[c].c = sh; aHiSrc[c].c = hp; aHiOut[c].c = o1; aHiOut2[c].c = o2;
        for (int k = 0; k < 4; ++k)
            aHiLp[k][c].c = ak::makeBiquad(ak::BQ::LowPass, sr, (double) hf * 0.58, q8[k], 0.0);
    }
}

void AudiokingsVocalsProcessor::copyScopeIn(float* destL, float* destR) const
{
    const int w = scopePos.load(std::memory_order_relaxed);
    for (int i = 0; i < scopeSize; ++i)
    {
        const size_t k = (size_t) ((w + i) & (scopeSize - 1));
        destL[i] = scopeInL[k];
        destR[i] = scopeInR[k];
    }
}

void AudiokingsVocalsProcessor::copyScope(float* destL, float* destR) const
{
    const int w = scopePos.load(std::memory_order_relaxed);
    for (int i = 0; i < scopeSize; ++i)
    {
        const size_t k = (size_t) ((w + i) & (scopeSize - 1));
        destL[i] = scopeL[k];
        destR[i] = scopeR[k];
    }
}

//==============================================================================
void AudiokingsVocalsProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int nch = juce::jmin(2, buffer.getNumChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, n);

    if (nch == 0 || n == 0)
        return;

    // input scope / correlation, captured before any processing
    {
        const float* i0 = buffer.getReadPointer(0);
        const float* i1 = nch > 1 ? buffer.getReadPointer(1) : i0;
        double sLR = 0.0, sLL = 0.0, sRR = 0.0;
        int w = scopePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const size_t k = (size_t) ((w + i) & (scopeSize - 1));
            scopeInL[k] = i0[i];
            scopeInR[k] = i1[i];
            sLR += (double) i0[i] * i1[i];
            sLL += (double) i0[i] * i0[i];
            sRR += (double) i1[i] * i1[i];
        }
        if (sLL > 1.0e-9 && sRR > 1.0e-9)
            correlationIn.store((float) (sLR / std::sqrt(sLL * sRR)), std::memory_order_relaxed);
    }

    auto V = [](std::atomic<float>* a) { return a->load(); };

    const float sens = V(P.sens), noise = V(P.noise), comp = V(P.compress);
    const float w1176 = V(P.w1176), w670 = V(P.w670);
    const int ratioIdx = juce::jlimit(0, 4, (int) std::lround(V(P.ratio)));
    const int tcIdx = juce::jlimit(0, 5, (int) std::lround(V(P.tc)));
    const float attMs = V(P.attack), relMs = V(P.release), deess = V(P.deess);
    const float deessF = V(P.deessf), deessR = V(P.deessr), deessQ = juce::jlimit(1.5f, 6.0f, V(P.deessq));
    const float deessGloF = V(P.deessglo), deessGuHiF = V(P.deessguhi);
    const bool deessWide = V(P.deessmode) > 0.5f, deessListen = V(P.deesslisten) > 0.5f;
    const bool meq5On = V(P.meq5on) > 0.5f;
    const bool fxNoCham = V(P.fxnocham) > 0.5f;
    const float airLow = V(P.airlow), airHigh = V(P.airhigh), fxv = V(P.fx);
    const int revType = juce::jlimit(0, 3, (int) std::lround(V(P.revtype)));
    const float revTone = V(P.revtone), decay = V(P.decay), revMixP = V(P.revmix);
    const int dlyDiv = juce::jlimit(0, 3, (int) std::lround(V(P.dlydiv)));
    const float delayAmt = V(P.delay), delayTone = V(P.delaytone), delayMixP = V(P.delaymix);
    const float outDb = V(P.output);
    const bool auto1176 = V(P.auto1176) > 0.5f, auto670 = V(P.auto670) > 0.5f;
    const float mk1176Manual = V(P.mk1176), mk670Manual = V(P.mk670);
    const float ptube = V(P.ptube);
    const float airLF = V(P.airlf), airHF = V(P.airhf);

    ak::EqParams ep {};
    ep.hpf = V(P.hpf);
    for (int i = 0; i < 5; ++i) { ep.g[i] = V(P.eqg[i]); ep.f[i] = V(P.eqf[i]); ep.q[i] = V(P.eqq[i]); }
    ep.lpOn = V(P.lpon) > 0.5f;
    ep.lpf = V(P.lpf);
    const ak::PultecParams pp { V(P.pbass), V(P.pbassatt), V(P.pbassf), V(P.ptreb), V(P.ptrebatt), V(P.ptrebf), ak::pultecQ(V(P.ptrebbw)) };
    updateFilters(ep, pp);

    float* ch0 = buffer.getWritePointer(0);
    float* ch1 = nch > 1 ? buffer.getWritePointer(1) : nullptr;

    // ---- 1) Level / Sens: input gain + input meter ----
    const float inDb = (sens - 50.f) * 0.36f;     // -18 .. +18 dB
    const float inG = juce::Decibels::decibelsToGain(inDb);
    buffer.applyGainRamp(0, n, lastInGain, inG);
    lastInGain = inG;

    {
        float pl = 0.f, pr = 0.f;
        for (int i = 0; i < n; ++i)
        {
            pl = std::max(pl, std::abs(ch0[i]));
            if (ch1) pr = std::max(pr, std::abs(ch1[i]));
        }
        upMax(meterInL, pl);
        upMax(meterInR, ch1 ? pr : pl);
    }

    // preamp "sensitivity": more gain = softer saturation of the preamp
    const float preDrive = 1.f + 0.045f * std::max(0.f, inDb);
    const bool preOn = preDrive > 1.001f;

    // ---- noise reduction (downward expander) ----
    const bool gateOn = noise > 0.5f;
    const float gateThr = -75.f + 0.45f * noise;
    const float gOpen = (float) std::exp(-1.0 / (sr * 0.003));
    const float gClose = (float) std::exp(-1.0 / (sr * 0.12));
    const float gEnvA = (float) std::exp(-1.0 / (sr * 0.001));
    const float gEnvR = (float) std::exp(-1.0 / (sr * 0.06));

    // ---- compressor A: 1176 (FET, fast, peak) ----
    const float eff1176 = juce::jlimit(0.f, 100.f, comp * w1176 * 0.01f);
    const float eff670 = juce::jlimit(0.f, 100.f, comp * w670 * 0.01f);
    const bool on1176 = eff1176 > 0.5f;
    const bool on670 = eff670 > 0.5f;
    static const float kRatio[5] = { 4.f, 8.f, 12.f, 20.f, 30.f };
    const float ratioA = kRatio[ratioIdx];
    const bool allBtn = ratioIdx == 4;
    const float thrA = -4.f - 0.30f * eff1176;
    const float kneeA = allBtn ? 0.5f : 3.f;
    const float mkA = (auto1176 && on1176 ? 0.30f * (-thrA) * (1.f - 1.f / ratioA) : 0.f) + mk1176Manual;
    const float atkA = allBtn ? std::max(0.02f, attMs * 0.5f) : attMs;
    const float relA = allBtn ? relMs * 0.5f : relMs;
    const float cAtkA = (float) std::exp(-1.0 / (sr * (double) atkA * 0.001));
    const float cRelA = (float) std::exp(-1.0 / (sr * (double) relA * 0.001));
    const float cRelAF = (float) std::exp(-1.0 / (sr * (double) std::max(30.f, relA * 0.25f) * 0.001));   // fast release stage
    const float fetSat = allBtn ? 0.10f : 0.025f;

    // ---- compressor B: Fairchild 670 (variable-mu, soft, slow) ----
    static const float kAtkB[6] = { 0.2f, 0.2f, 0.4f, 0.8f, 2.f, 4.f };
    static const float kRelF[6] = { 300.f, 800.f, 2000.f, 5000.f, 400.f, 500.f };
    static const float kRelS[6] = { 0.f, 0.f, 0.f, 0.f, 6000.f, 10000.f };
    const bool dualB = tcIdx >= 4;
    const float thrB = -4.f - 0.25f * eff670;
    const float mkB = (auto670 && on670 ? 0.85f * ak::vmReduction(-14.f - thrB + 6.f) : 0.f) + mk670Manual;
    const float cAtkB = (float) std::exp(-1.0 / (sr * (double) kAtkB[tcIdx] * 0.001));
    const float cRelFB = (float) std::exp(-1.0 / (sr * (double) kRelF[tcIdx] * 0.001));
    const float cRelSB = dualB ? (float) std::exp(-1.0 / (sr * (double) kRelS[tcIdx] * 0.001)) : cRelFB;

    auto smoothGR = [](float& st, float tgt, float cAtk, float cRel)
    {
        const float cf = tgt < st ? cAtk : cRel;
        st = cf * st + (1.f - cf) * tgt;
    };

    // ---- Pultec activity ----
    const ak::Meq5Params m5p { V(P.meqlm), V(P.meqlmf), V(P.mequm), V(P.mequmf), V(P.meqhm), V(P.meqhmf) };
    const bool meq5Active = meq5On && (m5p.lmBoost > 0.05f || m5p.umAtt > 0.05f || m5p.hmBoost > 0.05f);
    const std::array<float, 6> nowM5 { m5p.lmBoost, m5p.lmF, m5p.umAtt, m5p.umF, m5p.hmBoost, m5p.hmF };
    if (nowM5 != lastMeq5) { lastMeq5 = nowM5; const auto m = ak::makeMeq5(sr, m5p); for (int c=0;c<2;++c){meqLm[c].c=m.lm;meqUm[c].c=m.um;meqHm[c].c=m.hm;} }
    const bool pultecOn = pp.bass > 0.05f || pp.bassAtt > 0.05f || pp.treb > 0.05f || pp.trebAtt > 0.05f || ptube > 0.05f;

    // ---- Air exciter (acts directly on the voice, independent of FX) ----
    const float aLo = juce::jlimit(0.f, 1.f, airLow * 0.01f);
    const float aHi = juce::jlimit(0.f, 1.f, airHigh * 0.01f);
    const bool airOn = aLo > 0.002f || aHi > 0.002f;
    if (std::abs(aLo - lastAirLo) > 0.001f || airLF != lastAirLF)
    {
        lastAirLo = aLo; lastAirLF = airLF;
        updateAirLow(airLF, aLo);
    }
    if (std::abs(aHi - lastAirHi) > 0.001f || airHF != lastAirHF)
    {
        lastAirHi = aHi; lastAirHF = airHF;
        updateAirHigh(airHF, aHi);
    }

    // ---- de-esser (placed right after the Pultec): detector on a band around the sibilance frequency ----
    const bool deOn = deess > 0.5f || deessListen;
    const float deThr = -14.f - 0.28f * deess;
    const float deMax = 3.f + 0.15f * deessR;           // maximum reduction 3 .. 18 dB
    if (deessF != lastDeF || deessQ != lastDeQ)
    {
        lastDeF = deessF;
        lastDeQ = deessQ;
        const auto bp = ak::makeBiquad(ak::BQ::BandPass, sr, (double) deessF, (double) deessQ, 0.0);
        deessBp[0].c = bp; deessBp[1].c = bp;
    }
    if (deessGloF != lastDeGlo)
    {
        lastDeGlo = deessGloF;
        const auto hp = ak::makeBiquad(ak::BQ::HighPass, sr, (double) deessGloF, 0.707, 0.0);
        deessGlo[0].c = hp; deessGlo[1].c = hp;
    }
    if (deessGuHiF != lastDeGuHi)
    {
        lastDeGuHi = deessGuHiF;
        const auto lp = ak::makeBiquad(ak::BQ::LowPass, sr, (double) deessGuHiF, 0.707, 0.0);
        deessGuHi[0].c = lp; deessGuHi[1].c = lp;
    }
    float deMaxRed = 0.f, deEnvMax = 0.f;
    const float envAtk = (float) std::exp(-1.0 / (sr * (double) V(P.deessatk) * 0.001));
    const float envRel = (float) std::exp(-1.0 / (sr * (double) V(P.deessrel) * 0.001));

    // ---- FX macro: 0..50 = warm sat + parallel comp + chorus, 50..100 = distortion + flanger + chamber ----
    const float t = smoothStep(fxv / 50.f);
    const float u = smoothStep((fxv - 50.f) / 50.f);
    const float satMix = 0.22f * t * V(P.fxsat) * 0.01f, pcMix = 0.30f * t * V(P.fxpc) * 0.01f, chMix = 0.32f * t * V(P.fxch) * 0.01f;
    const float distMix = 0.35f * u * V(P.fxdist) * 0.01f, flMix = 0.45f * u * V(P.fxfl) * 0.01f;
    const float chamMix = fxNoCham ? 0.f : 0.30f * u * V(P.fxcham) * 0.01f;
    const bool satOn = satMix > 0.003f, pcOn = pcMix > 0.003f, chOn = chMix > 0.003f;
    const bool distOn = distMix > 0.003f, flOn = flMix > 0.003f;
    const float chInc = 0.55f / (float) sr, flInc = 0.22f / (float) sr;
    const float aDist = 1.f - std::exp(-2.f * 3.14159265f * 5000.f / (float) sr);
    const float pcAtk = (float) std::exp(-1.0 / (sr * 0.003));
    const float pcRel = (float) std::exp(-1.0 / (sr * 0.09));
    const int chSize = (int) chBuf[0].size();
    const int flSize = (int) flBuf[0].size();
    constexpr float twoPi = 6.28318531f;

    // level-adaptive cubic shaper: no linear term, so band leakage can never cancel the voice
    const float hAtk = (float) std::exp(-1.0 / (sr * 0.0003));
    const float hRel = (float) std::exp(-1.0 / (sr * 0.015));
    auto cubicHarm = [hAtk, hRel](float src, float& env)
    {
        const float a = std::abs(src);
        env = a > env ? hAtk * env + (1.f - hAtk) * a : hRel * env + (1.f - hRel) * a;
        const float e = env + 1.0e-5f;
        const float u = juce::jlimit(-1.f, 1.f, src / e);
        return u * u * u * e;
    };

    float gr1176Max = 0.f, gr670Max = 0.f, grTotMax = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { ch0[i], ch1 ? ch1[i] : 0.f };

        if (preOn)
            for (int c = 0; c < nch; ++c)
                x[c] = std::tanh(preDrive * x[c]) / preDrive;

        for (int c = 0; c < nch; ++c)
            x[c] = hpf[c].process(x[c]);

        // --- noise reduction ---
        if (gateOn)
        {
            float det = std::abs(x[0]);
            if (nch > 1) det = std::max(det, std::abs(x[1]));
            gateEnv = det > gateEnv ? gEnvA * gateEnv + (1.f - gEnvA) * det : gEnvR * gateEnv + (1.f - gEnvR) * det;
            const float envDb = juce::Decibels::gainToDecibels(gateEnv, -100.f);
            const float tgt = envDb < gateThr ? -std::min(18.f, gateThr - envDb) : 0.f;
            const float cf = tgt < gateDb ? gClose : gOpen;
            gateDb = cf * gateDb + (1.f - cf) * tgt;
            const float gg = juce::Decibels::decibelsToGain(gateDb);
            for (int c = 0; c < nch; ++c) x[c] *= gg;
        }

        // --- 2) 1176 (FET): fast peak detector, program-dependent release, FET colour, make-up ---
        float grA = 0.f;
        {
            float target = 0.f;
            if (on1176)
            {
                float det = std::abs(x[0]);
                if (nch > 1) det = std::max(det, std::abs(x[1]));
                const float lvl = juce::Decibels::gainToDecibels(det, -100.f);
                target = -ak::comp1176Reduction(lvl, thrA, ratioA, kneeA);
            }
            smoothGR(gr1176, target, cAtkA, cRelA);          // slow release stage
            smoothGR(gr1176F, target, cAtkA, cRelAF);        // fast release stage
            const float depthA = juce::jlimit(0.f, 1.f, -gr1176 / 12.f);
            grA = gr1176F * (1.f - depthA) + gr1176 * depthA;     // deeper reduction = slower release
            mk1176 += 0.0008f * (mkA - mk1176);
            const float g1 = juce::Decibels::decibelsToGain(grA + mk1176);
            for (int c = 0; c < nch; ++c) x[c] *= g1;
            if (on1176)    // FET colour: harmonic content grows with the amount of reduction
            {
                const float fet = fetSat + 0.05f * depthA;
                for (int c = 0; c < nch; ++c)
                    x[c] += fet * (std::tanh(2.5f * x[c]) / 2.5f - x[c]);
            }
            gr1176Max = std::max(gr1176Max, -grA);
        }

        // --- 3) Pultec EQ ---
        if (pultecOn)
        {
            for (int c = 0; c < nch; ++c)
            {
                x[c] = pBassB[c].process(x[c]);
                x[c] = pBassA[c].process(x[c]);
                x[c] = pTrebB[c].process(x[c]);
                x[c] = pTrebA[c].process(x[c]);
                if (ptube > 0.05f)    // valve warmth of the passive EQ
                    x[c] += ptube * 0.0035f * (std::tanh(1.8f * x[c]) / 1.8f - x[c]);
            }
        }
        if (meq5Active)
            for (int c = 0; c < nch; ++c)
            {
                x[c] = meqLm[c].process(x[c]);
                x[c] = meqUm[c].process(x[c]);
                x[c] = meqHm[c].process(x[c]);
            }

        // --- 4) Fairchild 670: variable-mu, side-chain high-pass, transformer and core colour, make-up ---
        {
            float target = 0.f;
            if (on670)
            {
                const float d0 = detHp[0].process(x[0]);
                const float d1 = nch > 1 ? detHp[1].process(x[1]) : 0.f;
                const float det = std::max(std::abs(d0), std::abs(d1));
                const float lvl = juce::Decibels::gainToDecibels(det, -100.f);
                target = -ak::comp670Reduction(lvl, thrB);
            }
            smoothGR(gr670F, target, cAtkB, cRelFB);
            float grB = gr670F;
            if (dualB)
            {
                smoothGR(gr670S, target, cAtkB, cRelSB);
                grB = 0.4f * gr670F + 0.6f * gr670S;
            }
            else
                gr670S = gr670F;

            mk670 += 0.0008f * (mkB - mk670);
            const float g2 = juce::Decibels::decibelsToGain(grB + mk670);
            for (int c = 0; c < nch; ++c) x[c] *= g2;
            if (on670)
            {
                const float depthB = juce::jlimit(0.f, 1.f, -grB / 12.f);
                for (int c = 0; c < nch; ++c)
                {
                    x[c] += 0.03f * (std::tanh(1.6f * x[c]) / 1.6f - x[c]);                 // output transformer
                    const float lf = lfLp[c].process(x[c]);                                   // core saturation (low band)
                    x[c] += (0.02f + 0.05f * depthB) * (std::tanh(3.f * lf) / 3.f - lf);
                }
            }
            gr670Max = std::max(gr670Max, -grB);
            grTotMax = std::max(grTotMax, -grA - grB);
        }

        // --- 5) Graphic EQ (+ optional low-pass) ---
        for (int c = 0; c < nch; ++c)
        {
            for (int k = 0; k < 5; ++k)
                x[c] = eq[k][c].process(x[c]);
            if (ep.lpOn)
            {
                x[c] = lp1[c].process(x[c]);
                x[c] = lp2[c].process(x[c]);
            }
        }

        // --- 6) Air exciter: harmonics + shelf, acting directly on the voice (it only adds, never cuts) ---
        if (airOn)
        {
            for (int c = 0; c < nch; ++c)
            {
                float s = x[c];
                if (aLo > 0.002f)
                {
                    float src = aLoSrc[c].process(x[c]);
                    for (int k = 0; k < 4; ++k) src = aLoLp[k][c].process(src);          // source band only
                    const float h = aLoOut2[c].process(aLoOut[c].process(cubicHarm(src, aLoEnv[c])));   // new harmonics only
                    s += 1.5f * aLo * h;
                    s = aLoPeak[c].process(s);
                }
                if (aHi > 0.002f)
                {
                    float src = aHiSrc[c].process(x[c]);
                    for (int k = 0; k < 4; ++k) src = aHiLp[k][c].process(src);          // 3.2 - 5.5 kHz source
                    const float h = aHiOut2[c].process(aHiOut[c].process(cubicHarm(src, aHiEnv[c])));   // harmonics above 9 kHz
                    s += 3.0f * aHi * h;
                    s = aHiShelf[c].process(s);                                           // +8 dB max boost, never a cut
                }
                x[c] = s;
            }
        }

        // --- de-esser: after the Air exciter, to catch the sibilance it creates ---
        if (deOn)
        {
            // Guard: only the signal between Guard Lo and Guard Hi goes into the de-esser path
            float guard[2] = { 0.f, 0.f };
            for (int c = 0; c < nch; ++c)
            {
                guard[c] = deessGlo[c].process(x[c]);
                guard[c] = deessGuHi[c].process(guard[c]);
            }
            float band[2] = { 0.f, 0.f };
            float det = 0.f;
            for (int c = 0; c < nch; ++c)
            {
                band[c] = deessBp[c].process(guard[c]);     // narrow sibilance band from the guarded signal
                det = std::max(det, std::abs(band[c]));
            }
            deessEnv = det > deessEnv ? envAtk * deessEnv + (1.f - envAtk) * det
                                      : envRel * deessEnv + (1.f - envRel) * det;
            const float envDb = juce::Decibels::gainToDecibels(deessEnv, -100.f);
            const float red = std::min(deMax, std::max(0.f, envDb - deThr) * 0.75f);
            deMaxRed = std::max(deMaxRed, red);
            deEnvMax = std::max(deEnvMax, deessEnv);
            const float dg = juce::Decibels::decibelsToGain(-red);
            if (deessListen)
                for (int c = 0; c < nch; ++c) x[c] = band[c] * 3.f;            // monitor the sibilance band
            else if (deessWide)
                for (int c = 0; c < nch; ++c) x[c] *= dg;                        // wideband ducking
            else
                for (int c = 0; c < nch; ++c) x[c] -= (1.f - dg) * band[c];      // split-band (transparent)
        }

        // --- 7) FX macro (independent of the reverb and delay) ---
        {
            const float xs[2] = { x[0], x[1] };

            // modulation lines always run so they are ready when FX opens
            float chWet[2] = { 0.f, 0.f }, flWet[2] = { 0.f, 0.f };
            chPh += chInc; if (chPh >= 1.f) chPh -= 1.f;
            flPh += flInc; if (flPh >= 1.f) flPh -= 1.f;
            for (int c = 0; c < nch; ++c)
                chBuf[c][(size_t) chWp] = xs[c];
            if (chOn)
                for (int c = 0; c < nch; ++c)
                {
                    const float lfo = std::sin(twoPi * (chPh + 0.25f * (float) c));
                    chWet[c] = readFrac(chBuf[c], chWp, (14.f + 4.f * lfo) * 0.001f * (float) sr);
                }
            if (++chWp >= chSize) chWp = 0;

            if (flOn)
                for (int c = 0; c < nch; ++c)
                {
                    const float lfo = std::sin(twoPi * (flPh + 0.2f * (float) c));
                    flWet[c] = readFrac(flBuf[c], flWp, (2.6f + 2.0f * lfo) * 0.001f * (float) sr);
                }
            for (int c = 0; c < nch; ++c)
                flBuf[c][(size_t) flWp] = xs[c] + 0.55f * flWet[c];
            if (++flWp >= flSize) flWp = 0;

            // parallel compression (heavily squashed copy of the voice)
            float pcGain = 0.f;
            if (pcOn)
            {
                float det = std::abs(xs[0]);
                if (nch > 1) det = std::max(det, std::abs(xs[1]));
                const float lvl = juce::Decibels::gainToDecibels(det, -100.f);
                const float tgt = -std::max(0.f, lvl + 30.f) * (1.f - 1.f / 6.f);
                smoothGR(pcGr, tgt, pcAtk, pcRel);
                pcGain = juce::Decibels::decibelsToGain(pcGr + 9.f);
            }

            for (int c = 0; c < nch; ++c)
            {
                float y = xs[c];
                if (satOn)  y += satMix * (std::tanh(2.2f * xs[c]) / 2.2f - xs[c]);   // warm, delicate
                if (pcOn)   y += pcMix * xs[c] * pcGain;
                if (chOn)   y += chMix * chWet[c];
                if (flOn)   y += flMix * flWet[c];
                if (distOn)
                {
                    const float d = std::tanh(8.f * xs[c]) * 0.5f;
                    distLp[c] += aDist * (d - distLp[c]);
                    y += distMix * distLp[c];
                }
                x[c] = y;
            }
        }

        ch0[i] = x[0];
        if (ch1) ch1[i] = x[1];
    }

    upMax(meterGR1176, gr1176Max);
    upMax(meterGR670, gr670Max);
    upMax(meterGRTotal, grTotMax);
    upMax(meterGRDeEss, deMaxRed);
    upMax(meterDeEssEnv, deEnvMax);

    // 8) chamber reverb (FX > 50%): small dense room, independent of the Reverb section
    if (chamMix > 0.003f)
    {
        if (chamBuf.getNumSamples() < n || chamBuf.getNumChannels() < 2)
            chamBuf.setSize(2, n, false, false, true);

        chamBuf.copyFrom(0, 0, ch0, n);
        chamBuf.copyFrom(1, 0, ch1 ? ch1 : ch0, n);

        juce::Reverb::Parameters cp;
        cp.roomSize = 0.38f; cp.damping = 0.45f; cp.wetLevel = 0.33f; cp.dryLevel = 0.f;
        cp.width = 1.f; cp.freezeMode = 0.f;
        chamber.setParameters(cp);
        chamber.processStereo(chamBuf.getWritePointer(0), chamBuf.getWritePointer(1), n);

        const float* cl = chamBuf.getReadPointer(0);
        const float* cr = chamBuf.getReadPointer(1);
        if (ch1) for (int i = 0; i < n; ++i) { ch0[i] += cl[i] * chamMix; ch1[i] += cr[i] * chamMix; }
        else     for (int i = 0; i < n; ++i) ch0[i] += 0.5f * (cl[i] + cr[i]) * chamMix;
    }

    // =================== parallel Delay + Reverb ===================
    const float tone01 = delayTone * 0.01f;
    const float dMix = delayMixP * 0.01f;
    const float dLevel = dMix * 0.7f;
    const bool dlyOn = dLevel > 0.002f;

    if (dlyOn)
    {
        if (dlyOut.getNumSamples() < n || dlyOut.getNumChannels() < 2)
            dlyOut.setSize(2, n, false, false, true);

        double bpm = 120.0;
        if (auto* ph = getPlayHead())
        {
            auto pos = ph->getPosition();
            if (pos.hasValue())
            {
                auto b = pos->getBpm();
                if (b.hasValue() && *b > 30.0) bpm = *b;
            }
        }
        hostBpm.store((float) bpm);
        static const double kBeats[5] = { 2.0, 1.0, 0.75, 0.5, 0.25 };
        const int dlySize = (int) dlyBuf[0].size();
        const int dlySamples = juce::jlimit(64, dlySize - 2, (int) (sr * 60.0 / bpm * kBeats[dlyDiv]));
        const float fb = 0.10f + 0.55f * delayAmt * 0.01f;        // Delay = amount of echo (repeats)
        const float fc = 1200.f * std::pow(8.f, tone01);          // 1.2 kHz .. 9.6 kHz
        const float a = 1.f - std::exp(-2.f * 3.14159265f * fc / (float) sr);
        const float dW = V(P.dlywidth) * 0.01f;                                   // 1 = full ping-pong, 0 = dual mono
        const float aHp = 1.f - std::exp(-2.f * 3.14159265f * V(P.dlyhp) / (float) sr);

        float* dl = dlyOut.getWritePointer(0);
        float* dr = dlyOut.getWritePointer(1);
        for (int i = 0; i < n; ++i)
        {
            const float inL = ch0[i];
            const float inR = ch1 ? ch1[i] : inL;
            const float mono = 0.5f * (inL + inR);
            int rp = dlyWp - dlySamples;
            if (rp < 0) rp += dlySize;
            const float tL = dlyBuf[0][(size_t) rp];
            const float tR = dlyBuf[1][(size_t) rp];

            const float inLw = mono + fb * (dW * tR + (1.f - dW) * tL);
            const float inRw = mono * (1.f - dW) + fb * (dW * tL + (1.f - dW) * tR);
            dlyLp[0] += a * (inLw - dlyLp[0]);
            dlyLp[1] += a * (inRw - dlyLp[1]);
            dlyHp[0] += aHp * (dlyLp[0] - dlyHp[0]);
            dlyHp[1] += aHp * (dlyLp[1] - dlyHp[1]);
            dlyBuf[0][(size_t) dlyWp] = dlyLp[0] - dlyHp[0];
            dlyBuf[1][(size_t) dlyWp] = dlyLp[1] - dlyHp[1];
            if (++dlyWp >= dlySize) dlyWp = 0;

            dl[i] = tL * dLevel;
            dr[i] = tR * dLevel;
        }
    }

    // reverb: parallel; a dark Delay Tone also blooms the echoes into the reverb
    const float revGain = revMixP * 0.006f + (dlyOn ? 0.15f * (1.f - tone01) * dMix : 0.f);

    const float* wl = nullptr;
    const float* wr = nullptr;
    if (revGain > 0.003f)
    {
        if (reverbBuf.getNumSamples() < n || reverbBuf.getNumChannels() < 2)
            reverbBuf.setSize(2, n, false, false, true);

        const auto& rt = kRev[revType];
        const bool studio = revType == 3;
        const float feed = dlyOn ? 1.2f * (1.f - tone01) : 0.f;
        const int preSize = (int) preBuf[0].size();
        const int preSamples = juce::jlimit(0, preSize - 2, (int) (sr * (rt.preMs + V(P.revpre)) * 0.001));
        const float* d0 = dlyOn ? dlyOut.getReadPointer(0) : nullptr;
        const float* d1 = dlyOn ? dlyOut.getReadPointer(1) : nullptr;
        float* rl = reverbBuf.getWritePointer(0);
        float* rr = reverbBuf.getWritePointer(1);

        for (int i = 0; i < n; ++i)
        {
            float inL = ch0[i];
            float inR = ch1 ? ch1[i] : inL;
            if (dlyOn) { inL += feed * d0[i]; inR += feed * d1[i]; }

            preBuf[0][(size_t) preWp] = inL;
            preBuf[1][(size_t) preWp] = inR;
            int rp = preWp - preSamples;
            if (rp < 0) rp += preSize;
            if (++preWp >= preSize) preWp = 0;

            float in0 = revHp[0].process(preBuf[0][(size_t) rp]);
            float in1 = revHp[1].process(preBuf[1][(size_t) rp]);
            if (studio)   // console-style warmth going into the room
            {
                in0 += 0.5f * (std::tanh(2.f * in0) * 0.5f - in0);
                in1 += 0.5f * (std::tanh(2.f * in1) * 0.5f - in1);
            }
            rl[i] = in0;
            rr[i] = in1;
        }

        juce::Reverb::Parameters rpar;
        rpar.roomSize = rt.sizeLo + (rt.sizeHi - rt.sizeLo) * decay * 0.01f;
        rpar.damping = juce::jlimit(0.f, 1.f, rt.damp + (50.f - revTone) * 0.008f);
        rpar.wetLevel = 0.33f;
        rpar.dryLevel = 0.f;
        rpar.width = V(P.revwidth) * 0.01f;
        rpar.freezeMode = 0.f;
        reverb.setParameters(rpar);
        reverb.processStereo(rl, rr, n);

        if (revTone != lastRevTone)
        {
            lastRevTone = revTone;
            const auto hs = ak::makeBiquad(ak::BQ::HighShelf, sr, 3500.0, 0.7, (double) ((revTone - 50.f) * 0.12f));
            wetHs[0].c = hs;
            wetHs[1].c = hs;
        }
        for (int i = 0; i < n; ++i)
        {
            float w0 = rl[i], w1 = rr[i];
            if (studio)   // subtle saturation on the tail
            {
                w0 += 0.25f * (std::tanh(3.f * w0) / 3.f - w0);
                w1 += 0.25f * (std::tanh(3.f * w1) / 3.f - w1);
            }
            rl[i] = wetHs[0].process(w0) * revGain;
            rr[i] = wetHs[1].process(w1) * revGain;
        }
        wl = rl;
        wr = rr;
    }

    // add the parallel signals
    if (dlyOn || wl != nullptr)
    {
        const float* d0 = dlyOn ? dlyOut.getReadPointer(0) : nullptr;
        const float* d1 = dlyOn ? dlyOut.getReadPointer(1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            float l = 0.f, r = 0.f;
            if (d0) { l += d0[i]; r += d1[i]; }
            if (wl) { l += wl[i];  r += wr[i]; }
            if (ch1) { ch0[i] += l; ch1[i] += r; }
            else     { ch0[i] += 0.5f * (l + r); }
        }
    }

    // ---- output gain ----
    const float outG = juce::Decibels::decibelsToGain(outDb);
    buffer.applyGainRamp(0, n, lastOutGain, outG);
    lastOutGain = outG;

    // ---- output meters, stereo correlation, scope feed ----
    {
        float pl = 0.f, pr = 0.f;
        double sLR = 0.0, sLL = 0.0, sRR = 0.0;
        int w = scopePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            const float l = ch0[i];
            const float r = ch1 ? ch1[i] : l;
            pl = std::max(pl, std::abs(l));
            pr = std::max(pr, std::abs(r));
            sLR += (double) l * r;
            sLL += (double) l * l;
            sRR += (double) r * r;
            const size_t k = (size_t) (w & (scopeSize - 1));
            scopeL[k] = l;
            scopeR[k] = r;
            ++w;
        }
        scopePos.store(w & (scopeSize - 1), std::memory_order_relaxed);
        upMax(meterOutL, pl);
        upMax(meterOutR, pr);
        if (sLL > 1.0e-9 && sRR > 1.0e-9)
            correlation.store((float) (sLR / std::sqrt(sLL * sRR)), std::memory_order_relaxed);
    }
}

//==============================================================================
// Presets, user presets, A/B
//==============================================================================
ak::Values AudiokingsVocalsProcessor::getValues() const
{
    ak::Values v {};
    for (int i = 0; i < ak::kNumParams; ++i)
        if (auto* p = apvts.getRawParameterValue(ak::kParams[i].id))
            v[(size_t) i] = p->load();
    return v;
}

void AudiokingsVocalsProcessor::setValues(const ak::Values& v)
{
    for (int i = 0; i < ak::kNumParams; ++i)
    {
        if (auto* prm = apvts.getParameter(ak::kParams[i].id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(v[(size_t) i]));
            prm->endChangeGesture();
        }
    }
}

int AudiokingsVocalsProcessor::getCurrentPreset() const
{
    return (int) apvts.state.getProperty("preset", 0);   // -1 when a user preset is active
}

void AudiokingsVocalsProcessor::loadFactoryPreset(int index)
{
    index = juce::jlimit(0, ak::numPresets() - 1, index);
    setValues(ak::resolve(ak::presetAt(index)));
    apvts.state.setProperty("preset", index, nullptr);
    apvts.state.setProperty("userpreset", "", nullptr);
}

juce::File AudiokingsVocalsProcessor::getAppDataDir()
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile("Application Support");     // JUCE returns ~/Library on macOS
   #endif
    return base.getChildFile("Audiokings").getChildFile("Vocals");
}

juce::File AudiokingsVocalsProcessor::getUserPresetDir()
{
    return getAppDataDir().getChildFile("Presets");
}

juce::StringArray AudiokingsVocalsProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getUserPresetDir().findChildFiles(juce::File::findFiles, false, "*.akpreset"))
        names.add(f.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}

juce::String AudiokingsVocalsProcessor::getCurrentUserPreset() const
{
    return apvts.state.getProperty("userpreset", "").toString();
}

bool AudiokingsVocalsProcessor::saveUserPreset(const juce::String& name)
{
    const auto legal = juce::File::createLegalFileName(name.trim());
    if (legal.isEmpty())
        return false;

    auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk())
        return false;

    juce::XmlElement root("AKPRESET");
    root.setAttribute("name", legal);
    const auto v = getValues();
    for (int i = 0; i < ak::kNumParams; ++i)
    {
        auto* e = root.createNewChildElement("P");
        e->setAttribute("id", ak::kParams[i].id);
        e->setAttribute("v", (double) v[(size_t) i]);
    }
    if (! root.writeTo(dir.getChildFile(legal + ".akpreset")))
        return false;

    apvts.state.setProperty("preset", -1, nullptr);
    apvts.state.setProperty("userpreset", legal, nullptr);
    return true;
}

bool AudiokingsVocalsProcessor::loadUserPreset(const juce::String& name)
{
    auto xml = juce::parseXML(getUserPresetDir().getChildFile(name + ".akpreset"));
    if (xml == nullptr || ! xml->hasTagName("AKPRESET"))
        return false;

    auto v = ak::defaultValues();
    for (auto* e : xml->getChildWithTagNameIterator("P"))
    {
        const int i = ak::indexOfParam(e->getStringAttribute("id").toRawUTF8());
        if (i >= 0)
            v[(size_t) i] = (float) e->getDoubleAttribute("v", (double) v[(size_t) i]);
    }
    setValues(v);
    apvts.state.setProperty("preset", -1, nullptr);
    apvts.state.setProperty("userpreset", name, nullptr);
    return true;
}

bool AudiokingsVocalsProcessor::deleteUserPreset(const juce::String& name)
{
    const bool ok = getUserPresetDir().getChildFile(name + ".akpreset").deleteFile();
    if (ok && getCurrentUserPreset() == name)
        apvts.state.setProperty("userpreset", "", nullptr);
    return ok;
}

int AudiokingsVocalsProcessor::getABSlot() const
{
    return (int) apvts.state.getProperty("ab", 0);
}

void AudiokingsVocalsProcessor::toggleAB()
{
    const int cur = getABSlot();
    const int other = 1 - cur;
    apvts.state.setProperty(cur == 0 ? "slotA" : "slotB", valuesToString(getValues()), nullptr);

    const auto s = apvts.state.getProperty(other == 0 ? "slotA" : "slotB", "").toString();
    if (s.isNotEmpty())
    {
        auto v = getValues();
        const auto tok = juce::StringArray::fromTokens(s, ",", "");
        for (int i = 0; i < ak::kNumParams && i < tok.size(); ++i)
            v[(size_t) i] = tok[i].getFloatValue();
        setValues(v);
    }
    apvts.state.setProperty("ab", other, nullptr);
}

void AudiokingsVocalsProcessor::copyToOtherAB()
{
    const int other = 1 - getABSlot();
    apvts.state.setProperty(other == 0 ? "slotA" : "slotB", valuesToString(getValues()), nullptr);
}

void AudiokingsVocalsProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void AudiokingsVocalsProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* AudiokingsVocalsProcessor::createEditor()
{
    return new AudiokingsVocalsEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudiokingsVocalsProcessor();
}
