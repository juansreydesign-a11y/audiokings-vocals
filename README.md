# Audiokings Vocals

Vocal channel strip (VST3, Windows x64) for **FL Studio**, **Ableton Live**, **Cubase** and **Reaper**, built with [JUCE](https://juce.com).
Compatibility is through the VST3 standard (FL Studio 20.8+, Live 11+, Cubase 11+, Reaper 6.20+). Presets are handled inside the plug-in, so no host re-applies them over your settings.

## Signal chain

`Level/Sens (preamp)` -> `Noise reduction` -> `1176 compressor` -> `Pultec EQ` -> `Fairchild 670 compressor`
-> `Graphic EQ (+ low-pass)` -> `Air exciter (direct)` -> `De-esser` -> `FX macro` -> `Reverb + Delay (parallel)` -> `Output`

## Interface

* **Big panel with 7 pages** (tabs): **EQ**, **COMP** (transfer curves, GR history, attack / release, make-up), **PULTEC** (detailed curve, tube, treble bandwidth), **DE-ESS** (sibilance band, threshold, reduction), **AIR** (source band -> harmonics), **FX** (the six effects with individual trims) and **SPACE** (animated room / particle sphere for reverb and delay, plus detailed parameters).
* **EQ live response** - 5 draggable nodes (Low, Low-Mid, High-Mid, Presence, Air) over a real-time spectrum. Drag = frequency / gain, **scroll wheel or Shift+drag = Q (bandwidth)**, double-click = reset. Plus a draggable **low-pass** handle (on/off button). The Pultec curve is overlaid in gold.
* **Meters** - input / 1176 GR / 670 GR / output, **vectorscope with IN / OUT selector** (before the chain or after the whole chain), stereo correlation and the overall **Gain Reduction** panel with history.
* **Input** - big **Level / Sens** knob with a green-yellow-red LED meter and OPTIMAL zone, plus **Noise** reduction.
* **Compression** - master **Compress** + **1176** / **670** weights, a **Make-up** knob and **AUTO** gain button after each compressor, 1176 ratio (4, 8, 12, 20, ALL) and 670 time constant (1-6).
* **Pultec EQ** - Bass / Atten / Freq (25-220 Hz) and Treble / Atten / Freq (4-20 kHz).
* **FX** - 0-50%: warm saturation, parallel compression and chorus (pop). 50-100%: distortion, flanger and chamber reverb (experimental / rock). Independent of Reverb and Delay.
* **De-esser** (after the Air exciter) - Amount, Freq (3-10 kHz), Range, Bandwidth (Q), Split / Wide mode, Listen, with its own reduction meter.
* **Reverb** (Room / Plate / Hall / **Studio** with subtle saturation, Tone, Decay, Mix) and **Delay** (tempo-synced, repeats, Tone, Mix) work in parallel.
* **Air Low / Air High** - Fresh-Air style harmonic exciter (harmonics generated from a separate source band + shelf boost; it only adds, it never cuts), acting directly on the voice.
* **Skins**: Digital, Technical, Analog. **Resizable window** (drag the bottom-right corner).
* **A/B compare**, **user presets** (SAVE / DEL, stored in `%APPDATA%\Audiokings\Vocals\Presets`).

## Presets

| Section | Presets |
|---|---|
| GENERAL - POP VOCALS | Pop Voice (base), Pop Bright, Pop Warm, Pop Powerful, Pop Airy, Pop Intimate, Backing Vocals |
| AUDIOKINGS - RAP | K1 Krespo, Ponze, Splash, Oral MCMP, Terko, JKing |

Factory presets live in `Source/Presets.h` (each one lists only what differs from the defaults in `Source/Params.h`).

## Build

Every push to `main` builds on GitHub Actions (Actions tab -> latest run -> Artifacts -> `AudiokingsVocals-Windows`):
the **Setup .exe** (recommended) and a zip with the VST3 and a manual `.bat` installer.
A tag such as `v3.0.0` also creates a GitHub Release.

Local build (Windows):
```
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Install

Run `AudiokingsVocals-Setup-x.y.z.exe`. It installs to `C:\Program Files\Common Files\VST3`, and automatically
removes older versions (installer-based or manual copies) before installing. Then rescan plugins:
* **FL Studio** (20.8+): Options -> Manage plugins -> Find installed plugins
* **Ableton Live** (11+): Preferences -> Plug-ins -> enable "Use VST3 Plug-in System Folders" -> Rescan

## License note

JUCE is AGPLv3 / commercial. Releasing this plugin commercially or closed-source requires a JUCE commercial license (and the VST3 SDK terms).
