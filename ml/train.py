#!/usr/bin/env python3
"""Train the on-device anomaly classifier and export it as a C header.

    python ml/generate_data.py      # synthetic dataset (once)
    python ml/train.py              # -> include/model_weights.h

Pipeline (identical integer maths to the firmware):
  samples -> 32-sample windows every 8 samples -> 15 features ->
  personal baseline from the first 8 windows -> int8 z-scores ->
  shallow decision tree -> 2-of-2 window alert hysteresis.

Subjects are split into train/test *by subject*, so every test wearer is
unseen. Tree depth is chosen by grouped cross-validation on the training
subjects only. *** All numbers come from SYNTHETIC data. ***
"""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import (accuracy_score, confusion_matrix, f1_score,
                             precision_score, recall_score)
from sklearn.model_selection import GroupKFold
from sklearn.tree import DecisionTreeClassifier

from ecosmart_ml import config as C
from ecosmart_ml.export import (save_model_json, tree_depth, tree_from_sklearn,
                                write_header)
from ecosmart_ml.pipeline import Baseline, run
from ecosmart_ml import plots

ML_DIR = Path(__file__).resolve().parent
ROOT = ML_DIR.parent
DEFAULT_DATA = ML_DIR / "data" / "synthetic_sweat.csv"
DEFAULT_HEADER = ROOT / "include" / "model_weights.h"
DEFAULT_MODEL_JSON = ML_DIR / "model" / "model.json"
DEFAULT_METRICS = ROOT / "docs" / "results" / "metrics.json"
DEFAULT_PLOTS = ROOT / "docs" / "img"

DEPTHS = (2, 3, 4, 5, 6)
MIN_SAMPLES_LEAF = 25
RANDOM_STATE = 0


# --------------------------------------------------------------------------- data

def load_subjects(path: Path) -> Dict[int, Dict[str, np.ndarray]]:
    cols = defaultdict(lambda: defaultdict(list))
    with path.open() as fh:
        for row in csv.DictReader(fh):
            s = cols[int(row["subject"])]
            s["ph"].append(round(float(row["ph"]) * 100))
            s["cond"].append(round(float(row["cond_mScm"]) * 100))
            s["temp"].append(round(float(row["temp_C"]) * 100))
            s["label"].append(int(row["label"]))
            s["type"].append(row["anomaly_type"])
    return {sid: {k: (np.array(v) if k != "type" else v) for k, v in d.items()}
            for sid, d in cols.items()}


def samples_of(s) -> List[tuple]:
    return list(zip(s["ph"].tolist(), s["cond"].tolist(), s["temp"].tolist()))


def window_label(labels: np.ndarray, end_sample: int) -> int:
    w = labels[end_sample - C.WINDOW_SIZE + 1:end_sample + 1]
    return int(2 * int(w.sum()) >= len(w))


def fit_scale_floors(train) -> List[int]:
    """Lower bound for each feature's baseline scale.

    Eight calibration windows span under two minutes, so their spread badly
    underestimates how far a healthy wearer drifts over a session; scaling by
    it alone saturates most normal windows at +/-127. The floor is therefore
    the pooled robust spread (1.4826 x MAD) of normal monitoring windows
    around each wearer's own calibration mean: the mean stays personal, the
    expected normal variability is learnt from the training population.
    """
    devs = []
    for s in train.values():
        events = run(samples_of(s), floors=None)
        b = Baseline()
        for ev in events[:C.CALIBRATION_WINDOWS]:
            b.accumulate(ev.features)
        b.finalize([1] * C.NUM_FEATURES)
        for ev in events[C.CALIBRATION_WINDOWS:]:
            if window_label(s["label"], ev.end_sample) == 0:
                devs.append([f - m for f, m in zip(ev.features, b.mean)])
    d = np.array(devs, dtype=float)
    mad = 1.4826 * np.median(np.abs(d), axis=0)
    return [max(1, int(round(v))) for v in mad]


def window_table(subjects, floors):
    """Monitoring-phase windows: z-scores, labels, subject ids, end samples."""
    X, y, groups, ends = [], [], [], []
    for sid, s in subjects.items():
        for ev in run(samples_of(s), floors=floors):
            if ev.z is None:
                continue
            X.append(ev.z)
            y.append(window_label(s["label"], ev.end_sample))
            groups.append(sid)
            ends.append(ev.end_sample)
    return np.array(X), np.array(y), np.array(groups), np.array(ends)


# ---------------------------------------------------------------- evaluation

def window_metrics(y_true, y_pred) -> Dict[str, object]:
    return {
        "windows": int(len(y_true)),
        "accuracy": round(float(accuracy_score(y_true, y_pred)), 4),
        "precision": round(float(precision_score(y_true, y_pred, zero_division=0)), 4),
        "recall": round(float(recall_score(y_true, y_pred, zero_division=0)), 4),
        "f1": round(float(f1_score(y_true, y_pred, zero_division=0)), 4),
        "confusion_matrix": confusion_matrix(y_true, y_pred, labels=[0, 1]).tolist(),
    }


def episodes_of(s) -> List[dict]:
    eps, start = [], None
    lab = s["label"]
    for i, v in enumerate(lab):
        if v and start is None:
            start = i
        elif not v and start is not None:
            eps.append({"start": start, "end": i, "type": s["type"][start]})
            start = None
    if start is not None:
        eps.append({"start": start, "end": len(lab), "type": s["type"][start]})
    return eps


def event_metrics(subjects, model) -> Dict[str, object]:
    """Alert-level view: did the debounced alert fire during each episode,
    how fast, and how often did it fire when nothing was wrong?"""
    detected, latencies, false_alerts, hours = 0, [], 0, 0.0
    per_type = defaultdict(lambda: [0, 0])
    total = 0
    for s in subjects.values():
        events = run(samples_of(s), model=model)
        eps = episodes_of(s)
        hours += len(s["label"]) / 3600.0
        slack = C.WINDOW_SIZE  # alert may trail the episode by one window
        for e in eps:
            total += 1
            per_type[e["type"]][1] += 1
            hits = [ev.end_sample for ev in events
                    if ev.alert and e["start"] <= ev.end_sample <= e["end"] + slack]
            if hits:
                detected += 1
                per_type[e["type"]][0] += 1
                latencies.append(hits[0] - e["start"])
        prev = False
        for ev in events:
            if ev.alert and not prev:
                if not any(e["start"] - slack <= ev.end_sample <= e["end"] + slack for e in eps):
                    false_alerts += 1
            prev = ev.alert
    return {
        "episodes": total,
        "episodes_detected": detected,
        "detection_rate": round(detected / total, 4) if total else None,
        "median_latency_s": float(np.median(latencies)) if latencies else None,
        "false_alerts": false_alerts,
        "monitored_hours": round(hours, 2),
        "false_alerts_per_hour": round(false_alerts / hours, 3) if hours else None,
        "detection_by_type": {k: f"{v[0]}/{v[1]}" for k, v in sorted(per_type.items())},
    }


def choose_depth(X, y, groups) -> Dict[int, float]:
    cv = GroupKFold(n_splits=5)
    scores = {}
    for d in DEPTHS:
        f1s = []
        for tr, va in cv.split(X, y, groups):
            clf = DecisionTreeClassifier(max_depth=d, min_samples_leaf=MIN_SAMPLES_LEAF,
                                         random_state=RANDOM_STATE).fit(X[tr], y[tr])
            f1s.append(f1_score(y[va], clf.predict(X[va]), zero_division=0))
        scores[d] = round(float(np.mean(f1s)), 4)
    return scores


# ---------------------------------------------------------------------- main

def main() -> None:
    ap = argparse.ArgumentParser(description="Train and export the on-device classifier.")
    ap.add_argument("--data", type=Path, default=DEFAULT_DATA)
    ap.add_argument("--header", type=Path, default=DEFAULT_HEADER)
    ap.add_argument("--model-json", type=Path, default=DEFAULT_MODEL_JSON)
    ap.add_argument("--metrics", type=Path, default=DEFAULT_METRICS)
    ap.add_argument("--plots", type=Path, default=DEFAULT_PLOTS,
                    help="directory for figures ('' to skip)")
    args = ap.parse_args()

    if not args.data.exists():
        print(f"{args.data} missing - generating the synthetic dataset first")
        subprocess.run([sys.executable, str(ML_DIR / "generate_data.py"), "--out",
                        str(args.data)], check=True)

    subjects = load_subjects(args.data)
    test = {k: v for k, v in subjects.items() if k % 4 == 0}
    train = {k: v for k, v in subjects.items() if k % 4 != 0}
    print(f"SYNTHETIC data: {len(train)} train / {len(test)} test subjects")

    floors = fit_scale_floors(train)
    Xtr, ytr, gtr, _ = window_table(train, floors)
    Xte, yte, _, _ = window_table(test, floors)
    print(f"windows: train {len(ytr)} ({ytr.mean():.1%} anomalous), "
          f"test {len(yte)} ({yte.mean():.1%} anomalous)")

    cv_f1 = choose_depth(Xtr, ytr, gtr)
    best = max(cv_f1.values())
    depth = min(d for d, f in cv_f1.items() if f >= best - 0.01)
    print(f"grouped 5-fold CV F1 by depth: {cv_f1} -> depth {depth}")

    clf = DecisionTreeClassifier(max_depth=depth, min_samples_leaf=MIN_SAMPLES_LEAF,
                                 random_state=RANDOM_STATE).fit(Xtr, ytr)
    model = tree_from_sklearn(clf, Xtr, ytr, floors)

    # Evaluate the *quantised, exported* model, not the sklearn object.
    pred_tr = np.array([model.predict(z)[0] for z in Xtr])
    pred_te = np.array([model.predict(z)[0] for z in Xte])
    agreement = float((pred_tr == clf.predict(Xtr)).mean())

    lr = LogisticRegression(max_iter=2000).fit(Xtr, ytr)
    metrics = {
        "data": "SYNTHETIC (ml/ecosmart_ml/synth.py), split by subject",
        "subjects": {"train": len(train), "test": len(test)},
        "model": {
            "type": "decision_tree",
            "max_depth": tree_depth(model),
            "nodes": len(model.nodes),
            "flash_bytes": 4 * len(model.nodes) + 4 * C.NUM_FEATURES,
            "cv_f1_by_depth": cv_f1,
            "sklearn_agreement_train": round(agreement, 4),
        },
        "window_level": {"train": window_metrics(ytr, pred_tr),
                         "test": window_metrics(yte, pred_te)},
        "event_level_test": event_metrics(test, model),
        "baseline_comparison_test": {
            "logistic_regression_f1": round(float(f1_score(yte, lr.predict(Xte))), 4),
            "decision_tree_f1": round(float(f1_score(yte, pred_te)), 4),
        },
        "feature_importance": {n: round(float(v), 4) for n, v in
                               zip(C.FEATURE_NAMES, clf.feature_importances_) if v > 0},
    }

    write_header(model, args.header, {
        "Training subjects": len(train),
        "Training windows": len(ytr),
        "Tree depth": tree_depth(model),
        "Seed": "dataset 2026 / tree 0",
    })
    save_model_json(model, args.model_json)
    args.metrics.parent.mkdir(parents=True, exist_ok=True)
    args.metrics.write_text(json.dumps(metrics, indent=2) + "\n")

    if str(args.plots):
        # Showcase the unseen test wearer with the most distinct episode types.
        sid = max(test, key=lambda k: (len({t for t in test[k]["type"] if t}), -k))
        plots.timeline(test[sid], model, args.plots / "test_subject_timeline.png",
                       f"Unseen synthetic test wearer #{sid}: signals, labelled anomalies "
                       "and on-device alert")
        plots.depth_sweep(cv_f1, depth, args.plots / "depth_selection.png")

    te = metrics["window_level"]["test"]
    ev = metrics["event_level_test"]
    print(f"test windows: acc {te['accuracy']:.3f}  precision {te['precision']:.3f}  "
          f"recall {te['recall']:.3f}  F1 {te['f1']:.3f}")
    print(f"confusion matrix [[TN FP] [FN TP]]: {te['confusion_matrix']}")
    print(f"test episodes: {ev['episodes_detected']}/{ev['episodes']} detected, median "
          f"latency {ev['median_latency_s']} s, {ev['false_alerts_per_hour']} false alerts/h")
    print(f"logistic-regression baseline F1 {metrics['baseline_comparison_test']['logistic_regression_f1']}")
    print(f"wrote {args.header}, {args.model_json}, {args.metrics}")


if __name__ == "__main__":
    main()
