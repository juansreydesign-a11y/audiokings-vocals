# Audiokings Vocals

Vocal channel strip for **Windows** (VST3, x64) and **macOS** (VST3 + Audio Unit, universal: Apple Silicon and Intel), for **FL Studio**, **Ableton Live**, **Cubase**, **Reaper** and, on macOS, **Logic Pro / GarageBand** (AU). Built with [JUCE](https://juce.com).
Compatibility is through the VST3 / AU standards (FL Studio 20.8+, Live 11+, Cubase 11+, Reaper 6.20+, macOS 10.13+). Presets are handled inside the plug-in, so no host re-applies them over your settings.

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

Every push to `main` builds on GitHub Actions (Actions tab -> latest run -> Artifacts):
* `AudiokingsVocals-Windows`: the **Setup .exe** (recommended) and a zip with the VST3.
* `AudiokingsVocals-macOS`: the **.pkg installer** (recommended) and a zip with the VST3, the Audio Unit and an uninstaller.

A tag such as `v3.5.0` also creates a GitHub Release with all the files.

Local build:
```
# Windows
cmake -S . -B build -A x64
cmake --build build --config Release

# macOS (universal binary, VST3 + AU)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Install

**Windows** - run `AudiokingsVocals-Setup-x.y.z.exe`. It installs to `C:\Program Files\Common Files\VST3` and removes older versions automatically.

**macOS** - open `AudiokingsVocals-macOS-x.y.z.pkg`. It installs
`/Library/Audio/Plug-Ins/VST3/Audiokings Vocals.vst3` and `/Library/Audio/Plug-Ins/Components/Audiokings Vocals.component`,
removing any previous version first. To uninstall, run `Uninstall-AudiokingsVocals.command`.

If macOS says the installer "cannot be opened because it is from an unidentified developer" (builds without an Apple Developer ID):
right-click the .pkg -> **Open**, or System Settings -> Privacy & Security -> **Open Anyway**.
For a plug-in blocked by Gatekeeper: `sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Audiokings Vocals.vst3"`.

Then rescan plugins:
* **FL Studio** (20.8+): Options -> Manage plugins -> Find installed plugins
* **Ableton Live** (11+): Preferences -> Plug-ins -> enable VST3 (Windows: "Use VST3 Plug-in System Folders") -> Rescan
* **Cubase / Reaper**: rescan the VST3 folder
* **Logic Pro / GarageBand** (macOS): the Audio Unit appears under Audio Units -> Audiokings (run *Reset & Rescan Selection* in the Plug-in Manager the first time)

## Signing and notarization (macOS, optional)

Without secrets the workflow signs ad-hoc. To produce a notarized installer, add these repository secrets
(Settings -> Secrets and variables -> Actions): `APPLE_CERT_APPLICATION_BASE64`, `APPLE_CERT_INSTALLER_BASE64`,
`APPLE_CERT_PASSWORD`, `APPLE_DEVELOPER_ID_APP`, `APPLE_DEVELOPER_ID_INSTALLER`, `APPLE_ID`, `APPLE_APP_PASSWORD`, `APPLE_TEAM_ID`.
An Apple Developer account is required.

## License note

JUCE is AGPLv3 / commercial. Releasing this plugin commercially or closed-source requires a JUCE commercial license (and the VST3 SDK terms).
