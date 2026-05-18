# Digital Filter Design

## Overview

The original Ambika hardware uses analog filters:
- **4-pole low-pass with LM13700** — OTA-based ladder filter
- **4-pole low-pass with SSM2164** — VCA-based ladder filter  
- **2-pole SVF with SSM2164** — state-variable filter, LP/BP/HP/Notch

The firmware only outputs control voltages via PWM to these analog chips.
The softsynth replaces this entirely with a digital implementation.

## TPT State-Variable Filter

Uses the **trapezoidal (TPT) state-variable filter** topology from
Vadim Zavalishin's "The Art of Filter" — a digital formulation of the
analog SVF that remains stable for all cutoff frequencies up to Nyquist.

### Topology

```
hp = (in - (r + g) * bp - lp) * g / (1 + g * (g + r))
bp = bp + g * hp
lp = lp + g * bp
```

Where:
- `g = tan(π * fc / fs)` — pre-warped frequency parameter
- `r = 1/(2*Q)` — damping parameter
- `lp`, `bp` — state variables (low-pass / band-pass integrators)

### Output Selection

| Mode | Output |
|------|--------|
| LP | `lp` |
| BP | `bp` |
| HP | `hp` |
| NOTCH | `lp + hp` |

### Parameter Mapping

- **Cutoff** (0–127): Maps to 20 Hz → ~7.5 kHz (g stays < 1 for
  numerical stability)
- **Resonance** (0–127): Maps to Q = 0.5 (critically damped) → Q = 15
  (near self-oscillation)

### 4-Pole LP Mode

Cascades two TPT SVF stages both in LP mode, with resonance distributed:
- Stage 1: full Q
- Stage 2: 70% Q (reduced to prevent instability)

## Integration

The filter is called in `Voice::ProcessBlock()` after the oscillator
mix and before noise/distortion:

```
Osc → Mix → SubOsc → DigitalFilter → Noise → Distortion → Output
    ↑                    ↑
    |              Filter parameters controlled by
    |              modulation_destinations_[FILTER_CUTOFF/RESONANCE]
    |
  Patch.filter[0].mode selects LP/BP/HP/NOTCH
  DigitalFilter::set_type() selects 2-pole / 4-pole
```

Filter parameters are set in `UpdateDestinations()` exactly as the
original hardware PWM outputs were — just applied digitally now.

## Frequency Response

2-pole SVF provides 12 dB/oct rolloff. 4-pole LP provides 24 dB/oct
rolloff (two stages). Both modes are verified by FFT-based tests
(test 17–20 in `tests/test_all.cc`).
