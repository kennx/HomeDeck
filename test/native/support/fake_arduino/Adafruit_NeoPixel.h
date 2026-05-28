#pragma once

#include <cstdint>

using neoPixelType = std::uint16_t;

inline constexpr neoPixelType NEO_GRB = 0x0001;
inline constexpr neoPixelType NEO_KHZ800 = 0x0100;

inline int gFakeNeoPixelBeginCount = 0;
inline int gFakeNeoPixelClearCount = 0;
inline int gFakeNeoPixelShowCount = 0;

inline void fakeNeoPixelReset() {
  gFakeNeoPixelBeginCount = 0;
  gFakeNeoPixelClearCount = 0;
  gFakeNeoPixelShowCount = 0;
}

class Adafruit_NeoPixel {
 public:
  Adafruit_NeoPixel(std::uint16_t, std::int16_t, neoPixelType) {
  }

  void begin() {
    ++gFakeNeoPixelBeginCount;
  }

  void clear() {
    ++gFakeNeoPixelClearCount;
  }

  void show() {
    ++gFakeNeoPixelShowCount;
  }
};
