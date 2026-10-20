#include <unity.h>

#include "ring_buffer.h"

void setUp() {}
void tearDown() {}

static void test_starts_empty() {
  RingBuffer<int16_t, 4> rb;
  TEST_ASSERT_TRUE(rb.empty());
  TEST_ASSERT_FALSE(rb.full());
  TEST_ASSERT_EQUAL_UINT8(0, rb.size());
  TEST_ASSERT_EQUAL_UINT8(4, rb.capacity());
}

static void test_push_until_full_keeps_order() {
  RingBuffer<int16_t, 4> rb;
  for (int16_t v = 1; v <= 4; ++v) rb.push(v);
  TEST_ASSERT_TRUE(rb.full());
  for (uint8_t i = 0; i < 4; ++i) TEST_ASSERT_EQUAL_INT16(i + 1, rb.at(i));
  TEST_ASSERT_EQUAL_INT16(4, rb.newest());
}

static void test_overwrites_oldest_when_full() {
  RingBuffer<int16_t, 4> rb;
  for (int16_t v = 1; v <= 7; ++v) rb.push(v);
  TEST_ASSERT_EQUAL_UINT8(4, rb.size());
  const int16_t expected[4] = {4, 5, 6, 7};
  int16_t out[4];
  rb.copyTo(out);
  TEST_ASSERT_EQUAL_INT16_ARRAY(expected, out, 4);
}

static void test_partial_fill_indexing() {
  RingBuffer<int16_t, 8> rb;
  rb.push(10);
  rb.push(20);
  rb.push(30);
  TEST_ASSERT_EQUAL_UINT8(3, rb.size());
  TEST_ASSERT_EQUAL_INT16(10, rb.at(0));
  TEST_ASSERT_EQUAL_INT16(30, rb.at(2));
  TEST_ASSERT_EQUAL_INT16(30, rb.newest());
}

static void test_wraps_many_times() {
  RingBuffer<int16_t, 32> rb;
  for (int16_t v = 0; v < 1000; ++v) rb.push(v);
  for (uint8_t i = 0; i < 32; ++i) TEST_ASSERT_EQUAL_INT16(968 + i, rb.at(i));
}

static void test_clear_resets_state() {
  RingBuffer<int16_t, 4> rb;
  rb.push(1);
  rb.push(2);
  rb.clear();
  TEST_ASSERT_TRUE(rb.empty());
  rb.push(9);
  TEST_ASSERT_EQUAL_INT16(9, rb.at(0));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_empty);
  RUN_TEST(test_push_until_full_keeps_order);
  RUN_TEST(test_overwrites_oldest_when_full);
  RUN_TEST(test_partial_fill_indexing);
  RUN_TEST(test_wraps_many_times);
  RUN_TEST(test_clear_resets_state);
  return UNITY_END();
}
