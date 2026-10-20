#include <unity.h>

#include "fixed_point.h"

void setUp() {}
void tearDown() {}

static void test_div_round_positive() {
  TEST_ASSERT_EQUAL_INT32(3, fx::divRound(5, 2));   // 2.5 rounds away from 0
  TEST_ASSERT_EQUAL_INT32(4, fx::divRound(7, 2));   // 3.5
  TEST_ASSERT_EQUAL_INT32(2, fx::divRound(9, 4));   // 2.25
  TEST_ASSERT_EQUAL_INT32(-3, fx::divRound(-5, 2)); // -2.5
}

static void test_div_round_negative_is_symmetric() {
  for (int32_t n = -1000; n <= 1000; ++n) {
    TEST_ASSERT_EQUAL_INT32(-fx::divRound(-n, 7), fx::divRound(n, 7));
  }
}

static void test_div_round64_matches_32bit() {
  for (int32_t n = -5000; n <= 5000; n += 13) {
    TEST_ASSERT_EQUAL_INT64(fx::divRound(n, 9), fx::divRound64(n, 9));
  }
}

static void test_isqrt_exact_and_floor() {
  TEST_ASSERT_EQUAL_UINT32(0, fx::isqrt64(0));
  TEST_ASSERT_EQUAL_UINT32(1, fx::isqrt64(3));
  TEST_ASSERT_EQUAL_UINT32(2, fx::isqrt64(4));
  TEST_ASSERT_EQUAL_UINT32(10000, fx::isqrt64(100000000ULL));
  TEST_ASSERT_EQUAL_UINT32(9999, fx::isqrt64(99999999ULL));
  TEST_ASSERT_EQUAL_UINT32(4294967295UL, fx::isqrt64(18446744073709551615ULL));
}

static void test_clamp() {
  TEST_ASSERT_EQUAL_INT32(-127, fx::clamp(-500, -127, 127));
  TEST_ASSERT_EQUAL_INT32(127, fx::clamp(500, -127, 127));
  TEST_ASSERT_EQUAL_INT32(42, fx::clamp(42, -127, 127));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_div_round_positive);
  RUN_TEST(test_div_round_negative_is_symmetric);
  RUN_TEST(test_div_round64_matches_32bit);
  RUN_TEST(test_isqrt_exact_and_floor);
  RUN_TEST(test_clamp);
  return UNITY_END();
}
