# Ambika Softsynth

A 6-voice polyphonic software synthesizer based on the
[Ambika](https://github.com/pichenettes/ambika) hybrid MIDI polysynth
by Emilie Gillet.

Ports the original ATmega voicecard firmware to standard C++ running
on desktop/Linux, replacing the analog audio path (DAC + analog filters)
with a digital implementation.

## Features

- **6-voice polyphony** — matching the 6 voicecards of the original hardware
- **20+ oscillator algorithms** — saw, square, triangle, sine, CZ phase
  distortion, FM, 8-bit, filtered noise, vowel, wavetable, wave sequencing
- **2 oscillators per voice** with sync, ring modulation, XOR, fold, bits
- **Digital state-variable filter** — 2-pole multimode (LP/BP/HP/Notch)
  and 4-pole LP, replacing the original LM13700/SSM2164 analog filters
- **Modulation matrix** — 14 routable modulation slots + 4 modifiers
- **3 envelopes** — ADSR with exponential response
- **4 LFOs** (3 patch-level + 1 voice-level) with multiple waveforms
- **Sub-oscillator** — square/triangle/pulse + transient generator
- **Distortion/noise** — wavetable-based waveshaping and noise mixing
- **Filter resonance peak** — TPT SVF topology with configurable Q
- **31 automatable parameters** in plugin mode
- **208 built-in patches** converted from Shruthi-1 format
- **External .PRO file loading** — Ambika hardware program format

## Architecture

```
softsynth/
├── avrlib/                # Portable AVR library replacement
│   ├── base.h             # Types: Word, uint24_t, DISALLOW_COPY
│   ├── op.h               # Fixed-point math (no AVR assembly)
│   ├── random.h / .cc     # LFSR random number generator
│   └── resources_manager.h # PROGMEM-free resource lookup
├── common/                # Shared protocol/patch definitions
│   ├── patch.h            # Patch data structure (31+ parameters)
│   ├── lfo.h              # LFO engine (triangle, square, S&H, ramp)
│   └── protocol.h         # SPI command protocol (hardware compat)
├── voicecard/             # Ported synthesis engine
│   ├── voicecard.h        # Constants (kAudioBlockSize=40, sample rate)
│   ├── voice.h / .cc      # Voice — main synthesis engine
│   ├── oscillator.h / .cc # 20+ oscillator rendering algorithms
│   ├── envelope.h         # 4-stage ADSR envelope generator
│   ├── sub_oscillator.h   # Sub-oscillator (square/triangle/pulse)
│   ├── transient_generator.h # Percussion transients (click, blow, etc.)
│   ├── filter.h           # Digital TPT state-variable filter
│   └── resources.h / .cc  # Wavetables + lookup tables (~150KB data)
├── host/                  # Host applications
│   ├── main.cc            # Standalone demo (WAV output)
│   ├── plugin_shared.h    # Shared plugin interface (VoiceManager + params)
│   ├── ambika_lv2.cpp     # LV2 plugin wrapper
│   ├── ambika_lv2.ttl     # LV2 Turtle manifest
│   └── ambika_vst3.cpp    # VST3 plugin wrapper
├── patches.h / .cc        # 208 built-in patches (auto-generated)
├── tests/                 # Test suite (50 total)
│   ├── test_all.cc        # 33 core synthesis tests
│   ├── test_plugin_manager.cc # 14 PluginVoiceManager tests
│   └── test_lv2_host.cc   # 3 LV2 integration tests
├── tools/
│   └── convert_patches.py # Shruthi-1 → Ambika patch converter
├── doc/
│   ├── tests.md           # Full test documentation
│   ├── patches.md         # Patch system documentation
│   ├── filter.md          # Filter implementation details
│   ├── plugins.md         # Plugin architecture
│   └── TODO.md            # Known issues & planned work
├── Makefile
└── README.md
```

### Signal Path

```
MIDI In → VoiceAllocator → Voice[0..5].Trigger()
    ↓
Voice.ProcessBlock() per voice:
  LoadSources()         → render envelopes + LFO 1-4 + noise
  ProcessModulationMatrix() → apply 14 mod routings
  UpdateDestinations()  → set osc/filter/env parameters
  RenderOscillators()   → 2 oscillators with sync
  Mix operators         → SUM/SYNC/RING/XOR/FOLD/BITS
  Mix sub-osc/transients
  DigitalFilter (TPT SVF) → LP/BP/HP/Notch or 4-pole LP
  Noise + Distortion    → wavetable distortion
  Output (uint8_t, 128=0)
    ↓
Mix all voices → float → gain → int16 → WAV / plugin output
```

## Build & Run

### Requirements

- C++11 compiler (gcc, clang)
- `make`
- For LV2 plugin: `lv2-dev` (`sudo apt install lv2-dev`)
- For VST3 plugin: [VST3 SDK](https://steinbergmedia.github.io/vst3_dev_portal/)

### Quick Start

```bash
cd softsynth

# Build standalone synth demo
make
./ambika_softsynth demo.wav
# → produces 31250 Hz, 16-bit mono WAV file

# Run the full core synthesis test suite (33 tests)
make test

# Run PluginVoiceManager tests (14 tests, no SDK needed)
make test_plugin

# Build and test LV2 plugin
sudo apt install lv2-dev
make ambika.so
make test_lv2
```

### Plugin Installation

```bash
# LV2 plugin
make lv2
cp -r Ambika.lv2 ~/.lv2/

# VST3 plugin (requires VST3 SDK)
make vst3 VST3_SDK_DIR=/path/to/vst3
cp -r Ambika.vst3 ~/.vst3/
```

## Plugin Parameters

The synth exposes 31 automatable parameters in plugin mode:

| Index | Name | Range | Default |
|-------|------|-------|---------|
| 0 | Filter Cutoff | 0.0–1.0 | 1.0 |
| 1 | Filter Resonance | 0.0–1.0 | 0.0 |
| 2 | Filter Mode | 0=LP, 1=BP, 2=HP, 3=NOTCH | 0 |
| 3 | Filter Type | 0=2pole, 1=4pole | 0 |
| 4 | Osc1 Shape | 0–22 (algorithm enum) | 0.0 |
| 5 | Osc2 Shape | 0–22 | 0.09 |
| 6 | Osc1 Range | -36–+36 semitones | 0.5 |
| 7 | Osc2 Detune | -100–+100 | 0.5 |
| 8 | Osc1 PWM | 0–127 | 0.25 |
| 9 | Osc2 Param | 0–127 | 0.0 |
| 10 | Mix Balance | 0–255 | 0.5 |
| 11 | Mix Operator | 0=SUM..5=BITS | 0.0 |
| 12 | Sub Osc | 0–127 | 0.3 |
| 13 | Noise | 0–127 | 0.06 |
| 14 | Fuzz | 0–127 | 0.0 |
| 15–18 | Env1 A/D/S/R | 0–127 | 0, 0.3, 0.63, 0.3 |
| 19–22 | Env2 A/D/S/R | 0–127 | 0, 0.3, 0.63, 0.3 |
| 23–26 | Env3 A/D/S/R | 0–127 | 0, 0.3, 0.63, 0.3 |
| 27 | LFO Shape | 0–3 | 0.0 |
| 28 | LFO Rate | 0–127 | 0.12 |
| 29 | Volume | 0.0–1.0 | 0.8 |
| 30 | Portamento | 0–127 | 0.0 |

## Tests

### Core Synthesis (33 tests)
Validates all synthesis components: silence, monophonic scales, legato,
polyphonic chords, voice stealing, patch differences, envelope release,
LFO 1-4 modulation, simultaneous patches, rapid fire, all-notes-off,
portamento, filter cutoff/modes/resonance/order, FFT-based spectral
verification, oscillator shape coverage (18 algorithms), operator modes
(SYNC/RING/XOR/FOLD/BITS), envelope edge cases, full 208-patch bank
validation (RMS + FFT + silence reach), and stuck-note stress test.

### Plugin Bridge (14 tests)
Validates the `PluginVoiceManager` class: MIDI note on/off, pitch bend
at extremes (-8192, +8191), parameter changes, polyphonic output levels,
panic/all-sound-off, silence verification.

### LV2 Integration (3 tests)
Validates the compiled `ambika.so` loaded via `dlopen`: full plugin
lifecycle (instantiate→connect→activate→run→deactivate→cleanup),
MIDI event parsing producing audio, parameter port connectivity.

See `doc/tests.md` for detailed documentation of all 50 tests.

## Filter Design

The digital filter uses the **trapezoidal (TPT) state-variable filter**
topology (Zavalishin), which is stable for all cutoff frequencies.
It replaces the original hardware analog filters:

| Voicecard | Hardware | Digital Equivalent |
|-----------|----------|-------------------|
| 2-pole SVF (SSM2164) | Multimode LP/BP/HP/Notch | TPT SVF, mode-selectable |
| 4-pole LP (SSM2164) | 4-pole LP | Cascaded 2-stage TPT LP |
| 4-pole LP (LM13700) | 4-pole LP | Cascaded 2-stage TPT LP |

Filter parameters are driven by the modulation matrix via
`modulation_destinations_[MOD_DST_FILTER_CUTOFF]` and
`MOD_DST_FILTER_RESONANCE`, exactly as the hardware PWM outputs were.

See `doc/filter.md` for implementation details.

## License

The underlying synthesis engine is GPL v3 (original Ambika firmware by
Emilie Gillet). The softsynth wrapper and plugin code is also GPL v3.
