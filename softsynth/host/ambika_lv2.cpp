// Ambika Softsynth — LV2 plugin
// Build: g++ -std=c++11 -O3 -fPIC -shared -I. ambika_lv2.cpp \
//        ../voicecard/voice.cc ../voicecard/oscillator.cc \
//        ../voicecard/resources.cc ../avrlib/random.cc \
//        $(pkg-config --cflags lv2) -o ambika.so \
//        -DDISABLE_WAVETABLE_LFOS

#include <lv2/core/lv2.h>
#include <lv2/midi/midi.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>
#include <lv2/parameters/parameters.h>

#include <cstdlib>
#include <cstring>

#include "plugin_shared.h"

// ---------------------------------------------------------------------------
// URIs — maps URIs to integers at runtime
// ---------------------------------------------------------------------------
enum URIs {
  URD_MIDI_EVENT = 0,
  URI_ATOM_SEQUENCE,
  URI_PARAM_GAIN,
  URI_COUNT
};

static const char* uri_map[] = {
  LV2_MIDI__MidiEvent,
  LV2_ATOM__Sequence,
  LV2_PARAMETERS__gain,
};

struct AmbikaLV2 {
  PluginVoiceManager synth;
  LV2_URID_Map* map;
  LV2_Atom_Sequence* midi_input;
  float* output_left;
  float* output_right;
  float* params[PARAM_COUNT];
  bool active;
};

// ---------------------------------------------------------------------------
// LV2 plugin implementation
// ---------------------------------------------------------------------------
static LV2_Handle Instantiate(const LV2_Descriptor* descriptor,
                               double rate, const char* bundle_path,
                               const LV2_Feature* const* features) {
  (void)descriptor; (void)bundle_path;

  AmbikaLV2* self = new AmbikaLV2();

  // Get URID map feature
  for (int i = 0; features && features[i]; ++i) {
    if (strcmp(features[i]->URI, LV2_URID__map) == 0) {
      self->map = (LV2_URID_Map*)features[i]->data;
    }
  }

  self->active = false;

  // Process one warm-up block to initialise the engine
  float dummy;
  self->output_left = &dummy;
  self->output_right = &dummy;
  for (int i = 0; i < PARAM_COUNT; ++i) {
    static float def[PARAM_COUNT];
    def[i] = PluginVoiceManager::GetDefault(i);
    self->params[i] = &def[i];
  }

  return (LV2_Handle)self;
}

static void ConnectPort(LV2_Handle instance, uint32_t port, void* data) {
  AmbikaLV2* self = (AmbikaLV2*)instance;

  // Port layout:
  // 0: MIDI input (atom sequence)
  // 1: Audio output left
  // 2: Audio output right
  // 3..3+PARAM_COUNT-1: Control inputs

  if (port == 0) {
    self->midi_input = (LV2_Atom_Sequence*)data;
  } else if (port == 1) {
    self->output_left = (float*)data;
  } else if (port == 2) {
    self->output_right = (float*)data;
  } else if (port >= 3 && port < (uint32_t)(3 + PARAM_COUNT)) {
    self->params[port - 3] = (float*)data;
  }
}

static void Activate(LV2_Handle instance) {
  AmbikaLV2* self = (AmbikaLV2*)instance;
  self->active = true;
}

static void Deactivate(LV2_Handle instance) {
  AmbikaLV2* self = (AmbikaLV2*)instance;
  self->active = false;
}

static void Run(LV2_Handle instance, uint32_t nframes) {
  AmbikaLV2* self = (AmbikaLV2*)instance;
  if (!self->active) return;

  // Read parameters
  for (int i = 0; i < PARAM_COUNT; ++i) {
    if (self->params[i]) {
      self->synth.SetParam(i, *self->params[i]);
    }
  }

  // Process MIDI events
  if (self->midi_input) {
    LV2_ATOM_SEQUENCE_FOREACH(self->midi_input, ev) {
      if (ev->body.type == self->map->map(self->map->handle, LV2_MIDI__MidiEvent)) {
        const uint8_t* msg = (const uint8_t*)(ev + 1);
        uint8_t status = msg[0] & 0xF0;
        uint8_t channel = msg[0] & 0x0F;

        if (status == 0x90 && msg[2] > 0) {  // Note On
          self->synth.NoteOn(channel, msg[1], msg[2]);
        } else if (status == 0x80 || (status == 0x90 && msg[2] == 0)) {  // Note Off
          self->synth.NoteOff(channel, msg[1], msg[2]);
        } else if (status == 0xE0) {  // Pitch Bend
          int16_t bend = ((int16_t)(msg[2] << 7 | msg[1])) - 8192;
          self->synth.PitchBend(channel, bend);
        } else if (status == 0xB0) {  // CC
          if (msg[1] == 0x01) {  // Modulation wheel
            self->synth.Modulation(channel, msg[2]);
          } else if (msg[1] == 0x7B) {  // All notes off
            self->synth.AllNotesOff();
          } else if (msg[1] == 0x78) {  // All sound off
            self->synth.AllSoundOff();
          }
        }
      }
    }
  }

  // Render audio
  float* outputs[2] = { self->output_left, self->output_right };
  self->synth.ProcessBlock(outputs, 2, nframes);
}

static void Cleanup(LV2_Handle instance) {
  delete (AmbikaLV2*)instance;
}

// ---------------------------------------------------------------------------
// LV2 descriptor
// ---------------------------------------------------------------------------
static const LV2_Descriptor descriptor = {
  "https://ambika.opencode/softsynth",
  Instantiate,
  ConnectPort,
  Activate,
  Run,
  Deactivate,
  Cleanup,
  NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  return index == 0 ? &descriptor : NULL;
}
