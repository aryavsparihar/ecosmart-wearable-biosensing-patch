#include "window_features.h"

#include "fixed_point.h"

namespace features {

void computeChannel(const int16_t* x, uint8_t n, int32_t* out) {
  int32_t sum = 0;
  int16_t lo = x[0];
  int16_t hi = x[0];
  int32_t weighted = 0;  // sum of (2i - (n - 1)) * x_i
  for (uint8_t i = 0; i < n; ++i) {
    const int16_t v = x[i];
    sum += v;
    if (v < lo) lo = v;
    if (v > hi) hi = v;
    weighted += static_cast<int32_t>(2 * i - (n - 1)) * v;
  }
  const int32_t mean = fx::divRound(sum, n);

  uint32_t sq = 0;
  for (uint8_t i = 0; i < n; ++i) {
    const int32_t d = x[i] - mean;
    sq += static_cast<uint32_t>(d * d);
  }

  out[kMean] = mean;
  out[kVariance] = static_cast<int32_t>((sq + n / 2) / n);
  out[kSlope] = fx::divRound(6 * weighted, static_cast<int32_t>(n) * n - 1);
  out[kMin] = lo;
  out[kMax] = hi;
}

}  // namespace features
