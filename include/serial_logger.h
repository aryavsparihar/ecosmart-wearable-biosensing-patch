// Serial CSV logging and diagnostics.
//
// Data rows are plain CSV; everything else (banner, events, RAM reports) is
// prefixed with '#' so tools/serial_plotter.py and spreadsheets can skip it.
// All literals go through F() so they stay in flash, and numbers are
// formatted from fixed-point integers (no float printf, no String).
#pragma once

#include <stdint.h>

#include "phase.h"
#include "pipeline.h"

struct LogRecord {
  uint32_t ms;
  int16_t ph;    // pH x 100
  int16_t cond;  // mS/cm x 100
  int16_t temp;  // degC x 100
  Phase phase;
  uint8_t pAnomaly;  // last window's anomaly probability, percent
  bool alert;
};

namespace logger {

void printBanner();
void printCsvHeader();
void printRecord(const LogRecord& r);
// "# window=12 phase=MONITOR label=1 p=94 leaf=7 alert=1"
void printWindow(uint16_t index, const WindowResult& w);
void printFreeRam();
int freeRam();  // bytes between the heap and the stack

}  // namespace logger
