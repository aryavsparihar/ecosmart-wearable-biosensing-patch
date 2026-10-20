// Streaming anomaly-detection pipeline, independent of any hardware:
//
//   push(sample) -> 3 ring buffers -> every kWindowHop samples once full:
//   features -> (first kCalibrationWindows windows) personal baseline
//            -> (afterwards) z-scores -> decision tree -> alert hysteresis
//
// main.cpp feeds it real or simulated samples; the native tests and the
// Python parity vectors drive it directly. Mirrors ml/ecosmart_ml/pipeline.py.
#pragma once

#include <stdint.h>

#include "alert_logic.h"
#include "baseline.h"
#include "classifier.h"
#include "config.h"
#include "features.h"
#include "phase.h"
#include "ring_buffer.h"

struct WindowResult {
  bool ready;            // true if this sample completed a window
  Phase phase;           // phase after the window was processed
  bool classified;       // true if `prediction` is valid (monitoring phase)
  Prediction prediction;
  bool alert;            // debounced alert state
};

class AnomalyPipeline {
 public:
  AnomalyPipeline();

  void reset();  // restart warm-up and personal calibration
  WindowResult push(int16_t ph, int16_t cond, int16_t temp);

  Phase phase() const { return phase_; }
  bool alertActive() const { return alert_.active(); }
  uint8_t lastPAnomaly() const { return lastPrediction_.pAnomaly; }
  uint16_t windowCount() const { return windows_; }

  // Introspection for tests / parity checks (valid after a ready window).
  const int32_t* lastFeatures() const { return features_; }
  const int8_t* lastZ() const { return z_; }
  const Baseline& baseline() const { return baseline_; }

 private:
  RingBuffer<int16_t, cfg::kWindowSize> buffers_[cfg::kNumChannels];
  Baseline baseline_;
  AlertLogic alert_;
  int32_t features_[features::kNumFeatures];
  int8_t z_[features::kNumFeatures];
  Prediction lastPrediction_;
  uint16_t windows_;
  uint8_t sinceLastWindow_;
  bool firstWindowDone_;
  Phase phase_;
};
