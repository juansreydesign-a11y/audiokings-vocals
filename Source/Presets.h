#pragma once
#include <vector>

namespace ak
{
constexpr int kNumParams = 24;

// Index of every parameter (the preset rows below follow exactly this order)
enum PIdx
{
    Sens = 0, Compress, Bass, Treble, Air, Fx,
    RevType, RevTone, Decay, Delay, DelayTone,
    AirLow, AirHigh, Output,
    Hpf, Attack, Release, DeEss, Drive,
    Mud, BassF, MudF, TrebF, AirF
};

inline const char* const kParamIds[kNumParams] = {
    "sens",      // 0  Level / Sens: input gain + preamp drive (0..100, 50 = 0 dB)
    "compress",  // 1  compression amount %
    "bass",      // 2  dB  low shelf gain
    "treble",    // 3  dB  presence bell gain
    "air",       // 4  dB  high shelf gain
    "fx",        // 5  %   macro: reverb + delay + harmonic saturation + brightness
    "revtype",   // 6  0 Room, 1 Plate, 2 Hall
    "revtone",   // 7  % (50 = neutral)
    "decay",     // 8  % reverb decay
    "delay",     // 9  % echo amount
    "delaytone", // 10 % dark/washed  <->  bright/crisp
    "airlow",    // 11 % harmonic exciter, mid-high band
    "airhigh",   // 12 % harmonic exciter, high band
    "output",    // 13 dB
    "hpf",       // 14 Hz
    "attack",    // 15 ms
    "release",   // 16 ms
    "deess",     // 17 %
    "drive",     // 18 % base harmonic saturation
    "mud",       // 19 dB  low-mid bell gain (EQ node)
    "bassf",     // 20 Hz  bass node frequency
    "mudf",      // 21 Hz  low-mid node frequency
    "trebf",     // 22 Hz  treble node frequency
    "airf"       // 23 Hz  air node frequency
};

struct Preset
{
    const char* name;
    float v[kNumParams];
};

struct PresetSection
{
    const char* name;
    std::vector<Preset> presets;
};

inline const std::vector<PresetSection>& sections()
{
    // sens cmp  bass  treb  air   fx  rTyp rTone dec dly dTone aLo aHi  out  hpf att  rel de drv   mud  bassF mudF trebF airF
    static const std::vector<PresetSection> s = {
        { "GENERAL - POP VOCALS", {
            { "Pop Voice",      { 50, 45, -1.0f, 2.5f, 3.0f, 35, 1, 50, 45, 10, 60, 20, 25,  0,  90, 12, 150, 35,  5, -2.0f, 120, 350, 3500, 10000 } },
            { "Pop Bright",     { 50, 50, -2.0f, 3.5f, 4.5f, 30, 1, 60, 40,  8, 65, 25, 40,  0, 100,  8, 120, 45,  3, -2.5f, 120, 350, 3800, 11000 } },
            { "Pop Warm",       { 50, 45,  1.5f, 1.5f, 1.5f, 35, 0, 40, 45, 10, 40, 15, 15,  0,  80, 14, 180, 30, 10,  0.0f, 120, 350, 3000,  9000 } },
            { "Pop Powerful",   { 52, 65, -0.5f, 3.0f, 3.0f, 32, 1, 50, 40,  8, 60, 25, 30, -1,  95,  6, 110, 40,  8, -1.5f, 120, 350, 3500, 10000 } },
            { "Pop Airy",       { 48, 40, -2.0f, 2.0f, 5.5f, 60, 2, 60, 70, 25, 45, 20, 55,  0, 110, 15, 200, 45,  0, -2.0f, 120, 350, 3500, 12000 } },
            { "Pop Intimate",   { 50, 35,  1.0f, 1.5f, 2.5f, 16, 0, 45, 25,  5, 55, 10, 20,  0,  85, 18, 220, 30,  8, -1.0f, 120, 350, 3200, 10000 } },
            { "Backing Vocals", { 50, 55, -2.0f, 0.5f, 2.0f, 55, 2, 45, 60, 20, 40, 10, 20,  0, 130, 10, 140, 40,  0, -1.0f, 120, 350, 3000,  9000 } },
        } },
        { "AUDIOKINGS - RAP", {
            // Boom bap downtempo, upfront voice, long reverb
            { "K1 Krespo",      { 55, 55,  1.5f, 3.0f, 2.0f, 60, 2, 45, 85, 25, 35, 20, 15,  0,  80, 15, 220, 30,  5, -1.5f, 110, 350, 3200, 10000 } },
            // 90s classic, dark and gritty (Mobb Deep style)
            { "Ponze",          { 50, 60,  2.0f, 1.5f,-1.0f, 20, 0, 35, 25,  8, 30, 10,  5,  0, 100, 20, 180, 20, 30,  1.0f, 120, 400, 3000,  8000 } },
            // Modern double-time, fast and compressed
            { "Splash",         { 55, 78, -1.5f, 3.0f, 4.0f, 25, 1, 55, 35, 10, 70, 20, 40, -1, 100,  3,  80, 50, 12, -2.0f, 120, 350, 3800, 11000 } },
            // 2000s style, compressed + bright, short reverb
            { "Oral MCMP",      { 52, 70,  0.0f, 4.0f, 5.0f, 30, 0, 55, 18, 12, 65, 25, 45, -1,  95,  6, 100, 40, 15, -1.0f, 120, 350, 4000, 12000 } },
            // Aggressive rap
            { "Terko",          { 58, 85,  1.0f, 4.5f, 2.5f, 25, 0, 50, 20,  5, 55, 20, 35, -2, 100,  2,  70, 30, 55,  0.0f, 120, 350, 3800, 10000 } },
            // Melodic R&B / Soul
            { "JKing",          { 48, 40,  1.5f, 2.0f, 4.0f, 55, 2, 40, 60, 22, 45, 20, 40,  0,  90, 18, 250, 40,  5, -1.0f, 120, 350, 3200, 11000 } },
        } }
    };
    return s;
}

inline int numPresets()
{
    int n = 0;
    for (const auto& sec : sections()) n += (int) sec.presets.size();
    return n;
}

inline const Preset& presetAt(int index)
{
    for (const auto& sec : sections())
    {
        if (index < (int) sec.presets.size()) return sec.presets[(size_t) index];
        index -= (int) sec.presets.size();
    }
    return sections()[0].presets[0];
}
} // namespace ak
