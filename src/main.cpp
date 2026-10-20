// Eco-Smart Wearable Biosensing Patch - firmware entry point.
//
// Samples the sweat pH, electrode conductivity and skin temperature channels
// at a fixed rate and streams them as CSV over the serial port.
#include <Arduino.h>

#include "conductivity_sensor.h"
#include "config.h"
#include "pins.h"
#include "ph_sensor.h"
#include "serial_logger.h"
#include "status_leds.h"
#include "temperature_sensor.h"

namespace {

PhSensor phSensor(pins::kPhSensor);
ConductivitySensor condSensor(pins::kConductivity);
TemperatureSensor tempSensor(pins::kTempOneWire);
StatusLeds leds(pins::kLedGreen, pins::kLedRed);

uint32_t lastSampleMs = 0;

}  // namespace

void setup() {
  Serial.begin(cfg::kSerialBaud);
  logger::printBanner();

  phSensor.begin();
  condSensor.begin();
  if (!tempSensor.begin()) Serial.println(F("# warn: DS18B20 not found on D2"));
  leds.begin();

  logger::printFreeRam();
  logger::printCsvHeader();
  lastSampleMs = millis();
}

void loop() {
  const uint32_t now = millis();
  tempSensor.update(now);
  leds.update(now);

  if (now - lastSampleMs < cfg::kSamplePeriodMs) return;
  lastSampleMs += cfg::kSamplePeriodMs;

  LogRecord rec;
  rec.ms = now;
  rec.ph = phSensor.readCenti();
  rec.cond = condSensor.readCenti();
  rec.temp = tempSensor.centi();
  rec.phase = Phase::kWarmup;
  rec.pAnomaly = 0;
  rec.alert = false;
  logger::printRecord(rec);

  leds.setMode(tempSensor.present() ? LedMode::kCalibrating : LedMode::kFault);
}
