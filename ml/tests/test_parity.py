"""Python side of the parity contract.

The C++ side lives in test/test_parity (pio test -e native). Together they
prove that the firmware and this package compute identical features,
z-scores, decisions and alerts for the same input.
"""

from pathlib import Path

from ecosmart_ml.export import load_model_json
from ecosmart_ml.parity import model_fingerprint, parity_samples, render_header
from ecosmart_ml.pipeline import run

ROOT = Path(__file__).resolve().parents[2]
MODEL = load_model_json(ROOT / "ml" / "model" / "model.json")


def test_committed_vectors_are_up_to_date():
    committed = (ROOT / "test" / "test_parity" / "parity_vectors.h").read_text()
    assert committed == render_header(MODEL), (
        "run: python ml/export_parity_vectors.py")


def test_vectors_cover_alerts_guard_and_clamping():
    samples = parity_samples()
    events = run(samples, model=MODEL)
    assert any(e.alert for e in events)
    assert any(e.leaf == 0xFE for e in events)  # saturation guard
    assert any(v < 0 or v > 10000 for s in samples for v in s)


def test_fingerprint_is_sensitive_to_the_model():
    import copy
    other = copy.deepcopy(MODEL)
    other.scale_floors[0] += 1
    assert model_fingerprint(other) != model_fingerprint(MODEL)
