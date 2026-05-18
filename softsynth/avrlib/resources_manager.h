#ifndef AVRLIB_RESOURCES_MANAGER_H_
#define AVRLIB_RESOURCES_MANAGER_H_

#include "avrlib/base.h"
#include <string.h>

namespace avrlib {

template<typename ResourceId = uint8_t, typename Tables = void>
class ResourcesManager {
 public:
  template<typename ResultType, typename IndexType>
  static inline ResultType Lookup(const uint8_t* p, IndexType i) {
    return ResultType(p[i]);
  }

  template<typename ResultType, typename IndexType>
  static inline ResultType Lookup(const uint16_t* p, IndexType i) {
    return ResultType(p[i]);
  }

  template<typename T>
  static void Load(const char* p, uint8_t i, T* destination) {
    memcpy(destination, p + i * sizeof(T), sizeof(T));
  }

  template<typename T, typename U>
  static void Load(const T* p, uint8_t i, U* destination) {
    STATIC_ASSERT(sizeof(T) == sizeof(U));
    memcpy(destination, p + i, sizeof(T));
  }

  template<typename T>
  static void Load(const T* p, uint8_t* destination, uint16_t size) {
    memcpy(destination, p, size);
  }
};

}  // namespace avrlib

#endif
