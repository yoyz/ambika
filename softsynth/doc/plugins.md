# Plugin Architecture

## Overview

The softsynth can be built as an LV2 or VST3 plugin. Both share a
common abstraction layer (`host/plugin_shared.h`) that wraps the
VoiceManager with MIDI handling, parameter routing, and preset
management.

```
Plugin Host (DAW)
    │
    ├── LV2: Run()
    │   └── ambika_lv2.cpp
    │       └── LV2_Descriptor {instantiate,
    │           connect_port, activate, run,
    │           deactivate, cleanup}
    │
    ├── VST3: process()
    │   └── ambika_vst3.cpp
    │       └── AudioEffect subclass
    │           {initialize, setupProcessing,
    │            process}
    │
    └── host/plugin_shared.h
        └── PluginVoiceManager
            ├── 6× Voice (synthesis engine)
            ├── MIDI: NoteOn/Off, PitchBend, CC
            ├── 80× float params (0..1)
            ├── 208 built-in presets
            └── ProcessBlock(float** output, nframes)
```

## PluginVoiceManager

The `PluginVoiceManager` class in `host/plugin_shared.h` provides:

**MIDI Methods:**
- `NoteOn(channel, note, velocity)` — triggers voice
- `NoteOff(channel, note, velocity)` — releases voice
- `PitchBend(channel, value)` — sets pitch bend (-8192..8191)
- `Modulation(channel, value)` — sets modulation wheel
- `AllSoundOff()` — immediate kill all voices (panic)
- `AllNotesOff()` — release all voices

**Control Methods:**
- `Panic()` — kills all voices immediately
- `ResetToInit()` — reloads init patch + panics
- `LoadPreset(index)` — loads one of 208 built-in presets

**Parameter Methods:**
- `SetParam(index, value)` — sets 0..1 float parameter
- `GetParam(index)` — returns current value
- `GetDefault(index)` — factory default
- `GetName(index)` — human-readable name

**Audio Processing:**
- `ProcessBlock(float** outputs, channels, frames)` — renders
  audio in blocks of kAudioBlockSize (40 samples), mixing all
  6 voices with master gain

## Port Layout

### LV2 (`ambika_lv2.cpp`)

| Port(s) | Type | Symbol |
|---------|------|--------|
| 0 | atom:Sequence (MIDI in) | `midi_in` |
| 1 | float (audio out L) | `audio_out_l` |
| 2 | float (audio out R) | `audio_out_r` |
| 3..82 | float (control in) | `osc1_shape` .. `preset` |

URID map feature is required. Plugin URI:
`https://ambika.opencode/softsynth`

### VST3 (`ambika_vst3.cpp`)

| Bus | Type |
|-----|------|
| Audio output | Stereo (SpeakerArr::kStereo) |
| Event input | 16-channel MIDI |

All 80 parameters are exposed via `RangeParameter` (0.0–1.0, 0.01 step).
Controller class handles parameter automation.

## Building

```bash
# LV2 plugin
sudo apt install lv2-dev
make ambika.so
make lv2
cp -r Ambika.lv2 ~/.lv2/

# VST3 plugin (requires MinGW cross-compiler + VST3 SDK)
sudo apt install g++-mingw-w64-x86-64
make -f Makefile_vst3_mingw-w64 vst3_win
```

The VST3 Makefile automatically downloads the VST3 SDK (v3.8.0)
to `vst3sdk/` on first build. Override with `VST3_SDK_DIR=<path>`.

## Parameters (80 total)

### Oscillators (0-7)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 0 | Osc1 Shape | 0..22 | Oscillator algorithm (saw, square, FM, wavetable…) |
| 1 | Osc1 PWM | 0..127 | Pulse width / algorithm parameter |
| 2 | Osc1 Range | -36..+36 | Transpose in semitones |
| 3 | Osc1 Detune | -100..+100 | Fine detune |
| 4 | Osc2 Shape | 0..22 | Oscillator algorithm |
| 5 | Osc2 Param | 0..127 | Pulse width / algorithm parameter |
| 6 | Osc2 Range | -36..+36 | Transpose in semitones |
| 7 | Osc2 Detune | -100..+100 | Fine detune |

### Filter (8-11)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 8 | Filter Cutoff | 0..127 | Low-pass filter cutoff |
| 9 | Filter Resonance | 0..127 | Resonance / emphasis |
| 10 | Filter Mode | 0..3 | LP, BP, HP, Notch |
| 11 | Filter Type | 0/1 | 2-pole SVF or 4-pole LP |

### Mix (12-16)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 12 | Mix Balance | 0..255 | Crossfade between osc 1 and osc 2 |
| 13 | Mix Operator | 0..5 | SUM, SYNC, RING, XOR, FOLD, BITS |
| 14 | Sub Osc | 0..127 | Sub oscillator level |
| 15 | Noise | 0..127 | Noise level |
| 16 | Fuzz | 0..127 | Wavefolder / distortion amount |

### Envelopes (17-28)

Three 4-stage ADSR envelopes:

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 17-20 | Env1 Attack / Decay / Sustain / Release | 0..127 | Amplitude envelope |
| 21-24 | Env2 Attack / Decay / Sustain / Release | 0..127 | Filter / VCA envelope |
| 25-28 | Env3 Attack / Decay / Sustain / Release | 0..127 | Auxiliary envelope |

### LFO (29-30)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 29 | LFO Shape | 0..3 | Triangle, Square, S&H, Ramp |
| 30 | LFO Rate | 0..127 | Voice LFO speed |

### Global (31-32)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 31 | Volume | 0..127 | Master volume |
| 32 | Portamento | 0..127 | Portamento / glide time |

### Modulation Matrix (33-74)

14 slots × 3 parameters each. Each slot routes a modulation source
to a modulation destination with a bipolar amount.

| Field | Normalized | Raw | Description |
|-------|-----------|-----|-------------|
| Source | 0..1 | 0..28 | Modulation source (env, lfo, velocity, wheel…) |
| Destination | 0..1 | 0..17 | Modulation target (cutoff, pitch, VCA, LFO rate…) |
| Amount | 0..1 | -128..127 | Modulation depth (0.5 = zero) |

**Modulation Sources:** Env 1-3, LFO 1-4, Operator 1-4, Seq 1-2,
Arp Step, Velocity, Aftertouch, Pitch Bend, Wheel 1-2, Expression,
Note, Gate, Noise, Random, Constant values (4/8/16/32/64/128/256)

**Modulation Destinations:** Osc 1-2 pitch, Osc 1-2 fine/coarse
detune, Mix balance/parameter/noise/sub/fuzz/crush,
Filter cutoff/resonance, Env attack/decay/release, LFO 4 rate, VCA

### Controls (75-79)

| Index | Name | Type | Description |
|-------|------|------|-------------|
| 75 | Panic | trigger (≥0.5) | Kill all voices immediately |
| 76 | Reset | trigger (≥0.5) | Reload init patch + panic |
| 77 | Prev Preset | trigger (≥0.5) | Previous built-in preset |
| 78 | Next Preset | trigger (≥0.5) | Next built-in preset |
| 79 | Preset | 0..207 | Direct preset selection |

## Presets

208 built-in patches (from the original Shruthi-1 / Ambika factory
bank) are accessible via the `Preset` parameter or Prev/Next buttons.
The preset list is defined in `patches.h` / `patches.cc`.

When a preset is loaded, all synthesis parameters are synchronized
to the stored patch values, and any playing notes are killed.

## Testing

```bash
# PluginVoiceManager (14 tests, no SDK)
make test_plugin

# LV2 integration (3 tests, requires lv2-dev + ambika.so)
make ambika.so
make test_lv2
```

See [tests.md](tests.md) for details on the full 50-test suite.
