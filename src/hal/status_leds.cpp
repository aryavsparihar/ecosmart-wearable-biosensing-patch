#include "status_leds.h"

#include <Arduino.h>

StatusLeds::StatusLeds(uint8_t greenPin, uint8_t redPin)
    : greenPin_(greenPin), redPin_(redPin), mode_(LedMode::kCalibrating) {}

void StatusLeds::begin() {
  pinMode(greenPin_, OUTPUT);
  pinMode(redPin_, OUTPUT);
  write(false, false);
}

void StatusLeds::update(uint32_t nowMs) {
  const bool blinkOn = (nowMs / 500) % 2 == 0;
  switch (mode_) {
    case LedMode::kCalibrating:
      write(blinkOn, false);
      break;
    case LedMode::kNormal:
      write(true, false);
      break;
    case LedMode::kAlert:
      write(false, true);
      break;
    case LedMode::kFault:
      write(blinkOn, blinkOn);
      break;
  }
}

void StatusLeds::write(bool green, bool red) {
  digitalWrite(greenPin_, green ? HIGH : LOW);
  digitalWrite(redPin_, red ? HIGH : LOW);
}
