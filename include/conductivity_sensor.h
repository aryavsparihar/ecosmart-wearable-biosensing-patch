// Sweat conductivity / moisture channel on A1.
//
// Two copper or aluminium foil electrodes form the upper leg of a voltage
// divider with a fixed series resistor to ground. Dry skin -> open circuit ->
// ~0 V; salty sweat bridging the electrodes -> conductance rises -> V rises.
#pragma once

#include <stdint.h>

#include "sensor_math.h"

class ConductivitySensor {
 public:
  explicit ConductivitySensor(
      uint8_t pin, sensor_math::ConductivityCalibration cal =
                       sensor_math::kDefaultConductivityCalibration);

  void begin();
  int16_t readCenti();  // mS/cm x 100
  uint16_t lastRaw() const { return lastRaw_; }

 private:
  uint8_t pin_;
  sensor_math::ConductivityCalibration cal_;
  uint16_t lastRaw_;
};
