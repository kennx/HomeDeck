# EPD 启动刷新与 RGB 断电修复 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修复 `appSetup()` 中未真正执行的 EPD 高质量基线刷新，并让 deep sleep 前的 RGB LED 同时完成灭灯和断供电。

**Architecture:** 先补 native fake 支撑，让 `M5PM1`、`Adafruit_NeoPixel` 和 `epd_mode_t` 能进入 `UNIT_TEST` 构建；再先写失败测试锁定行为，最后在 `app_runtime.cpp` 中抽出 `prepareEpdAfterWakeup()` 与 RGB 关闭辅助函数，用最小代码让测试通过。

**Tech Stack:** PlatformIO (Arduino/ESP32-S3), M5Unified + M5GFX, M5PM1, Adafruit NeoPixel, Unity native testing

---

## 文件结构

| 文件 | 责任 |
|------|------|
| `test/native/support/fake_arduino/M5Unified.h` | 为 `FakeDisplay` 增加 `epd_mode_t`、`clear()`、`setEpdMode()` 和 EPD 事件日志 |
| `test/native/support/fake_arduino/M5PM1.h` | 提供 `M5PM1` fake、LDO 调用记录和 reset helper |
| `test/native/support/fake_arduino/Adafruit_NeoPixel.h` | 提供 `Adafruit_NeoPixel` fake、`begin/clear/show` 计数和 reset helper |
| `test/native/test_app_runtime/test_main.cpp` | 新增 EPD 启动刷新、RGB 初始化、sleep 前 RGB 断电测试 |
| `src/app_runtime.cpp` | 拆出 `prepareEpdAfterWakeup()`、`clearRgbLedPixels()`、`disableRgbLedPower()`、`shutdownRgbLedForSleep()` 并接入现有流程 |

---

### Task 1: 补齐 native fake 支撑

**Files:**
- Modify: `test/native/support/fake_arduino/M5Unified.h`
- Create: `test/native/support/fake_arduino/M5PM1.h`
- Create: `test/native/support/fake_arduino/Adafruit_NeoPixel.h`
- Test: `test/native/test_app_runtime/test_main.cpp`

- [ ] **Step 1: 在 `M5Unified.h` 中补 `epd_mode_t` 和 Display 事件日志**

在 `test/native/support/fake_arduino/M5Unified.h` 的 `TFT_WHITE` 常量下方添加：

```cpp
enum class epd_mode_t {
  epd_quality = 0,
  epd_text = 1,
  epd_fast = 2,
  epd_fastest = 3,
};
```

在 `FakeDisplay` 成员区 `wakeupCount` 下方添加：

```cpp
  struct FakeEpdEvent {
    enum class Type {
      SetMode,
      Wakeup,
      Clear,
      WaitDisplay,
    };

    Type type = Type::Wakeup;
    epd_mode_t mode = epd_mode_t::epd_fast;
    std::uint32_t color = 0;
  };

  int clearCount = 0;
  std::vector<std::uint32_t> clearColors;
  std::vector<epd_mode_t> epdModes;
  std::vector<FakeEpdEvent> epdEvents;
```

把 `setEpdMode(int)` 替换为下面两个方法：

```cpp
  void clear(std::uint32_t color) {
    ++clearCount;
    clearColors.push_back(color);
    fillScreenColor = color;
    epdEvents.push_back({FakeEpdEvent::Type::Clear, epd_mode_t::epd_fast, color});
  }

  void setEpdMode(epd_mode_t mode) {
    epdModes.push_back(mode);
    epdEvents.push_back({FakeEpdEvent::Type::SetMode, mode, 0});
  }
```

并把 `wakeup()` 与 `waitDisplay()` 分别改成：

```cpp
  void wakeup() {
    ++wakeupCount;
    epdEvents.push_back({FakeEpdEvent::Type::Wakeup, epd_mode_t::epd_fast, 0});
  }

  void waitDisplay() {
    ++waitDisplayCount;
    epdEvents.push_back({FakeEpdEvent::Type::WaitDisplay, epd_mode_t::epd_fast, 0});
  }
```

- [ ] **Step 2: 创建 `M5PM1.h` fake**

新建 `test/native/support/fake_arduino/M5PM1.h`：

```cpp
#pragma once

#include <cstdint>
#include <vector>

struct FakeI2C;

using m5pm1_err_t = int;

inline constexpr m5pm1_err_t M5PM1_OK = 0;
inline constexpr std::uint8_t M5PM1_DEFAULT_ADDR = 0x6e;
inline constexpr std::uint32_t M5PM1_I2C_FREQ_100K = 100000;

inline int gFakePm1BeginCount = 0;
inline bool gFakePm1BeginShouldFail = false;
inline std::vector<bool> gFakePm1LdoEnableCalls;

inline void fakeM5Pm1Reset() {
  gFakePm1BeginCount = 0;
  gFakePm1BeginShouldFail = false;
  gFakePm1LdoEnableCalls.clear();
}

class M5PM1 {
 public:
  m5pm1_err_t begin(FakeI2C*, std::uint8_t, std::uint32_t) {
    ++gFakePm1BeginCount;
    return gFakePm1BeginShouldFail ? -1 : M5PM1_OK;
  }

  m5pm1_err_t setLdoEnable(bool enable) {
    gFakePm1LdoEnableCalls.push_back(enable);
    return M5PM1_OK;
  }
};
```

- [ ] **Step 3: 创建 `Adafruit_NeoPixel.h` fake**

新建 `test/native/support/fake_arduino/Adafruit_NeoPixel.h`：

```cpp
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
```

- [ ] **Step 4: 运行现有 `app_runtime` 测试确认 fake 支撑没有打坏旧行为**

Run: `pio test -e native -f test_app_runtime`
Expected: 现有 12 个测试继续通过，尚未新增 EPD/RGB 测试时不应出现编译错误

- [ ] **Step 5: Commit**

```bash
git add test/native/support/fake_arduino/M5Unified.h test/native/support/fake_arduino/M5PM1.h test/native/support/fake_arduino/Adafruit_NeoPixel.h
git commit -m "test: 补齐 EPD 与 RGB native fake 支撑

- FakeDisplay 增加 clear 和 EPD mode 调用记录
- 新增 M5PM1 fake 以记录 LDO 开关
- 新增 NeoPixel fake 以记录 begin clear show 调用"
```

---

### Task 2: 先写失败测试锁定目标行为

**Files:**
- Modify: `test/native/test_app_runtime/test_main.cpp`

- [ ] **Step 1: 引入新的 fake 头并在 `setUp()` 中重置计数器**

在文件头部 `#include <M5Unified.h>` 下方添加：

```cpp
#include <Adafruit_NeoPixel.h>
#include <M5PM1.h>
```

把 `setUp()` 改成下面这样：

```cpp
void setUp() {
  M5 = FakeM5Global{};
  fakeArduinoResetClock();
  fakeSntpReset();
  fakePreferencesReset();
  fakeEspSleepReset();
  fakeEspSleepResetExt0();
  fakeRtcIoReset();
  fakeM5Pm1Reset();
  fakeNeoPixelReset();
  homedeck::gRtcSavedView = homedeck::SystemView::Almanac;
}
```

- [ ] **Step 2: 新增 `appSetup()` 的 EPD 刷新时序测试**

在 `test_app_setup_reapplies_timezone_after_rtc_restore()` 下方添加：

```cpp
void test_app_setup_performs_quality_epd_baseline_refresh_before_fast_mode() {
  gFakePreferenceBools["configured"] = true;

  homedeck::appSetup();

  TEST_ASSERT_EQUAL(1, M5.Display.wakeupCount);
  TEST_ASSERT_EQUAL(1, M5.Display.clearCount);
  TEST_ASSERT_EQUAL_HEX32(TFT_WHITE, M5.Display.clearColors[0]);
  TEST_ASSERT_EQUAL(1, M5.Display.waitDisplayCount);
  TEST_ASSERT_EQUAL(2, static_cast<int>(M5.Display.epdModes.size()));
  TEST_ASSERT_EQUAL(static_cast<int>(epd_mode_t::epd_quality), static_cast<int>(M5.Display.epdModes[0]));
  TEST_ASSERT_EQUAL(static_cast<int>(epd_mode_t::epd_fast), static_cast<int>(M5.Display.epdModes[1]));
}
```

- [ ] **Step 3: 新增 RGB 初始化测试**

紧接着添加：

```cpp
void test_app_setup_enables_rgb_power_and_keeps_pixels_off() {
  gFakePreferenceBools["configured"] = true;

  homedeck::appSetup();

  TEST_ASSERT_EQUAL(1, gFakePm1BeginCount);
  TEST_ASSERT_EQUAL(1, static_cast<int>(gFakePm1LdoEnableCalls.size()));
  TEST_ASSERT_TRUE(gFakePm1LdoEnableCalls[0]);
  TEST_ASSERT_EQUAL(1, gFakeNeoPixelBeginCount);
  TEST_ASSERT_EQUAL(1, gFakeNeoPixelClearCount);
  TEST_ASSERT_EQUAL(1, gFakeNeoPixelShowCount);
}
```

- [ ] **Step 4: 新增 deep sleep 前 RGB 断电测试**

在现有 `test_enter_home_deep_sleep_configures_timer_button_c_gpio_and_display_sleep()` 下方添加：

```cpp
void test_enter_home_deep_sleep_turns_off_rgb_pixels_and_ldo_before_sleep() {
  gFakePreferenceBools["configured"] = true;
  homedeck::appSetup();

  const int clearCountBeforeSleep = gFakeNeoPixelClearCount;
  const int showCountBeforeSleep = gFakeNeoPixelShowCount;
  gFakePm1LdoEnableCalls.clear();

  homedeck::HomeSleepRequest request{};
  request.timerWakeupUs = 43200000000ULL;
  request.wakeupGpio = 1;
  request.wakeOnLow = true;

  homedeck::enterHomeDeepSleep(request);

  TEST_ASSERT_EQUAL(clearCountBeforeSleep + 1, gFakeNeoPixelClearCount);
  TEST_ASSERT_EQUAL(showCountBeforeSleep + 1, gFakeNeoPixelShowCount);
  TEST_ASSERT_EQUAL(1, static_cast<int>(gFakePm1LdoEnableCalls.size()));
  TEST_ASSERT_FALSE(gFakePm1LdoEnableCalls[0]);
}
```

- [ ] **Step 5: 注册新测试到 `main()`**

在 `main()` 里插入：

```cpp
  RUN_TEST(test_enter_home_deep_sleep_turns_off_rgb_pixels_and_ldo_before_sleep);
  RUN_TEST(test_app_setup_performs_quality_epd_baseline_refresh_before_fast_mode);
  RUN_TEST(test_app_setup_enables_rgb_power_and_keeps_pixels_off);
```

- [ ] **Step 6: 运行测试并确认它们失败**

Run: `pio test -e native -f test_app_runtime`
Expected: 新增 3 个测试失败

Expected failure summary:
- `test_app_setup_performs_quality_epd_baseline_refresh_before_fast_mode`: `epdEvents` 中找不到连续的 `SetMode(epd_quality) -> Wakeup -> Clear(TFT_WHITE) -> WaitDisplay -> SetMode(epd_fast)`
- `test_app_setup_enables_rgb_power_and_keeps_pixels_off`: `gFakePm1BeginCount` 仍为 `0`
- `test_enter_home_deep_sleep_turns_off_rgb_pixels_and_ldo_before_sleep`: `gFakePm1LdoEnableCalls` 为空

---

### Task 3: 用最小实现让测试转绿

**Files:**
- Modify: `src/app_runtime.cpp`
- Test: `test/native/test_app_runtime/test_main.cpp`

- [ ] **Step 1: 让 `UNIT_TEST` 构建也能编译 RGB 相关代码**

把 `src/app_runtime.cpp` 顶部 include 改成：

```cpp
#include <Arduino.h>
#include <ESP.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_sntp.h>
#include <esp_sleep.h>
#include <time.h>

#include <Adafruit_NeoPixel.h>
#include <M5PM1.h>

#ifndef UNIT_TEST
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <esp_attr.h>
#endif
```

同时把 RGB 常量、`gPm1`、`gPixels`、`gPm1Ready` 和相关辅助函数从 `#ifndef UNIT_TEST` 块中拿出来，保留 `i2cBusRecovery()` 仍只在非测试构建中存在。

- [ ] **Step 2: 添加 EPD 和 RGB 的职责分离辅助函数**

在匿名命名空间里用下面这组实现替换原来的 `initRgbLed()` / `turnOffRgbLed()`：

```cpp
constexpr uint8_t kRgbLedPin = 21;
constexpr uint8_t kRgbLedCount = 2;

M5PM1 gPm1;
Adafruit_NeoPixel gPixels(kRgbLedCount, kRgbLedPin, NEO_GRB + NEO_KHZ800);
bool gPm1Ready = false;

void clearRgbLedPixels() {
  gPixels.clear();
  gPixels.show();
}

void disableRgbLedPower() {
  if (gPm1Ready) {
    gPm1.setLdoEnable(false);
  }
}

void shutdownRgbLedForSleep() {
  clearRgbLedPixels();
  disableRgbLedPower();
}

void initRgbLed() {
  const m5pm1_err_t err = gPm1.begin(&M5.In_I2C, M5PM1_DEFAULT_ADDR, M5PM1_I2C_FREQ_100K);
  gPm1Ready = (err == M5PM1_OK);
  if (gPm1Ready) {
    gPm1.setLdoEnable(true);
  }
  gPixels.begin();
  clearRgbLedPixels();
}

void prepareEpdAfterWakeup() {
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.wakeup();
  M5.Display.clear(TFT_WHITE);
  M5.Display.waitDisplay();
  M5.Display.setEpdMode(epd_mode_t::epd_fast);
}
```

- [ ] **Step 3: 接入新的辅助函数**

把 `enterHomeDeepSleep()` 的函数开头前 3 行改成：

```cpp
void enterHomeDeepSleep(const HomeSleepRequest& request) {
  shutdownRgbLedForSleep();
  const auto wakeupGpio = static_cast<gpio_num_t>(request.wakeupGpio);
  pinMode(static_cast<std::uint8_t>(request.wakeupGpio), INPUT_PULLUP);
}
```

`pinMode(...)` 之后到 `M5.Power.deepSleep(...)` 的原有 RTC/GPIO/Display sleep 流程保持不变。

把 `appSetup()` 中 `M5.Display.setRotation(0);` 后的初始化逻辑改成：

```cpp
  M5.begin(cfg);
  M5.Display.setRotation(0);
  prepareEpdAfterWakeup();
  initRgbLed();
  gConfigStore.begin();
```

保留 `i2cBusRecovery()` 那段 `#ifndef UNIT_TEST` 逻辑不变。

- [ ] **Step 4: 运行聚焦测试确认转绿**

Run: `pio test -e native -f test_app_runtime`
Expected: 所有 `test_app_runtime` 用例通过，包括新增的 3 个测试

- [ ] **Step 5: 运行目标板编译**

Run: `pio run -e m5stack-papercolor`
Expected: `SUCCESS`，无编译错误

- [ ] **Step 6: Commit**

```bash
git add src/app_runtime.cpp test/native/test_app_runtime/test_main.cpp
git commit -m "fix: 修正 EPD 启动刷新与 RGB 睡前断电

- appSetup 新增 prepareEpdAfterWakeup 以执行一次 epd_quality 基线刷新
- RGB 关闭逻辑拆分为清灯和断电两步
- app_runtime native 测试覆盖 EPD 刷新与 RGB 初始化/断电行为"
```

---

### Task 4: 最终验证

**Files:**
- Test: `test/native/test_app_runtime/test_main.cpp`
- Test: `src/app_runtime.cpp`

- [ ] **Step 1: 跑完整 native 测试**

Run: `pio test -e native`
Expected: 所有 native 测试通过，不再出现 `0 test cases`

- [ ] **Step 2: 再跑一次目标板编译确认无回归**

Run: `pio run -e m5stack-papercolor`
Expected: `SUCCESS`

- [ ] **Step 3: 检查最终 diff 是否只覆盖计划范围**

Run: `git diff -- src/app_runtime.cpp test/native/support/fake_arduino/M5Unified.h test/native/support/fake_arduino/M5PM1.h test/native/support/fake_arduino/Adafruit_NeoPixel.h test/native/test_app_runtime/test_main.cpp`
Expected: 只包含 EPD 启动刷新、RGB 断电、fake 支撑和对应测试改动
