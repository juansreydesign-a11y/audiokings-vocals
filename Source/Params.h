#pragma once

namespace ak
{
enum Kind { KFloat = 0, KChoice, KBool };

struct ParamDef
{
    const char* id;
    const char* name;
    float lo, hi, step, centre, def;   // centre > 0 -> skew so that value sits in the middle of the knob
    const char* unit;
    int decimals;
    Kind kind;
    const char* choices;               // "a|b|c" for KChoice
};

// One table for everything: layout, presets (by id) and UI attachments.
inline const ParamDef kParams[] = {
    // ---- input / noise ----
    { "sens",      "Level / Sens",       0,  100, 0.1f,   0,   50, " %",  0, KFloat,  "" },
    { "noise",     "Noise Reduction",    0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    // ---- compression: master + 1176 + Fairchild 670 ----
    { "compress",  "Compress",           0,  100, 0.1f,   0,   40, " %",  0, KFloat,  "" },
    { "w1176",     "1176 Amount",        0,  100, 0.1f,   0,  100, " %",  0, KFloat,  "" },
    { "w670",      "670 Amount",         0,  100, 0.1f,   0,   60, " %",  0, KFloat,  "" },
    { "ratio1176", "1176 Ratio",         0,    4, 1,      0,    0, "",    0, KChoice, "4:1|8:1|12:1|20:1|ALL" },
    { "tc670",     "670 Time Constant",  0,    5, 1,      0,    2, "",    0, KChoice, "1|2|3|4|5|6" },
    // ---- Pultec EQ ----
    { "pbass",     "Pultec Bass",        0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    { "pbassatt",  "Pultec Bass Atten",  0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    { "pbassf",    "Pultec Bass Freq",  25,  220, 1,     80,   80, " Hz", 0, KFloat,  "" },
    { "ptreb",     "Pultec Treble",      0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    { "ptrebatt",  "Pultec Treble Atten",0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    { "ptrebf",    "Pultec Treble Freq",4000,20000, 10, 10000, 10000, " Hz", 0, KFloat, "" },
    // ---- graphic (live response) EQ: gains ----
    { "eqlow",     "EQ Low",           -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    { "eqlowmid",  "EQ Low-Mid",       -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    { "eqhimid",   "EQ High-Mid",      -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    { "eqpres",    "EQ Presence",      -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    { "eqair",     "EQ Air",           -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    // ---- graphic EQ: frequencies ----
    { "eqlowf",    "EQ Low Freq",       40,  400, 1,    100,  100, " Hz", 0, KFloat,  "" },
    { "eqlowmidf", "EQ Low-Mid Freq",  150, 1000, 1,    350,  350, " Hz", 0, KFloat,  "" },
    { "eqhimidf",  "EQ High-Mid Freq", 800, 4000, 1,   2000, 2000, " Hz", 0, KFloat,  "" },
    { "eqpresf",   "EQ Presence Freq",3000,10000, 1,   5000, 5000, " Hz", 0, KFloat,  "" },
    { "eqairf",    "EQ Air Freq",     6000,18000, 1,  11000,11000, " Hz", 0, KFloat,  "" },
    // ---- low-pass filter ----
    { "lpon",      "Low-Pass On",        0,    1, 1,      0,    0, "",    0, KBool,   "" },
    { "lpf",       "Low-Pass Freq",   3000,20000, 10, 10000,16000, " Hz", 0, KFloat,  "" },
    // ---- Air exciter (direct on the voice) ----
    { "airlow",    "Air Low",            0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    { "airhigh",   "Air High",           0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    // ---- FX macro ----
    { "fx",        "FX",                 0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    // ---- reverb (parallel) ----
    { "revtype",   "Reverb Type",        0,    2, 1,      0,    1, "",    0, KChoice, "Room|Plate|Hall" },
    { "revtone",   "Reverb Tone",        0,  100, 0.1f,   0,   50, " %",  0, KFloat,  "" },
    { "decay",     "Decay",              0,  100, 0.1f,   0,   40, " %",  0, KFloat,  "" },
    { "revmix",    "Reverb Mix",         0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    // ---- delay (parallel) ----
    { "delaydiv",  "Delay Time",         0,    3, 1,      0,    1, "",    0, KChoice, "1/4|1/8 dotted|1/8|1/16" },
    { "delay",     "Delay",              0,  100, 0.1f,   0,   30, " %",  0, KFloat,  "" },
    { "delaytone", "Delay Tone",         0,  100, 0.1f,   0,   55, " %",  0, KFloat,  "" },
    { "delaymix",  "Delay Mix",          0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" },
    // ---- output ----
    { "output",    "Output",           -12,   12, 0.1f,   0,    0, " dB", 1, KFloat,  "" },
    // ---- preset-driven fine settings ----
    { "hpf",       "High-Pass",         20,  300, 1,     90,   90, " Hz", 0, KFloat,  "" },
    { "attack",    "1176 Attack",     0.02f, 0.8f, 0.01f, 0.25f, 0.3f, " ms", 2, KFloat, "" },
    { "release",   "1176 Release",      50, 1100, 1,    300,  250, " ms", 0, KFloat,  "" },
    { "deess",     "De-Esser",           0,  100, 0.1f,   0,    0, " %",  0, KFloat,  "" }
};

constexpr int kNumParams = (int) (sizeof(kParams) / sizeof(kParams[0]));
} // namespace ak
