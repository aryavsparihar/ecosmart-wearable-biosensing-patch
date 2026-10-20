// Operating phase of the anomaly-detection pipeline.
#pragma once

#include <stdint.h>

enum class Phase : uint8_t {
  kWarmup = 0,       // filling the first window
  kCalibrating = 1,  // learning the wearer's personal baseline
  kMonitoring = 2,   // classifying windows against the baseline
};
