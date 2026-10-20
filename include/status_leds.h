// Green / red indicator LEDs (D5 / D6, each through 220 Ohm).
//
//   green blinking (1 Hz) -> warming up / calibrating the personal baseline
//   green steady          -> monitoring, readings within personal baseline
//   red steady            -> possible irregularity flagged
//   both blinking         -> sensor fault (e.g. temperature probe missing)
#pragma once

#include <stdint.h>

enum class LedMode : uint8_t { kCalibrating, kNormal, kAlert, kFault };

class StatusLeds {
 public:
  StatusLeds(uint8_t greenPin, uint8_t redPin);

  void begin();
  void setMode(LedMode mode) { mode_ = mode; }
  LedMode mode() const { return mode_; }
  void update(uint32_t nowMs);  // call every loop() for blinking

 private:
  void write(bool green, bool red);

  uint8_t greenPin_;
  uint8_t redPin_;
  LedMode mode_;
};
