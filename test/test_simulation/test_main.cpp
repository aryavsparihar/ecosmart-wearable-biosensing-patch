// End-to-end check on the host: the SIMULATION_MODE signal generator driven
// through the real pipeline and the generated model, exactly as on the Nano.
#include <unity.h>

#include "pipeline.h"
#include "sim_signal.h"

void setUp() {}
void tearDown() {}

static const uint16_t kCycles = 2 * SimSignal::kNumEpisodeTypes;
// After an episode ends, overlapping windows plus the 2-window release
// hysteresis may keep the alert up for a little while.
static const uint16_t kReleaseSlack = cfg::kWindowSize + 3 * cfg::kWindowHop;

static void test_generator_is_deterministic_and_in_range() {
  SimSignal a, b;
  for (uint16_t i = 0; i < 2000; ++i) {
    int16_t p1, c1, t1, p2, c2, t2;
    a.next(&p1, &c1, &t1);
    b.next(&p2, &c2, &t2);
    TEST_ASSERT_EQUAL_INT16(p1, p2);
    TEST_ASSERT_EQUAL_INT16(c1, c2);
    TEST_ASSERT_TRUE(p1 >= 0 && p1 <= 1400);
    TEST_ASSERT_TRUE(c1 >= 0 && c1 <= 10000);
    TEST_ASSERT_TRUE(t1 > 3000 && t1 < 3800);
  }
}

static void test_schedule_rotates_episode_types() {
  SimSignal s;
  int16_t p, c, t;
  for (uint16_t i = 0; i < SimSignal::kEpisodeStart; ++i) s.next(&p, &c, &t);
  TEST_ASSERT_TRUE(s.episode() == SimEpisode::kNone);
  s.next(&p, &c, &t);
  TEST_ASSERT_TRUE(s.episode() == SimEpisode::kAcidicPh);
  for (uint16_t i = 0; i < SimSignal::kCycle; ++i) s.next(&p, &c, &t);
  TEST_ASSERT_TRUE(s.episode() == SimEpisode::kTemperatureRise);
}

static void test_every_episode_alerts_and_normal_periods_stay_quiet() {
  SimSignal sim;
  AnomalyPipeline pipe;
  bool alertedThisEpisode = false;
  uint16_t detected = 0, falseAlertSamples = 0;
  for (uint32_t i = 0; i < static_cast<uint32_t>(kCycles) * SimSignal::kCycle; ++i) {
    int16_t p, c, t;
    sim.next(&p, &c, &t);
    pipe.push(p, c, t);
    const uint16_t inCycle = static_cast<uint16_t>(i % SimSignal::kCycle);
    if (sim.episode() != SimEpisode::kNone) {
      if (pipe.alertActive()) alertedThisEpisode = true;
      if (inCycle == SimSignal::kCycle - 1) {
        if (alertedThisEpisode) ++detected;
        else TEST_MESSAGE(SimSignal::name(sim.episode()));
        alertedThisEpisode = false;
      }
    } else if (inCycle >= kReleaseSlack && pipe.alertActive()) {
      ++falseAlertSamples;
    }
  }
  TEST_ASSERT_EQUAL_UINT16(kCycles, detected);
  TEST_ASSERT_EQUAL_UINT16(0, falseAlertSamples);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_generator_is_deterministic_and_in_range);
  RUN_TEST(test_schedule_rotates_episode_types);
  RUN_TEST(test_every_episode_alerts_and_normal_periods_stay_quiet);
  return UNITY_END();
}
