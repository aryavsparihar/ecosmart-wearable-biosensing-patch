"""Guards against the Python mirror drifting from include/config.h."""

import re
from pathlib import Path

from ecosmart_ml import config

CONFIG_H = Path(__file__).resolve().parents[2] / "include" / "config.h"


def _constexpr(name: str) -> int:
    m = re.search(rf"constexpr\s+\w+\s+{name}\s*=\s*(-?\d+)", CONFIG_H.read_text())
    assert m, f"{name} not found in config.h"
    return int(m.group(1))


def test_constants_match_firmware():
    assert config.NUM_CHANNELS == _constexpr("kNumChannels")
    assert config.SAMPLE_MIN == _constexpr("kSampleMin")
    assert config.SAMPLE_MAX == _constexpr("kSampleMax")
    assert config.WINDOW_SIZE == _constexpr("kWindowSize")
    assert config.WINDOW_HOP == _constexpr("kWindowHop")
    assert config.CALIBRATION_WINDOWS == _constexpr("kCalibrationWindows")
    assert config.Z_SCALE == _constexpr("kZScale")
    assert config.Z_CLAMP == _constexpr("kZClamp")
    assert config.SATURATION_GUARD == _constexpr("kSaturationGuard")
    assert config.GUARD_LEAF == 0xFE
    assert config.ALERT_ON_WINDOWS == _constexpr("kAlertOnWindows")
    assert config.ALERT_OFF_WINDOWS == _constexpr("kAlertOffWindows")


def test_feature_names():
    assert len(config.FEATURE_NAMES) == 15
    assert config.FEATURE_NAMES[0] == "ph_mean"
    assert config.FEATURE_NAMES[14] == "temp_max"
