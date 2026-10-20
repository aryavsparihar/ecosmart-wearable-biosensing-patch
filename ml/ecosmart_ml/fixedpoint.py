"""Integer helpers with the exact semantics of include/fixed_point.h."""

import math


def div_round(num: int, den: int) -> int:
    """num / den rounded half away from zero (den > 0), like fx::divRound."""
    if den <= 0:
        raise ValueError("den must be positive")
    if num >= 0:
        return (num + den // 2) // den
    return -((-num + den // 2) // den)


def isqrt(v: int) -> int:
    """floor(sqrt(v)), like fx::isqrt64."""
    return math.isqrt(v)


def clamp(v: int, lo: int, hi: int) -> int:
    return lo if v < lo else hi if v > hi else v
