#pragma once

#include <cstdint>

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
