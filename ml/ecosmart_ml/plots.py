"""Figures for docs/img, produced by ml/train.py (all from SYNTHETIC data)."""

from __future__ import annotations

from pathlib import Path
from typing import Dict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402

from .pipeline import TreeModel, run  # noqa: E402

SERIES = "#2a78d6"
TRUTH = "#e34948"
INK = "#52514e"
GRID = "#d9d8d4"


def _style(ax) -> None:
    ax.grid(True, color=GRID, lw=0.6)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    ax.tick_params(colors=INK, labelsize=8)


def _spans(mask):
    start = None
    for i, v in enumerate(mask):
        if v and start is None:
            start = i
        elif not v and start is not None:
            yield start, i
            start = None
    if start is not None:
        yield start, len(mask)


def timeline(subject: Dict, model: TreeModel, path: Path, title: str) -> None:
    """Signals, labelled anomaly episodes and the on-device alert for one wearer."""
    samples = list(zip(subject["ph"].tolist(), subject["cond"].tolist(),
                       subject["temp"].tolist()))
    events = run(samples, model=model)
    n = len(samples)
    alert = [0] * n
    for ev in events:
        for i in range(ev.end_sample, min(ev.end_sample + 8, n)):
            alert[i] = int(ev.alert)
    t = [i / 60 for i in range(n)]

    fig, axes = plt.subplots(4, 1, sharex=True, figsize=(10, 6.8),
                             gridspec_kw={"height_ratios": [3, 3, 3, 1]})
    panels = (("ph", "Sweat pH", 100), ("cond", "Conductivity\n(mS/cm)", 100),
              ("temp", "Skin temp.\n(°C)", 100))
    for ax, (key, label, div) in zip(axes, panels):
        ax.plot(t, subject[key] / div, color=SERIES, lw=1.1)
        for a, b in _spans(subject["label"]):
            ax.axvspan(a / 60, b / 60, color=TRUTH, alpha=0.13, lw=0)
        ax.set_ylabel(label, fontsize=9, color=INK)
        _style(ax)
    ax = axes[3]
    ax.fill_between(t, alert, step="post", color=TRUTH, alpha=0.85, lw=0)
    ax.set_yticks([0, 1], ["off", "on"])
    ax.set_ylabel("Red LED", fontsize=9, color=INK)
    ax.set_xlabel("time (min)", fontsize=9, color=INK)
    _style(ax)
    fig.suptitle(title, fontsize=10, color="#0b0b0b")
    fig.legend(handles=[Patch(color=TRUTH, alpha=0.25, label="labelled anomaly (ground truth)"),
                        Patch(color=TRUTH, alpha=0.85, label="device alert (red LED)")],
               loc="upper center", bbox_to_anchor=(0.5, 0.955), fontsize=8, frameon=False, ncol=2)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=140)
    plt.close(fig)


def depth_sweep(cv_f1: Dict[int, float], chosen: int, path: Path) -> None:
    depths = sorted(cv_f1)
    fig, ax = plt.subplots(figsize=(5.5, 3.2))
    ax.plot(depths, [cv_f1[d] for d in depths], color=SERIES, lw=2, marker="o", ms=7)
    ax.annotate(f"chosen: depth {chosen}\nF1 {cv_f1[chosen]:.3f}", (chosen, cv_f1[chosen]),
                textcoords="offset points", xytext=(-20, -60), fontsize=8, color=INK,
                arrowprops={"arrowstyle": "-", "color": INK, "lw": 0.8})
    ax.set_xticks(depths)
    ax.set_xlabel("max tree depth", fontsize=9, color=INK)
    ax.set_ylabel("window F1 (grouped 5-fold CV)", fontsize=9, color=INK)
    ax.set_ylim(0, 1)
    _style(ax)
    fig.suptitle("Smallest tree within 0.01 F1 of the best", fontsize=10)
    fig.tight_layout()
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=140)
    plt.close(fig)
