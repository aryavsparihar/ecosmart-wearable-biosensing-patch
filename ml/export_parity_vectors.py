#!/usr/bin/env python3
"""Write test/test_parity/parity_vectors.h from the exported model.

    python ml/export_parity_vectors.py          # after ml/train.py
    python ml/export_parity_vectors.py --check  # CI: fail if the file is stale
"""

import argparse
import sys
from pathlib import Path

from ecosmart_ml.export import load_model_json
from ecosmart_ml.parity import render_header

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MODEL = ROOT / "ml" / "model" / "model.json"
DEFAULT_OUT = ROOT / "test" / "test_parity" / "parity_vectors.h"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model-json", type=Path, default=DEFAULT_MODEL)
    ap.add_argument("--out", type=Path, default=DEFAULT_OUT)
    ap.add_argument("--check", action="store_true", help="verify instead of writing")
    args = ap.parse_args()

    text = render_header(load_model_json(args.model_json))
    if args.check:
        if not args.out.exists() or args.out.read_text() != text:
            print(f"{args.out} is stale - run python ml/export_parity_vectors.py")
            return 1
        print(f"{args.out} is up to date")
        return 0
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text)
    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
