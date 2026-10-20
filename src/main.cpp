// Eco-Smart Wearable Biosensing Patch - firmware entry point.
#include <Arduino.h>

#include "config.h"
#include "pins.h"

void setup() {
  Serial.begin(cfg::kSerialBaud);
  Serial.println(F("# EcoSmart patch firmware booting"));
  pinMode(pins::kLedGreen, OUTPUT);
  pinMode(pins::kLedRed, OUTPUT);
}

void loop() {}
