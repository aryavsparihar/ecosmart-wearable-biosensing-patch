"""Synthetic, physiologically plausible sweat-patch time series.

*** SYNTHETIC DATA - NOT RECORDED FROM HUMANS. ***

Each "subject" gets a personal resting baseline (sweat pH, sweat conductivity,
skin temperature), slow physiological drift, sensor noise, benign exertion
bumps (labelled normal - they are the hard negatives) and zero or more
labelled anomaly episodes. Values are pushed through the firmware's ADC /
fixed-point sensor model, so every sample sits on the integer grid the
Arduino Nano would actually produce.

Ranges are loose literature-style figures (eccrine sweat pH ~4.5-7, sweat
conductivity a few to ~20 mS/cm, skin temperature ~33-36 degC); they are not
calibrated against any real device or population.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List

import numpy as np

from . import sensor_model as sm

SAMPLE_RATE_HZ = 1

ANOMALY_TYPES = (
    "acidic_ph",              # sweat pH drops well below the wearer's baseline
    "alkaline_ph",            # sweat pH rises well above baseline
    "conductivity_surge",     # electrolyte concentration jumps
    "electrode_dropout",      # electrodes dry out / lift off the skin
    "temperature_rise",       # febrile-like skin-temperature rise
    "temp_and_conductivity",  # combined, smaller deviations
)


@dataclass
class Subject:
    subject_id: int
    ph: np.ndarray       # centi-pH (int)
    cond: np.ndarray     # centi-mS/cm (int)
    temp: np.ndarray     # centi-degC (int)
    label: np.ndarray    # 1 where an anomaly is at >= 50 % intensity
    anomaly_type: List[str]
    episodes: List[Dict]


def _smoothstep(x: np.ndarray) -> np.ndarray:
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3 - 2 * x)


def envelope(n: int, start: int, duration: int, ramp_in: int, ramp_out: int) -> np.ndarray:
    """0 -> 1 -> 0 intensity profile with smooth onset and recovery."""
    t = np.arange(n, dtype=float)
    up = _smoothstep((t - start) / max(ramp_in, 1))
    down = 1.0 - _smoothstep((t - (start + duration - ramp_out)) / max(ramp_out, 1))
    return up * down


def ar1(rng: np.random.Generator, n: int, phi: float, sigma: float) -> np.ndarray:
    """Stationary AR(1) process: slow, bounded physiological drift."""
    e = rng.normal(0.0, sigma, n)
    x = np.empty(n)
    x[0] = rng.normal(0.0, sigma / np.sqrt(1 - phi * phi))
    for i in range(1, n):
        x[i] = phi * x[i - 1] + e[i]
    return x


def _place_episodes(rng, n, count, earliest, min_gap=90):
    placed: List[tuple] = []
    for _ in range(50 * max(count, 1)):
        if len(placed) == count:
            break
        dur = int(rng.integers(60, 241))
        if n - dur <= earliest:
            continue  # recording too short for this episode
        start = int(rng.integers(earliest, n - dur))
        if all(start > e + min_gap or start + dur + min_gap < s for s, e in placed):
            placed.append((start, start + dur))
    return sorted(placed)


def generate_subject(rng: np.random.Generator, subject_id: int, n_samples: int = 2400,
                     max_episodes: int = 3, earliest_episode: int = 200) -> Subject:
    n = n_samples
    t = np.arange(n)

    ph0 = rng.uniform(4.8, 6.6)
    cond0 = rng.uniform(4.0, 14.0)
    temp0 = rng.uniform(33.0, 35.5)

    ph = ph0 + ar1(rng, n, 0.995, 0.004) + rng.normal(0, 0.02, n)
    cond_mult = np.exp(ar1(rng, n, 0.995, 0.003)) * (1 + rng.normal(0, 0.012, n))
    period = rng.uniform(900, 2400)
    temp = (temp0 + rng.uniform(0.05, 0.25) * np.sin(2 * np.pi * t / period + rng.uniform(0, 2 * np.pi))
            + ar1(rng, n, 0.998, 0.004) + rng.normal(0, 0.02, n))

    # Benign exertion bumps: more sweat, slightly warmer skin. Labelled NORMAL.
    for _ in range(int(rng.integers(0, 3))):
        dur = min(int(rng.integers(120, 400)), n - earliest_episode - 1)
        if dur < 60:
            continue  # recording too short for an exertion bump
        env = envelope(n, int(rng.integers(earliest_episode, n - dur)), dur,
                       int(rng.integers(40, 120)), int(rng.integers(40, 120)))
        cond_mult *= 1 + rng.uniform(0.08, 0.30) * env
        temp += rng.uniform(0.10, 0.40) * env
        ph += rng.uniform(0.03, 0.20) * env

    label = np.zeros(n, dtype=np.int8)
    kinds = [""] * n
    episodes = []
    k = int(rng.integers(0, max_episodes + 1))
    for start, end in _place_episodes(rng, n, k, earliest_episode):
        kind = str(rng.choice(ANOMALY_TYPES))
        env = envelope(n, start, end - start, int(rng.integers(8, 41)), int(rng.integers(8, 41)))
        if kind == "acidic_ph":
            ph -= rng.uniform(0.7, 1.6) * env
        elif kind == "alkaline_ph":
            ph += rng.uniform(0.8, 1.8) * env
        elif kind == "conductivity_surge":
            cond_mult *= 1 + rng.uniform(0.5, 1.6) * env
        elif kind == "electrode_dropout":
            cond_mult *= 1 - rng.uniform(0.75, 1.0) * env
        elif kind == "temperature_rise":
            temp += rng.uniform(0.7, 1.6) * env
        else:  # temp_and_conductivity
            temp += rng.uniform(0.5, 1.0) * env
            cond_mult *= 1 + rng.uniform(0.3, 0.8) * env
        active = env >= 0.5
        label[active] = 1
        for i in np.flatnonzero(active):
            kinds[i] = kind
        episodes.append({"start": start, "end": end, "type": kind})

    cond = cond0 * cond_mult
    return Subject(
        subject_id=subject_id,
        ph=np.array([sm.measure_ph(v) for v in ph], dtype=np.int32),
        cond=np.array([sm.measure_conductivity(v) for v in cond], dtype=np.int32),
        temp=np.array([sm.measure_temperature(v) for v in temp], dtype=np.int32),
        label=label,
        anomaly_type=kinds,
        episodes=episodes,
    )


def generate_dataset(n_subjects: int, seed: int, n_samples: int = 2400) -> List[Subject]:
    rng = np.random.default_rng(seed)
    return [generate_subject(rng, i, n_samples) for i in range(n_subjects)]
