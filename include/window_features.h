// Per-window time-series features, computed with integer arithmetic only.
//
// For each channel (pH, conductivity, temperature) over one window of n
// samples (centi-units, already clamped to [0, 10000]):
//   mean      round(sum / n)
//   variance  round(sum((x - mean)^2) / n)              (population, centi^2)
//   slope     least-squares slope x n, i.e. the fitted change across one
//             window length: round(6 * sum(w_i * x_i) / (n^2 - 1)),
//             with w_i = 2i - (n - 1)
//   min, max
//
// Overflow bounds for n = 32: |x - mean| <= 1e4 -> sum of squares <= 3.2e9
// (fits uint32); sum(|w_i|) * 1e4 * 6 ~= 3.1e7 (fits int32).
// ml/ecosmart_ml/features.py reproduces this exactly.
#pragma once

#include <stdint.h>

#include "config.h"

namespace features {

enum Kind : uint8_t { kMean = 0, kVariance = 1, kSlope = 2, kMin = 3, kMax = 4 };
constexpr uint8_t kPerChannel = 5;
constexpr uint8_t kNumFeatures = cfg::kNumChannels * kPerChannel;

constexpr uint8_t index(uint8_t channel, Kind kind) {
  return static_cast<uint8_t>(channel * kPerChannel + kind);
}

// Writes the kPerChannel features of one channel to out[0..4]. Requires n >= 2.
void computeChannel(const int16_t* x, uint8_t n, int32_t* out);

}  // namespace features
