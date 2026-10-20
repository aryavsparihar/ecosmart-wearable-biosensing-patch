#include "baseline.h"

#include "config.h"
#include "fixed_point.h"
#include "platform.h"

void Baseline::reset() {
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    sum_[i] = 0;
    sumSq_[i] = 0;
    mean_[i] = 0;
    scale_[i] = 1;
  }
  count_ = 0;
  ready_ = false;
}

void Baseline::accumulate(const int32_t* f) {
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    sum_[i] += f[i];
    sumSq_[i] += static_cast<uint64_t>(static_cast<int64_t>(f[i]) * f[i]);
  }
  ++count_;
}

void Baseline::finalize(const int32_t* scaleFloorP) {
  if (count_ == 0) return;
  const int64_t n = count_;
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    mean_[i] = static_cast<int32_t>(fx::divRound64(sum_[i], n));
    int64_t spread = n * static_cast<int64_t>(sumSq_[i]) - sum_[i] * sum_[i];
    if (spread < 0) spread = 0;  // cannot happen mathematically; be safe
    const uint32_t stddev =
        fx::isqrt64(static_cast<uint64_t>(fx::divRound64(spread, n * n)));
    const int32_t floor =
        static_cast<int32_t>(pgm_read_dword(&scaleFloorP[i]));
    int32_t s = stddev > 0x7FFFFFFFUL ? 0x7FFFFFFF : static_cast<int32_t>(stddev);
    if (s < floor) s = floor;
    if (s < 1) s = 1;
    scale_[i] = s;
  }
  ready_ = true;
}

void Baseline::normalize(const int32_t* f, int8_t* z) const {
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    const int64_t d = static_cast<int64_t>(f[i]) - mean_[i];
    const int64_t q = fx::divRound64(d * cfg::kZScale, scale_[i]);
    z[i] = static_cast<int8_t>(fx::clamp64(q, -cfg::kZClamp, cfg::kZClamp));
  }
}
