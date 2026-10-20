#include "temperature_sensor.h"

#include "sensor_math.h"

namespace {
constexpr uint8_t kCmdSkipRom = 0xCC;
constexpr uint8_t kCmdConvertT = 0x44;
constexpr uint8_t kCmdReadScratchpad = 0xBE;
constexpr uint32_t kConversionMs = 750;  // 12-bit resolution worst case
}  // namespace

TemperatureSensor::TemperatureSensor(uint8_t pin)
    : bus_(pin),
      requestedAtMs_(0),
      centi_(0),
      present_(false),
      converting_(false),
      valid_(false) {}

bool TemperatureSensor::begin() {
  present_ = bus_.reset() != 0;
  return present_;
}

void TemperatureSensor::update(uint32_t nowMs) {
  if (!converting_) {
    if (startConversion()) requestedAtMs_ = nowMs;
    return;
  }
  if (nowMs - requestedAtMs_ < kConversionMs) return;

  int16_t raw;
  if (readScratchpad(&raw)) {
    centi_ = sensor_math::ds18b20RawToCenti(raw);
    valid_ = true;
  }
  converting_ = false;
}

bool TemperatureSensor::startConversion() {
  present_ = bus_.reset() != 0;
  if (!present_) return false;
  bus_.write(kCmdSkipRom);
  bus_.write(kCmdConvertT);
  converting_ = true;
  return true;
}

bool TemperatureSensor::readScratchpad(int16_t* raw) {
  if (!bus_.reset()) {
    present_ = false;
    return false;
  }
  bus_.write(kCmdSkipRom);
  bus_.write(kCmdReadScratchpad);
  uint8_t data[9];
  for (uint8_t i = 0; i < 9; ++i) data[i] = bus_.read();
  if (OneWire::crc8(data, 8) != data[8]) return false;  // corrupted frame
  *raw = static_cast<int16_t>((static_cast<uint16_t>(data[1]) << 8) | data[0]);
  return true;
}
