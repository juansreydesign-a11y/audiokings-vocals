# Audiokings Vocals

Vocal channel strip plugin (VST3, Windows x64) for **FL Studio** and **Ableton Live**, built with [JUCE](https://juce.com).

**Signal chain:** Input → High-Pass → Compressor (one "amount" knob + attack/release, auto make-up) → 4-band EQ → De-Esser → Drive → Reverb → Stereo Width → Output

**UI:** animated modern interface with input / compression / output meters, stereo correlation + width meter, and a live EQ curve over a real-time spectrum.

## Presets

| Section | Presets |
|---|---|
| GENERAL - POP VOCALS | Pop Voice (base), Pop Bright, Pop Warm, Pop Powerful, Pop Airy, Pop Intimate, Backing Vocals |
| AUDIOKINGS - RAP | **K1 Krespo** (boom bap downtempo, upfront, long reverb) · **Ponze** (90s classic, Mobb Deep style) · **Splash** (modern double-time, compressed) · **Oral MCMP** (2000s, compressed + bright, short reverb) · **Terko** (aggressive) · **JKing** (melodic R&B / Soul) |

Presets are also exposed to the host as programs. Edit them in `Source/Presets.h`.

## Build (automatic)

Every push to `main` builds the plugin on GitHub Actions (Actions tab → latest run → Artifacts → `AudiokingsVocals-Windows-VST3`).
Pushing a tag such as `v1.0.0` also publishes a GitHub Release with the zip.

## Build (local, Windows)

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Output: `build/AudiokingsVocals_artefacts/Release/VST3/Audiokings Vocals.vst3`

## Install

Run `Install-AudiokingsVocals.bat` as Administrator (copies to `C:\Program Files\Common Files\VST3`), then rescan plugins:
- **FL Studio** (20.8+): Options → Manage plugins → Find installed plugins
- **Ableton Live** (11+): Preferences → Plug-ins → enable "Use VST3 Plug-in System Folders" → Rescan

## License note

JUCE is AGPLv3 / commercial. Releasing this plugin commercially or closed-source requires a JUCE commercial license (and the VST3 SDK terms).
