#ifndef AMBIKA_PLUGIN_SHARED_H_
#define AMBIKA_PLUGIN_SHARED_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "../voicecard/voice.h"
#include "../voicecard/voicecard.h"
#include "../patches.h"

using namespace ambika;

enum AmbikaParam {
  // Oscillators (0-7)
  PARAM_OSC1_SHAPE = 0,
  PARAM_OSC1_PARAMETER,
  PARAM_OSC1_RANGE,
  PARAM_OSC1_DETUNE,
  PARAM_OSC2_SHAPE,
  PARAM_OSC2_PARAMETER,
  PARAM_OSC2_RANGE,
  PARAM_OSC2_DETUNE,

  // Filter (8-11)
  PARAM_FILTER_CUTOFF,
  PARAM_FILTER_RESONANCE,
  PARAM_FILTER_MODE,
  PARAM_FILTER_TYPE,

  // Mix (12-16)
  PARAM_MIX_BALANCE,
  PARAM_MIX_OPERATOR,
  PARAM_MIX_SUB_OSC,
  PARAM_MIX_NOISE,
  PARAM_MIX_FUZZ,

  // Envelopes (17-28)
  PARAM_ENV1_ATTACK,
  PARAM_ENV1_DECAY,
  PARAM_ENV1_SUSTAIN,
  PARAM_ENV1_RELEASE,
  PARAM_ENV2_ATTACK,
  PARAM_ENV2_DECAY,
  PARAM_ENV2_SUSTAIN,
  PARAM_ENV2_RELEASE,
  PARAM_ENV3_ATTACK,
  PARAM_ENV3_DECAY,
  PARAM_ENV3_SUSTAIN,
  PARAM_ENV3_RELEASE,

  // LFO (29-30)
  PARAM_LFO_SHAPE,
  PARAM_LFO_RATE,

  // Global (31-32)
  PARAM_VOLUME,
  PARAM_PORTAMENTO,

  // Modulation matrix: 14 slots × 3 (33-74)
  PARAM_MOD_SOURCE_0,
  PARAM_MOD_DEST_0,
  PARAM_MOD_AMOUNT_0,
  PARAM_MOD_SOURCE_1,
  PARAM_MOD_DEST_1,
  PARAM_MOD_AMOUNT_1,
  PARAM_MOD_SOURCE_2,
  PARAM_MOD_DEST_2,
  PARAM_MOD_AMOUNT_2,
  PARAM_MOD_SOURCE_3,
  PARAM_MOD_DEST_3,
  PARAM_MOD_AMOUNT_3,
  PARAM_MOD_SOURCE_4,
  PARAM_MOD_DEST_4,
  PARAM_MOD_AMOUNT_4,
  PARAM_MOD_SOURCE_5,
  PARAM_MOD_DEST_5,
  PARAM_MOD_AMOUNT_5,
  PARAM_MOD_SOURCE_6,
  PARAM_MOD_DEST_6,
  PARAM_MOD_AMOUNT_6,
  PARAM_MOD_SOURCE_7,
  PARAM_MOD_DEST_7,
  PARAM_MOD_AMOUNT_7,
  PARAM_MOD_SOURCE_8,
  PARAM_MOD_DEST_8,
  PARAM_MOD_AMOUNT_8,
  PARAM_MOD_SOURCE_9,
  PARAM_MOD_DEST_9,
  PARAM_MOD_AMOUNT_9,
  PARAM_MOD_SOURCE_10,
  PARAM_MOD_DEST_10,
  PARAM_MOD_AMOUNT_10,
  PARAM_MOD_SOURCE_11,
  PARAM_MOD_DEST_11,
  PARAM_MOD_AMOUNT_11,
  PARAM_MOD_SOURCE_12,
  PARAM_MOD_DEST_12,
  PARAM_MOD_AMOUNT_12,
  PARAM_MOD_SOURCE_13,
  PARAM_MOD_DEST_13,
  PARAM_MOD_AMOUNT_13,

  // Controls (75-79)
  PARAM_PANIC,
  PARAM_RESET,
  PARAM_PREV_PRESET,
  PARAM_NEXT_PRESET,
  PARAM_PRESET,

  PARAM_COUNT
};

static const int kPluginVoices = 6;
static const int kModSlots = 14;
static const int kNumModSources = MOD_SRC_LAST;
static const int kNumModDests = MOD_DST_LAST;

class PluginVoiceManager {
 public:
  PluginVoiceManager() {
    for (int i = 0; i < kPluginVoices; ++i) {
      voices_[i].Init();
      note_[i] = -1;
    }
    current_preset_ = -1;
    ResetParams();
  }

  void ResetParams() {
    for (int i = 0; i < PARAM_COUNT; ++i) {
      params_[i] = GetDefault(i);
      if (i < PARAM_MOD_SOURCE_0 || i > PARAM_MOD_AMOUNT_13)
        ApplyParam(i);
    }
  }

  void Panic() {
    for (int i = 0; i < kPluginVoices; ++i) {
      voices_[i].Kill();
      note_[i] = -1;
    }
  }

  void ResetToInit() {
    for (int i = 0; i < kPluginVoices; ++i) {
      voices_[i].Init();
      note_[i] = -1;
    }
    current_preset_ = -1;
    ResetParams();
  }

  void LoadPreset(int index) {
    if (index < 0) index = 0;
    if (index >= kNumPatches) index = kNumPatches - 1;
    const Patch& src = kPatches[index];
    for (int i = 0; i < kPluginVoices; ++i) {
      voices_[i].mutable_patch() = src;
      // Re-init envelopes with new patch values
      for (int e = 0; e < kNumEnvelopes; ++e)
        voices_[i].mutable_patch().env_lfo[e].retrigger_mode = 0;
      note_[i] = -1;
    }
    current_preset_ = index;
    SyncParamsFromPatch(src);
  }

  // -- MIDI handling --

  void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    (void)channel;
    int v = FindNote(note);
    if (v < 0) v = AllocVoice();
    uint16_t pitch = note * 128;
    uint8_t vel = (uint16_t)velocity << 1;
    if (vel > 255) vel = 255;
    voices_[v].Trigger(pitch, vel, 0);
    note_[v] = note;
  }

  void NoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    (void)channel; (void)velocity;
    for (int i = 0; i < kPluginVoices; ++i) {
      if (note_[i] == (int)note) {
        voices_[i].Release();
        note_[i] = -1;
      }
    }
  }

  void PitchBend(uint8_t channel, int16_t bend) {
    (void)channel;
    uint8_t val = 128 + (bend * 128) / 8192;
    if (val > 255) val = (bend > 0) ? 255 : 0;
    for (int i = 0; i < kPluginVoices; ++i)
      voices_[i].set_modulation_source(MOD_SRC_PITCH_BEND, val);
  }

  void Modulation(uint8_t channel, uint8_t value) {
    (void)channel;
    for (int i = 0; i < kPluginVoices; ++i)
      voices_[i].set_modulation_source(MOD_SRC_WHEEL, value);
  }

  void AllSoundOff() { Panic(); }
  void AllNotesOff() { Panic(); }

  // -- Parameter handling --

  static float GetDefault(int param) {
    switch (param) {
      case PARAM_OSC1_SHAPE:      return 0.0f;
      case PARAM_OSC1_PARAMETER:  return 0.25f;
      case PARAM_OSC1_RANGE:      return 0.5f;
      case PARAM_OSC1_DETUNE:     return 0.5f;
      case PARAM_OSC2_SHAPE:      return 2.0f / 22.0f;
      case PARAM_OSC2_PARAMETER:  return 0.0f;
      case PARAM_OSC2_RANGE:      return 0.5f;
      case PARAM_OSC2_DETUNE:     return 0.5f;
      case PARAM_FILTER_CUTOFF:   return 1.0f;
      case PARAM_FILTER_RESONANCE: return 0.0f;
      case PARAM_FILTER_MODE:     return 0.0f;
      case PARAM_FILTER_TYPE:     return 0.0f;
      case PARAM_MIX_BALANCE:     return 0.5f;
      case PARAM_MIX_OPERATOR:    return 0.0f;
      case PARAM_MIX_SUB_OSC:     return 0.3f;
      case PARAM_MIX_NOISE:       return 0.06f;
      case PARAM_MIX_FUZZ:        return 0.0f;
      case PARAM_ENV1_ATTACK:     return 0.0f;
      case PARAM_ENV1_DECAY:      return 0.3f;
      case PARAM_ENV1_SUSTAIN:    return 0.63f;
      case PARAM_ENV1_RELEASE:    return 0.3f;
      case PARAM_ENV2_ATTACK:     return 0.0f;
      case PARAM_ENV2_DECAY:      return 0.3f;
      case PARAM_ENV2_SUSTAIN:    return 0.63f;
      case PARAM_ENV2_RELEASE:    return 0.3f;
      case PARAM_ENV3_ATTACK:     return 0.0f;
      case PARAM_ENV3_DECAY:      return 0.3f;
      case PARAM_ENV3_SUSTAIN:    return 0.63f;
      case PARAM_ENV3_RELEASE:    return 0.3f;
      case PARAM_LFO_SHAPE:       return 0.0f;
      case PARAM_LFO_RATE:        return 0.12f;
      case PARAM_VOLUME:          return 0.8f;
      case PARAM_PORTAMENTO:      return 0.0f;
      case PARAM_PANIC:
      case PARAM_RESET:
      case PARAM_PREV_PRESET:
      case PARAM_NEXT_PRESET:     return 0.0f;
      case PARAM_PRESET:          return 0.0f;
      default:
        if (param >= PARAM_MOD_SOURCE_0 && param <= PARAM_MOD_AMOUNT_13)
          return 0.5f;
        return 0.0f;
    }
  }

  static const char* GetName(int param) {
    switch (param) {
      case PARAM_OSC1_SHAPE:      return "Osc1 Shape";
      case PARAM_OSC1_PARAMETER:  return "Osc1 PWM";
      case PARAM_OSC1_RANGE:      return "Osc1 Range";
      case PARAM_OSC1_DETUNE:     return "Osc1 Detune";
      case PARAM_OSC2_SHAPE:      return "Osc2 Shape";
      case PARAM_OSC2_PARAMETER:  return "Osc2 Param";
      case PARAM_OSC2_RANGE:      return "Osc2 Range";
      case PARAM_OSC2_DETUNE:     return "Osc2 Detune";
      case PARAM_FILTER_CUTOFF:   return "Filter Cutoff";
      case PARAM_FILTER_RESONANCE: return "Filter Resonance";
      case PARAM_FILTER_MODE:     return "Filter Mode";
      case PARAM_FILTER_TYPE:     return "Filter Type";
      case PARAM_MIX_BALANCE:     return "Mix Balance";
      case PARAM_MIX_OPERATOR:    return "Mix Operator";
      case PARAM_MIX_SUB_OSC:     return "Sub Osc";
      case PARAM_MIX_NOISE:       return "Noise";
      case PARAM_MIX_FUZZ:        return "Fuzz";
      case PARAM_ENV1_ATTACK:     return "Env1 Attack";
      case PARAM_ENV1_DECAY:      return "Env1 Decay";
      case PARAM_ENV1_SUSTAIN:    return "Env1 Sustain";
      case PARAM_ENV1_RELEASE:    return "Env1 Release";
      case PARAM_ENV2_ATTACK:     return "Env2 Attack";
      case PARAM_ENV2_DECAY:      return "Env2 Decay";
      case PARAM_ENV2_SUSTAIN:    return "Env2 Sustain";
      case PARAM_ENV2_RELEASE:    return "Env2 Release";
      case PARAM_ENV3_ATTACK:     return "Env3 Attack";
      case PARAM_ENV3_DECAY:      return "Env3 Decay";
      case PARAM_ENV3_SUSTAIN:    return "Env3 Sustain";
      case PARAM_ENV3_RELEASE:    return "Env3 Release";
      case PARAM_LFO_SHAPE:       return "LFO Shape";
      case PARAM_LFO_RATE:        return "LFO Rate";
      case PARAM_VOLUME:          return "Volume";
      case PARAM_PORTAMENTO:      return "Portamento";
      case PARAM_PANIC:           return "Panic";
      case PARAM_RESET:           return "Reset";
      case PARAM_PREV_PRESET:     return "Prev Preset";
      case PARAM_NEXT_PRESET:     return "Next Preset";
      case PARAM_PRESET:          return "Preset";
      default:
        if (param >= PARAM_MOD_SOURCE_0 && param <= PARAM_MOD_AMOUNT_13) {
          int slot = (param - PARAM_MOD_SOURCE_0) / 3;
          int field = (param - PARAM_MOD_SOURCE_0) % 3;
          const char* labels[] = { "Source", "Dest", "Amt" };
          static char buf[24];
          snprintf(buf, sizeof(buf), "Mod %d %s", slot, labels[field]);
          return buf;
        }
        return "Unknown";
    }
  }

  void SetParam(int param, float value) {
    if (param < 0 || param >= PARAM_COUNT) return;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;

    // Handle trigger-style params: fire action, don't store value
    if (param == PARAM_PANIC && value >= 0.5f) {
      Panic();
      return;
    }
    if (param == PARAM_RESET && value >= 0.5f) {
      ResetToInit();
      return;
    }
    if (param == PARAM_PREV_PRESET && value >= 0.5f) {
      int next = (current_preset_ < 0) ? kNumPatches - 1 : current_preset_ - 1;
      LoadPreset(next);
      return;
    }
    if (param == PARAM_NEXT_PRESET && value >= 0.5f) {
      int next = (current_preset_ < 0) ? 0 : current_preset_ + 1;
      LoadPreset(next);
      return;
    }

    if (param == PARAM_PRESET) {
      int idx = (int)(value * (kNumPatches - 1) + 0.5f);
      if (idx >= kNumPatches) idx = kNumPatches - 1;
      if (idx != current_preset_) {
        LoadPreset(idx);
        params_[param] = value;
      }
      return;
    }

    if (params_[param] != value) {
      params_[param] = value;
      ApplyParam(param);
    }
  }

  float GetParam(int param) const {
    if (param < 0 || param >= PARAM_COUNT) return 0.0f;
    return params_[param];
  }

  // -- Audio processing --

  void ProcessBlock(float** outputs, int num_channels, int num_frames) {
    int pos = 0;
    while (pos < num_frames) {
      int todo = num_frames - pos;
      if (todo > kAudioBlockSize) todo = kAudioBlockSize;

      for (int v = 0; v < kPluginVoices; ++v)
        voices_[v].ProcessBlock();

      for (int i = 0; i < todo; ++i) {
        float sum = 0.0f;
        for (int v = 0; v < kPluginVoices; ++v) {
          float s = (voices_[v].output()[i] - 128.0f) / 128.0f;
          sum += s;
        }
        float sample = sum * master_gain_;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        for (int ch = 0; ch < num_channels && ch < 2; ++ch)
          outputs[ch][pos + i] = sample;
      }
      pos += todo;
    }
  }

 private:
  int FindNote(int note) {
    for (int i = 0; i < kPluginVoices; ++i)
      if (note_[i] == note) return i;
    return -1;
  }

  int AllocVoice() {
    for (int i = 0; i < kPluginVoices; ++i)
      if (note_[i] < 0) return i;
    return 0;
  }

  void SyncParamsFromPatch(const Patch& p) {
    params_[PARAM_OSC1_SHAPE] = p.osc[0].shape / (float)(WAVEFORM_LAST - 2);
    params_[PARAM_OSC1_PARAMETER] = p.osc[0].parameter / 127.0f;
    params_[PARAM_OSC1_RANGE] = (p.osc[0].range + 36) / 72.0f;
    params_[PARAM_OSC1_DETUNE] = (p.osc[0].detune + 100) / 200.0f;
    params_[PARAM_OSC2_SHAPE] = p.osc[1].shape / (float)(WAVEFORM_LAST - 2);
    params_[PARAM_OSC2_PARAMETER] = p.osc[1].parameter / 127.0f;
    params_[PARAM_OSC2_RANGE] = (p.osc[1].range + 36) / 72.0f;
    params_[PARAM_OSC2_DETUNE] = (p.osc[1].detune + 100) / 200.0f;
    params_[PARAM_FILTER_CUTOFF] = p.filter[0].cutoff / 127.0f;
    params_[PARAM_FILTER_RESONANCE] = p.filter[0].resonance / 127.0f;
    params_[PARAM_FILTER_MODE] = p.filter[0].mode / 3.0f;
    params_[PARAM_FILTER_TYPE] = 0.0f;
    params_[PARAM_MIX_BALANCE] = p.mix_balance / 255.0f;
    params_[PARAM_MIX_OPERATOR] = p.mix_op / 5.0f;
    params_[PARAM_MIX_SUB_OSC] = p.mix_sub_osc / 127.0f;
    params_[PARAM_MIX_NOISE] = p.mix_noise / 127.0f;
    params_[PARAM_MIX_FUZZ] = p.mix_fuzz / 127.0f;
    params_[PARAM_ENV1_ATTACK] = p.env_lfo[0].attack / 127.0f;
    params_[PARAM_ENV1_DECAY] = p.env_lfo[0].decay / 127.0f;
    params_[PARAM_ENV1_SUSTAIN] = p.env_lfo[0].sustain / 127.0f;
    params_[PARAM_ENV1_RELEASE] = p.env_lfo[0].release / 127.0f;
    params_[PARAM_ENV2_ATTACK] = p.env_lfo[1].attack / 127.0f;
    params_[PARAM_ENV2_DECAY] = p.env_lfo[1].decay / 127.0f;
    params_[PARAM_ENV2_SUSTAIN] = p.env_lfo[1].sustain / 127.0f;
    params_[PARAM_ENV2_RELEASE] = p.env_lfo[1].release / 127.0f;
    params_[PARAM_ENV3_ATTACK] = p.env_lfo[2].attack / 127.0f;
    params_[PARAM_ENV3_DECAY] = p.env_lfo[2].decay / 127.0f;
    params_[PARAM_ENV3_SUSTAIN] = p.env_lfo[2].sustain / 127.0f;
    params_[PARAM_ENV3_RELEASE] = p.env_lfo[2].release / 127.0f;
    params_[PARAM_LFO_SHAPE] = p.voice_lfo_shape / 3.0f;
    params_[PARAM_LFO_RATE] = p.voice_lfo_rate / 127.0f;
    for (int i = 0; i < kModSlots; ++i) {
      int base = PARAM_MOD_SOURCE_0 + i * 3;
      params_[base + 0] = p.modulation[i].source / (float)(kNumModSources - 2);
      params_[base + 1] = p.modulation[i].destination / (float)(kNumModDests - 2);
      params_[base + 2] = (p.modulation[i].amount + 128) / 255.0f;
    }
  }

  void ApplyParam(int param) {
    float v = params_[param];

    switch (param) {
      case PARAM_OSC1_SHAPE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[0].shape = OscShape(v);
        break;
      case PARAM_OSC1_PARAMETER:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[0].parameter = (uint8_t)(v * 127.0f);
        break;
      case PARAM_OSC1_RANGE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[0].range = (int8_t)((v - 0.5f) * 72.0f);
        break;
      case PARAM_OSC1_DETUNE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[0].detune = (int8_t)((v - 0.5f) * 200.0f);
        break;
      case PARAM_OSC2_SHAPE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[1].shape = OscShape(v);
        break;
      case PARAM_OSC2_PARAMETER:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[1].parameter = (uint8_t)(v * 127.0f);
        break;
      case PARAM_OSC2_RANGE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[1].range = (int8_t)((v - 0.5f) * 72.0f);
        break;
      case PARAM_OSC2_DETUNE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().osc[1].detune = (int8_t)((v - 0.5f) * 200.0f);
        break;
      case PARAM_FILTER_CUTOFF:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().filter[0].cutoff = (uint8_t)(v * 127.0f);
        break;
      case PARAM_FILTER_RESONANCE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().filter[0].resonance = (uint8_t)(v * 127.0f);
        break;
      case PARAM_FILTER_MODE:
        { uint8_t m = (uint8_t)(v * 3.0f + 0.5f);
          if (m > 3) m = 3;
          for (int i = 0; i < kPluginVoices; ++i)
            voices_[i].mutable_patch().filter[0].mode = m;
        }
        break;
      case PARAM_FILTER_TYPE:
        { uint8_t t = v > 0.5f ? FILTER_4POLE_LP : FILTER_2POLE_SVF;
          for (int i = 0; i < kPluginVoices; ++i)
            voices_[i].set_filter_type(t);
        }
        break;
      case PARAM_MIX_BALANCE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().mix_balance = (uint8_t)(v * 255.0f);
        break;
      case PARAM_MIX_OPERATOR:
        { uint8_t op = (uint8_t)(v * 5.0f + 0.5f);
          if (op > 5) op = 5;
          for (int i = 0; i < kPluginVoices; ++i)
            voices_[i].mutable_patch().mix_op = op;
        }
        break;
      case PARAM_MIX_SUB_OSC:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().mix_sub_osc = (uint8_t)(v * 127.0f);
        break;
      case PARAM_MIX_NOISE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().mix_noise = (uint8_t)(v * 127.0f);
        break;
      case PARAM_MIX_FUZZ:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().mix_fuzz = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV1_ATTACK:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[0].attack = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV1_DECAY:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[0].decay = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV1_SUSTAIN:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[0].sustain = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV1_RELEASE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[0].release = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV2_ATTACK:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[1].attack = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV2_DECAY:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[1].decay = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV2_SUSTAIN:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[1].sustain = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV2_RELEASE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[1].release = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV3_ATTACK:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[2].attack = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV3_DECAY:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[2].decay = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV3_SUSTAIN:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[2].sustain = (uint8_t)(v * 127.0f);
        break;
      case PARAM_ENV3_RELEASE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().env_lfo[2].release = (uint8_t)(v * 127.0f);
        break;
      case PARAM_LFO_SHAPE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().voice_lfo_shape = (uint8_t)(v * 3.0f);
        break;
      case PARAM_LFO_RATE:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_patch().voice_lfo_rate = (uint8_t)(v * 127.0f);
        break;
      case PARAM_VOLUME:
        master_gain_ = v * 0.5f;
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_part().volume = (uint8_t)(v * 127.0f);
        break;
      case PARAM_PORTAMENTO:
        for (int i = 0; i < kPluginVoices; ++i)
          voices_[i].mutable_part().portamento_time = (uint8_t)(v * 127.0f);
        break;
      default:
        if (param >= PARAM_MOD_SOURCE_0 && param <= PARAM_MOD_AMOUNT_13) {
          int slot = (param - PARAM_MOD_SOURCE_0) / 3;
          int field = (param - PARAM_MOD_SOURCE_0) % 3;
          for (int i = 0; i < kPluginVoices; ++i) {
            if (field == 0)
              voices_[i].mutable_patch().modulation[slot].source = (uint8_t)(v * (kNumModSources - 2));
            else if (field == 1)
              voices_[i].mutable_patch().modulation[slot].destination = (uint8_t)(v * (kNumModDests - 2));
            else
              voices_[i].mutable_patch().modulation[slot].amount = (int8_t)(v * 255.0f - 128.0f);
          }
        }
        break;
    }
  }

  static uint8_t OscShape(float v) {
    uint8_t idx = (uint8_t)(v * (WAVEFORM_LAST - 2));
    return idx;
  }

  Voice voices_[kPluginVoices];
  int note_[kPluginVoices];
  float params_[PARAM_COUNT];
  float master_gain_{0.5f};
  int current_preset_;
};

#endif
