// Storage layout of the on-device decision tree (generated into
// model_weights.h by ml/train.py and kept in flash via PROGMEM).
//
// Internal node: feature >= 0; go to `left` if z[feature] <= threshold,
//                otherwise to `right`. Thresholds are in the same quantised
//                z units (1/16 sigma) as the features, so inference is pure
//                int8 comparisons - no multiplications, no floats.
// Leaf node:     feature == kLeafNode; `left` holds the share of anomalous
//                training windows that reached this leaf (percent) and
//                `right` the predicted label (0 = normal, 1 = anomaly).
#pragma once

#include <stdint.h>

struct ModelNode {
  int8_t feature;
  int8_t threshold;
  uint8_t left;
  uint8_t right;
};

constexpr int8_t kLeafNode = -1;

static_assert(sizeof(ModelNode) == 4, "ModelNode must stay 4 bytes");
