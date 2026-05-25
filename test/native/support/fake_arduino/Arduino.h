#pragma once

#include <cstdint>
#include <string>

inline std::uint32_t gFakeMillis = 0;
inline std::string gFakeTimezone;
inline std::string gFakeNtpServer;

inline void configTzTime(const char* timezone, const char* server) {
  gFakeTimezone = timezone != nullptr ? timezone : "";
  gFakeNtpServer = server != nullptr ? server : "";
}
inline void (*gFakeDelayCallback)(unsigned long) = nullptr;

// Native host keeps unsigned long at the platform ABI width (64-bit on Darwin),
// so this fake only constrains the stored clock value to 32 bits. It cannot make
// production expressions like millis() - start execute with Arduino's 32-bit math.
inline unsigned long millis() {
  return static_cast<unsigned long>(gFakeMillis);
}

inline void fakeArduinoSetMillis(unsigned long value) {
  gFakeMillis = static_cast<std::uint32_t>(value);
}

constexpr std::uint8_t INPUT_PULLUP = 0x05;
inline int gFakeLastPinModePin = -1;
inline std::uint8_t gFakeLastPinModeMode = 0;
inline int gFakePinModeCalls = 0;

inline void pinMode(std::uint8_t pin, std::uint8_t mode) {
  ++gFakePinModeCalls;
  gFakeLastPinModePin = pin;
  gFakeLastPinModeMode = mode;
}

inline void fakeArduinoResetClock() {
  gFakeMillis = 0;
  gFakeDelayCallback = nullptr;
  gFakeLastPinModePin = -1;
  gFakeLastPinModeMode = 0;
  gFakePinModeCalls = 0;
}

inline void delay(unsigned long ms) {
  if (gFakeDelayCallback != nullptr) {
    gFakeDelayCallback(ms);
    return;
  }

  gFakeMillis = static_cast<std::uint32_t>(gFakeMillis + static_cast<std::uint32_t>(ms));
}
