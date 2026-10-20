// Hysteresis on top of the per-window classifier: the alert is raised only
// after `onWindows` consecutive anomalous windows and cleared only after
// `offWindows` consecutive normal ones, so a single noisy window cannot
// flicker the red LED. Mirrored by ml/ecosmart_ml/pipeline.py.
#pragma once

#include <stdint.h>

#include "config.h"

class AlertLogic {
 public:
  explicit AlertLogic(uint8_t onWindows = cfg::kAlertOnWindows,
                      uint8_t offWindows = cfg::kAlertOffWindows)
      : onWindows_(onWindows), offWindows_(offWindows) {
    reset();
  }

  void reset() {
    streak_ = 0;
    active_ = false;
  }

  // Feeds one window decision; returns the (possibly updated) alert state.
  bool update(bool anomalous) {
    const bool contradicts = active_ ? !anomalous : anomalous;
    streak_ = contradicts ? static_cast<uint8_t>(streak_ + 1) : 0;
    if (streak_ >= (active_ ? offWindows_ : onWindows_)) {
      active_ = !active_;
      streak_ = 0;
    }
    return active_;
  }

  bool active() const { return active_; }

 private:
  uint8_t onWindows_;
  uint8_t offWindows_;
  uint8_t streak_;
  bool active_;
};
