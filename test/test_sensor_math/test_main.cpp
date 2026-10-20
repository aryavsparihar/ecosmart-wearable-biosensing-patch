#include <unity.h>

#include "sensor_math.h"

using namespace sensor_math;

void setUp() {}
void tearDown() {}

static void test_adc_to_millivolts_endpoints() {
  TEST_ASSERT_EQUAL_INT16(0, adcToMillivolts(0));
  TEST_ASSERT_EQUAL_INT16(5000, adcToMillivolts(1023));
  TEST_ASSERT_EQUAL_INT16(2502, adcToMillivolts(512));
}

static void test_ph_neutral_and_slope() {
  TEST_ASSERT_EQUAL_INT16(700, phCentiFromMillivolts(2500, kDefaultPhCalibration));
  // 177 mV above neutral -> one pH unit more acidic.
  TEST_ASSERT_EQUAL_INT16(600, phCentiFromMillivolts(2677, kDefaultPhCalibration));
  TEST_ASSERT_EQUAL_INT16(800, phCentiFromMillivolts(2323, kDefaultPhCalibration));
}

static void test_ph_is_clamped() {
  TEST_ASSERT_EQUAL_INT16(1400, phCentiFromMillivolts(0, kDefaultPhCalibration));
  TEST_ASSERT_EQUAL_INT16(0, phCentiFromMillivolts(5000, kDefaultPhCalibration));
}

static void test_two_point_calibration_round_trips() {
  // Synthetic probe: 2450 mV at pH 7, 165 mV/pH.
  const int16_t mv4 = 2450 + 3 * 165;  // pH 4.00
  const int16_t mv9 = 2450 - 2 * 165;  // pH 9.00
  PhCalibration cal = phCalibrationFromTwoPoints(mv4, 400, mv9, 900);
  TEST_ASSERT_EQUAL_INT16(165, cal.mvPerPh);
  TEST_ASSERT_EQUAL_INT16(2450, cal.neutralMv);
  TEST_ASSERT_EQUAL_INT16(400, phCentiFromMillivolts(mv4, cal));
  TEST_ASSERT_EQUAL_INT16(900, phCentiFromMillivolts(mv9, cal));
}

static void test_two_point_calibration_rejects_degenerate_input() {
  PhCalibration cal = phCalibrationFromTwoPoints(2500, 700, 2500, 700);
  TEST_ASSERT_EQUAL_INT16(kDefaultPhCalibration.neutralMv, cal.neutralMv);
  TEST_ASSERT_EQUAL_INT16(kDefaultPhCalibration.mvPerPh, cal.mvPerPh);
}

static void test_conductivity_divider() {
  const ConductivityCalibration& cal = kDefaultConductivityCalibration;
  TEST_ASSERT_EQUAL_INT16(0, conductivityCentiFromAdc(0, cal));
  TEST_ASSERT_EQUAL_INT16(10000, conductivityCentiFromAdc(1023, cal));
  // R_sweat == R_series -> 1 mS conductance -> 10 mS/cm with K = 10 /cm.
  TEST_ASSERT_INT16_WITHIN(5, 1000, conductivityCentiFromAdc(511, cal));
  // Artificial sweat (~9 mS/cm) sits mid-scale.
  TEST_ASSERT_INT16_WITHIN(5, 901, conductivityCentiFromAdc(485, cal));
}

static void test_conductivity_is_monotonic() {
  int16_t prev = -1;
  for (uint16_t adc = 0; adc <= 1023; ++adc) {
    int16_t v = conductivityCentiFromAdc(adc, kDefaultConductivityCalibration);
    TEST_ASSERT_TRUE(v >= prev);
    prev = v;
  }
}

static void test_ds18b20_conversion() {
  TEST_ASSERT_EQUAL_INT16(2500, ds18b20RawToCenti(0x0190));   // +25.0000
  TEST_ASSERT_EQUAL_INT16(3406, ds18b20RawToCenti(0x0221));   // +34.0625
  TEST_ASSERT_EQUAL_INT16(-1006, ds18b20RawToCenti(-161));    // -10.0625
  TEST_ASSERT_EQUAL_INT16(8500, ds18b20RawToCenti(0x0550));   // power-on value
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_adc_to_millivolts_endpoints);
  RUN_TEST(test_ph_neutral_and_slope);
  RUN_TEST(test_ph_is_clamped);
  RUN_TEST(test_two_point_calibration_round_trips);
  RUN_TEST(test_two_point_calibration_rejects_degenerate_input);
  RUN_TEST(test_conductivity_divider);
  RUN_TEST(test_conductivity_is_monotonic);
  RUN_TEST(test_ds18b20_conversion);
  return UNITY_END();
}
