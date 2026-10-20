#include "analog_input.h"

#include <Arduino.h>

#include "config.h"

uint16_t readAdcAveraged(uint8_t pin) {
  uint16_t sum = 0;  // 8 x 1023 fits comfortably in 16 bits
  for (uint8_t i = 0; i < cfg::kAdcOversample; ++i) sum += analogRead(pin);
  return static_cast<uint16_t>((sum + cfg::kAdcOversample / 2) / cfg::kAdcOversample);
}
