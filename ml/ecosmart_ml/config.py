"""Constants mirrored from include/config.h (checked by tests/test_config.py)."""

NUM_CHANNELS = 3
CHANNELS = ("ph", "cond", "temp")

SAMPLE_MIN = 0
SAMPLE_MAX = 10000

WINDOW_SIZE = 32
WINDOW_HOP = 8
CALIBRATION_WINDOWS = 8

Z_SCALE = 16
Z_CLAMP = 127

SATURATION_GUARD = 6
GUARD_LEAF = 0xFE

ALERT_ON_WINDOWS = 2
ALERT_OFF_WINDOWS = 2

FEATURE_KINDS = ("mean", "var", "slope", "min", "max")
FEATURES_PER_CHANNEL = len(FEATURE_KINDS)
NUM_FEATURES = NUM_CHANNELS * FEATURES_PER_CHANNEL
FEATURE_NAMES = tuple(f"{c}_{k}" for c in CHANNELS for k in FEATURE_KINDS)
