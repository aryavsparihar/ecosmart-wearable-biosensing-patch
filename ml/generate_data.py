#!/usr/bin/env python3
"""Generate the SYNTHETIC training/evaluation dataset as CSV.

    python ml/generate_data.py                 # -> ml/data/synthetic_sweat.csv

*** This is synthetic data, not measurements from people. ***
Columns: subject, t_s, ph, cond_mScm, temp_C, label, anomaly_type.
"""

import argparse
import csv
from pathlib import Path

from ecosmart_ml.synth import generate_dataset

DEFAULT_OUT = Path(__file__).resolve().parent / "data" / "synthetic_sweat.csv"
DEFAULT_SEED = 2026


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT)
    ap.add_argument("--subjects", type=int, default=120)
    ap.add_argument("--samples", type=int, default=2400, help="samples per subject (1 Hz)")
    ap.add_argument("--seed", type=int, default=DEFAULT_SEED)
    args = ap.parse_args()

    subjects = generate_dataset(args.subjects, args.seed, args.samples)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    rows = 0
    with args.out.open("w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["subject", "t_s", "ph", "cond_mScm", "temp_C", "label", "anomaly_type"])
        for s in subjects:
            for t in range(len(s.ph)):
                w.writerow([s.subject_id, t, f"{s.ph[t] / 100:.2f}", f"{s.cond[t] / 100:.2f}",
                            f"{s.temp[t] / 100:.2f}", int(s.label[t]), s.anomaly_type[t]])
                rows += 1

    n_eps = sum(len(s.episodes) for s in subjects)
    anom = sum(int(s.label.sum()) for s in subjects)
    print(f"SYNTHETIC dataset: {len(subjects)} subjects, {rows} samples, {n_eps} anomaly "
          f"episodes ({100 * anom / rows:.1f}% anomalous samples), seed={args.seed}")
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
