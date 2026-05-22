#include "sensor_service.h"

#include <Arduino.h>
#include <M5Unified.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

constexpr std::uint8_t kSht40Address = 0x44;
constexpr std::uint8_t kSht40MeasureHighPrecision = 0xFD;
constexpr std::uint32_t kI2cFrequency = 100000;

std::uint8_t crc8(const std::uint8_t* data, std::size_t length) {
  std::uint8_t crc = 0xFF;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80U) ? static_cast<std::uint8_t>((crc << 1U) ^ 0x31U)
                          : static_cast<std::uint8_t>(crc << 1U);
    }
  }
  return crc;
}

SensorSnapshot unavailableSnapshot() {
  return SensorSnapshot{
      "--",
      "--",
      false,
  };
}

bool readMeasurement(std::uint8_t* buffer, std::size_t length) {
  if (!M5.In_I2C.start(kSht40Address, false, kI2cFrequency)) {
    return false;
  }
  if (!M5.In_I2C.write(kSht40MeasureHighPrecision)) {
    M5.In_I2C.stop();
    return false;
  }
  if (!M5.In_I2C.stop()) {
    return false;
  }

  delay(10);

  if (!M5.In_I2C.start(kSht40Address, true, kI2cFrequency)) {
    return false;
  }

  const bool readOk = M5.In_I2C.read(buffer, length, true);
  const bool stopOk = M5.In_I2C.stop();
  return readOk && stopOk;
}

}  // namespace

bool SensorService::begin() {
  available_ = M5.In_I2C.isEnabled() && M5.In_I2C.scanID(kSht40Address, kI2cFrequency);
  return available_;
}

SensorSnapshot SensorService::sample() {
  if (!available_) {
    available_ = M5.In_I2C.isEnabled() && M5.In_I2C.scanID(kSht40Address, kI2cFrequency);
  }

  if (!available_) {
    return unavailableSnapshot();
  }

  std::uint8_t data[6] = {};
  if (!readMeasurement(data, sizeof(data))) {
    return unavailableSnapshot();
  }

  if (crc8(data, 2) != data[2] || crc8(data + 3, 2) != data[5]) {
    return unavailableSnapshot();
  }

  const std::uint16_t rawTemperature =
      static_cast<std::uint16_t>(data[0] << 8U) | data[1];
  const std::uint16_t rawHumidity =
      static_cast<std::uint16_t>(data[3] << 8U) | data[4];

  const float temperature =
      -45.0f + 175.0f * static_cast<float>(rawTemperature) / 65535.0f;
  const float humidity = std::clamp(
      -6.0f + 125.0f * static_cast<float>(rawHumidity) / 65535.0f,
      0.0f,
      100.0f);

  char temperatureBuffer[16] = {};
  char humidityBuffer[8] = {};
  std::snprintf(temperatureBuffer, sizeof(temperatureBuffer), "%.1f°C", temperature);
  std::snprintf(
      humidityBuffer,
      sizeof(humidityBuffer),
      "%d%%",
      static_cast<int>(std::lround(humidity)));

  return SensorSnapshot{
      temperatureBuffer,
      humidityBuffer,
      true,
  };
}
