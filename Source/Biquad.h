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

enum class BQ { LowShelf, HighShelf, Peak, HighPass };

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

// The complete EQ section: high-pass + 4 movable bands
struct EqParams
{
    float hpf, bassG, bassF, mudG, mudF, trebG, trebF, airG, airF;
};

struct EqSet
{
    BiquadCoeffs hp, bass, mud, treb, air;
};

inline EqSet makeEq(double sr, const EqParams& p)
{
    EqSet e;
    e.hp   = makeBiquad(BQ::HighPass,  sr, p.hpf,   0.707, 0.0);
    e.bass = makeBiquad(BQ::LowShelf,  sr, p.bassF, 0.7,   p.bassG);
    e.mud  = makeBiquad(BQ::Peak,      sr, p.mudF,  0.9,   p.mudG);
    e.treb = makeBiquad(BQ::Peak,      sr, p.trebF, 0.8,   p.trebG);
    e.air  = makeBiquad(BQ::HighShelf, sr, p.airF,  0.7,   p.airG);
    return e;
}
} // namespace ak
