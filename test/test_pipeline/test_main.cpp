#include <unity.h>

#include "pipeline.h"

void setUp() {}
void tearDown() {}

static const uint16_t kCalibrationEndSample =
    cfg::kWindowSize + (cfg::kCalibrationWindows - 1) * cfg::kWindowHop;  // 88

// Feeds `n` copies of a sample; returns the number of completed windows.
static uint16_t feed(AnomalyPipeline& p, uint16_t n, int16_t ph, int16_t cond,
                     int16_t temp, bool* anyAlert = 0) {
  uint16_t windows = 0;
  for (uint16_t i = 0; i < n; ++i) {
    WindowResult r = p.push(ph, cond, temp);
    if (r.ready) ++windows;
    if (anyAlert && r.alert) *anyAlert = true;
  }
  return windows;
}

static void test_first_window_after_window_size_samples() {
  AnomalyPipeline p;
  TEST_ASSERT_EQUAL_UINT16(0, feed(p, cfg::kWindowSize - 1, 550, 900, 3400));
  TEST_ASSERT_TRUE(p.phase() == Phase::kWarmup);
  WindowResult r = p.push(550, 900, 3400);
  TEST_ASSERT_TRUE(r.ready);
  TEST_ASSERT_TRUE(r.phase == Phase::kCalibrating);
  TEST_ASSERT_FALSE(r.classified);
}

static void test_windows_every_hop_samples() {
  AnomalyPipeline p;
  feed(p, cfg::kWindowSize, 550, 900, 3400);
  TEST_ASSERT_EQUAL_UINT16(0, feed(p, cfg::kWindowHop - 1, 550, 900, 3400));
  TEST_ASSERT_EQUAL_UINT16(1, feed(p, 1, 550, 900, 3400));
  TEST_ASSERT_EQUAL_UINT16(10, feed(p, 10 * cfg::kWindowHop, 550, 900, 3400));
}

static void test_calibration_completes_after_configured_windows() {
  AnomalyPipeline p;
  feed(p, kCalibrationEndSample - 1, 550, 900, 3400);
  TEST_ASSERT_TRUE(p.phase() == Phase::kCalibrating);
  feed(p, 1, 550, 900, 3400);
  TEST_ASSERT_TRUE(p.phase() == Phase::kMonitoring);
  TEST_ASSERT_TRUE(p.baseline().ready());
  TEST_ASSERT_EQUAL_INT32(550, p.baseline().mean(0));
}

static void test_steady_signal_stays_normal() {
  AnomalyPipeline p;
  bool alert = false;
  feed(p, 600, 550, 900, 3400, &alert);
  TEST_ASSERT_FALSE(alert);
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    TEST_ASSERT_EQUAL_INT8(0, p.lastZ()[i]);
  }
}

static void test_temperature_rise_raises_then_clears_alert() {
  AnomalyPipeline p;
  feed(p, 200, 550, 900, 3400);
  bool alert = false;
  feed(p, 120, 550, 900, 3550, &alert);  // +1.5 degC
  TEST_ASSERT_TRUE(alert);
  TEST_ASSERT_TRUE(p.alertActive());
  feed(p, 200, 550, 900, 3400);  // back to baseline
  TEST_ASSERT_FALSE(p.alertActive());
}

static void test_out_of_range_samples_are_clamped() {
  AnomalyPipeline p;
  feed(p, cfg::kWindowSize, -500, 32000, 3400);
  TEST_ASSERT_EQUAL_INT32(cfg::kSampleMin, p.lastFeatures()[0]);
  TEST_ASSERT_EQUAL_INT32(cfg::kSampleMax, p.lastFeatures()[5]);
}

static void test_reset_restarts_calibration() {
  AnomalyPipeline p;
  feed(p, 300, 550, 900, 3400);
  p.reset();
  TEST_ASSERT_TRUE(p.phase() == Phase::kWarmup);
  TEST_ASSERT_FALSE(p.baseline().ready());
  TEST_ASSERT_EQUAL_UINT16(0, p.windowCount());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_first_window_after_window_size_samples);
  RUN_TEST(test_windows_every_hop_samples);
  RUN_TEST(test_calibration_completes_after_configured_windows);
  RUN_TEST(test_steady_signal_stays_normal);
  RUN_TEST(test_temperature_rise_raises_then_clears_alert);
  RUN_TEST(test_out_of_range_samples_are_clamped);
  RUN_TEST(test_reset_restarts_calibration);
  return UNITY_END();
}
