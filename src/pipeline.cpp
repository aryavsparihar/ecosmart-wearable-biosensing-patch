#include "pipeline.h"

#include "fixed_point.h"

namespace {

int16_t clampSample(int16_t v) {
  return static_cast<int16_t>(fx::clamp(v, cfg::kSampleMin, cfg::kSampleMax));
}

}  // namespace

AnomalyPipeline::AnomalyPipeline() { reset(); }

void AnomalyPipeline::reset() {
  for (uint8_t c = 0; c < cfg::kNumChannels; ++c) buffers_[c].clear();
  baseline_.reset();
  alert_.reset();
  for (uint8_t i = 0; i < features::kNumFeatures; ++i) {
    features_[i] = 0;
    z_[i] = 0;
  }
  Prediction none = {0, 0, 0};
  lastPrediction_ = none;
  windows_ = 0;
  sinceLastWindow_ = 0;
  firstWindowDone_ = false;
  phase_ = Phase::kWarmup;
}

WindowResult AnomalyPipeline::push(int16_t ph, int16_t cond, int16_t temp) {
  buffers_[cfg::kChPh].push(clampSample(ph));
  buffers_[cfg::kChConductivity].push(clampSample(cond));
  buffers_[cfg::kChTemperature].push(clampSample(temp));
  if (sinceLastWindow_ < 0xFF) ++sinceLastWindow_;

  WindowResult r = {false, phase_, false, lastPrediction_, alert_.active()};
  if (!buffers_[0].full()) return r;
  if (firstWindowDone_ && sinceLastWindow_ < cfg::kWindowHop) return r;
  sinceLastWindow_ = 0;
  firstWindowDone_ = true;
  ++windows_;

  int16_t window[cfg::kWindowSize];
  for (uint8_t c = 0; c < cfg::kNumChannels; ++c) {
    buffers_[c].copyTo(window);
    features::computeChannel(window, cfg::kWindowSize,
                             &features_[c * features::kPerChannel]);
  }
  r.ready = true;

  if (!baseline_.ready()) {
    baseline_.accumulate(features_);
    if (baseline_.count() >= cfg::kCalibrationWindows) {
      baseline_.finalize(classifier::scaleFloors());
      phase_ = Phase::kMonitoring;
    } else {
      phase_ = Phase::kCalibrating;
    }
    r.phase = phase_;
    return r;
  }

  baseline_.normalize(features_, z_);
  lastPrediction_ = classifier::predict(z_);
  r.classified = true;
  r.prediction = lastPrediction_;
  r.alert = alert_.update(lastPrediction_.label == 1);
  r.phase = phase_;
  return r;
}
