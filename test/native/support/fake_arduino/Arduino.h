#pragma once

#include <cstdint>

inline void (*gFakeDelayCallback)(unsigned long) = nullptr;

inline void delay(unsigned long) {
  if (gFakeDelayCallback != nullptr) {
    gFakeDelayCallback(0);
  }
}
