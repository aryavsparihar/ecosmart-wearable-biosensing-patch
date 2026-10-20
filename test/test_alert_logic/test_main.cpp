#include <unity.h>

#include "alert_logic.h"

void setUp() {}
void tearDown() {}

static void test_single_anomaly_does_not_alert() {
  AlertLogic a(2, 2);
  TEST_ASSERT_FALSE(a.update(true));
  TEST_ASSERT_FALSE(a.update(false));
  TEST_ASSERT_FALSE(a.update(true));
  TEST_ASSERT_FALSE(a.active());
}

static void test_consecutive_anomalies_raise_alert() {
  AlertLogic a(2, 2);
  a.update(true);
  TEST_ASSERT_TRUE(a.update(true));
}

static void test_alert_needs_consecutive_normals_to_clear() {
  AlertLogic a(2, 3);
  a.update(true);
  a.update(true);
  TEST_ASSERT_TRUE(a.update(false));
  TEST_ASSERT_TRUE(a.update(true));  // streak broken
  TEST_ASSERT_TRUE(a.update(false));
  TEST_ASSERT_TRUE(a.update(false));
  TEST_ASSERT_FALSE(a.update(false));
}

static void test_immediate_mode() {
  AlertLogic a(1, 1);
  TEST_ASSERT_TRUE(a.update(true));
  TEST_ASSERT_FALSE(a.update(false));
}

static void test_reset() {
  AlertLogic a(1, 1);
  a.update(true);
  a.reset();
  TEST_ASSERT_FALSE(a.active());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_single_anomaly_does_not_alert);
  RUN_TEST(test_consecutive_anomalies_raise_alert);
  RUN_TEST(test_alert_needs_consecutive_normals_to_clear);
  RUN_TEST(test_immediate_mode);
  RUN_TEST(test_reset);
  return UNITY_END();
}
