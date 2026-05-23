#include "led_service.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <cmath>

bool LedService::begin() {
  turnOff();
  return true;
}

bool LedService::isUsbConnected() const {
  if (M5.Power.getVBUSVoltage() > 4000) {
    return true;
  }
#if !defined(UNIT_TEST)
  if (M5.In_I2C.isEnabled()) {
    std::uint8_t val = M5.In_I2C.readRegister8(0x6E, 0x04, 100000);
    return (val & 0x07) == 1; // 1 = 5VIN
  }
#endif
  return false;
}

int LedService::getBatteryLevel() const {
  return M5.Power.getBatteryLevel();
}

void LedService::setRgb(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t brightness) {
  M5.Led.setBrightness(brightness);
  M5.Led.setAllColor(r, g, b);
  M5.Led.display();
}

void LedService::turnOff() {
  setRgb(0, 0, 0, 0);
}

void LedService::update() {
  if (isUsbConnected()) {
    int batteryLevel = getBatteryLevel();
    if (batteryLevel >= 100) {
      setRgb(0, 255, 0, 60); // Steady Green
    } else {
      setRgb(255, 128, 0, 60); // Steady Orange
    }
  } else {
    int batteryLevel = getBatteryLevel();
    if (batteryLevel >= 0 && batteryLevel < 5) {
      // Breathing Red LED (1.5s period) using sine wave
      unsigned long ms = millis();
      float angle = (ms % 1500) * 2.0f * 3.14159265f / 1500.0f;
      std::uint8_t brightness = static_cast<std::uint8_t>((std::sin(angle) + 1.0f) * 40.0f + 10.0f); // 10 to 90
      setRgb(255, 0, 0, brightness);
    } else {
      turnOff();
    }
  }
}
