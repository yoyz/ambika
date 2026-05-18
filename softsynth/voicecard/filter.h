// Digital filter emulations for the Ambika softsynth.
// TPT state-variable filter topology (Zavalishin).

#ifndef VOICECARD_FILTER_H_
#define VOICECARD_FILTER_H_

#include "avrlib/base.h"
#include "common/patch.h"
#include "voicecard/voicecard.h"
#include <cmath>

namespace ambika {

enum FilterVoicecardType {
  FILTER_2POLE_SVF,
  FILTER_4POLE_LP,
};

class DigitalFilter {
 public:
  DigitalFilter() { Init(); }

  void Init() {
    lp1_ = bp1_ = lp2_ = bp2_ = 0.0f;
    g_ = 0.0f;
    r_ = 1.0f;
    type_ = FILTER_2POLE_SVF;
    mode_ = FILTER_MODE_LP;
  }

  void set_type(FilterVoicecardType t) { type_ = t; }
  void set_mode(uint8_t m) { mode_ = m; }
  int type() const { return type_; }

  void set_cutoff(uint8_t cutoff, uint8_t resonance) {
    static const float kFs = 31250.0f;
    static const float kPi = 3.14159265f;

    // Map 0-127 → 20Hz .. ~7.5kHz (g stays < 1 for TPT stability)
    float cutoff_norm = cutoff / 127.0f;
    float freq_hz = 20.0f + (kFs * 0.24f - 20.0f) * cutoff_norm;

    g_ = tanf(kPi * freq_hz / kFs);
    if (g_ > 0.95f) g_ = 0.95f;

    // Map resonance 0-127 to Q = 0.5..15 → r = 1/(2*Q) = 1.0..0.033
    float q = 0.5f + (resonance / 127.0f) * 14.5f;
    r_ = 1.0f / (2.0f * q);
    if (r_ > 1.0f) r_ = 1.0f;
    if (r_ < 0.02f) r_ = 0.02f;
  }

  void ProcessBlock(const uint8_t* input, uint8_t* output, uint8_t block_size) {
    if (type_ == FILTER_4POLE_LP)
      Process4PoleLP(input, output, block_size);
    else
      Process2PoleSVF(input, output, block_size);
  }

  void Reset() { lp1_ = bp1_ = lp2_ = bp2_ = 0.0f; }

 private:
  void Process2PoleSVF(const uint8_t* input, uint8_t* output, uint8_t n) {
    float g = g_;
    float r = r_;
    float g1 = g / (1.0f + g * (g + r));
    float r1 = r + g;

    for (uint8_t i = 0; i < n; ++i) {
      float in = (input[i] - 128.0f) / 128.0f;
      float hp = (in - r1 * bp1_ - lp1_) * g1;
      float bp = bp1_ + g * hp;
      float lp = lp1_ + g * bp;
      bp1_ = bp;
      lp1_ = lp;

      float out;
      switch (mode_) {
        case FILTER_MODE_LP:    out = lp;  break;
        case FILTER_MODE_BP:    out = bp;  break;
        case FILTER_MODE_HP:    out = hp;  break;
        case FILTER_MODE_NOTCH: out = lp + hp;  break;
        default:                out = in;  break;
      }

      float scaled = out * 128.0f + 128.0f;
      if (scaled < 0.0f) scaled = 0.0f;
      if (scaled > 255.0f) scaled = 255.0f;
      output[i] = static_cast<uint8_t>(scaled + 0.5f);
    }
  }

  void Process4PoleLP(const uint8_t* input, uint8_t* output, uint8_t n) {
    float g = g_;
    float r1 = r_;
    float r2 = r_ * 0.7f + 0.1f;
    if (r2 > 1.0f) r2 = 1.0f;
    float g1 = g / (1.0f + g * (g + r1));
    float g2 = g / (1.0f + g * (g + r2));
    float ra1 = r1 + g;
    float ra2 = r2 + g;

    for (uint8_t i = 0; i < n; ++i) {
      float in = (input[i] - 128.0f) / 128.0f;
      float hp1 = (in - ra1 * bp1_ - lp1_) * g1;
      float bp1 = bp1_ + g * hp1;
      float lp1 = lp1_ + g * bp1;
      bp1_ = bp1;
      lp1_ = lp1;
      float hp2 = (lp1 - ra2 * bp2_ - lp2_) * g2;
      float bp2 = bp2_ + g * hp2;
      float lp2 = lp2_ + g * bp2;
      bp2_ = bp2;
      lp2_ = lp2;
      float scaled = lp2 * 128.0f + 128.0f;
      if (scaled < 0.0f) scaled = 0.0f;
      if (scaled > 255.0f) scaled = 255.0f;
      output[i] = static_cast<uint8_t>(scaled + 0.5f);
    }
  }

  float lp1_, bp1_, lp2_, bp2_;
  float g_, r_;
  FilterVoicecardType type_;
  uint8_t mode_;
};

}  // namespace ambika

#endif
