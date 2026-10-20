#include "sensor_math.h"

#include "fixed_point.h"

namespace sensor_math {

int16_t adcToMillivolts(uint16_t adc) {
  if (adc > kAdcMax) adc = kAdcMax;
  return static_cast<int16_t>(
      fx::divRound(static_cast<int32_t>(adc) * kAdcRefMillivolts, kAdcMax));
}

int16_t phCentiFromMillivolts(int16_t millivolts, const PhCalibration& cal) {
  const int32_t delta = static_cast<int32_t>(cal.neutralMv) - millivolts;
  const int32_t ph = 700 + fx::divRound(delta * 100, cal.mvPerPh);
  return static_cast<int16_t>(fx::clamp(ph, 0, 1400));
}

PhCalibration phCalibrationFromTwoPoints(int16_t mvA, int16_t phCentiA,
                                         int16_t mvB, int16_t phCentiB) {
  const int32_t dPh = static_cast<int32_t>(phCentiB) - phCentiA;
  const int32_t dMv = static_cast<int32_t>(mvA) - mvB;
  if (dPh == 0 || dMv == 0) return kDefaultPhCalibration;
  // mv = neutral - (pH - 7) * slope  =>  slope = (mvA - mvB) / (pHB - pHA)
  int32_t slope = fx::divRound(dMv * 100, dPh);
  if (slope <= 0) return kDefaultPhCalibration;  // wrong polarity: ignore
  const int32_t neutral = mvA + fx::divRound((phCentiA - 700) * slope, 100);
  PhCalibration cal = {static_cast<int16_t>(neutral),
                       static_cast<int16_t>(slope)};
  return cal;
}

int16_t conductivityCentiFromAdc(uint16_t adc,
                                 const ConductivityCalibration& cal) {
  if (adc == 0) return 0;  // open circuit: dry or lifted electrodes
  if (adc >= kAdcMax) return 10000;
  // sigma[mS/cm] * 100 = K[0.1/cm] * 1e4 * adc / (R * (1023 - adc))
  const uint32_t num = static_cast<uint32_t>(cal.cellConstantDeci) * 10000UL * adc;
  const uint32_t den =
      static_cast<uint32_t>(cal.seriesResistorOhm) * (kAdcMax - adc);
  const uint32_t sigma = (num + den / 2) / den;
  return static_cast<int16_t>(sigma > 10000UL ? 10000UL : sigma);
}

int16_t ds18b20RawToCenti(int16_t raw) {
  return static_cast<int16_t>(fx::divRound(static_cast<int32_t>(raw) * 25, 4));
}

}  // namespace sensor_math
