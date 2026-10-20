"""Same expectations as test/test_features (firmware)."""

from ecosmart_ml.features import MAX, MEAN, MIN, SLOPE, VAR, compute_channel

N = 32


def test_constant():
    assert compute_channel([552] * N) == [552, 0, 0, 552, 552]


def test_ramp():
    f = compute_channel([10 * i for i in range(N)])
    assert f[MEAN] == 155 and f[VAR] == 8525 and f[SLOPE] == 320
    assert f[MIN] == 0 and f[MAX] == 310


def test_falling_ramp():
    assert compute_channel([3700 - 5 * i for i in range(N)])[SLOPE] == -160


def test_alternating():
    f = compute_channel([600 if i % 2 else 500 for i in range(N)])
    assert f[MEAN] == 550 and f[VAR] == 2500


def test_extremes():
    f = compute_channel([0] * 16 + [10000] * 16)
    assert f[MEAN] == 5000 and f[VAR] == 25_000_000 and f[SLOPE] > 0
