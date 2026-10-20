# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

## [1.0.0] - 2026-10-21

### Added
- README covering the patch layer stack, architecture diagrams, BOM, pin map,
  wiring, fabrication recipes, artificial sweat test, build, flash and
  simulation instructions, the ML pipeline, synthetic-data results, the memory
  budget, limitations and future work, plus a prominent medical disclaimer.
- Design notes and a step-by-step fabrication guide in `docs/`.
- Result figures: test-wearer timeline, depth selection, and the simavr demo
  run of the real firmware.
- Wokwi circuit (`diagram.json`, `wokwi.toml`) and a time-compressed `wokwi`
  build.
- CI: AVR builds with memory report, native Unity tests, reproducible model
  export and parity check, an `arduino-cli` compile of the exported sketch,
  firmware execution on simavr, and an optional Wokwi run.
- `tools/serial_plotter.py` (live, file, and time-scaled plotting) and
  `tools/make_arduino_sketch.py` (Arduino IDE export).
- `SIMULATION_MODE`: an integer on-device synthetic signal generator with
  rotating anomaly episodes.
- Contributing guide and this changelog.

### Changed
- Simulator episode names are kept in flash.

### Fixed
- `features.h` renamed to `window_features.h`, so it no longer shadows glibc's
  `<features.h>` in Linux host builds.

## [0.2.0] - 2026-10-21

### Added
- Integer window feature extraction (mean, variance, slope, min, max),
  personal baseline calibration with int8 z-scores, and alert hysteresis.
- Synthetic, seeded sweat-signal generator with personal baselines, drift,
  benign exertion bumps and six labelled anomaly types, quantised through the
  firmware's sensor model.
- Bit-exact Python mirror of the firmware's features, baseline, tree inference
  and alert logic.
- Training script: subject-wise split, grouped-CV depth selection,
  window-level and event-level metrics, and a logistic-regression comparison.
- Quantised depth-5 decision tree (21 nodes) exported to
  `include/model_weights.h` in PROGMEM, with per-feature scale floors.
- On-device classifier, streaming pipeline and main-loop integration with
  window logging, events and the `r` / `m` serial commands.
- Saturation guard for out-of-distribution windows.
- Golden-vector parity test between Python and C++.

### Changed
- Baseline scale floors learnt from the pooled normal variability (test F1
  0.31 → 0.91).
- Calibration accumulators reused for the frozen baseline, saving 120 B of
  SRAM.

## [0.1.0] - 2026-10-21

### Added
- PlatformIO project for the Arduino Nano, with a pin map and build
  configuration.
- Ring buffer, fixed-point helpers and sensor conversion maths with Unity
  tests.
- pH (A0), electrode conductivity (A1) and non-blocking DS18B20 (D2) drivers.
- Status LEDs, serial CSV logger with free-RAM report, and a 1 Hz sampling
  loop.

[Unreleased]: https://github.com/aryavsparihar/ecosmart-wearable-biosensing-patch/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/aryavsparihar/ecosmart-wearable-biosensing-patch/compare/v0.2.0...v1.0.0
[0.2.0]: https://github.com/aryavsparihar/ecosmart-wearable-biosensing-patch/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/aryavsparihar/ecosmart-wearable-biosensing-patch/releases/tag/v0.1.0
