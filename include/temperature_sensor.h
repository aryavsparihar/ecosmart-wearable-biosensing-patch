// DS18B20 skin-temperature channel on D2 (OneWire, 4.7 kOhm pull-up).
//
// A minimal, non-blocking driver: a 12-bit conversion takes up to 750 ms, so
// instead of blocking the 1 Hz loop we start a conversion, return, and collect
// the result on a later update() call. The latest valid value is cached.
// Talking to the sensor directly over OneWire (single device, SKIP ROM) saves
// the flash and RAM the DallasTemperature library would cost.
#pragma once

#include <OneWire.h>
#include <stdint.h>

class TemperatureSensor {
 public:
  explicit TemperatureSensor(uint8_t pin);

  bool begin();                 // true if a device answered the reset pulse
  void update(uint32_t nowMs);  // drive the conversion state machine
  bool present() const { return present_; }
  bool hasReading() const { return valid_; }
  int16_t centi() const { return centi_; }  // degC x 100 (last valid)

 private:
  bool startConversion();
  bool readScratchpad(int16_t* raw);

  OneWire bus_;
  uint32_t requestedAtMs_;
  int16_t centi_;
  bool present_;
  bool converting_;
  bool valid_;
};
