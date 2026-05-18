#ifndef AVRLIB_BASE_H_
#define AVRLIB_BASE_H_

#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#define NULL 0
#endif

typedef union {
  uint16_t value;
  uint8_t bytes[2];
} Word;

typedef union {
  uint32_t value;
  uint16_t words[2];
  uint8_t bytes[4];
} LongWord;

struct uint24_t {
  uint16_t integral;
  uint8_t fractional;
};

struct uint24c_t {
  uint8_t carry;
  uint16_t integral;
  uint8_t fractional;
};

template<uint32_t a, uint32_t b, uint32_t c, uint32_t d>
struct FourCC {
  static const uint32_t value = (((((d << 8) | c) << 8) | b) << 8) | a;
};

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

template<bool b>
inline void StaticAssertImplementation() {
  char static_assert_size_mismatch[b] = { 0 };
}

#define STATIC_ASSERT(expression) StaticAssertImplementation<(expression)>()

// Stub progmem types for desktop
#define PROGMEM
#define prog_uint8_t uint8_t
#define prog_uint16_t uint16_t
#define prog_char char
#define pgm_read_byte(x) (*(const uint8_t*)(x))
#define pgm_read_word(x) (*(const uint16_t*)(x))

#endif
