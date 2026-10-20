// Arduino Nano pin map. Only included by hardware-facing code.
#pragma once

#include <Arduino.h>

namespace pins {

constexpr uint8_t kPhSensor = A0;      // analog pH front-end output
constexpr uint8_t kConductivity = A1;  // electrode / reference-resistor divider
constexpr uint8_t kTempOneWire = 2;    // DS18B20 data line (4.7 kOhm pull-up)
constexpr uint8_t kLedGreen = 5;       // "normal" indicator (220 Ohm)
constexpr uint8_t kLedRed = 6;         // "possible irregularity" (220 Ohm)

}  // namespace pins
