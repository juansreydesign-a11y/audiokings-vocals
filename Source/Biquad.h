#pragma once
#include <algorithm>
#include <cmath>

// Small RBJ-cookbook biquad used by the audio engine and by the EQ curve display.
namespace ak
{
struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

enum class BQ { LowShelf, HighShelf, Peak, HighPass, LowPass };

inline BiquadCoeffs makeBiquad(BQ type, double sr, double f0, double q, double gainDb)
{
    constexpr double pi = 3.14159265358979323846;
    f0 = std::min(std::max(f0, 10.0), sr * 0.45);
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * pi * f0 / sr;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double sqA = std::sqrt(A);
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (type)
    {
        case BQ::Peak:
            b0 = 1 + alpha * A;  b1 = -2 * cw;  b2 = 1 - alpha * A;
            a0 = 1 + alpha / A;  a1 = -2 * cw;  a2 = 1 - alpha / A;
            break;
        case BQ::LowShelf:
            b0 = A * ((A + 1) - (A - 1) * cw + 2 * sqA * alpha);
            b1 = 2 * A * ((A - 1) - (A + 1) * cw);
            b2 = A * ((A + 1) - (A - 1) * cw - 2 * sqA * alpha);
            a0 = (A + 1) + (A - 1) * cw + 2 * sqA * alpha;
            a1 = -2 * ((A - 1) + (A + 1) * cw);
            a2 = (A + 1) + (A - 1) * cw - 2 * sqA * alpha;
            break;
        case BQ::HighShelf:
            b0 = A * ((A + 1) + (A - 1) * cw + 2 * sqA * alpha);
            b1 = -2 * A * ((A - 1) + (A + 1) * cw);
            b2 = A * ((A + 1) + (A - 1) * cw - 2 * sqA * alpha);
            a0 = (A + 1) - (A - 1) * cw + 2 * sqA * alpha;
            a1 = 2 * ((A - 1) - (A + 1) * cw);
            a2 = (A + 1) - (A - 1) * cw - 2 * sqA * alpha;
            break;
        case BQ::HighPass:
            b0 = (1 + cw) / 2;  b1 = -(1 + cw);  b2 = (1 + cw) / 2;
            a0 = 1 + alpha;     a1 = -2 * cw;    a2 = 1 - alpha;
            break;
        case BQ::LowPass:
            b0 = (1 - cw) / 2;  b1 = 1 - cw;     b2 = (1 - cw) / 2;
            a0 = 1 + alpha;     a1 = -2 * cw;    a2 = 1 - alpha;
            break;
    }

    BiquadCoeffs c;
    c.b0 = b0 / a0;  c.b1 = b1 / a0;  c.b2 = b2 / a0;
    c.a1 = a1 / a0;  c.a2 = a2 / a0;
    return c;
}

struct Biquad
{
    BiquadCoeffs c;
    double z1 = 0.0, z2 = 0.0;

    inline float process(float x) noexcept
    {
        const double in = (double) x;
        const double out = c.b0 * in + z1;
        z1 = c.b1 * in - c.a1 * out + z2;
        z2 = c.b2 * in - c.a2 * out;
        return (float) out;
    }
    void reset() noexcept { z1 = z2 = 0.0; }
};

inline double magnitudeAt(const BiquadCoeffs& c, double f, double sr)
{
    constexpr double pi = 3.14159265358979323846;
    const double w = 2.0 * pi * f / sr;
    const double cw = std::cos(w), sw = std::sin(w);
    const double c2 = std::cos(2 * w), s2 = std::sin(2 * w);
    const double nr = c.b0 + c.b1 * cw + c.b2 * c2;
    const double ni = -(c.b1 * sw + c.b2 * s2);
    const double dr = 1.0 + c.a1 * cw + c.a2 * c2;
    const double di = -(c.a1 * sw + c.a2 * s2);
    return std::sqrt((nr * nr + ni * ni) / (dr * dr + di * di + 1.0e-30));
}

// ---------------------------------------------------------------------------
// Graphic ("live response") EQ: 5 movable nodes + high-pass + optional low-pass (24 dB/oct)
// node 0 = low shelf, 1 = low-mid bell, 2 = high-mid bell, 3 = presence bell, 4 = air shelf
struct EqParams
{
    float hpf;
    float g[5];
    float f[5];
    bool  lpOn;
    float lpf;
};

struct EqSet
{
    BiquadCoeffs hp, n[5], lp1, lp2;
    bool lpOn = false;
};

inline EqSet makeEq(double sr, const EqParams& p)
{
    EqSet e;
    e.hp   = makeBiquad(BQ::HighPass,  sr, p.hpf,  0.707, 0.0);
    e.n[0] = makeBiquad(BQ::LowShelf,  sr, p.f[0], 0.7,   p.g[0]);
    e.n[1] = makeBiquad(BQ::Peak,      sr, p.f[1], 0.9,   p.g[1]);
    e.n[2] = makeBiquad(BQ::Peak,      sr, p.f[2], 0.9,   p.g[2]);
    e.n[3] = makeBiquad(BQ::Peak,      sr, p.f[3], 0.8,   p.g[3]);
    e.n[4] = makeBiquad(BQ::HighShelf, sr, p.f[4], 0.7,   p.g[4]);
    e.lpOn = p.lpOn;
    e.lp1  = makeBiquad(BQ::LowPass,   sr, p.lpf,  0.5412, 0.0);   // 4th-order Butterworth
    e.lp2  = makeBiquad(BQ::LowPass,   sr, p.lpf,  1.3066, 0.0);
    return e;
}

// ---------------------------------------------------------------------------
// Pultec-style passive EQ (after the 1176): boost + attenuation at the same frequency
struct PultecParams
{
    float bass, bassAtt, bassF, treb, trebAtt, trebF;   // amounts 0..100, freqs in Hz
};

struct PultecSet
{
    BiquadCoeffs bBoost, bAtt, tBoost, tAtt;
};

inline PultecSet makePultec(double sr, const PultecParams& p)
{
    PultecSet s;
    s.bBoost = makeBiquad(BQ::LowShelf,  sr, p.bassF,        0.9,  p.bass    * 0.14);
    s.bAtt   = makeBiquad(BQ::LowShelf,  sr, p.bassF * 2.2,  0.5, -p.bassAtt * 0.14);
    s.tBoost = makeBiquad(BQ::Peak,      sr, p.trebF,        0.55, p.treb    * 0.14);
    s.tAtt   = makeBiquad(BQ::HighShelf, sr, p.trebF * 1.35, 0.6, -p.trebAtt * 0.14);
    return s;
}
} // namespace ak
