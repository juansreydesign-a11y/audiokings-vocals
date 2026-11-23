#pragma once
#include <array>
#include <cstring>
#include <utility>
#include <vector>
#include "Params.h"

namespace ak
{
using Values = std::array<float, kNumParams>;

inline int indexOfParam(const char* id)
{
    for (int i = 0; i < kNumParams; ++i)
        if (std::strcmp(kParams[i].id, id) == 0)
            return i;
    return -1;
}

inline Values defaultValues()
{
    Values v {};
    for (int i = 0; i < kNumParams; ++i)
        v[(size_t) i] = kParams[i].def;
    return v;
}

// A preset only lists what differs from the defaults (choices are indices: ratio1176 0..4, tc670 0..5,
// revtype 0..3 = Room/Plate/Hall/Studio
// delaydiv 0..4 = 1/2, 1/4, 1/8 dotted, 1/8, 1/16, delaydiv 0..3 = 1/4, 1/8 dotted, 1/8, 1/16).
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> set;
};

struct PresetSection
{
    const char* name;
    std::vector<Preset> presets;
};

inline const std::vector<PresetSection>& sections()
{
    static const std::vector<PresetSection> s = {
        { "GENERAL - POP VOCALS", {
            { "Pop Voice", { {"noise",10}, {"compress",45}, {"w670",60}, {"ratio1176",0}, {"tc670",1},
                {"pbass",15}, {"pbassatt",8}, {"pbassf",90}, {"ptreb",25}, {"ptrebatt",10}, {"ptrebf",12000},
                {"eqlowmid",-2}, {"eqpres",1.5f}, {"eqair",1}, {"airlow",20}, {"airhigh",25}, {"fx",25},
                {"revtype",1}, {"revtone",50}, {"decay",45}, {"revmix",14}, {"delaydiv",2}, {"delay",25}, {"delaytone",60}, {"delaymix",6},
                {"deess",35}, {"hpf",90}, {"attack",0.3f}, {"release",250} } },
            { "Pop Bright", { {"noise",10}, {"compress",50}, {"w670",50}, {"ratio1176",1}, {"tc670",1},
                {"pbass",5}, {"pbassatt",12}, {"pbassf",120}, {"ptreb",40}, {"ptrebatt",6}, {"ptrebf",12000},
                {"eqlowmid",-2.5f}, {"eqpres",2}, {"airlow",25}, {"airhigh",40}, {"fx",20},
                {"revtype",1}, {"revtone",60}, {"decay",40}, {"revmix",12}, {"delay",20}, {"delaytone",65}, {"delaymix",5},
                {"deess",45}, {"hpf",100}, {"attack",0.2f}, {"release",200} } },
            { "Pop Warm", { {"noise",10}, {"compress",45}, {"w670",70}, {"ratio1176",0}, {"tc670",2},
                {"pbass",35}, {"pbassatt",12}, {"pbassf",70}, {"ptreb",12}, {"ptrebatt",8}, {"ptrebf",8000},
                {"eqpres",1}, {"airlow",15}, {"airhigh",15}, {"fx",35},
                {"revtype",0}, {"revtone",40}, {"decay",45}, {"revmix",14}, {"delay",20}, {"delaytone",40}, {"delaymix",5},
                {"deess",30}, {"hpf",80}, {"attack",0.4f}, {"release",300} } },
            { "Pop Powerful", { {"sens",52}, {"noise",10}, {"compress",65}, {"w670",70}, {"ratio1176",1}, {"tc670",1},
                {"pbass",20}, {"pbassatt",8}, {"pbassf",100}, {"ptreb",30}, {"ptrebatt",10}, {"ptrebf",10000},
                {"eqlowmid",-1.5f}, {"eqhimid",1.5f}, {"airlow",25}, {"airhigh",30}, {"fx",30},
                {"revtype",1}, {"decay",40}, {"revmix",12}, {"delay",20}, {"delaytone",60}, {"delaymix",5},
                {"deess",40}, {"hpf",95}, {"attack",0.1f}, {"release",150}, {"output",-1} } },
            { "Pop Airy", { {"sens",48}, {"noise",10}, {"compress",40}, {"w670",60}, {"ratio1176",0}, {"tc670",2},
                {"pbassatt",15}, {"pbassf",100}, {"ptreb",45}, {"ptrebatt",5}, {"ptrebf",14000},
                {"eqlowmid",-2}, {"airlow",20}, {"airhigh",55}, {"fx",45},
                {"revtype",2}, {"revtone",60}, {"decay",70}, {"revmix",26}, {"delay",35}, {"delaytone",40}, {"delaymix",14},
                {"deess",45}, {"hpf",110}, {"attack",0.3f}, {"release",300} } },
            { "Pop Intimate", { {"noise",15}, {"compress",35}, {"w670",40}, {"ratio1176",0}, {"tc670",2},
                {"pbass",25}, {"pbassatt",5}, {"pbassf",80}, {"ptreb",15}, {"ptrebatt",5}, {"ptrebf",9000},
                {"eqlowmid",-1}, {"airlow",10}, {"airhigh",20}, {"fx",15},
                {"revtype",3}, {"revtone",45}, {"decay",25}, {"revmix",8}, {"delay",15}, {"delaymix",0},
                {"deess",30}, {"hpf",85}, {"attack",0.4f}, {"release",350} } },
            { "Backing Vocals", { {"noise",10}, {"compress",55}, {"w670",80}, {"ratio1176",1}, {"tc670",1},
                {"pbassatt",18}, {"pbassf",120}, {"ptreb",20}, {"ptrebatt",10}, {"ptrebf",9000},
                {"eqlowmid",-1}, {"airlow",10}, {"airhigh",20}, {"fx",40},
                {"revtype",2}, {"revtone",45}, {"decay",60}, {"revmix",24}, {"delay",25}, {"delaymix",10},
                {"deess",40}, {"hpf",130}, {"attack",0.3f}, {"release",250} } },
        } },
        { "AUDIOKINGS - RAP", {
            // Boom bap downtempo, upfront voice, long reverb
            // delay div updated: 0=1/2 1=1/4 2=1/8dot 3=1/8 4=1/16
        { "K1 Krespo", { {"sens",55}, {"noise",10}, {"compress",55}, {"w670",70}, {"ratio1176",1}, {"tc670",2},
                {"pbass",40}, {"pbassatt",10}, {"pbassf",70}, {"ptreb",25}, {"ptrebatt",8}, {"ptrebf",8000},
                {"eqlowmid",-1.5f}, {"eqhimid",2}, {"eqpres",1}, {"airlow",20}, {"airhigh",15}, {"fx",30},
                {"revtype",2}, {"revtone",45}, {"decay",85}, {"revmix",24}, {"delaydiv",1}, {"delay",30}, {"delaytone",35}, {"delaymix",8},
                {"deess",30}, {"hpf",80}, {"attack",0.3f}, {"release",350} } },
            // 90s classic, dark and gritty (Mobb Deep style)
            { "Ponze", { {"noise",5}, {"compress",60}, {"w670",80}, {"ratio1176",1}, {"tc670",3},
                {"pbass",45}, {"pbassatt",15}, {"pbassf",100}, {"ptrebatt",20}, {"ptrebf",8000},
                {"eqlowmid",1}, {"eqhimid",2}, {"eqair",-2}, {"lpon",1}, {"lpf",12000}, {"airlow",10}, {"airhigh",5}, {"fx",55},
                {"revtype",0}, {"revtone",35}, {"decay",25}, {"revmix",8}, {"delay",20}, {"delaytone",25}, {"delaymix",0},
                {"deess",20}, {"hpf",100}, {"attack",0.5f}, {"release",300} } },
            // Modern double-time, fast and compressed
            { "Splash", { {"sens",55}, {"noise",10}, {"compress",78}, {"w670",60}, {"ratio1176",2}, {"tc670",0},
                {"pbass",10}, {"pbassatt",15}, {"pbassf",100}, {"ptreb",35}, {"ptrebatt",8}, {"ptrebf",12000},
                {"eqlowmid",-2}, {"eqhimid",1}, {"eqpres",2}, {"airlow",20}, {"airhigh",40}, {"fx",30},
                {"revtype",1}, {"revtone",55}, {"decay",35}, {"revmix",10}, {"delaydiv",4}, {"delay",25}, {"delaytone",70}, {"delaymix",5},
                {"deess",50}, {"hpf",100}, {"attack",0.05f}, {"release",90}, {"output",-1} } },
            // 2000s style: compressed + bright, short reverb
            { "Oral MCMP", { {"sens",52}, {"noise",10}, {"compress",70}, {"w670",80}, {"ratio1176",2}, {"tc670",1},
                {"pbass",20}, {"pbassatt",8}, {"pbassf",100}, {"ptreb",45}, {"ptrebatt",5}, {"ptrebf",10000},
                {"eqlowmid",-1}, {"eqhimid",2}, {"eqpres",3}, {"airlow",25}, {"airhigh",45}, {"fx",35},
                {"revtype",3}, {"revtone",55}, {"decay",18}, {"revmix",12}, {"delay",20}, {"delaytone",65}, {"delaymix",4},
                {"deess",40}, {"hpf",95}, {"attack",0.1f}, {"release",120}, {"output",-1} } },
            // Aggressive rap
            { "Terko", { {"sens",58}, {"noise",15}, {"compress",85}, {"w670",80}, {"ratio1176",3}, {"tc670",0},
                {"pbass",15}, {"pbassatt",10}, {"pbassf",90}, {"ptreb",40}, {"ptrebatt",5}, {"ptrebf",8000},
                {"eqhimid",3}, {"eqpres",2}, {"airlow",25}, {"airhigh",30}, {"fx",65},
                {"revtype",0}, {"decay",20}, {"revmix",6}, {"delay",15}, {"delaymix",0},
                {"deess",30}, {"hpf",100}, {"attack",0.03f}, {"release",70}, {"output",-2} } },
            // Melodic R&B / Soul
            { "JKing", { {"sens",48}, {"noise",10}, {"compress",40}, {"w1176",70}, {"w670",80}, {"ratio1176",0}, {"tc670",2},
                {"pbass",30}, {"pbassatt",8}, {"pbassf",80}, {"ptreb",35}, {"ptrebatt",8}, {"ptrebf",12000},
                {"eqlowmid",-1}, {"airlow",20}, {"airhigh",40}, {"fx",40},
                {"revtype",1}, {"revtone",40}, {"decay",60}, {"revmix",22}, {"delay",30}, {"delaytone",45}, {"delaymix",10},
                {"deess",40}, {"hpf",90}, {"attack",0.4f}, {"release",400} } },
        } }
,
        { "MODERN HIP HOP - TRAP & DRILL", {
            // Trap/Drill: 808-friendly, airy, compressed, fxnocham=1 (no chamber), more chorus+flanger
            { "Trap Vibe",      { {"sens",54}, {"noise",15}, {"compress",72}, {"w1176",100}, {"w670",50}, {"ratio1176",2}, {"tc670",0},
                {"pbass",5}, {"pbassatt",20}, {"pbassf",120}, {"ptreb",35}, {"ptrebf",14000},
                {"eqlowmid",-3}, {"eqhimid",2}, {"eqpres",4}, {"eqair",5}, {"airlow",35}, {"airhigh",60}, {"fx",62},
                {"fxsat",80}, {"fxpc",90}, {"fxch",100}, {"fxdist",50}, {"fxfl",80}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",1}, {"revtone",62}, {"decay",28}, {"revmix",10}, {"delaydiv",3}, {"delay",35}, {"delaytone",72}, {"delaymix",10},
                {"deess",55}, {"deessf",7500}, {"deessr",65}, {"hpf",110}, {"attack",0.03f}, {"release",60}, {"output",-1} } },
            // Dark Drill UK
            { "Drill Dark",     { {"sens",56}, {"noise",12}, {"compress",80}, {"w1176",100}, {"w670",60}, {"ratio1176",3}, {"tc670",0},
                {"pbass",10}, {"pbassatt",25}, {"pbassf",100}, {"ptreb",20}, {"ptrebf",10000},
                {"eqlowmid",-4}, {"eqhimid",1}, {"eqpres",2}, {"eqair",2}, {"airlow",20}, {"airhigh",35}, {"fx",55},
                {"fxsat",70}, {"fxpc",100}, {"fxch",80}, {"fxdist",65}, {"fxfl",100}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",3}, {"revtone",40}, {"decay",22}, {"revmix",8}, {"delaydiv",4}, {"delay",30}, {"delaytone",50}, {"delaymix",6},
                {"deess",60}, {"deessf",7000}, {"deessr",70}, {"hpf",100}, {"attack",0.02f}, {"release",55}, {"output",-2} } },
            // Melodic Trap (emo, slow)
            { "Melodic Trap",   { {"sens",50}, {"noise",10}, {"compress",60}, {"w1176",80}, {"w670",80}, {"ratio1176",1}, {"tc670",2},
                {"pbass",25}, {"pbassatt",8}, {"pbassf",80}, {"ptreb",30}, {"ptrebf",12000},
                {"eqlowmid",-2}, {"airlow",25}, {"airhigh",45}, {"fx",52},
                {"fxsat",90}, {"fxpc",80}, {"fxch",100}, {"fxdist",20}, {"fxfl",60}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",1}, {"revtone",55}, {"decay",60}, {"revmix",22}, {"delaydiv",2}, {"delay",35}, {"delaytone",50}, {"delaymix",14},
                {"deess",45}, {"deessf",7500}, {"deessr",60}, {"hpf",90}, {"attack",0.1f}, {"release",200} } },
            // Autotune-ready (lots of presence and compression for heavy pitch correction)
            { "Autotune Ready", { {"sens",53}, {"noise",10}, {"compress",75}, {"w1176",100}, {"w670",60}, {"ratio1176",2}, {"tc670",0},
                {"pbass",8}, {"pbassatt",18}, {"pbassf",100}, {"ptreb",40}, {"ptrebf",13000},
                {"eqlowmid",-3}, {"eqhimid",3}, {"eqpres",5}, {"airlow",30}, {"airhigh",60}, {"fx",60},
                {"fxsat",75}, {"fxpc",100}, {"fxch",90}, {"fxdist",40}, {"fxfl",70}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",1}, {"revtone",60}, {"decay",25}, {"revmix",8}, {"delaydiv",3}, {"delay",25}, {"delaytone",70}, {"delaymix",8},
                {"deess",55}, {"deessf",7500}, {"deessr",65}, {"hpf",105}, {"attack",0.03f}, {"release",70}, {"output",-1} } },
            // Hard hitting, ad-lib heavy
            { "Ad-Lib Bang",    { {"sens",57}, {"noise",15}, {"compress",85}, {"w1176",100}, {"w670",50}, {"ratio1176",3}, {"tc670",0},
                {"pbass",5}, {"pbassatt",22}, {"pbassf",100}, {"ptreb",30}, {"ptrebf",12000},
                {"eqlowmid",-3}, {"eqhimid",3}, {"eqpres",4}, {"airlow",25}, {"airhigh",50}, {"fx",65},
                {"fxsat",80}, {"fxpc",100}, {"fxch",80}, {"fxdist",75}, {"fxfl",100}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",0}, {"revtone",55}, {"decay",18}, {"revmix",6}, {"delaydiv",4}, {"delay",20}, {"delaytone",65}, {"delaymix",0},
                {"deess",65}, {"deessf",8000}, {"deessr",70}, {"hpf",110}, {"attack",0.02f}, {"release",50}, {"output",-2} } },
            // Soft, intimate trap ballad
            { "Trap Ballad",    { {"sens",48}, {"noise",10}, {"compress",50}, {"w1176",70}, {"w670",90}, {"ratio1176",0}, {"tc670",3},
                {"pbass",30}, {"pbassatt",8}, {"pbassf",80}, {"ptreb",25}, {"ptrebf",11000},
                {"eqlowmid",-1}, {"airlow",20}, {"airhigh",40}, {"fx",55},
                {"fxsat",90}, {"fxpc",70}, {"fxch",100}, {"fxdist",10}, {"fxfl",50}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",1}, {"revtone",48}, {"decay",55}, {"revmix",22}, {"delaydiv",0}, {"delay",25}, {"delaytone",40}, {"delaymix",12},
                {"deess",40}, {"deessf",7000}, {"deessr",55}, {"hpf",85}, {"attack",0.3f}, {"release",350} } },
            // E100 Essyboy: pop/rnb commercial, bright, melodic, heavily compressed
            { "E100 Essyboy",   { {"sens",52}, {"noise",10}, {"compress",78}, {"w1176",100}, {"w670",100}, {"ratio1176",2}, {"tc670",1},
                {"pbass",15}, {"pbassatt",10}, {"pbassf",100}, {"ptreb",55}, {"ptrebatt",8}, {"ptrebf",14000},
                {"eqlowmid",-2}, {"eqhimid",2}, {"eqpres",4}, {"eqair",4}, {"airlow",40}, {"airhigh",75}, {"fx",58},
                {"fxsat",90}, {"fxpc",100}, {"fxch",100}, {"fxdist",30}, {"fxfl",60}, {"fxcham",0}, {"fxnocham",1},
                {"revtype",1}, {"revtone",62}, {"decay",50}, {"revmix",20}, {"delaydiv",2}, {"delay",30}, {"delaytone",62}, {"delaymix",10},
                {"deess",50}, {"deessf",7500}, {"deessr",60}, {"hpf",95}, {"attack",0.1f}, {"release",150} } },
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

inline Values resolve(const Preset& p)
{
    Values v = defaultValues();
    for (const auto& kv : p.set)
    {
        const int i = indexOfParam(kv.first);
        if (i >= 0) v[(size_t) i] = kv.second;
    }
    return v;
}
} // namespace ak
