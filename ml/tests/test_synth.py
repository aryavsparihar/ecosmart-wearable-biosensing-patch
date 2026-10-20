import numpy as np

from ecosmart_ml.synth import ANOMALY_TYPES, envelope, generate_dataset


def test_reproducible_with_seed():
    a = generate_dataset(3, seed=7, n_samples=600)
    b = generate_dataset(3, seed=7, n_samples=600)
    for x, y in zip(a, b):
        assert np.array_equal(x.ph, y.ph) and np.array_equal(x.cond, y.cond)
        assert np.array_equal(x.temp, y.temp) and np.array_equal(x.label, y.label)


def test_values_are_on_firmware_grid_and_plausible():
    for s in generate_dataset(10, seed=1, n_samples=1200):
        normal = s.label == 0
        assert s.ph.dtype.kind == "i"
        assert 400 <= np.median(s.ph[normal]) <= 700
        assert 300 <= np.median(s.cond[normal]) <= 1600
        assert 3250 <= np.median(s.temp[normal]) <= 3600


def test_episodes_are_labelled_and_after_calibration():
    subjects = generate_dataset(20, seed=3, n_samples=2400)
    episodes = [e for s in subjects for e in s.episodes]
    assert episodes, "expected at least one anomaly episode"
    for s in subjects:
        assert s.label[:200].sum() == 0
        for e in s.episodes:
            assert e["type"] in ANOMALY_TYPES
            assert s.label[e["start"]:e["end"]].any()


def test_envelope_shape():
    env = envelope(100, start=20, duration=50, ramp_in=10, ramp_out=10)
    assert env[0] == 0 and env[45] == 1 and env[99] == 0
    assert np.all((env >= 0) & (env <= 1))
