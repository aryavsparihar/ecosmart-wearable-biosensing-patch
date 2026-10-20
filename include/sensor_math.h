// Pure conversion maths from raw readings to fixed-point physical units.
// Kept free of Arduino calls so it can be unit-tested on the host.
#pragma once

#include <stdint.h>

namespace sensor_math {

constexpr uint16_t kAdcMax = 1023;        // 10-bit ADC full scale
constexpr int32_t kAdcRefMillivolts = 5000;  // AVcc reference on the Nano

// Analog pH front-end (e.g. a PH-4502C style board): output is `neutralMv`
// at pH 7 and falls by `mvPerPh` for every pH unit above 7.
struct PhCalibration {
  int16_t neutralMv;
  int16_t mvPerPh;
};
constexpr PhCalibration kDefaultPhCalibration = {2500, 177};

// Electrode divider: 5 V -> electrodes (R_sweat) -> A1 -> R_series -> GND.
// Conductivity = cell constant x conductance of the sweat path.
struct ConductivityCalibration {
  uint16_t seriesResistorOhm;
  uint16_t cellConstantDeci;  // cell constant in 0.1 cm^-1
};
constexpr ConductivityCalibration kDefaultConductivityCalibration = {1000, 100};

int16_t adcToMillivolts(uint16_t adc);

// Returns pH x 100, clamped to [0, 1400].
int16_t phCentiFromMillivolts(int16_t millivolts, const PhCalibration& cal);

// Two-point calibration from buffer readings, e.g. pH 4.00 and pH 9.00 buffers.
// pH values are x100. Returns the default calibration if the points coincide.
PhCalibration phCalibrationFromTwoPoints(int16_t mvA, int16_t phCentiA,
                                         int16_t mvB, int16_t phCentiB);

// Returns conductivity in mS/cm x 100, clamped to [0, 10000].
int16_t conductivityCentiFromAdc(uint16_t adc,
                                 const ConductivityCalibration& cal);

// DS18B20 12-bit raw reading (1/16 degC per LSB) to degC x 100.
int16_t ds18b20RawToCenti(int16_t raw);

}  // namespace sensor_math
