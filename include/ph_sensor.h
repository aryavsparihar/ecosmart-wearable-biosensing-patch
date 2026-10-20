// pH channel on A0.
//
// The paper prototype reads pH visually from universal-indicator strips; this
// driver is for an analog pH front-end (glass/ISFET probe + amplifier board)
// wired to A0, or a potentiometer standing in for one in the Wokwi simulation.
#pragma once

#include <stdint.h>

#include "sensor_math.h"

class PhSensor {
 public:
  explicit PhSensor(uint8_t pin,
                    sensor_math::PhCalibration cal = sensor_math::kDefaultPhCalibration);

  void begin();
  int16_t readCenti();  // pH x 100
  uint16_t lastRaw() const { return lastRaw_; }
  void setCalibration(const sensor_math::PhCalibration& cal) { cal_ = cal; }

 private:
  uint8_t pin_;
  sensor_math::PhCalibration cal_;
  uint16_t lastRaw_;
};
