# VST3 Cross-Compilation Guide

## Overview

Ambika's VST3 plugin is cross-compiled from Ubuntu 24.04 to Windows 64-bit
using MinGW-w64. The build requires the VST3 SDK 3.8 and roughly 20 SDK
source files to be compiled and linked alongside the synth engine.

## Build Process

### Prerequisites

```bash
sudo apt install g++-mingw-w64-x86-64
```

### Building

```bash
cd softsynth
make -f Makefile_vst3_mingw-w64          # builds vst3_win target
make -f Makefile_vst3_mingw-w64 clean    # removes Ambika.vst3/
```

The Makefile auto-downloads the VST3 SDK to `vst3sdk/` on first build.
Override with `VST3_SDK_DIR=/custom/path`.

### Output

```
Ambika.vst3/
└── Contents/
    └── x86_64-win/
        └── Ambika.vst3      # 1.3 MB PE32+ DLL
```

**Install on Windows:** Copy `Ambika.vst3/` to `C:\Program Files\Common
Files\VST3\Ambika.vst3\` or `%USERPROFILE%\Documents\VST3\Ambika.vst3\`.

## VST3 SDK Sources Required

The following SDK source files must be compiled and linked with the plugin.
The list is in `Makefile_vst3_mingw-w64` under `VST3_SDK_SRCS`:

| Category | Files |
|----------|-------|
| Core interfaces | `pluginterfaces/base/coreiids.cpp`, `funknown.cpp`, `conststringtable.cpp`, `ustring.cpp` |
| Base utilities | `base/source/baseiids.cpp`, `fobject.cpp`, `fstring.cpp`, `fbuffer.cpp`, `fdebug.cpp`, `updatehandler.cpp` |
| Threading | `base/thread/source/flock.cpp` |
| Plugin factory | `public.sdk/source/main/pluginfactory.cpp`, `dllmain.cpp`, `moduleinit.cpp` |
| SDK common | `public.sdk/source/common/pluginview.cpp`, `memorystream.cpp`, `commoniids.cpp` |
| VST classes | `public.sdk/source/vst/vstbus.cpp`, `vstaudioeffect.cpp`, `vstcomponent.cpp`, `vstcomponentbase.cpp`, `vsteditcontroller.cpp`, `vstparameters.cpp`, `vstinitiids.cpp` |

### SDK Patch

The VST3 SDK 3.8 `base/source/fstring.cpp` is missing `#include <limits>`.
The Makefile auto-patches this after cloning:

```bash
sed -i '/^#include <cstdlib>/a #include <limits>' \
    "$(VST3_SDK_DIR)/base/source/fstring.cpp"
```

## Architecture

```
host/ambika_vst3.cpp
├── AmbikaProcessor : AudioEffect          # Audio + MIDI processing
│   ├── setControllerClass(UID)            # Links to controller
│   ├── initialize()                       # Adds audio + event busses
│   ├── process()                          # MIDI in → synth → audio out
│   └── PluginVoiceManager synth_          # 6-voice engine wrapper
│
├── AmbikaController : EditControllerEx1   # Parameters + UI (IUnitInfo)
│   ├── 80 RangeParameter (0.0–1.0)        # All synth params
│   ├── IUnitInfo: 9 unit groups           # Osc, Filter, Mix, Env, LFO, etc.
│   ├── ProgramList: 208 patches           # Built-in preset names
│   └── createView() → nullptr             # No custom GUI (host generic)
│
└── Factory (BEGIN_FACTORY/END_FACTORY)
    └── DEF_VST3_CLASS("Ambika", ...)      # Registers processor + controller
```

### Key Design Decisions

- **Processor and controller are separate** (standard VST3 pattern). The
  processor calls `setControllerClass()` in its constructor to link to the
  controller's UID.
- **No custom GUI** — `createView()` returns `nullptr`. The host shows
  generic sliders for all 80 parameters.
- **IUnitInfo groups parameters** into 9 units (Root, Oscillators, Filter,
  Mix, Envelopes, LFO, Global, Mod Matrix, Controls) for better organization
  in hosts that support it.
- **Preset names** are exposed via `IUnitInfo::getProgramListInfo()` and
  `getProgramName()`, providing access to all 208 built-in patches.

## Parameter Registration

### Critical: Constructor Argument Order

The `RangeParameter` constructor has 10 parameters. The argument order
matters and is easy to get wrong:

```cpp
RangeParameter(
    const TChar* title,     // UTF-16 parameter name
    ParamID tag,            // unique ID matching process() param ID
    const TChar* units,     // unit string or nullptr
    ParamValue minPlain,    // 0.0 for normalized
    ParamValue maxPlain,    // 1.0 for normalized
    ParamValue defaultValuePlain,  // initial value (0.0–1.0)
    int32 stepCount,        // 0 = continuous
    int32 flags,            // ParameterInfo::kCanAutomate for automation
    UnitID unitID,          // kRootUnitId or custom unit ID
    const TChar* shortTitle // optional short name
);
```

**BUG**: Earlier versions passed arguments in wrong positions, resulting in
`defaultValuePlain` being `0.01` instead of `GetDefault(i)`, `stepCount`
getting the default value, and `flags` being `0` (no automation flag).
Parameters appeared in the host as non-automatable and invisible.

### Correct Usage

```cpp
String128 name;
str8ToStr16(name, PluginVoiceManager::GetName(i));
auto p = new RangeParameter(
    name, i, nullptr,         // title, tag, units
    0.0, 1.0,                 // min, max (normalized)
    PluginVoiceManager::GetDefault(i),  // default value
    0,                        // step count (continuous)
    ParameterInfo::kCanAutomate,        // automatable
    unitId);                  // unit group ID
parameters.addParameter(p);
```

### Unit Organization

Parameters are grouped into units based on index:

| Index Range | Unit | Description |
|-------------|------|-------------|
| 0–7 | Oscillators | Osc1/2 shape, pwm, range, detune |
| 8–11 | Filter | Cutoff, resonance, mode, type |
| 12–16 | Mix | Balance, operator, sub, noise, fuzz |
| 17–28 | Envelopes | Env1-3 A/D/S/R |
| 29–30 | LFO | Shape, rate |
| 31–32 | Global | Volume, portamento |
| 33–74 | Mod Matrix | 14 slots × source/dest/amount |
| 75–79 | Controls | Panic, reset, prev/next, preset |

## DLL Exports

Windows requires three exported functions in the VST3 DLL:

- `GetPluginFactory` — created by `BEGIN_FACTORY`/`END_FACTORY` macros
- `InitDll` / `ExitDll` — from `dllmain.cpp` in the VST3 SDK

These must be present in the export table. The `SMTG_EXPORT_SYMBOL` macro
(which expands to `__declspec(dllexport)` on Windows) must be active. With
MinGW cross-compilation, this works correctly.

## Known Issues

### 1. Voice Aliasing

The `PluginVoiceManager::FindNote()` may not find an already-playing note
if `note_[]` was cleared by `NoteOff()`, causing the same pitch to trigger
a new voice while the old one is still releasing. This creates a
chorus-like artifact when notes overlap. A proper voice stealing / note
tracking fix would improve this.

### 2. Parameter Mapping

Some parameters may not work as expected (e.g., filter cutoff has no
effect). This is likely caused by the mismatch between the parameter
indices in the VST3 plugin and the actual synth engine fields in
`ApplyParam()`. When `host/plugin_shared.h` is updated (e.g., reordering
parameters), the `ApplyParam()` switch cases must be updated to match the
new indices.

### 3. VCA Always Open (FIXED)

The VCA was always open because `Voice::UpdateDestinations()` at
`voice.cc:213` overwrote the envelope→VCA modulation with `volume << 1`.
Fixed by:
- Storing the base VCA in `dst_[MOD_DST_VCA]` instead of
  `modulation_destinations_[MOD_DST_VCA]`
- Initializing the VCA accumulator from `dst_[VCA]` at the top of
  `ProcessModulationMatrix()`
- Each VCA slot multiplies the accumulator, compounding env2→VCA,
  velocity→VCA, etc.
- Result: no note = env2 = 0 → VCA = 0 → silence. Note playing =
  env2 > 0 → VCA > 0 → audio passes.

### 4. Placeholder UIDs

The FUIDs used for the processor and controller are generated but not
registered. If two Ambika VST3 builds with the same UIDs are installed,
they may conflict. Generate unique UIDs for distribution builds using a
UUID generator.

### 4. No Custom GUI

`createView()` returns `nullptr`. The host provides generic sliders. A
VSTGUI-based editor could be added but requires ~40 additional VSTGUI
source files and platform-specific drawing code.

## Testing

The VST3 SDK includes a **Plugin Test Host** that validates the plugin
independently of a DAW:

```
public.sdk/samples/vst-hosting/validator/
```

Build it on Windows (requires MSVC or MinGW natively) and run against
your `.vst3` bundle. It checks exports, factory, instantiation, parameter
count, and basic processing.

## Comparison with LV2

The LV2 plugin (`host/ambika_lv2.cpp`) shares the same
`PluginVoiceManager` backend as the VST3. Key differences:

| Aspect | LV2 | VST3 |
|--------|-----|------|
| Ports | Indexed 0..2 (MIDI + audio), 3..82 (controls) | Named busses (stereo out, MIDI in) |
| Parameters | Via port connections (float buffers) | Via `RangeParameter` objects |
| GUI | No editor (generic host UI) | No editor (generic host UI) |
| Build | Native g++, LV2 SDK | Cross-compile with MinGW, VST3 SDK |
| Presets | Via TTL file | Via `IUnitInfo::getProgramListInfo()` |

## Files Reference

| File | Purpose |
|------|---------|
| `host/ambika_vst3.cpp` | VST3 processor, controller, factory |
| `host/plugin_shared.h` | PluginVoiceManager (synth engine wrapper) |
| `host/ambika_lv2.cpp` | LV2 plugin (for reference) |
| `Makefile_vst3_mingw-w64` | Cross-compile Makefile |
| `softsynth/patches.h` | 208 built-in patches |
