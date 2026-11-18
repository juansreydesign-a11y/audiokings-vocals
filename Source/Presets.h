#pragma once
#include <vector>

namespace ak
{
constexpr int kNumParams = 16;

// Order matters: every preset below lists its values in exactly this order.
inline const char* const kParamIds[kNumParams] = {
    "input",   // 0  dB      -12..12
    "hpf",     // 1  Hz       20..300
    "comp",    // 2  %         0..100  (compression amount)
    "attack",  // 3  ms      0.3..60
    "release", // 4  ms       20..800
    "deess",   // 5  %         0..100
    "low",     // 6  dB      -12..12  (low shelf 120 Hz)
    "lowmid",  // 7  dB      -12..12  (bell 350 Hz)
    "presence",// 8  dB      -12..12  (bell 3.5 kHz)
    "air",     // 9  dB      -12..12  (high shelf 10 kHz)
    "drive",   // 10 %         0..100  (saturation)
    "revmix",  // 11 %         0..100
    "revsize", // 12 %         0..100
    "revdamp", // 13 %         0..100
    "width",   // 14 %         0..200
    "output"   // 15 dB      -12..12
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
    //                     in   hpf  comp  att  rel  de   low  lmid pres air  drv  rMix rSz  rDmp wid  out
    static const std::vector<PresetSection> s = {
        { "GENERAL - POP VOCALS", {
            { "Pop Voice",      {  0, 90,  45, 12, 150, 35, -1.0f, -2.0f, 2.5f, 3.0f, 10, 14, 40, 50, 110,  0 } },
            { "Pop Bright",     {  0, 100, 50,  8, 120, 45, -2.0f, -2.5f, 3.5f, 4.5f,  8, 12, 38, 40, 115,  0 } },
            { "Pop Warm",       {  0, 80,  45, 14, 180, 30,  1.5f,  0.0f, 1.5f, 1.5f, 18, 14, 45, 65, 105,  0 } },
            { "Pop Powerful",   {  0, 95,  65,  6, 110, 40, -0.5f, -1.5f, 3.0f, 3.0f, 15, 12, 40, 50, 110, -1 } },
            { "Pop Airy",       {  0, 110, 40, 15, 200, 45, -2.0f, -2.0f, 2.0f, 5.5f,  6, 28, 70, 35, 130,  0 } },
            { "Pop Intimate",   {  0, 85,  35, 18, 220, 30,  1.0f, -1.0f, 1.5f, 2.5f, 12,  6, 25, 60, 100,  0 } },
            { "Backing Vocals", {  0, 130, 55, 10, 140, 40, -2.0f, -1.0f, 0.5f, 2.0f,  6, 24, 60, 55, 150,  0 } },
        } },
        { "AUDIOKINGS - RAP", {
            // Boom bap downtempo, upfront voice, long reverb
            { "K1 Krespo",      {  0, 80,  55, 15, 220, 30,  1.5f, -1.5f, 3.0f, 2.0f, 20, 24, 85, 55, 100,  0 } },
            // 90s classic, dark and gritty (Mobb Deep style)
            { "Ponze",          {  0, 100, 60, 20, 180, 20,  2.0f,  1.0f, 1.5f,-1.0f, 35,  8, 25, 70,  90,  0 } },
            // Modern double-time, fast and compressed
            { "Splash",         {  0, 100, 78,  3,  80, 50, -1.5f, -2.0f, 3.0f, 4.0f, 15, 10, 35, 40, 115, -1 } },
            // 2000s style, compressed + bright, short reverb
            { "Oral MCMP",      {  0, 95,  70,  6, 100, 40,  0.0f, -1.0f, 4.0f, 5.0f, 22, 12, 18, 45, 105, -1 } },
            // Aggressive rap
            { "Terko",          {  0, 100, 85,  2,  70, 30,  1.0f,  0.0f, 4.5f, 2.5f, 55,  6, 22, 60, 100, -2 } },
            // Melodic R&B / Soul
            { "JKing",          {  0, 90,  40, 18, 250, 40,  1.5f, -1.0f, 2.0f, 4.0f, 12, 22, 60, 40, 125,  0 } },
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
