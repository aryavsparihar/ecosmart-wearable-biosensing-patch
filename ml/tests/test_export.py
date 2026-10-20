import itertools

import numpy as np
from sklearn.tree import DecisionTreeClassifier

from ecosmart_ml import config as C
from ecosmart_ml.export import (load_model_json, save_model_json, tree_depth,
                                tree_from_sklearn, write_header)
from ecosmart_ml.pipeline import LEAF


def _toy(seed=0, n=4000):
    rng = np.random.default_rng(seed)
    X = rng.integers(-127, 128, size=(n, C.NUM_FEATURES))
    y = ((np.abs(X[:, 0]) > 60) | (X[:, 10] > 40)).astype(int)
    return X, y


def test_quantised_tree_matches_sklearn_decisions():
    X, y = _toy()
    clf = DecisionTreeClassifier(max_depth=4, min_samples_leaf=10, random_state=0).fit(X, y)
    model = tree_from_sklearn(clf, X, y, [1] * C.NUM_FEATURES)
    Xq, _ = _toy(seed=1)
    ours = np.array([model.predict(z)[0] for z in Xq])
    assert np.array_equal(ours, clf.predict(Xq))
    # Integer thresholds are exact on the integer grid, boundaries included.
    for v in range(-127, 128):
        z = [0] * C.NUM_FEATURES
        z[0] = v
        assert model.predict(z)[0] == clf.predict([z])[0]


def test_redundant_splits_are_collapsed():
    X, y = _toy()
    clf = DecisionTreeClassifier(max_depth=6, min_samples_leaf=5, random_state=0).fit(X, y)
    model = tree_from_sklearn(clf, X, y, [1] * C.NUM_FEATURES)
    assert len(model.nodes) <= clf.tree_.node_count
    for f, _, l, r in model.nodes:
        if f != LEAF:
            a, b = model.nodes[l], model.nodes[r]
            assert not (a[0] == b[0] == LEAF and a[3] == b[3])
    assert tree_depth(model) <= 6


def test_header_and_json_round_trip(tmp_path):
    X, y = _toy()
    clf = DecisionTreeClassifier(max_depth=3, random_state=0).fit(X, y)
    model = tree_from_sklearn(clf, X, y, list(range(1, 16)))
    header = tmp_path / "model_weights.h"
    write_header(model, header, {"Seed": 0})
    text = header.read_text()
    assert f"#define MODEL_NUM_NODES {len(model.nodes)}" in text
    assert "PROGMEM" in text and "SYNTHETIC" in text
    save_model_json(model, tmp_path / "m.json")
    again = load_model_json(tmp_path / "m.json")
    assert again.nodes == model.nodes and again.scale_floors == model.scale_floors
    for z in itertools.islice(X, 200):
        assert again.predict(z) == model.predict(z)
