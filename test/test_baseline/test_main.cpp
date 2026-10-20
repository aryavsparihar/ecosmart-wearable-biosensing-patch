#include <unity.h>

#include "baseline.h"
#include "platform.h"

using features::kNumFeatures;

void setUp() {}
void tearDown() {}

static const int32_t kFloors[kNumFeatures] PROGMEM = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

static void fill(int32_t* f, int32_t v) {
  for (uint8_t i = 0; i < kNumFeatures; ++i) f[i] = v;
}

static void test_not_ready_until_finalized() {
  Baseline b;
  int32_t f[kNumFeatures];
  fill(f, 100);
  b.accumulate(f);
  TEST_ASSERT_FALSE(b.ready());
  TEST_ASSERT_EQUAL_UINT8(1, b.count());
  b.finalize(kFloors);
  TEST_ASSERT_TRUE(b.ready());
}

static void test_mean_and_std() {
  Baseline b;
  int32_t f[kNumFeatures];
  // Values 90 and 110 alternate -> mean 100, population std 10.
  for (uint8_t w = 0; w < 8; ++w) {
    fill(f, (w % 2) ? 110 : 90);
    b.accumulate(f);
  }
  b.finalize(kFloors);
  TEST_ASSERT_EQUAL_INT32(100, b.mean(0));
  TEST_ASSERT_EQUAL_INT32(10, b.scale(0));
}

static void test_floor_applies_to_flat_features() {
  Baseline b;
  int32_t f[kNumFeatures];
  fill(f, 3400);
  for (uint8_t w = 0; w < 8; ++w) b.accumulate(f);
  b.finalize(kFloors);
  TEST_ASSERT_EQUAL_INT32(5, b.scale(3));
}

static void test_normalize_quantises_and_clamps() {
  Baseline b;
  int32_t f[kNumFeatures];
  for (uint8_t w = 0; w < 8; ++w) {
    fill(f, (w % 2) ? 110 : 90);
    b.accumulate(f);
  }
  b.finalize(kFloors);

  fill(f, 100);
  f[0] = 120;    // +2 sigma -> 32
  f[1] = 85;     // -1.5 sigma -> -24
  f[2] = 10000;  // way out -> saturates
  f[3] = -9000;
  int8_t z[kNumFeatures];
  b.normalize(f, z);
  TEST_ASSERT_EQUAL_INT8(32, z[0]);
  TEST_ASSERT_EQUAL_INT8(-24, z[1]);
  TEST_ASSERT_EQUAL_INT8(127, z[2]);
  TEST_ASSERT_EQUAL_INT8(-127, z[3]);
  TEST_ASSERT_EQUAL_INT8(0, z[4]);
}

static void test_reset_clears_calibration() {
  Baseline b;
  int32_t f[kNumFeatures];
  fill(f, 1);
  b.accumulate(f);
  b.finalize(kFloors);
  b.reset();
  TEST_ASSERT_FALSE(b.ready());
  TEST_ASSERT_EQUAL_UINT8(0, b.count());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_not_ready_until_finalized);
  RUN_TEST(test_mean_and_std);
  RUN_TEST(test_floor_applies_to_flat_features);
  RUN_TEST(test_normalize_quantises_and_clamps);
  RUN_TEST(test_reset_clears_calibration);
  return UNITY_END();
}
