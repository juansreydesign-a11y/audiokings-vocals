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
        .withStringFromValueFunction([unit, decimals](float v, int) { return juce::String(v, decimals) + unit; });

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ak::kParamIds[idx], 1 }, name, range, ak::presetAt(0).v[idx], attrs));
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    addFloat(l,  0, "Input",         -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    addFloat(l,  1, "High-Pass",      20.f, 300.f, 1.0f, 90.f,   " Hz",  0);
    addFloat(l,  2, "Compression",     0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l,  3, "Attack",          0.3f, 60.f, 0.1f,  8.f,   " ms",  1);
    addFloat(l,  4, "Release",        20.f, 800.f, 1.0f, 150.f,  " ms",  0);
    addFloat(l,  5, "De-Esser",        0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l,  6, "Low",           -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    addFloat(l,  7, "Low-Mid",       -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    addFloat(l,  8, "Presence",      -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    addFloat(l,  9, "Air",           -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    addFloat(l, 10, "Drive",           0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l, 11, "Reverb Mix",      0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l, 12, "Reverb Size",     0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l, 13, "Reverb Damp",     0.f, 100.f, 0.1f,  0.f,   " %",   0);
    addFloat(l, 14, "Stereo Width",    0.f, 200.f, 1.0f,  0.f,   " %",   0);
    addFloat(l, 15, "Output",        -12.f,  12.f, 0.1f,  0.f,   " dB",  1);
    return l;
}

inline void upMax(std::atomic<float>& a, float v)
{
    if (v > a.load(std::memory_order_relaxed))
        a.store(v, std::memory_order_relaxed);
}
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

    eqValid = false;
    for (int c = 0; c < 2; ++c)
    {
        hpf[c].reset(); lowShelf[c].reset(); lowMid[c].reset(); pres[c].reset(); air[c].reset(); deessHp[c].reset();
        deessHp[c].c = ak::makeBiquad(ak::BQ::HighPass, sr, 5500.0, 0.707, 0.0);
    }

    grState = 0.f;
    makeupCur = 0.f;
    deessEnv = 0.f;
    lastInGain  = juce::Decibels::decibelsToGain(paramPtr[0]->load());
    lastOutGain = juce::Decibels::decibelsToGain(paramPtr[15]->load());
}

void AudiokingsVocalsProcessor::updateFilters(float hp, float lo, float lm, float pr, float ai)
{
    const std::array<float, 5> now { hp, lo, lm, pr, ai };
    if (eqValid && now == lastEq)
        return;

    lastEq = now;
    eqValid = true;
    const auto e = ak::makeEq(sr, hp, lo, lm, pr, ai);
    for (int c = 0; c < 2; ++c)
    {
        hpf[c].c = e.hp;
        lowShelf[c].c = e.lo;
        lowMid[c].c = e.lm;
        pres[c].c = e.pr;
        air[c].c = e.ai;
    }
}

void AudiokingsVocalsProcessor::copyScope(float* dest) const
{
    const int w = scopePos.load(std::memory_order_relaxed);
    for (int i = 0; i < scopeSize; ++i)
        dest[i] = scope[(size_t) ((w + i) & (scopeSize - 1))];
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
    const float inDb = P(0), hpfHz = P(1), comp = P(2), attMs = P(3), relMs = P(4), deess = P(5);
    const float lowG = P(6), lowMidG = P(7), presG = P(8), airG = P(9);
    const float drive = P(10), revMix = P(11), revSize = P(12), revDamp = P(13);
    const float width = P(14) * 0.01f, outDb = P(15);

    updateFilters(hpfHz, lowG, lowMidG, presG, airG);

    float* ch0 = buffer.getWritePointer(0);
    float* ch1 = nch > 1 ? buffer.getWritePointer(1) : nullptr;

    // ---- Input gain + input meter ----
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

    // ---- Compressor settings ----
    const bool compOn = comp > 0.5f;
    const float thr = -6.f - 0.30f * comp;               // -6 .. -36 dB
    const float ratio = 1.5f + 0.065f * comp;            // 1.5 .. 8
    const float knee = 6.f;
    const float makeupTarget = compOn ? 0.35f * (-thr) * (1.f - 1.f / ratio) : 0.f;
    const double attC = std::exp(-1.0 / (sr * (double) juce::jmax(0.05f, attMs) * 0.001));
    const double relC = std::exp(-1.0 / (sr * (double) juce::jmax(1.f, relMs) * 0.001));

    // ---- De-esser settings ----
    const bool deOn = deess > 0.5f;
    const float deThr = -16.f - 0.2f * deess;            // -16 .. -36 dB
    const float envAtk = (float) std::exp(-1.0 / (sr * 0.0005));
    const float envRel = (float) std::exp(-1.0 / (sr * 0.04));

    // ---- Saturation settings ----
    const float satMix = drive * 0.01f;
    const float satDrive = 1.f + drive * 0.045f;

    float grMax = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { ch0[i], ch1 ? ch1[i] : 0.f };

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

        // --- EQ ---
        for (int c = 0; c < nch; ++c)
        {
            x[c] = lowShelf[c].process(x[c]);
            x[c] = lowMid[c].process(x[c]);
            x[c] = pres[c].process(x[c]);
            x[c] = air[c].process(x[c]);
        }

        // --- De-esser (splits the high band and ducks only that band) ---
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

        // --- Saturation ---
        if (satMix > 0.005f)
            for (int c = 0; c < nch; ++c)
                x[c] += satMix * (std::tanh(satDrive * x[c]) / satDrive - x[c]);

        ch0[i] = x[0];
        if (ch1) ch1[i] = x[1];
    }

    upMax(meterGR, grMax);

    // ---- Reverb (parallel send) ----
    if (revMix > 0.5f)
    {
        if (reverbBuf.getNumSamples() < n || reverbBuf.getNumChannels() < 2)
            reverbBuf.setSize(2, n, false, false, true);

        reverbBuf.copyFrom(0, 0, ch0, n);
        reverbBuf.copyFrom(1, 0, ch1 ? ch1 : ch0, n);

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.25f + 0.0072f * revSize;
        rp.damping = revDamp * 0.01f;
        rp.wetLevel = 0.33f;
        rp.dryLevel = 0.f;
        rp.width = 1.f;
        rp.freezeMode = 0.f;
        reverb.setParameters(rp);
        reverb.processStereo(reverbBuf.getWritePointer(0), reverbBuf.getWritePointer(1), n);

        const float wet = revMix * 0.01f;
        const float* wl = reverbBuf.getReadPointer(0);
        const float* wr = reverbBuf.getReadPointer(1);
        if (ch1)
        {
            for (int i = 0; i < n; ++i) { ch0[i] += wl[i] * wet; ch1[i] += wr[i] * wet; }
        }
        else
        {
            for (int i = 0; i < n; ++i) ch0[i] += 0.5f * (wl[i] + wr[i]) * wet;
        }
    }

    // ---- Stereo width (mid/side) ----
    if (ch1 && std::abs(width - 1.f) > 0.002f)
    {
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (ch0[i] + ch1[i]);
            const float s = 0.5f * (ch0[i] - ch1[i]) * width;
            ch0[i] = m + s;
            ch1[i] = m - s;
        }
    }

    // ---- Output gain ----
    const float outG = juce::Decibels::decibelsToGain(outDb);
    buffer.applyGainRamp(0, n, lastOutGain, outG);
    lastOutGain = outG;

    // ---- Output meters, stereo correlation, spectrum feed ----
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
            scope[(size_t) (w & (scopeSize - 1))] = 0.5f * (l + r);
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
