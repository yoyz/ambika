#ifndef AVRLIB_RANDOM_H_
#define AVRLIB_RANDOM_H_

#include "avrlib/base.h"

namespace avrlib {

class Random {
 public:
  static void Update() {
    rng_state_ = (rng_state_ >> 1) ^ (-(rng_state_ & 1) & 0xb400);
  }

  static inline uint16_t state() { return rng_state_; }
  static inline void Seed(uint16_t seed) { rng_state_ = seed; }
  static inline uint8_t state_msb() {
    return static_cast<uint8_t>(rng_state_ >> 8);
  }

  static inline uint8_t GetByte() {
    Update();
    return state_msb();
  }

  static inline uint16_t GetWord() {
    Update();
    return state();
  }

 private:
  static uint16_t rng_state_;
  DISALLOW_COPY_AND_ASSIGN(Random);
};

}  // namespace avrlib

#endif
