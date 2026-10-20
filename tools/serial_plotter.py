#!/usr/bin/env python3
"""Plot the patch's serial CSV stream live, or a saved log.

    python tools/serial_plotter.py --port /dev/ttyUSB0          # live from a Nano
    python tools/serial_plotter.py --file session.csv           # saved log
    python tools/serial_plotter.py --file session.csv --save session.png
    python tools/serial_plotter.py --file sim.csv --time-scale 50   # undo time compression

Lines starting with '#' (banner, window decisions, events, RAM reports) are
echoed to the console; data rows are
    ms,ph,cond_mScm,temp_C,phase,p_anomaly,alert
Shaded regions: grey = warm-up / calibration, red = alert raised.
To record a log: pio device monitor | tee session.csv  (or any terminal logger).
"""

from __future__ import annotations

import argparse
import sys
import threading
from collections import deque
from dataclasses import dataclass, field
from typing import Deque, Iterable, List, Optional

CHANNELS = (("ph", "Sweat pH", "pH"),
            ("cond", "Conductivity", "mS/cm"),
            ("temp", "Skin temperature", "°C"))
SERIES = "#2a78d6"
ALERT = "#e34948"
CALIB = "#9a9a96"


@dataclass
class Row:
    t: float  # seconds
    ph: float
    cond: float
    temp: float
    phase: str
    p_anomaly: int
    alert: int


def parse_line(line: str, time_scale: float = 1.0) -> Optional[Row]:
    """Returns a Row for data lines, None for comments / headers / garbage."""
    line = line.strip()
    if not line or line.startswith("#") or line.startswith("ms,"):
        return None
    parts = line.split(",")
    if len(parts) != 7:
        return None
    try:
        return Row(int(parts[0]) / 1000.0 * time_scale, float(parts[1]), float(parts[2]),
                   float(parts[3]), parts[4], int(parts[5]), int(parts[6]))
    except ValueError:
        return None


@dataclass
class Buffer:
    maxlen: int = 900
    rows: Deque[Row] = field(default_factory=deque)
    lock: threading.Lock = field(default_factory=threading.Lock)

    def add(self, row: Row) -> None:
        with self.lock:
            self.rows.append(row)
            while len(self.rows) > self.maxlen:
                self.rows.popleft()

    def snapshot(self) -> List[Row]:
        with self.lock:
            return list(self.rows)


def _spans(rows: List[Row], pred) -> Iterable[tuple]:
    start = None
    for r in rows:
        if pred(r) and start is None:
            start = r.t
        elif not pred(r) and start is not None:
            yield start, r.t
            start = None
    if start is not None and rows:
        yield start, rows[-1].t


def draw(fig, axes, rows: List[Row], title: str) -> None:
    for ax, (key, name, unit) in zip(axes, CHANNELS):
        ax.clear()
        ax.plot([r.t for r in rows], [getattr(r, key) for r in rows], color=SERIES, lw=1.4)
        for a, b in _spans(rows, lambda r: r.phase in ("WARMUP", "CALIB")):
            ax.axvspan(a, b, color=CALIB, alpha=0.15, lw=0)
        for a, b in _spans(rows, lambda r: r.alert == 1):
            ax.axvspan(a, b, color=ALERT, alpha=0.18, lw=0)
        ax.set_ylabel(f"{name}\n({unit})", fontsize=9)
        ax.grid(True, color="#d9d8d4", lw=0.6)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
    axes[-1].set_xlabel("time since boot (s)")
    status = ""
    if rows:
        last = rows[-1]
        status = (f"  |  {last.phase}  p_anomaly={last.p_anomaly}%  "
                  f"{'ALERT' if last.alert else 'normal'}")
    fig.suptitle(title + status, fontsize=10)
    if not fig.legends:
        from matplotlib.patches import Patch
        fig.legend(handles=[Patch(color=CALIB, alpha=0.3, label="warm-up / calibrating"),
                            Patch(color=ALERT, alpha=0.35, label="alert raised (red LED)")],
                   loc="upper center", bbox_to_anchor=(0.5, 0.955), fontsize=8,
                   frameon=False, ncol=2)


def read_serial(port: str, baud: int, buf: Buffer, time_scale: float) -> None:
    import serial  # pyserial
    with serial.Serial(port, baud, timeout=1) as ser:
        while True:
            line = ser.readline().decode("ascii", errors="replace")
            if line.startswith("#"):
                print(line.rstrip())
            row = parse_line(line, time_scale)
            if row:
                buf.add(row)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--port", help="serial port, e.g. /dev/ttyUSB0 or COM3")
    src.add_argument("--file", help="saved CSV log")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--window", type=int, default=900, help="samples kept on screen (live)")
    ap.add_argument("--save", help="write the plot to this image instead of showing it")
    ap.add_argument("--title", help="plot title (default: port or file name)")
    ap.add_argument("--time-scale", type=float, default=1.0,
                    help="multiply timestamps, e.g. 50 for the 20 ms/sample simavr build")
    args = ap.parse_args()

    import matplotlib
    if args.save:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(3, 1, sharex=True, figsize=(10, 6.5))

    if args.file:
        rows = []
        with open(args.file, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                if line.startswith("#"):
                    print(line.rstrip())
                row = parse_line(line, args.time_scale)
                if row:
                    rows.append(row)
        if not rows:
            print("no data rows found", file=sys.stderr)
            return 1
        from pathlib import Path
        draw(fig, axes, rows, args.title or f"Eco-Smart patch log: {Path(args.file).name}")
        fig.tight_layout(rect=(0, 0, 1, 0.93))
        if args.save:
            fig.savefig(args.save, dpi=150)
            print(f"wrote {args.save}")
        else:
            plt.show()
        return 0

    from matplotlib.animation import FuncAnimation
    buf = Buffer(maxlen=args.window)
    threading.Thread(target=read_serial, args=(args.port, args.baud, buf, args.time_scale),
                     daemon=True).start()

    def update(_):
        draw(fig, axes, buf.snapshot(), args.title or f"Eco-Smart patch live: {args.port}")
        fig.tight_layout(rect=(0, 0, 1, 0.93))

    anim = FuncAnimation(fig, update, interval=1000, cache_frame_data=False)  # noqa: F841
    plt.show()
    return 0


if __name__ == "__main__":
    sys.exit(main())
