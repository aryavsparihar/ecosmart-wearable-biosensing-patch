// Personal baseline: learns how *this wearer's* features behave at rest
// during the first cfg::kCalibrationWindows windows, then expresses every new
// window as quantised z-scores relative to that baseline.
//
//   mean_i  = round(sum_i / n)
//   std_i   = isqrt(round((n * sumsq_i - sum_i^2) / n^2))
//   scale_i = max(std_i, floor_i, 1)
//   z_i     = clamp(round((f_i - mean_i) * kZScale / scale_i), -127, 127)
//
// floor_i comes from the trained model (model_weights.h): the typical normal
// variability of that feature around a wearer's own baseline. A two-minute
// calibration cannot observe slow physiological drift, so without the floor a
// quiet calibration would turn ordinary drift into saturated z-scores.
// Mirrored by ml/ecosmart_ml/pipeline.py.
#pragma once

#include <stdint.h>

#include "features.h"

class Baseline {
 public:
  Baseline() { reset(); }

  void reset();
  void accumulate(const int32_t* f);  // add one calibration window
  uint8_t count() const { return count_; }

  // Freezes the baseline. `scaleFloorP` points to kNumFeatures int32 values
  // stored in PROGMEM.
  void finalize(const int32_t* scaleFloorP);
  bool ready() const { return ready_; }

  void normalize(const int32_t* f, int8_t* z) const;

  int32_t mean(uint8_t i) const { return mean_[i]; }
  int32_t scale(uint8_t i) const { return scale_[i]; }

 private:
  int64_t sum_[features::kNumFeatures];
  uint64_t sumSq_[features::kNumFeatures];
  int32_t mean_[features::kNumFeatures];
  int32_t scale_[features::kNumFeatures];
  uint8_t count_;
  bool ready_;
};
