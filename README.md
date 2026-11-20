# Audiokings Vocals

Vocal channel strip plugin (VST3, Windows x64) for **FL Studio** and **Ableton Live**, built with [JUCE](https://juce.com).

## Interface

**EQ · live response** – spectrum + EQ curve with 4 draggable nodes (Bass, Low-Mid, Treble, Air). Drag a node to change its gain (up/down) and frequency (left/right). Double-click a node to reset it.

**Meters** – input / compression / output meters, real-time **vectorscope** with stereo correlation, and a **gain-reduction meter** with numeric read-out (dB) and a scrolling history graph.

**Dynamics** – big **Level / Sens** knob (input gain + preamp sensitivity) with a green/yellow/red LED meter and an OPTIMAL input zone, plus **Compress, Bass, Treble, Air** and **FX** (macro: reverb + delay + harmonic saturation + shine).

**Tone & Space** – 3 reverb types (**Room / Plate / Hall**), **Reverb Tone**, **Decay**, **Delay** (tempo-synced dotted 1/8), **Delay Tone** (dark and washed into the reverb <-> bright and crisp), **Air Low / Air High** (harmonic exciter, Fresh-Air style) and **Output**.

## Presets

| Section | Presets |
|---|---|
| GENERAL - POP VOCALS | Pop Voice (base), Pop Bright, Pop Warm, Pop Powerful, Pop Airy, Pop Intimate, Backing Vocals |
| AUDIOKINGS - RAP | **K1 Krespo** · **Ponze** · **Splash** · **Oral MCMP** · **Terko** · **JKing** |

Presets are also exposed to the host as programs. Edit them in `Source/Presets.h`.

## Build (automatic)

Every push to `main` builds the plugin on GitHub Actions (Actions tab -> latest run -> Artifacts -> `AudiokingsVocals-Windows-VST3`).
Pushing a tag such as `v1.0.0` also publishes a GitHub Release with the zip.

## Build (local, Windows)

```
cmake -S . -B build -A x64
cmake --build build --config Release
```
Output: `build/AudiokingsVocals_artefacts/Release/VST3/Audiokings Vocals.vst3`

## Install

Run `Install-AudiokingsVocals.bat` as Administrator (copies to `C:\Program Files\Common Files\VST3`), then rescan plugins:
- **FL Studio** (20.8+): Options -> Manage plugins -> Find installed plugins
- **Ableton Live** (11+): Preferences -> Plug-ins -> enable "Use VST3 Plug-in System Folders" -> Rescan

## License note

JUCE is AGPLv3 / commercial. Releasing this plugin commercially or closed-source requires a JUCE commercial license (and the VST3 SDK terms).
