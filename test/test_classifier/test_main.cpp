#include <unity.h>

#include "classifier.h"
#include "config.h"
#include "features.h"
#include "platform.h"

using features::index;
using features::kNumFeatures;

void setUp() {}
void tearDown() {}

// z[0] <= 10 ? (z[10] <= -5 ? leafA : leafB) : leafC
static const ModelNode kToyTree[] PROGMEM = {
    {0, 10, 1, 4},
    {10, -5, 2, 3},
    {kLeafNode, 0, 80, 1},
    {kLeafNode, 0, 3, 0},
    {kLeafNode, 0, 97, 1},
};

static void zeros(int8_t* z) {
  for (uint8_t i = 0; i < kNumFeatures; ++i) z[i] = 0;
}

static void test_toy_tree_routes_on_threshold_boundary() {
  int8_t z[kNumFeatures];
  zeros(z);
  z[0] = 10;  // equal to threshold -> left
  Prediction p = classifier::predictWith(kToyTree, 5, z);
  TEST_ASSERT_EQUAL_UINT8(3, p.leaf);
  TEST_ASSERT_EQUAL_UINT8(0, p.label);
  TEST_ASSERT_EQUAL_UINT8(3, p.pAnomaly);

  z[0] = 11;
  p = classifier::predictWith(kToyTree, 5, z);
  TEST_ASSERT_EQUAL_UINT8(4, p.leaf);
  TEST_ASSERT_EQUAL_UINT8(1, p.label);
  TEST_ASSERT_EQUAL_UINT8(97, p.pAnomaly);
}

static void test_toy_tree_second_level() {
  int8_t z[kNumFeatures];
  zeros(z);
  z[10] = -5;
  Prediction p = classifier::predictWith(kToyTree, 5, z);
  TEST_ASSERT_EQUAL_UINT8(2, p.leaf);
  TEST_ASSERT_EQUAL_UINT8(1, p.label);
}

static void test_corrupt_tree_terminates() {
  static const ModelNode kLoop[] PROGMEM = {{0, 0, 1, 1}, {0, 0, 0, 0}};
  int8_t z[kNumFeatures];
  zeros(z);
  Prediction p = classifier::predictWith(kLoop, 2, z);
  TEST_ASSERT_EQUAL_UINT8(0xFF, p.leaf);
  TEST_ASSERT_EQUAL_UINT8(0, p.label);
}

// Sanity checks on the generated model: a window identical to the personal
// baseline is normal; a large deviation on any channel, either way, is not.
static void test_model_baseline_window_is_normal() {
  int8_t z[kNumFeatures];
  zeros(z);
  TEST_ASSERT_EQUAL_UINT8(0, classifier::predict(z).label);
}

static void expect_flagged(uint8_t channel, int8_t value) {
  int8_t z[kNumFeatures];
  zeros(z);
  z[index(channel, features::kMean)] = value;
  z[index(channel, features::kMin)] = value;
  z[index(channel, features::kMax)] = value;
  TEST_ASSERT_EQUAL_UINT8(1, classifier::predict(z).label);
}

static void test_model_flags_large_deviations() {
  expect_flagged(cfg::kChPh, -127);
  expect_flagged(cfg::kChPh, 127);
  expect_flagged(cfg::kChConductivity, -127);
  expect_flagged(cfg::kChConductivity, 127);
  expect_flagged(cfg::kChTemperature, 127);
}

static void test_scale_floors_are_positive() {
  const int32_t* floors = classifier::scaleFloors();
  for (uint8_t i = 0; i < kNumFeatures; ++i) {
    TEST_ASSERT_TRUE(static_cast<int32_t>(pgm_read_dword(&floors[i])) >= 1);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_toy_tree_routes_on_threshold_boundary);
  RUN_TEST(test_toy_tree_second_level);
  RUN_TEST(test_corrupt_tree_terminates);
  RUN_TEST(test_model_baseline_window_is_normal);
  RUN_TEST(test_model_flags_large_deviations);
  RUN_TEST(test_scale_floors_are_positive);
  return UNITY_END();
}
