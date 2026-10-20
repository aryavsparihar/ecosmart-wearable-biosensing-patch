from ecosmart_ml import config as C
from ecosmart_ml.pipeline import (LEAF, PHASE_CALIBRATING, PHASE_MONITORING,
                                  AlertLogic, Baseline, TreeModel, run)

FLOORS = [5] * C.NUM_FEATURES


def test_baseline_matches_firmware_test():
    b = Baseline()
    for w in range(8):
        b.accumulate([110 if w % 2 else 90] * C.NUM_FEATURES)
    b.finalize(FLOORS)
    assert b.mean[0] == 100 and b.scale[0] == 10
    z = b.normalize([120, 85, 10000, -9000] + [100] * 11)
    assert z[:5] == [32, -24, 127, -127, 0]


def test_alert_hysteresis():
    a = AlertLogic(2, 3)
    assert [a.update(x) for x in (True, True, False, True, False, False, False)] == [
        False, True, True, True, True, True, False]


def test_tree_predict():
    # z[0] <= 10 -> normal leaf, else anomaly leaf
    model = TreeModel(nodes=[(0, 10, 1, 2), (LEAF, 0, 5, 0), (LEAF, 0, 95, 1)],
                      scale_floors=FLOORS)
    assert model.predict([10] + [0] * 14) == (0, 5, 1)
    assert model.predict([11] + [0] * 14) == (1, 95, 2)


def test_window_schedule_and_phases():
    samples = [(550, 900, 3400)] * 200
    events = run(samples, floors=FLOORS)
    assert events[0].end_sample == C.WINDOW_SIZE - 1
    assert all(b.end_sample - a.end_sample == C.WINDOW_HOP for a, b in zip(events, events[1:]))
    assert events[C.CALIBRATION_WINDOWS - 2].phase == PHASE_CALIBRATING
    assert events[C.CALIBRATION_WINDOWS - 1].phase == PHASE_MONITORING
    assert events[C.CALIBRATION_WINDOWS].z == [0] * C.NUM_FEATURES
