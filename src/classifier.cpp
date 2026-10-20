#include "classifier.h"

#include "config.h"
#include "window_features.h"
#include "model_weights.h"
#include "platform.h"

// The generated model must agree with the firmware's window configuration.
static_assert(MODEL_NUM_FEATURES == features::kNumFeatures, "model feature count");
static_assert(MODEL_WINDOW_SIZE == cfg::kWindowSize, "model window size");
static_assert(MODEL_WINDOW_HOP == cfg::kWindowHop, "model window hop");
static_assert(MODEL_CALIBRATION_WINDOWS == cfg::kCalibrationWindows,
              "model calibration windows");
static_assert(MODEL_Z_SCALE == cfg::kZScale, "model z scale");
static_assert(MODEL_NUM_NODES <= 255, "node indices are uint8");

namespace classifier {

Prediction predictWith(const ModelNode* nodesP, uint8_t numNodes, const int8_t* z) {
  uint8_t i = 0;
  // A tree of n nodes is at most n levels deep; the bound guards against a
  // corrupted table looping forever.
  for (uint8_t step = 0; step < numNodes && i < numNodes; ++step) {
    ModelNode node;
    memcpy_P(&node, &nodesP[i], sizeof(node));
    if (node.feature == kLeafNode) {
      Prediction p = {node.right, node.left, i};
      return p;
    }
    i = z[node.feature] <= node.threshold ? node.left : node.right;
  }
  Prediction invalid = {0, 0, 0xFF};
  return invalid;
}

Prediction predict(const int8_t* z) {
  return predictWith(kModelNodes, MODEL_NUM_NODES, z);
}

const int32_t* scaleFloors() { return kModelScaleFloor; }

}  // namespace classifier
