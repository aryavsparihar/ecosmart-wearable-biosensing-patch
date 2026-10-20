"""Bit-exact port of src/window_features.cpp."""

from typing import List, Sequence

from .config import FEATURES_PER_CHANNEL
from .fixedpoint import div_round

MEAN, VAR, SLOPE, MIN, MAX = range(FEATURES_PER_CHANNEL)


def compute_channel(x: Sequence[int]) -> List[int]:
    """Returns [mean, variance, slope-per-window, min, max] for one window."""
    n = len(x)
    if n < 2:
        raise ValueError("need at least two samples")
    x = [int(v) for v in x]
    mean = div_round(sum(x), n)
    sq = sum((v - mean) ** 2 for v in x)
    weighted = sum((2 * i - (n - 1)) * v for i, v in enumerate(x))
    return [
        mean,
        (sq + n // 2) // n,
        div_round(6 * weighted, n * n - 1),
        min(x),
        max(x),
    ]


def compute_window(channels: Sequence[Sequence[int]]) -> List[int]:
    """Concatenates per-channel features in firmware order (15 values)."""
    out: List[int] = []
    for x in channels:
        out.extend(compute_channel(x))
    return out
