#include "serial_logger.h"

#include <Arduino.h>

// Linker-provided heap bounds (avr-libc).
extern int __heap_start;
extern int* __brkval;

namespace {

// Prints a centi-unit value as a decimal with two places, e.g. 552 -> 5.52.
void printFixed2(int16_t centi) {
  int32_t v = centi;
  if (v < 0) {
    Serial.print('-');
    v = -v;
  }
  Serial.print(static_cast<uint16_t>(v / 100));
  Serial.print('.');
  const uint8_t frac = static_cast<uint8_t>(v % 100);
  if (frac < 10) Serial.print('0');
  Serial.print(frac);
}

void printPhase(Phase phase) {
  switch (phase) {
    case Phase::kWarmup:
      Serial.print(F("WARMUP"));
      break;
    case Phase::kCalibrating:
      Serial.print(F("CALIB"));
      break;
    case Phase::kMonitoring:
      Serial.print(F("MONITOR"));
      break;
  }
}

}  // namespace

namespace logger {

void printBanner() {
  Serial.println(F("# Eco-Smart Wearable Biosensing Patch"));
  Serial.println(F("# Research prototype - NOT a medical device, not for diagnosis."));
#if defined(SIMULATION_MODE)
  Serial.println(F("# SIMULATION_MODE: signals are synthetic"));
#endif
}

void printCsvHeader() {
  Serial.println(F("ms,ph,cond_mScm,temp_C,phase,p_anomaly,alert"));
}

void printRecord(const LogRecord& r) {
  Serial.print(r.ms);
  Serial.print(',');
  printFixed2(r.ph);
  Serial.print(',');
  printFixed2(r.cond);
  Serial.print(',');
  printFixed2(r.temp);
  Serial.print(',');
  printPhase(r.phase);
  Serial.print(',');
  Serial.print(r.pAnomaly);
  Serial.print(',');
  Serial.println(r.alert ? 1 : 0);
}

int freeRam() {
  int top;
  const int heapEnd = __brkval == 0 ? reinterpret_cast<int>(&__heap_start)
                                    : reinterpret_cast<int>(__brkval);
  return reinterpret_cast<int>(&top) - heapEnd;
}

void printFreeRam() {
  Serial.print(F("# free_ram_bytes="));
  Serial.println(freeRam());
}

}  // namespace logger
