// Oversampled analogRead() to tame ADC noise on the high-impedance inputs.
#pragma once

#include <stdint.h>

// Averages cfg::kAdcOversample consecutive conversions on `pin` (0..1023).
uint16_t readAdcAveraged(uint8_t pin);
