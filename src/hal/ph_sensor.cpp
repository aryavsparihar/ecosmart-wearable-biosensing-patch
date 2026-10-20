#include "ph_sensor.h"

#include <Arduino.h>

#include "analog_input.h"

PhSensor::PhSensor(uint8_t pin, sensor_math::PhCalibration cal)
    : pin_(pin), cal_(cal), lastRaw_(0) {}

void PhSensor::begin() { pinMode(pin_, INPUT); }

int16_t PhSensor::readCenti() {
  lastRaw_ = readAdcAveraged(pin_);
  return sensor_math::phCentiFromMillivolts(sensor_math::adcToMillivolts(lastRaw_), cal_);
}
