#include "sim_signal.h"

#include "fixed_point.h"
#include "platform.h"

namespace {

const char kNameNone[] PROGMEM = "none";
const char kNameAcidic[] PROGMEM = "acidic_ph";
const char kNameTemp[] PROGMEM = "temperature_rise";
const char kNameSurge[] PROGMEM = "conductivity_surge";
const char kNameAlkaline[] PROGMEM = "alkaline_ph";
const char kNameDropout[] PROGMEM = "electrode_dropout";

// Personal baseline of the simulated wearer.
constexpr int16_t kPh = 560;     // pH 5.60
constexpr int16_t kCond = 850;   // 8.50 mS/cm
constexpr int16_t kTemp = 3420;  // 34.20 degC

// Triangle wave in [-amp, amp] with the given period (samples).
int32_t triangle(uint32_t t, uint16_t period, int16_t amp) {
  const int32_t phase = static_cast<int32_t>(t % period);
  const int32_t half = period / 2;
  const int32_t up = phase < half ? phase : period - phase;  // 0..half..0
  return fx::divRound((up * 4 - period) * static_cast<int32_t>(amp), period);
}

}  // namespace

SimSignal::SimSignal(uint32_t seed)
    : state_(seed ? seed : 1), t_(0), episode_(SimEpisode::kNone) {}

uint32_t SimSignal::rand32() {
  // xorshift32: tiny and good enough for sensor noise.
  state_ ^= state_ << 13;
  state_ ^= state_ >> 17;
  state_ ^= state_ << 5;
  return state_;
}

int16_t SimSignal::noise(int16_t amplitude) {
  // Sum of two uniforms -> triangular distribution, no floating point.
  const int32_t span = 2 * static_cast<int32_t>(amplitude) + 1;
  const int32_t a = static_cast<int32_t>(rand32() % span) - amplitude;
  const int32_t b = static_cast<int32_t>(rand32() % span) - amplitude;
  return static_cast<int16_t>((a + b) / 2);
}

void SimSignal::next(int16_t* ph, int16_t* cond, int16_t* temp) {
  const uint16_t inCycle = static_cast<uint16_t>(t_ % kCycle);
  const uint32_t cycle = t_ / kCycle;

  // Episode intensity 0..256 with linear onset and recovery ramps.
  int32_t env = 0;
  episode_ = SimEpisode::kNone;
  if (inCycle >= kEpisodeStart) {
    const uint16_t into = inCycle - kEpisodeStart;
    const uint16_t left = kCycle - 1 - inCycle;
    const uint16_t edge = into < left ? into : left;
    env = edge >= kRamp ? 256 : (static_cast<int32_t>(edge) * 256) / kRamp;
    episode_ = static_cast<SimEpisode>(1 + cycle % kNumEpisodeTypes);
  }

  // Normal physiology: slow drift + sensor noise around the baseline.
  int32_t p = kPh + triangle(t_, 420, 4) + noise(3);
  int32_t c = kCond + triangle(t_ + 100, 540, 25);
  int32_t tc = kTemp + triangle(t_, 600, 8) + noise(2);

  switch (episode_) {
    case SimEpisode::kAcidicPh:
      p -= (110 * env) / 256;
      break;
    case SimEpisode::kTemperatureRise:
      tc += (120 * env) / 256;
      break;
    case SimEpisode::kConductivitySurge:
      c += (c * 80 / 100) * env / 256;
      break;
    case SimEpisode::kAlkalinePh:
      p += (130 * env) / 256;
      break;
    case SimEpisode::kElectrodeDropout:
      c -= (c * 90 / 100) * env / 256;
      break;
    case SimEpisode::kNone:
      break;
  }
  c += c * noise(12) / 1000;  // ~1.2 % multiplicative noise

  // DS18B20 delivers 1/16 degC steps: quantise like the real sensor.
  const int32_t raw = fx::divRound(tc * 16, 100);
  tc = fx::divRound(raw * 100, 16);

  *ph = static_cast<int16_t>(fx::clamp(p, 0, 1400));
  *cond = static_cast<int16_t>(fx::clamp(c, 0, 10000));
  *temp = static_cast<int16_t>(tc);
  ++t_;
}

const char* SimSignal::name(SimEpisode e) {
  switch (e) {
    case SimEpisode::kAcidicPh:
      return kNameAcidic;
    case SimEpisode::kTemperatureRise:
      return kNameTemp;
    case SimEpisode::kConductivitySurge:
      return kNameSurge;
    case SimEpisode::kAlkalinePh:
      return kNameAlkaline;
    case SimEpisode::kElectrodeDropout:
      return kNameDropout;
    case SimEpisode::kNone:
      break;
  }
  return kNameNone;
}
