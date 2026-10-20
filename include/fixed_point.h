// Integer helpers used by the signal-processing core.
//
// The rounding rules here are mirrored bit-for-bit by ml/ecosmart_ml/fixedpoint.py;
// change both together or the parity test will fail.
#pragma once

#include <stdint.h>

namespace fx {

// num / den rounded half away from zero. `den` must be > 0.
inline int32_t divRound(int32_t num, int32_t den) {
  return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den);
}

inline int64_t divRound64(int64_t num, int64_t den) {
  return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den);
}

// floor(sqrt(v)), bit-by-bit; no floating point.
inline uint32_t isqrt64(uint64_t v) {
  uint64_t result = 0;
  uint64_t bit = static_cast<uint64_t>(1) << 62;
  while (bit > v) bit >>= 2;
  while (bit != 0) {
    if (v >= result + bit) {
      v -= result + bit;
      result = (result >> 1) + bit;
    } else {
      result >>= 1;
    }
    bit >>= 2;
  }
  return static_cast<uint32_t>(result);
}

inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

inline int64_t clamp64(int64_t v, int64_t lo, int64_t hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace fx
