// Ambika Softsynth - Host Application
// Converts the Ambika voicecard firmware into a polyphonic software synthesizer.
//
// Build: g++ -std=c++03 -O3 -I.. main.cc ../voicecard/resources.cc \
//        ../voicecard/oscillator.cc ../voicecard/voice.cc ../avrlib/random.cc \
//        -o ambika_softsynth -DDISABLE_WAVETABLE_LFOS

#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

#include "voicecard/voice.h"
#include "voicecard/voicecard.h"

using namespace ambika;

// -----------------------------------------------------------------------------
// Simple WAV writer
// -----------------------------------------------------------------------------

struct WavWriter {
  FILE* f;
  long data_size_pos;
  uint32_t n_samples;

  WavWriter() : f(0), data_size_pos(0), n_samples(0) {}

  bool Open(const char* path, uint32_t sample_rate, uint16_t num_channels) {
    f = fopen(path, "wb");
    if (!f) return false;

    // RIFF header
    fwrite("RIFF", 1, 4, f);
    uint32_t dummy = 0;
    fwrite(&dummy, 4, 1, f);  // total size placeholder
    fwrite("WAVE", 1, 4, f);

    // fmt chunk
    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    uint16_t audio_fmt = 1;  // PCM
    uint16_t channels = num_channels;
    uint32_t srate = sample_rate;
    uint16_t bits_per_sample = 16;
    uint16_t block_align = channels * (bits_per_sample / 8);
    uint32_t byte_rate = srate * block_align;

    fwrite(&fmt_size, 4, 1, f);
    fwrite(&audio_fmt, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&srate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    // data chunk header
    fwrite("data", 1, 4, f);
    data_size_pos = ftell(f);
    fwrite(&dummy, 4, 1, f);  // data size placeholder

    return true;
  }

  void WriteSample(int16_t sample) {
    fwrite(&sample, 2, 1, f);
    n_samples++;
  }

  void Close() {
    if (!f) return;
    // Fix up sizes
    uint32_t data_bytes = n_samples * 2;
    uint32_t total_bytes = 36 + data_bytes;

    fseek(f, 4, SEEK_SET);
    fwrite(&total_bytes, 4, 1, f);

    fseek(f, data_size_pos, SEEK_SET);
    fwrite(&data_bytes, 4, 1, f);

    fclose(f);
    f = 0;
  }
};

// -----------------------------------------------------------------------------
// Simple polyphonic voice manager
// -----------------------------------------------------------------------------

static const int kNumVoices = 6;

class VoiceManager {
 public:
  VoiceManager() {
    for (int i = 0; i < kNumVoices; ++i) {
      voices_[i].Init();
      note_[i] = -1;
    }
  }

  int AllocateVoice() {
    // Find a voice currently not sounding (gate off and envelope dead)
    for (int i = 0; i < kNumVoices; ++i) {
      if (note_[i] < 0) return i;
    }
    // Steal: find the oldest sounding voice
    int oldest = 0;
    for (int i = 1; i < kNumVoices; ++i) {
      if (note_start_[i] < note_start_[oldest]) oldest = i;
    }
    return oldest;
  }

  int FindVoice(int midi_note) {
    for (int i = 0; i < kNumVoices; ++i) {
      if (note_[i] == midi_note) return i;
    }
    return -1;
  }

  void NoteOn(int midi_note, int velocity) {
    // Check if note already playing
    int v = FindVoice(midi_note);
    if (v < 0) {
      v = AllocateVoice();
    }
    // Convert MIDI note to 14-bit pitch (7.7 fixed point)
    uint16_t pitch = midi_note * 128;
    // Retrigger for hardware compatibility
    uint8_t vel = velocity << 1;
    if (velocity > 127) vel = 255;
    voices_[v].Trigger(pitch, vel, 0);
    note_[v] = midi_note;
    note_start_[v] = time_;
  }

  void NoteOff(int midi_note) {
    for (int i = 0; i < kNumVoices; ++i) {
      if (note_[i] == midi_note) {
        voices_[i].Release();
        note_[i] = -1;
      }
    }
  }

  void AllNotesOff() {
    for (int i = 0; i < kNumVoices; ++i) {
      voices_[i].Release();
      note_[i] = -1;
    }
  }

  Voice& voice(int i) { return voices_[i]; }

  void ProcessBlock() {
    for (int i = 0; i < kNumVoices; ++i) {
      voices_[i].ProcessBlock();
    }
    time_++;
  }

  bool IsActive(int i) const {
    return note_[i] >= 0 ||
           voices_[i].modulation_destination(MOD_DST_VCA) > 2;
  }

 private:
  Voice voices_[kNumVoices];
  int note_[kNumVoices];
  int note_start_[kNumVoices];
  int time_;
};

// -----------------------------------------------------------------------------
// Test pattern: a simple melody + chords
// -----------------------------------------------------------------------------

struct Event {
  enum Type { NOTE_ON, NOTE_OFF, TEMPO };
  Type type;
  int note;
  int velocity;
  int delay_samples;  // for TEMPO events: BPM
};

// Simple test pattern
static void BuildTestPattern(std::vector<Event>& events, int sample_rate) {
  int pos = 0;
  int beat_samples = sample_rate * 60 / 120;  // 120 BPM

  // C major scale ascending
  int scale[] = { 60, 62, 64, 65, 67, 69, 71, 72 };
  int num_scale = sizeof(scale) / sizeof(scale[0]);

  for (int i = 0; i < num_scale; ++i) {
    events.push_back({Event::NOTE_ON, scale[i], 100, pos});
    pos += beat_samples / 4;
    events.push_back({Event::NOTE_OFF, scale[i], 0, pos});
    pos += beat_samples / 8;
  }

  // Hold last note a bit
  pos += beat_samples / 4;

  // C major chord arpeggio
  int chord[] = { 60, 64, 67, 72, 67, 64, 60, 64 };
  int num_chord = sizeof(chord) / sizeof(chord[0]);
  for (int i = 0; i < num_chord; ++i) {
    events.push_back({Event::NOTE_ON, chord[i], 100, pos});
    pos += beat_samples / 8;
    events.push_back({Event::NOTE_OFF, chord[i], 0, pos});
    pos += beat_samples / 16;
  }

  // Chord stabs
  pos += beat_samples / 4;
  int stab[] = { 60, 64, 67 };  // C major
  for (int i = 0; i < 3; ++i) {
    events.push_back({Event::NOTE_ON, stab[0], 90, pos});
    events.push_back({Event::NOTE_ON, stab[1], 90, pos});
    events.push_back({Event::NOTE_ON, stab[2], 90, pos});
    pos += beat_samples / 2;
    events.push_back({Event::NOTE_OFF, stab[0], 0, pos});
    events.push_back({Event::NOTE_OFF, stab[1], 0, pos});
    events.push_back({Event::NOTE_OFF, stab[2], 0, pos});
    pos += beat_samples / 4;
  }
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  const int sample_rate = 31250;
  const int block_size = kAudioBlockSize;  // 40

  const char* output_path = "ambika_test.wav";
  if (argc > 1) output_path = argv[1];

  // Build test events
  std::vector<Event> events;
  BuildTestPattern(events, sample_rate);

  // Sort by sample position
  // (already in order from BuildTestPattern, but just in case)

  // Open WAV
  WavWriter wav;
  if (!wav.Open(output_path, sample_rate, 1)) {
    fprintf(stderr, "Failed to open %s\n", output_path);
    return 1;
  }

  VoiceManager* vm = new VoiceManager();
  int event_idx = 0;
  int total_samples = events.back().delay_samples + sample_rate * 2;  // +2s tail

  fprintf(stderr, "Rendering %d samples (%d voices)...\n",
          total_samples, kNumVoices);

  // Process events and render audio
  for (int pos = 0; pos < total_samples; pos += block_size) {
    // Dispatch events up to current sample position
    while (event_idx < (int)events.size() &&
           events[event_idx].delay_samples <= pos) {
      const Event& e = events[event_idx];
      if (e.type == Event::NOTE_ON) {
        vm->NoteOn(e.note, e.velocity);
      } else if (e.type == Event::NOTE_OFF) {
        vm->NoteOff(e.note);
      }
      event_idx++;
    }

    // Render one block from all voices
    vm->ProcessBlock();

    // Mix voices to float buffer and output
    float mix_buffer[block_size];
    memset(mix_buffer, 0, sizeof(mix_buffer));

    for (int v = 0; v < kNumVoices; ++v) {
      const uint8_t* samples = vm->voice(v).output();
      for (int i = 0; i < block_size; ++i) {
        // Convert 8-bit unsigned (128=0) to float, scale by volume
        float f = (samples[i] - 128.0f) / 128.0f;
        mix_buffer[i] += f;
      }
    }

    // Write to WAV (clip to 16-bit range)
    for (int i = 0; i < block_size; ++i) {
      float sample = mix_buffer[i];
      // Apply a master volume
      sample *= 0.5f;
      if (sample > 1.0f) sample = 1.0f;
      if (sample < -1.0f) sample = -1.0f;
      int16_t out = static_cast<int16_t>(sample * 32767.0f);
      wav.WriteSample(out);
    }
  }

  wav.Close();
  delete vm;
  fprintf(stderr, "Done! Wrote %s\n", output_path);
  return 0;
}
