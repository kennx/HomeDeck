#pragma once

#include <cstdint>

#if defined(UNIT_TEST)
struct LedServicePixelState {
  bool begun = false;
  std::uint8_t brightness = 0;
  std::uint8_t r[2] = {0, 0};
  std::uint8_t g[2] = {0, 0};
  std::uint8_t b[2] = {0, 0};
  int showCalls = 0;
};

const LedServicePixelState& ledServicePixelStateForTest();
void resetLedServicePixelStateForTest();
#endif

class LedService {
 public:
  LedService() = default;

  bool begin();
  void update();
  void turnOff();

  bool isUsbConnected() const;
  int getBatteryLevel() const;

 private:
  void setRgb(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t brightness);
};
