#ifndef AVRLIB_OP_H_
#define AVRLIB_OP_H_

#include "avrlib/base.h"

namespace avrlib {

static inline int16_t Clip(int16_t value, int16_t min, int16_t max) {
  return value < min ? min : (value > max ? max : value);
}

static inline int16_t S16ClipU14(int16_t value) {
  uint8_t msb = static_cast<uint16_t>(value) >> 8;
  if (msb & 0x80) return 0;
  if (msb & 0x40) return 16383;
  return value;
}

static inline uint8_t U8AddClip(uint8_t value, uint8_t increment, uint8_t max) {
  value += increment;
  if (value > max) value = max;
  return value;
}

static inline uint8_t S16ShiftRight8(int16_t value) {
  return static_cast<uint16_t>(value) >> 8;
}

static inline uint24c_t U24AddC(uint24c_t a, uint24_t b) {
  uint32_t av = static_cast<uint32_t>(a.integral) << 8 | a.fractional;
  uint32_t bv = static_cast<uint32_t>(b.integral) << 8 | b.fractional;
  uint32_t sum = av + bv;
  uint24c_t result;
  result.integral = sum >> 8;
  result.fractional = sum & 0xff;
  result.carry = (sum >> 24) != 0;
  return result;
}

static inline uint24_t U24Add(uint24_t a, uint24_t b) {
  uint32_t av = static_cast<uint32_t>(a.integral) << 8 | a.fractional;
  uint32_t bv = static_cast<uint32_t>(b.integral) << 8 | b.fractional;
  uint32_t sum = av + bv;
  uint24_t result;
  result.integral = sum >> 8;
  result.fractional = sum & 0xff;
  return result;
}

static inline uint24_t U24Sub(uint24_t a, uint24_t b) {
  uint32_t av = static_cast<uint32_t>(a.integral) << 8 | a.fractional;
  uint32_t bv = static_cast<uint32_t>(b.integral) << 8 | b.fractional;
  uint32_t diff = av - bv;
  uint24_t result;
  result.integral = diff >> 8;
  result.fractional = diff & 0xff;
  return result;
}

static inline uint24_t U24ShiftRight(uint24_t a) {
  uint32_t v = (static_cast<uint32_t>(a.integral) << 8) | a.fractional;
  v >>= 1;
  uint24_t result;
  result.integral = v >> 8;
  result.fractional = v & 0xff;
  return result;
}

static inline uint24_t U24ShiftLeft(uint24_t a) {
  uint32_t v = (static_cast<uint32_t>(a.integral) << 8) | a.fractional;
  v <<= 1;
  uint24_t result;
  result.integral = v >> 8;
  result.fractional = v & 0xff;
  return result;
}

static inline uint8_t S16ClipU8(int16_t value) {
  return value < 0 ? 0 : (value > 255 ? 255 : static_cast<uint8_t>(value));
}

static inline int8_t S16ClipS8(int16_t value) {
  return value < -128 ? -128 : (value > 127 ? 127 : static_cast<int8_t>(value));
}

static inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t balance) {
  return (a * (255 - balance) + b * balance) >> 8;
}

static inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t gain_a, uint8_t gain_b) {
  return (a * gain_a + b * gain_b) >> 8;
}

static inline int8_t S8Mix(int8_t a, int8_t b, uint8_t gain_a, uint8_t gain_b) {
  return (a * gain_a + b * gain_b) >> 8;
}

static inline uint16_t U8MixU16(uint8_t a, uint8_t b, uint8_t balance) {
  return a * (255 - balance) + b * balance;
}

static inline uint8_t U8U4MixU8(uint8_t a, uint8_t b, uint8_t balance) {
  return (a * (15 - balance) + b * balance) >> 4;
}

static inline uint16_t U8U4MixU12(uint8_t a, uint8_t b, uint8_t balance) {
  return a * (15 - balance) + b * balance;
}

static inline uint8_t U8ShiftRight4(uint8_t a) { return a >> 4; }
static inline uint8_t U8ShiftLeft4(uint8_t a) { return a << 4; }
static inline uint8_t U8Swap4(uint8_t a) { return (a << 4) | (a >> 4); }

static inline uint8_t U8U8MulShift8(uint8_t a, uint8_t b) {
  return (a * b) >> 8;
}

static inline int8_t S8U8MulShift8(int8_t a, uint8_t b) {
  return (a * b) >> 8;
}

static inline int16_t S8U8Mul(int8_t a, uint8_t b) { return a * b; }
static inline int16_t S8S8Mul(int8_t a, int8_t b) { return a * b; }
static inline uint16_t U8U8Mul(uint8_t a, uint8_t b) { return a * b; }

static inline int8_t S8S8MulShift8(int8_t a, int8_t b) {
  return (a * b) >> 8;
}

static inline uint8_t U14ShiftRight6(uint16_t value) { return value >> 6; }
static inline uint8_t U15ShiftRight7(uint16_t value) { return value >> 7; }
static inline uint16_t U16ShiftRight4(uint16_t a) { return a >> 4; }

static inline int16_t S16U16MulShift16(int16_t a, uint16_t b) {
  return (static_cast<int32_t>(a) * static_cast<uint32_t>(b)) >> 16;
}

static inline int16_t S16U8MulShift8(int16_t a, uint8_t b) {
  return (static_cast<int32_t>(a) * static_cast<uint32_t>(b)) >> 8;
}

static inline uint16_t U16U8MulShift8(uint16_t a, uint8_t b) {
  return (static_cast<uint32_t>(a) * static_cast<uint32_t>(b)) >> 8;
}

static inline uint16_t U16U16MulShift16(uint16_t a, uint16_t b) {
  return (static_cast<uint32_t>(a) * static_cast<uint32_t>(b)) >> 16;
}

static inline uint8_t InterpolateSample(const uint8_t* table, uint16_t phase) {
  uint8_t idx = phase >> 8;
  uint8_t a = pgm_read_byte(table + idx);
  uint8_t b = idx < 255 ? pgm_read_byte(table + idx + 1) : a;
  return U8Mix(a, b, phase & 0xff);
}

}  // namespace avrlib

#endif
