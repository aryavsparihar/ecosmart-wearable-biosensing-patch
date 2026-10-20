// Eco-Smart Wearable Biosensing Patch - firmware entry point.
//
// Samples sweat pH, electrode conductivity and skin temperature at 1 Hz,
// learns the wearer's personal baseline over the first windows, then runs a
// decision tree on every window to flag possible irregularities - entirely on
// the Arduino Nano, no cloud. Output: CSV over serial plus green/red LEDs.
//
// Serial commands: 'r' restarts the personal calibration, 'm' prints free RAM.
#include <Arduino.h>

#include "conductivity_sensor.h"
#include "config.h"
#include "ph_sensor.h"
#include "pins.h"
#include "pipeline.h"
#include "serial_logger.h"
#include "status_leds.h"
#include "temperature_sensor.h"

namespace {

PhSensor phSensor(pins::kPhSensor);
ConductivitySensor condSensor(pins::kConductivity);
TemperatureSensor tempSensor(pins::kTempOneWire);
StatusLeds leds(pins::kLedGreen, pins::kLedRed);
AnomalyPipeline pipeline;

uint32_t lastSampleMs = 0;

void handleSerialCommands() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == 'r' || c == 'R') {
      pipeline.reset();
      Serial.println(F("# event: recalibrating personal baseline"));
    } else if (c == 'm' || c == 'M') {
      logger::printFreeRam();
    }
  }
}

LedMode ledModeFor(const AnomalyPipeline& p) {
  if (!tempSensor.present()) return LedMode::kFault;
  if (p.phase() != Phase::kMonitoring) return LedMode::kCalibrating;
  return p.alertActive() ? LedMode::kAlert : LedMode::kNormal;
}

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
  handleSerialCommands();

  if (now - lastSampleMs < cfg::kSamplePeriodMs) return;
  lastSampleMs += cfg::kSamplePeriodMs;

  LogRecord rec;
  rec.ms = now;
  rec.ph = phSensor.readCenti();
  rec.cond = condSensor.readCenti();
  rec.temp = tempSensor.centi();

  const Phase before = pipeline.phase();
  const bool alertBefore = pipeline.alertActive();
  const WindowResult w = pipeline.push(rec.ph, rec.cond, rec.temp);

  rec.phase = pipeline.phase();
  rec.pAnomaly = pipeline.lastPAnomaly();
  rec.alert = pipeline.alertActive();
  logger::printRecord(rec);

  if (w.ready) logger::printWindow(pipeline.windowCount(), w);
  if (before != Phase::kMonitoring && w.phase == Phase::kMonitoring) {
    Serial.println(F("# event: personal baseline calibrated, monitoring"));
    logger::printFreeRam();
  }
  if (w.alert != alertBefore) {
    Serial.println(w.alert ? F("# event: ALERT - possible irregularity vs baseline")
                           : F("# event: alert cleared"));
  }
  leds.setMode(ledModeFor(pipeline));
}
