// Build-time configuration shared by the firmware and the native tests.
//
// Fixed-point convention: every physiological channel is carried as a signed
// 16-bit integer in hundredths of its physical unit ("centi-units"):
//   pH            -> pH x 100          (e.g. 552  = pH 5.52)
//   conductivity  -> mS/cm x 100       (e.g. 901  = 9.01 mS/cm)
//   temperature   -> degC x 100        (e.g. 3421 = 34.21 degC)
// The Python pipeline in ml/ uses the exact same convention.
#pragma once

#include <stdint.h>

#ifndef SAMPLE_PERIOD_MS
#define SAMPLE_PERIOD_MS 1000UL  // 1 Hz; the Wokwi build compresses time
#endif

namespace cfg {

constexpr uint32_t kSerialBaud = 115200UL;
constexpr uint32_t kSamplePeriodMs = SAMPLE_PERIOD_MS;

// Signal channels, in the order they are stored and logged.
enum Channel : uint8_t { kChPh = 0, kChConductivity = 1, kChTemperature = 2 };
constexpr uint8_t kNumChannels = 3;

// Every channel is clamped to this range before it enters the pipeline so the
// integer feature maths can never overflow (see window_features.cpp).
constexpr int16_t kSampleMin = 0;
constexpr int16_t kSampleMax = 10000;

// Sliding window: 32 samples (32 s at 1 Hz), re-evaluated every 8 samples.
constexpr uint8_t kWindowSize = 32;
constexpr uint8_t kWindowHop = 8;

// Personal baseline: the first N complete windows after power-up.
constexpr uint8_t kCalibrationWindows = 8;

// Baseline-relative features are quantised to int8 z-scores in units of
// 1/kZScale standard deviations, saturating at +/-kZClamp (~7.9 sigma).
constexpr int32_t kZScale = 16;
constexpr int32_t kZClamp = 127;

// Out-of-distribution guard: if at least this many of the 15 z-scores are
// saturated at +/-kZClamp, the window is far outside anything the tree saw in
// training (shorted probe, loose electrode, garbage input) and is flagged
// regardless of the tree's vote. Fires on <0.1 % of normal synthetic windows.
constexpr uint8_t kSaturationGuard = 6;
constexpr uint8_t kGuardLeaf = 0xFE;  // reported as the leaf of guarded windows

// Alert debouncing: consecutive anomalous / normal windows needed to switch.
constexpr uint8_t kAlertOnWindows = 2;
constexpr uint8_t kAlertOffWindows = 2;

// ADC oversampling for the analog channels (power of two).
constexpr uint8_t kAdcOversample = 8;

}  // namespace cfg
