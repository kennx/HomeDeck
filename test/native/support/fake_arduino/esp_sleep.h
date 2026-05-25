#pragma once

#include <cstdint>

#include "Arduino.h"

using esp_err_t = int;
using gpio_num_t = int;

enum esp_deepsleep_gpio_wake_up_mode_t {
  ESP_GPIO_WAKEUP_GPIO_LOW = 0,
  ESP_GPIO_WAKEUP_GPIO_HIGH = 1,
};

enum esp_sleep_wakeup_cause_t {
  ESP_SLEEP_WAKEUP_UNDEFINED = 0,
  ESP_SLEEP_WAKEUP_TIMER = 1,
};

inline esp_sleep_wakeup_cause_t gFakeWakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
inline uint64_t gFakeSleepDurationUs = 0;
inline bool gDeepSleepCalled = false;
inline bool gGpioHoldCalled = false;
inline bool gFakeTimerWakeupConfigured = false;
inline bool gFakeGpioWakeupConfigured = false;
inline uint64_t gFakeGpioWakeupMask = 0;
inline esp_deepsleep_gpio_wake_up_mode_t gFakeGpioWakeupMode = ESP_GPIO_WAKEUP_GPIO_LOW;
inline std::uint8_t gFakeRtcMemory[512] = {};

inline void fakeEspSleepSetWakeupCause(esp_sleep_wakeup_cause_t cause) {
  gFakeWakeupCause = cause;
}

inline void fakeEspSleepReset() {
  gFakeWakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
  gFakeSleepDurationUs = 0;
  gDeepSleepCalled = false;
  gGpioHoldCalled = false;
  gFakeTimerWakeupConfigured = false;
  gFakeGpioWakeupConfigured = false;
  gFakeGpioWakeupMask = 0;
  gFakeGpioWakeupMode = ESP_GPIO_WAKEUP_GPIO_LOW;
  for (auto& byte : gFakeRtcMemory) {
    byte = 0;
  }
}

inline void fakeEspSleepReboot() {
  fakeArduinoResetClock();
  gFakeWakeupCause = (gDeepSleepCalled && gFakeTimerWakeupConfigured)
      ? ESP_SLEEP_WAKEUP_TIMER
      : ESP_SLEEP_WAKEUP_UNDEFINED;
  gFakeSleepDurationUs = 0;
  gDeepSleepCalled = false;
  gGpioHoldCalled = false;
  gFakeTimerWakeupConfigured = false;
  gFakeGpioWakeupConfigured = false;
  gFakeGpioWakeupMask = 0;
  gFakeGpioWakeupMode = ESP_GPIO_WAKEUP_GPIO_LOW;
}

inline std::uint8_t* fakeEspSleepRtcMemory() {
  return gFakeRtcMemory;
}

inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() {
  return gFakeWakeupCause;
}

inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
  gFakeSleepDurationUs = time_in_us;
  gFakeTimerWakeupConfigured = true;
}

inline void esp_deep_sleep_enable_gpio_wakeup(
    uint64_t gpio_pin_mask,
    esp_deepsleep_gpio_wake_up_mode_t mode) {
  gFakeGpioWakeupConfigured = true;
  gFakeGpioWakeupMask = gpio_pin_mask;
  gFakeGpioWakeupMode = mode;
}

inline int gFakeExt0Gpio = -1;
inline int gFakeExt0Level = 0;

inline void fakeEspSleepResetExt0() {
  gFakeExt0Gpio = -1;
  gFakeExt0Level = 0;
}

inline esp_err_t esp_sleep_enable_ext0_wakeup(int gpio_num, int level) {
  gFakeExt0Gpio = gpio_num;
  gFakeExt0Level = level;
  return 0;
}

inline void esp_deep_sleep_start() {
  gDeepSleepCalled = true;
}

inline void gpio_deep_sleep_hold_en() {
  gGpioHoldCalled = true;
}
