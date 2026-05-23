#include "led_service.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <cmath>

#if !defined(UNIT_TEST)
#include <Adafruit_NeoPixel.h>

static constexpr uint8_t LED_PIN   = 21;
static constexpr uint8_t LED_COUNT = 2;

static Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static bool neopixelReady = false;
#endif

bool LedService::begin() {
#if !defined(UNIT_TEST)
  Serial.printf("[LedService] Board: %d, VBUS: %d mV, Bat: %d%%\n",
                (int)M5.getBoard(), M5.Power.getVBUSVoltage(), M5.Power.getBatteryLevel());

  // Ensure LDO3V3 (RGB power) is enabled via M5PM1
  if (M5.In_I2C.isEnabled()) {
    uint8_t reg06 = M5.In_I2C.readRegister8(0x6E, 0x06, 100000);
    Serial.printf("[LedService] PMU 0x06 = 0x%02X (LDO=%d)\n", reg06, (reg06 >> 2) & 1);
    if (!(reg06 & (1 << 2))) {
      M5.In_I2C.bitOn(0x6E, 0x06, 1 << 2, 100000);
      Serial.println("[LedService] LDO3V3 was off, enabled it.");
    }
  }

  // Use Adafruit_NeoPixel directly — bypasses M5.Led entirely
  pixels.begin();
  pixels.setBrightness(80);
  neopixelReady = true;
  Serial.println("[LedService] Adafruit_NeoPixel initialized on GPIO21.");
#endif

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
    return (val & 0x07) == 1;
  }
#endif
  return false;
}

int LedService::getBatteryLevel() const {
  return M5.Power.getBatteryLevel();
}

void LedService::setRgb(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t brightness) {
#if !defined(UNIT_TEST)
  if (neopixelReady) {
    pixels.setBrightness(brightness);
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.setPixelColor(1, pixels.Color(r, g, b));
    pixels.show();
  }
#else
  M5.Led.setBrightness(brightness);
  M5.Led.setAllColor(r, g, b);
  M5.Led.display();
#endif
}

void LedService::turnOff() {
  setRgb(0, 0, 0, 0);
}

void LedService::update() {
#if !defined(UNIT_TEST)
  static unsigned long lastLogMs = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastLogMs >= 5000UL) {
    lastLogMs = nowMs;
    Serial.printf("[LedService] update: usb_conn=%d, bat=%d%%, vbus=%d mV\n",
                  isUsbConnected(), getBatteryLevel(), M5.Power.getVBUSVoltage());
  }
#endif

  if (isUsbConnected()) {
    int batteryLevel = getBatteryLevel();
    if (batteryLevel >= 100) {
      setRgb(0, 255, 0, 60);
    } else {
      setRgb(255, 128, 0, 60);
    }
  } else {
    int batteryLevel = getBatteryLevel();
    if (batteryLevel >= 0 && batteryLevel < 5) {
      unsigned long ms = millis();
      float angle = (ms % 1500) * 2.0f * 3.14159265f / 1500.0f;
      std::uint8_t brightness = static_cast<std::uint8_t>((std::sin(angle) + 1.0f) * 40.0f + 10.0f);
      setRgb(255, 0, 0, brightness);
    } else {
      turnOff();
    }
  }
}
