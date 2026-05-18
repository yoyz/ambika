// PluginVoiceManager standalone test — validates MIDI handling,
// parameter routing, and audio output without requiring LV2/VST3 SDKs.

#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

#include "../host/plugin_shared.h"

// ---------------------------------------------------------------------------
// Simple test framework
// ---------------------------------------------------------------------------
static int g_tests = 0, g_passed = 0;

#define TEST(name) static void test_##name()
#define CHECK(cond, fmt, ...) do { \
  g_tests++; \
  if (!(cond)) { \
    printf("  FAIL: " fmt "\n", ##__VA_ARGS__); \
  } else { \
    g_passed++; \
  } \
} while(0)

static double ComputeRMS(const float* samples, int n) {
  double sum = 0;
  for (int i = 0; i < n; ++i) sum += samples[i] * samples[i];
  return sqrt(sum / n);
}

// ---------------------------------------------------------------------------
// Test: PluginVoiceManager MIDI note produces audio
// ---------------------------------------------------------------------------
TEST(midi_note) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);
  vm.ProcessBlock(outputs, 2, 256);
  double rms = ComputeRMS(left, 256);
  CHECK(rms > 0.001, "NoteOn should produce audio (RMS=%.6f)", rms);

  vm.NoteOff(0, 60, 0);
}

// ---------------------------------------------------------------------------
// Test: Silence when no notes playing
// ---------------------------------------------------------------------------
TEST(silence) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.ProcessBlock(outputs, 2, 256);
  double rms = ComputeRMS(left, 256);
  CHECK(rms < 0.001, "No notes should silence (RMS=%.6f)", rms);
}

// ---------------------------------------------------------------------------
// Test: Note off stops audio (via Kill)
// ---------------------------------------------------------------------------
TEST(note_off_stops) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);
  vm.ProcessBlock(outputs, 2, 256);
  double rms_on = ComputeRMS(left, 256);
  CHECK(rms_on > 0.001, "Note on should be audible (RMS=%.6f)", rms_on);

  // Kill = immediate silence (envelope to DEAD)
  vm.AllSoundOff();
  vm.ProcessBlock(outputs, 2, 256);
  double rms_off = ComputeRMS(left, 256);
  CHECK(rms_off < rms_on * 0.1,
        "Kill silences voice (on=%.6f off=%.6f)", rms_on, rms_off);
}

// ---------------------------------------------------------------------------
// Test: Pitch bend changes output
// ---------------------------------------------------------------------------
TEST(pitch_bend) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);

  // Bend down to -8192 (minimum)
  vm.PitchBend(0, -8192);
  vm.ProcessBlock(outputs, 2, 256);
  double rms_min = ComputeRMS(left, 256);
  CHECK(rms_min > 0.001, "Bend -8192 audible (RMS=%.6f)", rms_min);

  // Bend up to +8191 (maximum)
  vm.PitchBend(0, 8191);
  vm.ProcessBlock(outputs, 2, 256);
  double rms_max = ComputeRMS(left, 256);
  CHECK(rms_max > 0.001, "Bend +8191 audible (RMS=%.6f)", rms_max);

  // Center (no bend)
  vm.PitchBend(0, 0);
  vm.ProcessBlock(outputs, 2, 256);
  double rms_center = ComputeRMS(left, 256);
  CHECK(rms_center > 0.001, "Bend center audible (RMS=%.6f)", rms_center);

  // Verify min and max produce different output from center
  float diff_min = 0, diff_max = 0;
  for (int i = 0; i < 256; ++i) {
    diff_min += fabsf(left[i] - (i < 256 ? left[i] : 0));
  }
  // Re-render center reference
  vm.PitchBend(0, 0);
  vm.ProcessBlock(outputs, 2, 256);
  float ref[256];
  memcpy(ref, left, sizeof(ref));

  vm.PitchBend(0, -8192);
  vm.ProcessBlock(outputs, 2, 256);
  for (int i = 0; i < 256; ++i) diff_min += fabsf(left[i] - ref[i]);
  diff_min /= 256;

  vm.PitchBend(0, 8191);
  vm.ProcessBlock(outputs, 2, 256);
  for (int i = 0; i < 256; ++i) diff_max += fabsf(left[i] - ref[i]);
  diff_max /= 256;

  CHECK(diff_min > 0.0001f, "Min bend changes output (diff=%.6f)", diff_min);
  CHECK(diff_max > 0.0001f, "Max bend changes output (diff=%.6f)", diff_max);

  vm.NoteOff(0, 60, 0);
}

// ---------------------------------------------------------------------------
// Test: Parameter changes affect output
// ---------------------------------------------------------------------------
TEST(parameters) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);

  // Render with default params
  float ref[256];
  vm.ProcessBlock(outputs, 2, 256);
  memcpy(ref, left, sizeof(ref));

  // Change cutoff and re-render
  vm.SetParam(PARAM_FILTER_CUTOFF, 0.0f);
  vm.ProcessBlock(outputs, 2, 256);

  float diff = 0;
  for (int i = 0; i < 256; ++i)
    diff += fabs(left[i] - ref[i]);
  diff /= 256;

  CHECK(diff > 0.0001f, "Cutoff change alters output (diff=%.6f)", diff);

  // Verify param get/set round-trips
  vm.SetParam(PARAM_ENV2_RELEASE, 0.25f);
  float v = vm.GetParam(PARAM_ENV2_RELEASE);
  CHECK(fabs(v - 0.25f) < 0.01f, "Release param get/set (got=%.4f)", v);

  vm.NoteOff(0, 60, 0);
}

// ---------------------------------------------------------------------------
// Test: Polyphonic — multiple notes simultaneously
// ---------------------------------------------------------------------------
TEST(polyphony) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);
  vm.NoteOn(0, 64, 100);
  vm.NoteOn(0, 67, 100);
  vm.ProcessBlock(outputs, 2, 256);
  double rms_chord = ComputeRMS(left, 256);

  // Compare with single note (separate instance)
  PluginVoiceManager vm2;
  vm2.NoteOn(0, 60, 100);
  vm2.ProcessBlock(outputs, 2, 256);
  double rms_single = ComputeRMS(left, 256);

  CHECK(rms_chord > rms_single * 1.2f,
        "Chord louder than single (chord=%.6f single=%.6f)",
        rms_chord, rms_single);

  vm.AllNotesOff();
}

// ---------------------------------------------------------------------------
// Test: All sound off / panic
// ---------------------------------------------------------------------------
TEST(panic) {
  PluginVoiceManager vm;
  float left[256], right[256];
  float* outputs[2] = { left, right };

  vm.NoteOn(0, 60, 100);
  vm.ProcessBlock(outputs, 2, 256);
  CHECK(ComputeRMS(left, 256) > 0.001, "Note should play");

  vm.AllSoundOff();
  vm.ProcessBlock(outputs, 2, 256);
  double rms = ComputeRMS(left, 256);
  CHECK(rms < 0.01, "AllSoundOff drops output (RMS=%.6f)", rms);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
  printf("\n  PluginVoiceManager Test Suite\n");
  printf("  ===============================\n\n");

  test_midi_note();
  test_silence();
  test_note_off_stops();
  test_pitch_bend();
  test_parameters();
  test_polyphony();
  test_panic();

  printf("\n  ===============================\n");
  printf("  Results: %d/%d passed, %d failed\n\n",
         g_passed, g_tests, g_tests - g_passed);
  return g_passed == g_tests ? 0 : 1;
}
