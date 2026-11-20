#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
              int idx, const juce::String& name, float lo, float hi, float step,
              float skewCentre, const juce::String& unit, int decimals)
{
    juce::NormalisableRange<float> range(lo, hi, step);
    if (skewCentre > 0.f)
        range.setSkewForCentre(skewCentre);

    auto attrs = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([unit, decimals, idx](float v, int)
        {
            if (idx == ak::Sens)   // Level/Sens is displayed as input gain in dB
            {
                const float db = (v - 50.f) * 0.36f;
                return juce::String(db, 1) + " dB";
            }
            return juce::String(v, decimals) + unit;
        });

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ak::kParamIds[idx], 1 }, name, range, ak::presetAt(0).v[idx], attrs));
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    using namespace ak;
    addFloat(l, Sens,      "Level / Sens",   0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Compress,  "Compress",       0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Bass,      "Bass",         -12.f,   12.f, 0.1f,  0.f,    " dB", 1);
    addFloat(l, Treble,    "Treble",       -12.f,   12.f, 0.1f,  0.f,    " dB", 1);
    addFloat(l, Air,       "Air",          -12.f,   12.f, 0.1f,  0.f,    " dB", 1);
    addFloat(l, Fx,        "FX",             0.f,  100.f, 0.1f,  0.f,    " %",  0);

    l.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kParamIds[RevType], 1 }, "Reverb Type",
        juce::StringArray { "Room", "Plate", "Hall" }, (int) presetAt(0).v[RevType]));

    addFloat(l, RevTone,   "Reverb Tone",    0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Decay,     "Decay",          0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Delay,     "Delay",          0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, DelayTone, "Delay Tone",     0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, AirLow,    "Air Low",        0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, AirHigh,   "Air High",       0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Output,    "Output",       -12.f,   12.f, 0.1f,  0.f,    " dB", 1);
    addFloat(l, Hpf,       "High-Pass",     20.f,  300.f, 1.0f, 90.f,    " Hz", 0);
    addFloat(l, Attack,    "Comp Attack",    0.3f,  60.f, 0.1f,  8.f,    " ms", 1);
    addFloat(l, Release,   "Comp Release",  20.f,  800.f, 1.0f, 150.f,   " ms", 0);
    addFloat(l, DeEss,     "De-Esser",       0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Drive,     "Saturation",     0.f,  100.f, 0.1f,  0.f,    " %",  0);
    addFloat(l, Mud,       "Low-Mid",      -12.f,   12.f, 0.1f,  0.f,    " dB", 1);
    addFloat(l, BassF,     "Bass Freq",     40.f,  400.f, 1.0f, 120.f,   " Hz", 0);
    addFloat(l, MudF,      "Low-Mid Freq", 150.f, 1000.f, 1.0f, 350.f,   " Hz", 0);
    addFloat(l, TrebF,     "Treble Freq", 1500.f, 8000.f, 1.0f, 3500.f,  " Hz", 0);
    addFloat(l, AirF,      "Air Freq",    6000.f, 18000.f, 1.0f, 10000.f, " Hz", 0);
    return l;
}

inline void upMax(std::atomic<float>& a, float v)
{
    if (v > a.load(std::memory_order_relaxed))
        a.store(v, std::memory_order_relaxed);
}

struct RevType { float sizeLo, sizeHi, preMs, damp; };
constexpr RevType kRev[3] = {
    { 0.15f, 0.55f,  0.f, 0.55f },   // Room
    { 0.40f, 0.80f, 12.f, 0.25f },   // Plate
    { 0.60f, 0.97f, 28.f, 0.40f }    // Hall
};
} // namespace

//==============================================================================
AudiokingsVocalsProcessor::AudiokingsVocalsProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < ak::kNumParams; ++i)
        paramPtr[(size_t) i] = apvts.getRawParameterValue(ak::kParamIds[i]);

    apvts.state.setProperty("preset", 0, nullptr);
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
    reverb.setSampleRate(sr);
    reverb.reset();
    reverbBuf.setSize(2, juce::jmax(samplesPerBlock, 512));
    dlyOut.setSize(2, juce::jmax(samplesPerBlock, 512));

    const int dlySize = (int) (sr * 1.5) + 8;
    const int preSize = (int) (sr * 0.1) + 8;
    for (int c = 0; c < 2; ++c)
    {
        dlyBuf[c].assign((size_t) dlySize, 0.f);
        preBuf[c].assign((size_t) preSize, 0.f);

        hpf[c].reset(); bassS[c].reset(); mudB[c].reset(); trebB[c].reset(); airS[c].reset();
        deessHp[c].reset(); aLoHp[c].reset(); aLoHp2[c].reset(); aHiHp[c].reset(); aHiHp2[c].reset();
        revHp[c].reset(); wetHs[c].reset();

        deessHp[c].c = ak::makeBiquad(ak::BQ::HighPass, sr, 5500.0, 0.707, 0.0);
        aLoHp[c].c   = ak::makeBiquad(ak::BQ::HighPass, sr, 2500.0, 0.707, 0.0);
        aLoHp2[c].c  = aLoHp[c].c;
        aHiHp[c].c   = ak::makeBiquad(ak::BQ::HighPass, sr, 8000.0, 0.707, 0.0);
        aHiHp2[c].c  = aHiHp[c].c;
        revHp[c].c   = ak::makeBiquad(ak::BQ::HighPass, sr, 180.0, 0.707, 0.0);
    }
    dlyWp = preWp = 0;
    dlyLp[0] = dlyLp[1] = 0.f;

    eqValid = false;
    lastRevTone = -1.f;
    grState = 0.f;
    makeupCur = 0.f;
    deessEnv = 0.f;
    lastInGain  = juce::Decibels::decibelsToGain((paramPtr[ak::Sens]->load() - 50.f) * 0.36f);
    lastOutGain = juce::Decibels::decibelsToGain(paramPtr[ak::Output]->load());
}

void AudiokingsVocalsProcessor::updateFilters(const ak::EqParams& p)
{
    const std::array<float, 9> now { p.hpf, p.bassG, p.bassF, p.mudG, p.mudF, p.trebG, p.trebF, p.airG, p.airF };
    if (eqValid && now == lastEq)
        return;

    lastEq = now;
    eqValid = true;
    const auto e = ak::makeEq(sr, p);
    for (int c = 0; c < 2; ++c)
    {
        hpf[c].c = e.hp;
        bassS[c].c = e.bass;
        mudB[c].c = e.mud;
        trebB[c].c = e.treb;
        airS[c].c = e.air;
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

    auto P = [this](int i) { return paramPtr[(size_t) i]->load(); };
    const float sens = P(ak::Sens), comp = P(ak::Compress), fx = P(ak::Fx);
    const int revType = juce::jlimit(0, 2, (int) std::lround(P(ak::RevType)));
    const float revTone = P(ak::RevTone), decay = P(ak::Decay);
    const float delayAmt = P(ak::Delay), delayTone = P(ak::DelayTone);
    const float airLow = P(ak::AirLow), airHigh = P(ak::AirHigh), outDb = P(ak::Output);
    const float attMs = P(ak::Attack), relMs = P(ak::Release), deess = P(ak::DeEss), drive = P(ak::Drive);

    ak::EqParams ep { P(ak::Hpf), P(ak::Bass), P(ak::BassF), P(ak::Mud), P(ak::MudF),
                      P(ak::Treble), P(ak::TrebF), P(ak::Air), P(ak::AirF) };
    updateFilters(ep);

    float* ch0 = buffer.getWritePointer(0);
    float* ch1 = nch > 1 ? buffer.getWritePointer(1) : nullptr;

    // ---- Level / Sens: input gain + input meter ----
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

    // preamp "sensitivity": the more gain, the more the preamp saturates softly
    const float preDrive = 1.f + 0.045f * std::max(0.f, inDb);
    const bool preOn = preDrive > 1.001f;

    // ---- Compressor settings ----
    const bool compOn = comp > 0.5f;
    const float thr = -6.f - 0.30f * comp;               // -6 .. -36 dB
    const float ratio = 1.5f + 0.065f * comp;            // 1.5 .. 8
    const float knee = 6.f;
    const float makeupTarget = compOn ? 0.35f * (-thr) * (1.f - 1.f / ratio) : 0.f;
    const double attC = std::exp(-1.0 / (sr * (double) juce::jmax(0.05f, attMs) * 0.001));
    const double relC = std::exp(-1.0 / (sr * (double) juce::jmax(1.f, relMs) * 0.001));

    // ---- Air exciter (Fresh-Air style) ----
    const float aLo = juce::jlimit(0.f, 1.f, airLow * 0.01f);
    const float aHi = juce::jlimit(0.f, 1.f, airHigh * 0.01f + fx * 0.002f);   // FX adds some brightness
    const bool airOn = aLo > 0.002f || aHi > 0.002f;

    // ---- De-esser ----
    const bool deOn = deess > 0.5f;
    const float deThr = -16.f - 0.2f * deess;
    const float envAtk = (float) std::exp(-1.0 / (sr * 0.0005));
    const float envRel = (float) std::exp(-1.0 / (sr * 0.04));

    // ---- Harmonic saturation: base drive + part of the FX macro ----
    const float satMix = juce::jlimit(0.f, 1.f, (drive + 0.25f * fx) * 0.01f);
    const float satDrive = 1.f + satMix * 4.5f;

    float grMax = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { ch0[i], ch1 ? ch1[i] : 0.f };

        if (preOn)
            for (int c = 0; c < nch; ++c)
                x[c] = std::tanh(preDrive * x[c]) / preDrive;

        for (int c = 0; c < nch; ++c)
            x[c] = hpf[c].process(x[c]);

        // --- Compressor (linked, soft knee, auto make-up) ---
        float target = 0.f;
        if (compOn)
        {
            float det = std::abs(x[0]);
            if (nch > 1) det = std::max(det, std::abs(x[1]));
            const float lvl = juce::Decibels::gainToDecibels(det, -100.f);
            const float over = lvl - thr;
            float outDb2 = lvl;
            if (2.f * over > knee)
                outDb2 = thr + over / ratio;
            else if (2.f * over > -knee)
            {
                const float t = over + knee * 0.5f;
                outDb2 = lvl + (1.f / ratio - 1.f) * t * t / (2.f * knee);
            }
            target = outDb2 - lvl;   // <= 0
        }
        const float coef = (float) (target < grState ? attC : relC);
        grState = coef * grState + (1.f - coef) * target;
        grMax = std::max(grMax, -grState);
        makeupCur += 0.0008f * (makeupTarget - makeupCur);
        const float cg = juce::Decibels::decibelsToGain(grState + makeupCur);
        for (int c = 0; c < nch; ++c)
            x[c] *= cg;

        // --- EQ (4 movable nodes) ---
        for (int c = 0; c < nch; ++c)
        {
            x[c] = bassS[c].process(x[c]);
            x[c] = mudB[c].process(x[c]);
            x[c] = trebB[c].process(x[c]);
            x[c] = airS[c].process(x[c]);
        }

        // --- Air exciter: generates harmonics in two high bands and adds them back ---
        if (airOn)
        {
            for (int c = 0; c < nch; ++c)
            {
                float s = x[c];
                if (aLo > 0.002f)
                {
                    const float b = aLoHp[c].process(x[c]);
                    s += 0.12f * aLo * aLoHp2[c].process(std::tanh(5.f * b));
                }
                if (aHi > 0.002f)
                {
                    const float b = aHiHp[c].process(x[c]);
                    s += 0.15f * aHi * aHiHp2[c].process(std::tanh(5.f * b));
                }
                x[c] = s;
            }
        }

        // --- De-esser ---
        if (deOn)
        {
            float band[2] = { 0.f, 0.f }, low[2] = { 0.f, 0.f };
            float det = 0.f;
            for (int c = 0; c < nch; ++c)
            {
                band[c] = deessHp[c].process(x[c]);
                low[c] = x[c] - band[c];
                det = std::max(det, std::abs(band[c]));
            }
            deessEnv = det > deessEnv ? envAtk * deessEnv + (1.f - envAtk) * det
                                      : envRel * deessEnv + (1.f - envRel) * det;
            const float envDb = juce::Decibels::gainToDecibels(deessEnv, -100.f);
            const float red = std::min(12.f, std::max(0.f, envDb - deThr) * 0.7f);
            const float dg = juce::Decibels::decibelsToGain(-red);
            for (int c = 0; c < nch; ++c)
                x[c] = low[c] + band[c] * dg;
        }

        // --- Harmonic saturation ---
        if (satMix > 0.005f)
            for (int c = 0; c < nch; ++c)
                x[c] += satMix * (std::tanh(satDrive * x[c]) / satDrive - x[c]);

        ch0[i] = x[0];
        if (ch1) ch1[i] = x[1];
    }

    upMax(meterGR, grMax);

    // =========================== Delay + Reverb ===========================
    const float tone01 = delayTone * 0.01f;
    const float dAmt = delayAmt * 0.01f;
    const float dLevel = dAmt * 0.55f * (0.6f + 0.4f * fx * 0.01f);
    const bool dlyOn = dLevel > 0.002f;

    if (dlyOn)
    {
        if (dlyOut.getNumSamples() < n || dlyOut.getNumChannels() < 2)
            dlyOut.setSize(2, n, false, false, true);

        // tempo-synced dotted eighth (falls back to 120 bpm)
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
        {
            auto pos = ph->getPosition();
            if (pos.hasValue())
            {
                auto b = pos->getBpm();
                if (b.hasValue() && *b > 20.0) bpm = *b;
            }
        }
        const int dlySize = (int) dlyBuf[0].size();
        const int dlySamples = juce::jlimit(64, dlySize - 2, (int) (sr * 60.0 / bpm * 0.75));
        const float fb = 0.28f + 0.22f * (1.f - tone01);
        const float fc = 1200.f * std::pow(8.f, tone01);        // 1.2 kHz .. 9.6 kHz
        const float a = 1.f - std::exp(-2.f * 3.14159265f * fc / (float) sr);

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

            dlyLp[0] += a * ((mono + fb * tR) - dlyLp[0]);
            dlyLp[1] += a * ((fb * tL) - dlyLp[1]);
            dlyBuf[0][(size_t) dlyWp] = dlyLp[0];
            dlyBuf[1][(size_t) dlyWp] = dlyLp[1];
            if (++dlyWp >= dlySize) dlyWp = 0;

            dl[i] = tL * dLevel;
            dr[i] = tR * dLevel;
        }
    }

    // reverb: FX macro sets the level; a dark Delay Tone also blooms the echoes into the reverb
    const float revMix = fx * 0.004f;                                     // 0 .. 0.4
    const float revGain = revMix + (dlyOn ? 0.12f * (1.f - tone01) * dAmt : 0.f);

    const float* wl = nullptr;
    const float* wr = nullptr;
    if (revGain > 0.003f)
    {
        if (reverbBuf.getNumSamples() < n || reverbBuf.getNumChannels() < 2)
            reverbBuf.setSize(2, n, false, false, true);

        const auto& rt = kRev[revType];
        const float feed = dlyOn ? 1.2f * (1.f - tone01) : 0.f;
        const int preSize = (int) preBuf[0].size();
        const int preSamples = juce::jlimit(0, preSize - 2, (int) (sr * rt.preMs * 0.001));
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

            rl[i] = revHp[0].process(preBuf[0][(size_t) rp]);
            rr[i] = revHp[1].process(preBuf[1][(size_t) rp]);
        }

        juce::Reverb::Parameters rp;
        rp.roomSize = rt.sizeLo + (rt.sizeHi - rt.sizeLo) * decay * 0.01f;
        rp.damping = juce::jlimit(0.f, 1.f, rt.damp + (50.f - revTone) * 0.008f);
        rp.wetLevel = 0.33f;
        rp.dryLevel = 0.f;
        rp.width = 1.f;
        rp.freezeMode = 0.f;
        reverb.setParameters(rp);
        reverb.processStereo(rl, rr, n);

        // wet tone: tilt the top end of the reverb with Reverb Tone
        if (revTone != lastRevTone)
        {
            lastRevTone = revTone;
            const auto hs = ak::makeBiquad(ak::BQ::HighShelf, sr, 3500.0, 0.7, (double) ((revTone - 50.f) * 0.12f));
            wetHs[0].c = hs;
            wetHs[1].c = hs;
        }
        for (int i = 0; i < n; ++i)
        {
            rl[i] = wetHs[0].process(rl[i]) * revGain;
            rr[i] = wetHs[1].process(rr[i]) * revGain;
        }
        wl = rl;
        wr = rr;
    }

    // mix wet signals into the output
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

    // ---- Output gain ----
    const float outG = juce::Decibels::decibelsToGain(outDb);
    buffer.applyGainRamp(0, n, lastOutGain, outG);
    lastOutGain = outG;

    // ---- Output meters, stereo correlation, scope feed ----
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
int AudiokingsVocalsProcessor::getCurrentProgram()
{
    return juce::jlimit(0, ak::numPresets() - 1, (int) apvts.state.getProperty("preset", 0));
}

void AudiokingsVocalsProcessor::setCurrentProgram(int index)
{
    index = juce::jlimit(0, ak::numPresets() - 1, index);
    const auto& p = ak::presetAt(index);
    for (int i = 0; i < ak::kNumParams; ++i)
    {
        if (auto* prm = apvts.getParameter(ak::kParamIds[i]))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost(prm->convertTo0to1(p.v[i]));
            prm->endChangeGesture();
        }
    }
    apvts.state.setProperty("preset", index, nullptr);
}

const juce::String AudiokingsVocalsProcessor::getProgramName(int index)
{
    return juce::String(ak::presetAt(juce::jlimit(0, ak::numPresets() - 1, index)).name);
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
