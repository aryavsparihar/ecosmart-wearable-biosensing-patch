// Eco-Smart Wearable Biosensing Patch - firmware entry point.
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println(F("# EcoSmart patch firmware booting"));
}

void loop() {}
