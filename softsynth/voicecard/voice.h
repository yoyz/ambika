// Copyright 2011 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------

#ifndef VOICECARD_VOICE_H_
#define VOICECARD_VOICE_H_

#include "voicecard/voicecard.h"

#include "common/lfo.h"
#include "common/patch.h"

#include "voicecard/envelope.h"
#include "voicecard/oscillator.h"
#include "voicecard/sub_oscillator.h"
#include "voicecard/transient_generator.h"
#include "voicecard/filter.h"

namespace ambika {

// Used for MIDI -> oscillator increment conversion.
static const int16_t kLowestNote = 0 * 128;
static const int16_t kHighestNote = 120 * 128;
static const int16_t kOctave = 12 * 128;
static const int16_t kPitchTableStart = 116 * 128;

// This mirrors the beginning of the Part data structure in the controller.
struct Part {
  uint8_t volume;
  uint8_t padding[4];
  uint8_t legato;
  uint8_t portamento_time;
};

class Voice {
 public:
  Voice();
  void Init();

  void Trigger(uint16_t note, uint8_t velocity, uint8_t legato);
  void Release();
  void Kill() { TriggerEnvelope(DEAD); }

  void ProcessBlock();

  void set_filter_type(uint8_t type) {
    filter_.set_type(static_cast<FilterVoicecardType>(type));
  }

  uint8_t cutoff() const {
    return modulation_destinations_[MOD_DST_FILTER_CUTOFF];
  }
  uint8_t vca() const {
    return modulation_destinations_[MOD_DST_VCA];
  }
  uint8_t crush() const {
    return modulation_destinations_[MOD_DST_MIX_CRUSH];
  }
  uint8_t resonance() const {
    return modulation_destinations_[MOD_DST_FILTER_RESONANCE];
  }
  uint8_t modulation_source(uint8_t i) const {
    return modulation_sources_[i];
  }
  uint8_t modulation_destination(uint8_t i) const {
    return modulation_destinations_[i];
  }

  void set_modulation_source(uint8_t i, uint8_t value) {
    modulation_sources_[i] = value;
  }

  void set_patch_data(uint8_t address, uint8_t value) {
    patch_data_[address] = value;
  }
  void set_part_data(uint8_t address, uint8_t value) {
    part_data_[address] = value;
  }
  uint8_t* mutable_patch_data() {
    return patch_data_;
  }

  const Patch& patch() const { return patch_; }
  Patch& mutable_patch() { return patch_; }
  Part& mutable_part() { return part_; }

  Envelope* mutable_envelope(uint8_t i) { return &envelope_[i]; }
  void TriggerEnvelope(uint8_t stage);
  void TriggerEnvelope(uint8_t index, uint8_t stage);

  void ResetAllControllers();

  const uint8_t* output() const { return output_; }

 private:
  inline void LoadSources() __attribute__((always_inline));
  inline void ProcessModulationMatrix() __attribute__((always_inline));
  inline void UpdateDestinations() __attribute__((always_inline));
  inline void RenderOscillators() __attribute__((always_inline));

  uint8_t *patch_data_;
  Patch patch_;
  uint8_t *part_data_;
  Part part_;

  Envelope envelope_[kNumEnvelopes];
  uint8_t gate_;
  Lfo lfo_1_;
  Lfo lfo_2_;
  Lfo lfo_3_;
  Lfo voice_lfo_;

  uint8_t modulation_sources_[kNumModulationSources];
  int8_t modulation_destinations_[kNumModulationDestinations];
  int16_t dst_[kNumModulationDestinations];

  int16_t pitch_increment_;
  int16_t pitch_target_;
  int16_t pitch_value_;

  uint8_t buffer_[kAudioBlockSize];
  uint8_t osc2_buffer_[kAudioBlockSize];
  uint8_t sync_state_[kAudioBlockSize];
  uint8_t no_sync_[kAudioBlockSize];
  uint8_t dummy_sync_state_[kAudioBlockSize];

  uint8_t output_[kAudioBlockSize];

  Oscillator osc_1_;
  Oscillator osc_2_;
  SubOscillator sub_osc_;
  TransientGenerator transient_generator_;
  DigitalFilter filter_;

  DISALLOW_COPY_AND_ASSIGN(Voice);
};

}  // namespace ambika

#endif  // VOICECARD_VOICE_H_
