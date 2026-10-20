#include "baseline.h"

#include "config.h"
#include "fixed_point.h"
#include "platform.h"

void Baseline::reset() {
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    mem_.acc.sum[i] = 0;
    mem_.acc.sumSq[i] = 0;
  }
  count_ = 0;
  ready_ = false;
}

void Baseline::accumulate(const int32_t* f) {
  if (ready_) return;
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    mem_.acc.sum[i] += f[i];
    mem_.acc.sumSq[i] += static_cast<uint64_t>(static_cast<int64_t>(f[i]) * f[i]);
  }
  ++count_;
}

void Baseline::finalize(const int32_t* scaleFloorP) {
  if (count_ == 0 || ready_) return;
  const int64_t n = count_;
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    const int64_t sum = mem_.acc.sum[i];
    const int64_t sumSq = static_cast<int64_t>(mem_.acc.sumSq[i]);
    int64_t spread = n * sumSq - sum * sum;
    if (spread < 0) spread = 0;  // cannot happen mathematically; be safe
    const uint32_t stddev =
        fx::isqrt64(static_cast<uint64_t>(fx::divRound64(spread, n * n)));
    const int32_t floor =
        static_cast<int32_t>(pgm_read_dword(&scaleFloorP[i]));
    int32_t s = stddev > 0x7FFFFFFFUL ? 0x7FFFFFFF : static_cast<int32_t>(stddev);
    if (s < floor) s = floor;
    if (s < 1) s = 1;
    // Overwrites the bytes of sum[i] only; sum[i] and sumSq[i] are read above.
    Stat& st = mem_.stats[i];
    st.mean = static_cast<int32_t>(fx::divRound64(sum, n));
    st.scale = s;
  }
  ready_ = true;
}

void Baseline::normalize(const int32_t* f, int8_t* z) const {
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    const Stat& st = mem_.stats[i];
    const int64_t d = static_cast<int64_t>(f[i]) - st.mean;
    const int64_t q = fx::divRound64(d * cfg::kZScale, st.scale);
    z[i] = static_cast<int8_t>(fx::clamp64(q, -cfg::kZClamp, cfg::kZClamp));
  }
}
