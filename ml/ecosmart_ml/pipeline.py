"""Bit-exact port of the on-device pipeline (src/pipeline.cpp and friends).

Sample stream -> sliding windows -> features -> personal baseline ->
quantised z-scores -> decision tree -> debounced alert.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field
from typing import List, Optional, Sequence, Tuple

from . import config as C
from .features import compute_window
from .fixedpoint import clamp, div_round, isqrt

PHASE_WARMUP, PHASE_CALIBRATING, PHASE_MONITORING = 0, 1, 2

LEAF = -1


@dataclass
class TreeModel:
    """Flat decision tree, same layout as ModelNode in include/model_types.h.

    Internal node: feature >= 0, go left if z[feature] <= threshold.
    Leaf: feature == LEAF, left = anomaly probability (percent), right = label.
    """

    nodes: List[Tuple[int, int, int, int]]
    scale_floors: List[int]

    def predict(self, z: Sequence[int]) -> Tuple[int, int, int]:
        """Returns (label, p_anomaly_percent, leaf_index)."""
        i = 0
        while True:
            feature, threshold, left, right = self.nodes[i]
            if feature == LEAF:
                return right, left, i
            i = left if z[feature] <= threshold else right


class Baseline:
    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.sum = [0] * C.NUM_FEATURES
        self.sum_sq = [0] * C.NUM_FEATURES
        self.mean = [0] * C.NUM_FEATURES
        self.scale = [1] * C.NUM_FEATURES
        self.count = 0
        self.ready = False

    def accumulate(self, f: Sequence[int]) -> None:
        for i, v in enumerate(f):
            self.sum[i] += v
            self.sum_sq[i] += v * v
        self.count += 1

    def raw_std(self) -> List[int]:
        n = self.count
        return [
            isqrt(div_round(max(n * sq - s * s, 0), n * n))
            for s, sq in zip(self.sum, self.sum_sq)
        ]

    def finalize(self, floors: Sequence[int]) -> None:
        if self.count == 0:
            return
        n = self.count
        self.mean = [div_round(s, n) for s in self.sum]
        self.scale = [max(sd, fl, 1) for sd, fl in zip(self.raw_std(), floors)]
        self.ready = True

    def normalize(self, f: Sequence[int]) -> List[int]:
        return [
            clamp(div_round((v - m) * C.Z_SCALE, s), -C.Z_CLAMP, C.Z_CLAMP)
            for v, m, s in zip(f, self.mean, self.scale)
        ]


class AlertLogic:
    def __init__(self, on_windows: int = C.ALERT_ON_WINDOWS,
                 off_windows: int = C.ALERT_OFF_WINDOWS) -> None:
        self.on_windows = on_windows
        self.off_windows = off_windows
        self.reset()

    def reset(self) -> None:
        self.streak = 0
        self.active = False

    def update(self, anomalous: bool) -> bool:
        contradicts = (not anomalous) if self.active else anomalous
        self.streak = self.streak + 1 if contradicts else 0
        if self.streak >= (self.off_windows if self.active else self.on_windows):
            self.active = not self.active
            self.streak = 0
        return self.active


@dataclass
class WindowEvent:
    window_index: int        # 0-based count of completed windows
    end_sample: int          # index of the newest sample in the window
    phase: int               # phase *after* this window was processed
    features: List[int]
    z: Optional[List[int]] = None
    label: int = 0
    p_anomaly: int = 0
    leaf: int = -1
    alert: bool = False


def clamp_sample(v: int) -> int:
    return clamp(int(v), C.SAMPLE_MIN, C.SAMPLE_MAX)


@dataclass
class AnomalyPipeline:
    """Streaming pipeline. With model=None it stops after calibration and only
    reports features (used to fit the per-feature scale floors)."""

    model: Optional[TreeModel] = None
    floors: Optional[Sequence[int]] = None
    buffers: List[deque] = field(default_factory=lambda: [
        deque(maxlen=C.WINDOW_SIZE) for _ in range(C.NUM_CHANNELS)])
    baseline: Baseline = field(default_factory=Baseline)
    alert: AlertLogic = field(default_factory=AlertLogic)
    samples_seen: int = 0
    since_last_window: int = 0
    windows: int = 0
    phase: int = PHASE_WARMUP

    def __post_init__(self) -> None:
        if self.floors is None and self.model is not None:
            self.floors = self.model.scale_floors

    def push(self, ph: int, cond: int, temp: int) -> Optional[WindowEvent]:
        for buf, v in zip(self.buffers, (ph, cond, temp)):
            buf.append(clamp_sample(v))
        self.samples_seen += 1
        self.since_last_window += 1
        if len(self.buffers[0]) < C.WINDOW_SIZE:
            return None
        if self.windows > 0 and self.since_last_window < C.WINDOW_HOP:
            return None
        self.since_last_window = 0

        feats = compute_window([list(b) for b in self.buffers])
        event = WindowEvent(self.windows, self.samples_seen - 1, self.phase, feats)
        self.windows += 1

        if not self.baseline.ready:
            self.baseline.accumulate(feats)
            if self.baseline.count >= C.CALIBRATION_WINDOWS and self.floors is not None:
                self.baseline.finalize(self.floors)
                self.phase = PHASE_MONITORING
            else:
                self.phase = PHASE_CALIBRATING
            event.phase = self.phase
            return event

        event.z = self.baseline.normalize(feats)
        if self.model is not None:
            event.label, event.p_anomaly, event.leaf = self.model.predict(event.z)
            saturated = sum(1 for v in event.z if abs(v) >= C.Z_CLAMP)
            if saturated >= C.SATURATION_GUARD:
                event.label, event.p_anomaly, event.leaf = 1, 100, C.GUARD_LEAF
        event.alert = self.alert.update(event.label == 1)
        event.phase = self.phase
        return event


def run(samples: Sequence[Tuple[int, int, int]], model: Optional[TreeModel] = None,
        floors: Optional[Sequence[int]] = None) -> List[WindowEvent]:
    pipe = AnomalyPipeline(model=model, floors=floors)
    events = []
    for ph, cond, temp in samples:
        ev = pipe.push(ph, cond, temp)
        if ev is not None:
            events.append(ev)
    return events
