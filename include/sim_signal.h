// On-device synthetic signal generator used by SIMULATION_MODE builds, so the
// complete firmware (calibration, classifier, alerts, LEDs, CSV) can be
// demonstrated with nothing attached to the Nano. Integer-only, ~20 bytes RAM.
//
// Timeline (in samples, 1 per second by default), repeating every kCycle:
//   [0, kEpisodeStart)        normal: personal baseline + drift + noise
//   [kEpisodeStart, kCycle)   scripted anomaly episode with smooth onset and
//                             recovery; the type rotates every cycle.
// The first episode starts well after the 88-sample personal calibration.
//
// *** Synthetic signals for demonstration only - not physiological data. ***
#pragma once

#include <stdint.h>

enum class SimEpisode : uint8_t {
  kNone = 0,
  kAcidicPh,
  kTemperatureRise,
  kConductivitySurge,
  kAlkalinePh,
  kElectrodeDropout,
};

class SimSignal {
 public:
  static constexpr uint16_t kCycle = 300;
  static constexpr uint16_t kEpisodeStart = 180;
  static constexpr uint8_t kRamp = 16;
  static constexpr uint8_t kNumEpisodeTypes = 5;

  explicit SimSignal(uint32_t seed = 0x5EED2026UL);

  // Produces the next sample in centi-units (pH x100, mS/cm x100, degC x100).
  void next(int16_t* ph, int16_t* cond, int16_t* temp);

  // Episode active for the most recently produced sample.
  SimEpisode episode() const { return episode_; }
  uint32_t samples() const { return t_; }

  // Episode name as a PROGMEM string (print with a __FlashStringHelper cast).
  static const char* name(SimEpisode e);

 private:
  int16_t noise(int16_t amplitude);  // roughly triangular in [-amp, amp]
  uint32_t rand32();

  uint32_t state_;
  uint32_t t_;
  SimEpisode episode_;
};
