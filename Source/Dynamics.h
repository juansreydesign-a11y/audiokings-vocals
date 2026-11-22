#pragma once
#include <algorithm>
#include <cmath>

// Shared dynamics maths: used by the audio engine and by the graphs in the editor,
// so the curves you see are exactly the curves that are being applied.
namespace ak
{
inline float smooth01(float x)
{
    x = std::min(1.f, std::max(0.f, x));
    return x * x * (3.f - 2.f * x);
}

// FX macro: first half = pop effects, second half = creative effects
inline float fxT(float fx) { return smooth01(fx / 50.f); }
inline float fxU(float fx) { return smooth01((fx - 50.f) / 50.f); }

// 1176 (FET) static curve: gain reduction in dB (>= 0) for an input level in dB
inline float comp1176Reduction(float lvlDb, float thr, float ratio, float knee)
{
    const float over = lvlDb - thr;
    float outL = lvlDb;
    if (2.f * over > knee)
        outL = thr + over / ratio;
    else if (2.f * over > -knee)
    {
        const float t = over + knee * 0.5f;
        outL = lvlDb + (1.f / ratio - 1.f) * t * t / (2.f * knee);
    }
    return lvlDb - outL;
}

// Fairchild 670 (variable-mu): soft onset, ratio increasing with level
inline float vmReduction(float over)
{
    return over > 0.f ? 0.85f * over * over / (over + 20.f) : 0.f;
}
inline float comp670Reduction(float lvlDb, float thr) { return vmReduction(lvlDb - thr + 6.f); }

// Pultec treble bandwidth knob (0..100) -> Q of the boost bell
inline float pultecQ(float bw) { return 1.4f * std::pow(0.25f, bw * 0.01f); }
} // namespace ak
