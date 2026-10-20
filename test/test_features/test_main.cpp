#include <unity.h>

#include "window_features.h"

using namespace features;

void setUp() {}
void tearDown() {}

static const uint8_t N = cfg::kWindowSize;

static void test_constant_signal() {
  int16_t x[N];
  for (uint8_t i = 0; i < N; ++i) x[i] = 552;
  int32_t f[kPerChannel];
  computeChannel(x, N, f);
  TEST_ASSERT_EQUAL_INT32(552, f[kMean]);
  TEST_ASSERT_EQUAL_INT32(0, f[kVariance]);
  TEST_ASSERT_EQUAL_INT32(0, f[kSlope]);
  TEST_ASSERT_EQUAL_INT32(552, f[kMin]);
  TEST_ASSERT_EQUAL_INT32(552, f[kMax]);
}

static void test_linear_ramp() {
  int16_t x[N];
  for (uint8_t i = 0; i < N; ++i) x[i] = static_cast<int16_t>(10 * i);
  int32_t f[kPerChannel];
  computeChannel(x, N, f);
  TEST_ASSERT_EQUAL_INT32(155, f[kMean]);
  TEST_ASSERT_EQUAL_INT32(8525, f[kVariance]);  // 100 * (N^2 - 1) / 12
  TEST_ASSERT_EQUAL_INT32(320, f[kSlope]);      // 10 per sample * 32
  TEST_ASSERT_EQUAL_INT32(0, f[kMin]);
  TEST_ASSERT_EQUAL_INT32(310, f[kMax]);
}

static void test_falling_ramp_has_negative_slope() {
  int16_t x[N];
  for (uint8_t i = 0; i < N; ++i) x[i] = static_cast<int16_t>(3700 - 5 * i);
  int32_t f[kPerChannel];
  computeChannel(x, N, f);
  TEST_ASSERT_EQUAL_INT32(-160, f[kSlope]);
}

static void test_alternating_signal_variance() {
  int16_t x[N];
  for (uint8_t i = 0; i < N; ++i) x[i] = (i % 2) ? 600 : 500;
  int32_t f[kPerChannel];
  computeChannel(x, N, f);
  TEST_ASSERT_EQUAL_INT32(550, f[kMean]);
  TEST_ASSERT_EQUAL_INT32(2500, f[kVariance]);
  TEST_ASSERT_EQUAL_INT32(500, f[kMin]);
  TEST_ASSERT_EQUAL_INT32(600, f[kMax]);
}

static void test_extreme_values_do_not_overflow() {
  int16_t x[N];
  for (uint8_t i = 0; i < N; ++i) x[i] = (i < N / 2) ? 0 : cfg::kSampleMax;
  int32_t f[kPerChannel];
  computeChannel(x, N, f);
  TEST_ASSERT_EQUAL_INT32(5000, f[kMean]);
  TEST_ASSERT_EQUAL_INT32(25000000L, f[kVariance]);
  TEST_ASSERT_TRUE(f[kSlope] > 0);
}

static void test_feature_indexing() {
  TEST_ASSERT_EQUAL_UINT8(15, kNumFeatures);
  TEST_ASSERT_EQUAL_UINT8(0, index(cfg::kChPh, kMean));
  TEST_ASSERT_EQUAL_UINT8(7, index(cfg::kChConductivity, kSlope));
  TEST_ASSERT_EQUAL_UINT8(14, index(cfg::kChTemperature, kMax));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_constant_signal);
  RUN_TEST(test_linear_ramp);
  RUN_TEST(test_falling_ramp_has_negative_slope);
  RUN_TEST(test_alternating_signal_variance);
  RUN_TEST(test_extreme_values_do_not_overflow);
  RUN_TEST(test_feature_indexing);
  return UNITY_END();
}
