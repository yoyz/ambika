# Ambika Softsynth — Test Suite Documentation

## Overview

The test suite validates the Ambika softsynth across four levels:
1. **Core synthesis engine** (33 tests, no SDK)
2. **PluginVoiceManager bridge** (14 tests, no SDK)
3. **LV2 plugin integration** (3 tests, requires `lv2-dev`)

All tests are self-contained C++ programs that render audio and validate
properties via RMS analysis, FFT spectral analysis, waveform comparison,
and statistical checks.

---

## Level 1: Core Synthesis Engine

**File:** `tests/test_all.cc`
**Build:** `make test` (or `make ambika_test && ./ambika_test`)
**SDK:** None

### 1. `silence` — No events → silence
Creates a VoiceManager with all voices muted (volume=0, no modulation).
Verifies RMS < 0.001.
**WAV:** `test_silence.wav`

### 2. `monophonic` — Sequential 8-note scale
Plays C4-D4-E4-F4-G4-A4-B4-C5 with gaps between notes.
Verifies each note segment RMS > 0.001.
**WAV:** `test_mono.wav`

### 3. `legato` — Legato transitions
Plays 4 notes with the legato flag set (no re-trigger of envelopes).
Verifies continuous sound between note transitions (RMS at transition > 0.001).
**WAV:** `test_legato.wav`

### 4. `polyphonic` — 3-note and 6-note chords
Compares single note vs C-major triad vs full 6-note chord.
Verifies: chord RMS > 1.2× single, 6-voice RMS > 1.5× single.
**WAVs:** `test_poly_ref.wav`, `test_poly_c3.wav`, `test_poly_c6.wav`

### 5. `voice_stealing` — 7 notes with only 6 voices
Triggers 7 notes sequentially (must steal one voice).
Verifies output audible despite voice stealing.
**WAV:** `test_steal.wav`

### 6. `patch_differences` — Different oscillator shapes
Creates 4 patches: saw, square, triangle, sine.
Verifies all patches produce audible output and at least 5/6 pairwise
differences exceed threshold.
**WAVs:** `test_patch_saw.wav`, `test_patch_sq.wav`, `test_patch_tri.wav`,
`test_patch_sin.wav`

### 7. `envelope` — Release time comparison
Creates two patches: short release (5) and long release (100) on ENV2
(VCA envelope). Short release must fade faster — verifies the last 25%
of the long-release tail has higher RMS.
**WAVs:** `filt_env_short.wav`, `filt_env_long.wav`

### 8. `modulation` — LFO vibrato (validation of LFO 1/2/3 rendering)
Creates patches with/without LFO → pitch modulation. LFO 1/2/3 now render
correctly (previously always 0). Verifies the modulated output differs
from unmodulated.
**WAVs:** `filt_mod_none.wav`, `filt_mod_vib.wav`

### 9. `simultaneous_patches` — 6 voices, 6 different patches
Each voice gets a different oscillator algorithm (saw, square, triangle,
sine, FM, vowel). All play simultaneously. Verifies output is audible.
**WAV:** `test_simul_patches.wav`

### 10. `rapid_fire` — 50 rapid note on/off cycles
Triggers 50 notes at ~1ms intervals. Verifies all events process
without error and output is audible.
**WAV:** `test_rapid.wav`

### 11. `all_notes_off` — Release all 6 voices
Plays 6 simultaneous notes, releases all, waits for envelope tail.
Verifies the final portion of the output is silent (RMS < 0.005).
**WAV:** `test_alloff.wav`

### 12. `portamento` — Pitch glide between notes
Sets portamento time, plays C4 then C5 with legato.
Verifies zero-crossing rate differs between first and second half
of the note (confirming pitch change).
**WAV:** `test_porta.wav`

### 13. `filter_cutoff` — Cutoff sweep comparison
Compares low cutoff (20), high cutoff (100), and bypass (127).
Verifies that low cutoff produces a waveform that differs from bypass,
and also differs from high cutoff.
**WAVs:** `filt_cutoff_low.wav`, `filt_cutoff_high.wav`, `filt_cutoff_byp.wav`

### 14. `filter_modes` — LP/BP/HP/Notch produce different output
Renders through each filter mode with same patch.
Verifies at least 3/6 cross-mode output pairs differ.
**WAVs:** `filt_mode_lp.wav`, `filt_mode_hp.wav`, `filt_mode_bp.wav`,
`filt_mode_notch.wav`

### 15. `filter_resonance` — Resonance changes output
Compares Q=0.5 vs Q≈11 at same cutoff.
Verifies the output differs between low and high resonance.
**WAVs:** `filt_res_no.wav`, `filt_res_hi.wav`

### 16. `filter_order` — 2-pole SVF smoke test
Verifies the 2-pole filter processes audio without error.
**WAV:** `filt_order.wav`

### 17. `filter_closed_fft` — FFT: closed LP attenuates C2
Plays C2 (65 Hz) through LP filter at minimum cutoff.
Uses 2048-pt FFT with Hann window to verify C2 magnitude is
< 25% of the bypass magnitude. Confirms the filter truly closes.
**WAVs:** `filt_closed_C2_LP.wav`, `filt_closed_C2_byp.wav`

### 18. `filter_sweep_fft` — FFT: cutoff sweep for LP/HP/4P
For each of 2-pole LP, 2-pole HP, and 4-pole LP:
- Renders at low cutoff and high cutoff
- Computes high-frequency energy ratio (above 2kHz)
- LP: HF ratio grows as cutoff opens
- HP: low-frequency energy drops as cutoff rises
- 4-pole LP: same as 2-pole, steeper slope
**WAVs:** `filt_sweep_*.wav` (6 files)

### 19. `filter_modes_spectral_fft` — FFT: spectral band analysis
Renders saw+sub+noise through all 4 filter modes at cutoff=50.
Computes energy in 3 bands (50-500Hz, 500-2000Hz, 2000-8000Hz):
- LP: low band dominates
- HP: high band dominates
- BP: mid+high > low
- Notch: both low and high bands present
**WAVs:** `filt_spec_LP.wav`, `filt_spec_HP.wav`, `filt_spec_BP.wav`,
`filt_spec_NOTCH.wav`

### 20. `filter_resonance_peak_fft` — FFT: resonance peak
Renders saw+noise through LP at cutoff=55 with Q=0.5 and Q≈11.
4096-pt FFT: verifies high-Q spectrum has a measurable peak near the
expected cutoff (3.2 kHz), and magnitude at cutoff exceeds low-Q.
**WAVs:** `filt_res_lowQ.wav`, `filt_res_highQ.wav`

### 21. `lfo_nonzero` — LFO 1-4 produce varying values
Verifies all 4 LFO sources (LFO_1/2/3 now rendered correctly, LFO_4 as
voice LFO) produce changing values over successive blocks.
Also verifies LFO freezes when rate=0.
**No WAV.**

### 22. `lfo_audible_modulation` — LFO→pitch varies output
Routes LFO_1 → pitch fine, renders audio, verifies sample variance
between block-size windows (pitch wobble is audible).
**WAV:** `test_lfo_mod.wav`

### 23. `lfo_rate_changes_pitch` — Different rates differ
Renders LFO→pitch at slow (8) and fast (64) rates.
Verifies the outputs differ (rate changes pitch modulation speed).
**WAVs:** `test_lfo_slow.wav`, `test_lfo_fast.wav`

### 24. `lfo_shape_differs` — Triangle/Square/S&H/Ramp differ
Renders LFO→pitch with each of the 4 LFO waveforms at the same rate.
At least 3/6 shape pairs must differ.
**WAVs:** `test_lfo_shape_tri.wav`, `test_lfo_shape_sq.wav`,
`test_lfo_shape_sh.wav`, `test_lfo_shape_ramp.wav`

### 25. `lfo_independent` — All 4 LFOs have independent rates
Sets different rates on LFO_1/2/3/4, verifies all 4 change value
across blocks, and at least one pair has a different change count.
**No WAV.**

### 26. `mod_amount_linear` — Modulation amount→cutoff scaling
Routes LFO_1 → filter cutoff with amounts 16/32/48/64.
Verifies different amounts produce different cutoff values.
**No WAV.**

### 27. `mod_modifier_product` — Modifier PRODUCT(ENV1, 128) matches ref
Routes ENV_1 and CONSTANT_128 through modifier PRODUCT,
verifies output equals `(env1 * 128) >> 8` (within 1 LSB).
**No WAV.**

### 28. `lfo_modulates_lfo` — Wheel→LFO_4 rate changes LFO_4
Enables wheel modulation of LFO_4 rate (init patch slot 13).
Compares LFO_4 output with wheel=0 vs wheel=255 — must differ.
**No WAV.**

### 29. `oscillator_shapes` — 18 untested algorithms
Renders all previously uncovered oscillator algorithms:
CZ SAW, CZ SAW LP/PK/BP/HP, CZ Pulse LP/PK/BP/HP, CZ Tri LP,
Quad Saw Pad, 8-Bit Land, Dirty PWM, Filtered Noise,
Wavetable 1/8/16, Wavequence.
Verifies each is audible and most differ from each other. Also
verifies CZ SAW LP/PK/BP/HP type variants differ within the
shared RenderCzResoSaw function, and similarly for CZ Pulse types.
**WAVs:** `test_osc_*.wav` (18 files), `test_cz_*.wav` (4 files),
`test_cp_*.wav` (4 files)

### 30. `envelope_edges` — Edge case validation
Tests 5 envelope edge cases:
1. sustain=0 with long release: tail reaches silence
2. release=0: note cuts off instantly (lookup[0]=65535)
3. attack=0: immediate sound (no fade-in delay)
4. Re-trigger during attack: audio stays continuous
5. DEAD stage: all voices go silent after full cycle
**WAVs:** `test_env_sus0.wav`, `test_env_rel0.wav`, `test_env_atk0.wav`

### 31. `operators` — SYNC/RING/XOR/FOLD/BITS
Renders both oscillators (saw + detuned square) through each operator.
Verifies each operator is audible, all 5 differ from OP_SUM reference,
and they differ from each other (≥ 2/3 pairs).
**WAVs:** `test_op_sum.wav`, `test_op_*.wav` (5 files)

### 32. `patch_bank` — 208 built-in patches validated
Loads each of the 208 Shruthi-converted patches, plays note C4,
verifies: (1) RMS > 0.001 for ≥80%, (2) clear spectral peak for ≥50%,
(3) silence reached within 16s after note-off for ≥80%.
Prints a per-patch table with RMS, FFT peak ratio, silence time, status.
**No individual WAVs.**
**Summary:** 208R 171F 203S/208 (typical)

### 33. `no_stuck_notes` — Random stress test
Generates 32 random note on/off pairs with random pitch (36-84),
velocity (60-127), gaps (0-3 blocks), holds (2-13 blocks).
After all events, renders 4 seconds of tail.
Verifies final portion RMS < 0.001 — no voices stuck.
**WAV:** `test_no_stuck.wav`

---

## Level 2: PluginVoiceManager Bridge

**File:** `tests/test_plugin_manager.cc`
**Build:** `make test_plugin`
**SDK:** None

Tests the `PluginVoiceManager` class that bridges the synthesis engine
to LV2 and VST3 plugin hosts.

### 1. `midi_note` — NoteOn produces audio
Sends MIDI NoteOn C4 vel 100 through `PluginVoiceManager::NoteOn()`,
renders one block, verifies RMS > 0.001.

### 2. `silence` — No notes = silence
Renders one block with no active notes.
Verifies RMS < 0.001.

### 3. `note_off_stops` — Kill silences immediately
Plays a note, calls `AllSoundOff()`, renders one block.
Verifies RMS < 10% of the "on" RMS.

### 4. `pitch_bend` — Pitch bend at boundaries
Plays a note, applies pitch bend at -8192 (min), +8191 (max),
and 0 (center). Verifies all produce audible output and
min/max bend output differs from center.

### 5. `parameters` — Param changes affect output
Renders with default params, changes filter cutoff to 0,
re-renders. Verifies the difference between the two blocks
exceeds noise floor. Also tests param get/set round-trip.

### 6. `polyphony` — Chord louder than single
Plays a 3-note chord vs single note in separate instances.
Verifies chord RMS > 1.2× single.

### 7. `panic` — AllSoundOff produces silence
Renders with a note playing, calls `AllSoundOff()`.
Verifies the resulting output has RMS < 0.01.

---

## Level 3: LV2 Plugin Integration

**File:** `tests/test_lv2_host.cc`
**Build:** `make test_lv2` (requires `lv2-dev`)
**SDK:** LV2 (1.18+)

Tests the compiled `ambika.so` LV2 plugin by loading it via `dlopen`
and exercising the full LV2 plugin API.

### 1. `lv2_lifecycle` — Full plugin lifecycle
1. `dlopen("ambika.so")`, get `lv2_descriptor`
2. Verify all 8 descriptor function pointers exist
3. `instantiate()` with URID map feature at 48kHz
4. `connect_port()` for MIDI input, stereo output, 31 control params
5. `activate()` the plugin
6. Run with empty MIDI: verify silence (RMS < 0.001)
7. Run with MIDI NoteOn C4: verify audio output (RMS > 0.001)
8. Run with MIDI NoteOff to release
9. `deactivate()` and `cleanup()`
10. `dlclose()`

### 2. `lv2_params` — Parameter ports connected
Same lifecycle but exercises the control ports by setting parameter
values that are read by the plugin during `Run()`. Verifies the
plugin processes without crashing.

### 3. `lv2_descriptor` — Descriptor metadata
Checks the plugin URI is non-empty and the descriptor is valid.

---

## Running the Tests

```bash
cd softsynth

# Core synthesis (33 tests, no SDK)
make test
./ambika_test                   # if already built

# Plugin bridge (14 tests, no SDK)
make test_plugin

# LV2 integration (3 tests, requires lv2-dev)
sudo apt install lv2-dev        # one-time
make ambika.so                  # build plugin
make test_lv2                   # build & run tests

# Clean all test artifacts
make clean
```

## Adding New Tests

1. Add a new `TEST(my_test)` block in `tests/test_all.cc`
2. Register it in the `tests[]` array in `main()`
3. Run `make test` to build and verify

For FFT-based tests, use `ComputeSpectrum()`, `SpectrumAtAvg()`, and the
custom `ComputeDifference()` helper. The FFT is a 2048/4096-point radix-2
Cooley-Tukey implementation with Hann windowing.
