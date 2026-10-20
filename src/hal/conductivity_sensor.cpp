#include "conductivity_sensor.h"

#include <Arduino.h>

#include "analog_input.h"

ConductivitySensor::ConductivitySensor(uint8_t pin,
                                       sensor_math::ConductivityCalibration cal)
    : pin_(pin), cal_(cal), lastRaw_(0) {}

void ConductivitySensor::begin() { pinMode(pin_, INPUT); }

int16_t ConductivitySensor::readCenti() {
  lastRaw_ = readAdcAveraged(pin_);
  return sensor_math::conductivityCentiFromAdc(lastRaw_, cal_);
}
