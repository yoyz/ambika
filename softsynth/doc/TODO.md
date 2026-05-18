# Ambika Softsynth — TODO & Known Issues

## Untested Code (No Test Coverage)

### Sub-Oscillator & Transients
mix_sub_osc is always 0 in MakePatch(). Untested:
- Square/triangle/pulse sub-oscillator (3 shapes x 2 octaves)
- Transient generators: CLICK, GLITCH, BLOW, METALLIC, POP

### Modulation Modifiers
All 4 modifier slots are zeroed in MakePatch(). Only PRODUCT tested:
- SUM, ATTENUATE, MAX, MIN, XOR, GE, LE, QUANTIZE, LAG

### Resource table bounds
`ResourcesManager::Lookup<T,U>` is called with user-controlled indices
(e.g. `parameter_` driving wavetable selection in `RenderInterpolatedWavetable`).
No test validates that out-of-range indices are handled safely.

### Fixed-point overflow
Operations like `S8S8Mul`, `U8Mix`, `S16ClipU14` can overflow silently.
Tests check "sounds different" or "RMS > threshold" but never verify
numerical correctness with known sample values.

### VST3 plugin
No integration test exists for the VST3 path. Only LV2 is tested via
`test_lv2_host.cc`. The VST3 wrapper `ambika_vst3.cpp` is never
compiled or run in CI.

### 4-pole LP filter mode
The `filter_order` test uses default (2-pole SVF). 4-pole LP is tested
only in the FFT sweep test (`filter_sweep_fft`), not in any explicit
behavior test. Plugin PARAM_FILTER_TYPE now works (fix applied).

## Known Bugs

### FIXED: LFO 1/2/3 never render in softsynth
`Voice::LoadSources()` now renders LFO 1/2/3 from `lfo_1_/2_/3_` members,
using `patch_.env_lfo[i].shape` and `patch_.env_lfo[i].rate`. The 3 Lfo
objects were added to `Voice`, and `UpdateDestinations()` sets their
phase increments from the same lookup table as the voice LFO.

### FIXED: Filter Type parameter no-op in plugin mode
`PARAM_FILTER_TYPE` now calls `Voice::set_filter_type()` which delegates
to `DigitalFilter::set_type()`. Added `Voice::set_filter_type()` in
`voice.h`.

### FIXED: Potential OOB read in InterpolateSample
`InterpolateSample` in `op.h` — when `phase >> 8 == 255`, the second
read was at `table + 256` (one past end). Fixed by computing both
indices first, then clamping: `b = idx < 255 ? table[idx+1] : a`.

### FIXED: Modulation test spurious pass
The existing `modulation` test passed because of differing env_lfo values,
not actual LFO modulation. LFO 1/2/3 now render correctly, and lfo_audible_modulation
test verifies time-varying output from LFO→pitch routing.

### PluginVoiceManager steals voice 0 always
`plugin_shared.h:280-281` always steals voice 0 rather than the
oldest/longest-held note. The test `VoiceManager` has a correct
oldest-note steal algorithm (`test_all.cc:112-114`).

## Missing Feature Coverage

### Pitch bend extremes
bend = -8192 and +8191 now tested in plugin_shared tests.
No off-by-one errors found.

### Envelope edge cases
sustain=0, release=0, attack=0, retrigger, DEAD transition all now
tested in `envelope_edges` test. All behave correctly.
