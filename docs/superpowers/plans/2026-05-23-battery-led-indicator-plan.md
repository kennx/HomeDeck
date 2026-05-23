# 电池充电指示灯 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 HomeDeck 上实现电池充电 RGB LED 指示灯（充电黄/绿灯常亮，低电量红灯闪烁）

**Architecture:** 新增 BatteryLedService 类，遵循项目 service 模式，直接使用 M5.Power 和 M5.Led API。在 main.cpp loop 中调用，从 BootController::begin() 中处理唤醒闪烁。

**Tech Stack:** Arduino (C++17), M5Unified, PlatformIO, Unity 测试框架

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/battery_led_service.h` | 新建 | BatteryLedService 类声明 |
| `src/battery_led_service.cpp` | 新建 | LED 控制逻辑实现 |
| `src/main.cpp` | 修改 | 集成 BatteryLedService |
| `src/boot_controller.cpp` | 修改 | 唤醒时低电量闪烁 |
| `test/native/support/fake_arduino/M5Unified.h` | 修改 | 增加 FakePower 和 FakeLed |
| `test/native/test_battery_led_service/test_main.cpp` | 新建 | 单元测试 |

---

### Task 1: 扩展模拟 M5Unified，增加 FakePower 和 FakeLed

**Files:**
- Modify: `test/native/support/fake_arduino/M5Unified.h`

- [ ] **Step 1: 在 M5Unified.h 末尾添加 FakePower 类和 FakeLed 类**

在 `FakeM5Global` 之前，添加以下两个结构体：

```cpp
#include <cstdint>
#include <string>
#include <vector>

namespace m5 {

struct Power_Class {
    enum charge_status_t {
        is_charging    = 0,
        is_discharging = 1,
        charge_unknown = 2,
    };
};

}  // namespace m5

struct FakePower {
    int batteryLevel = 100;
    m5::Power_Class::charge_status_t chargeStatus = m5::Power_Class::is_discharging;

    int getBatteryLevel() const {
        return batteryLevel;
    }

    m5::Power_Class::charge_status_t isCharging() const {
        return chargeStatus;
    }
};

struct FakeLedCall {
    uint8_t brightness = 0;
    uint8_t red   = 0;
    uint8_t green = 0;
    uint8_t blue  = 0;
};

struct FakeLed {
    uint8_t lastBrightness = 0;
    uint8_t lastRed   = 0;
    uint8_t lastGreen = 0;
    uint8_t lastBlue  = 0;
    int displayCallCount = 0;
    std::vector<FakeLedCall> calls;

    void setBrightness(uint8_t val) {
        lastBrightness = val;
    }

    void setAllColor(uint8_t r, uint8_t g, uint8_t b) {
        lastRed   = r;
        lastGreen = g;
        lastBlue  = b;
    }

    void display() {
        displayCallCount++;
        calls.push_back({lastBrightness, lastRed, lastGreen, lastBlue});
    }
};
```

然后修改 `FakeM5Global`，新增两个成员：

```cpp
    FakePower Power;
    FakeLed Led;
```

完整修改后的 `FakeM5Global`：

```cpp
struct FakeM5Global {
    FakeRtc Rtc;
    FakeI2C In_I2C;
    FakeDisplay Display;
    FakeButton BtnA;
    FakeButton BtnB;
    FakePower Power;
    FakeLed Led;

    void begin() {
    }

    void update() {
    }
};
```

- [ ] **Step 2: 新增全局 M5.h 重置辅助函数**

在 `extern FakeM5Global M5;` 之后添加：

```cpp
inline void fakeM5Reset() {
    M5 = FakeM5Global{};
}
```

- [ ] **Step 3: 验证编译**

```bash
pio test -e native -f test_battery_led_service
```
Expected: 编译错误（测试文件还不存在），但 M5Unified.h 本身没有语法错误。

---

### Task 2: 创建 BatteryLedService 头文件

**Files:**
- Create: `src/battery_led_service.h`

- [ ] **Step 1: 编写头文件**

```cpp
#pragma once

#include <cstdint>

class BatteryLedService {
public:
    void begin();
    void update();

    static constexpr unsigned long kCheckIntervalMs = 1000;
    static constexpr unsigned long kBlinkPeriodMs   = 3000;
    static constexpr unsigned long kBlinkOnMs       = 200;
    static constexpr int kLowBatteryThreshold       = 5;
    static constexpr int kFullBatteryThreshold      = 100;
    static constexpr uint8_t kChargingBrightness    = 64;

private:
    void setLed(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void turnOff();

    unsigned long lastCheckMs_ = 0;
    int lastLevel_             = -1;
    int lastChargeStatus_      = -1;
};
```

- [ ] **Step 2: 验证编译**

```bash
pio test -e native -f test_battery_led_service
```
Expected: 链接错误（缺少实现），但头文件语法正确。

---

### Task 3: 创建 BatteryLedService 实现

**Files:**
- Create: `src/battery_led_service.cpp`

- [ ] **Step 1: 编写实现文件**

```cpp
#include "battery_led_service.h"

#if !defined(UNIT_TEST)
#include <Arduino.h>
#include <M5Unified.h>
#else
#include "support/fake_arduino/M5Unified.h"

unsigned long fakeMillis = 0;
unsigned long millis() {
    return fakeMillis;
}

void advanceTime(unsigned long ms) {
    fakeMillis += ms;
}
#endif

void BatteryLedService::begin() {
    lastCheckMs_ = 0;
    lastLevel_   = -1;
    lastChargeStatus_ = -1;
    turnOff();
}

void BatteryLedService::update() {
#if !defined(UNIT_TEST)
    unsigned long now = millis();
#else
    unsigned long now = fakeMillis;
#endif

    if (now - lastCheckMs_ < kCheckIntervalMs) {
        return;
    }
    lastCheckMs_ = now;

    int level         = M5.Power.getBatteryLevel();
    int chargeStatus  = static_cast<int>(M5.Power.isCharging());

    if (chargeStatus == static_cast<int>(m5::Power_Class::is_charging)) {
        if (level >= kFullBatteryThreshold) {
            setLed(0, 255, 0, kChargingBrightness);
        } else {
            setLed(255, 255, 0, kChargingBrightness);
        }
    } else {
        if (level < kLowBatteryThreshold) {
            unsigned long phase = now % kBlinkPeriodMs;
            if (phase < kBlinkOnMs) {
                setLed(255, 0, 0, kChargingBrightness);
            } else {
                turnOff();
            }
        } else {
            turnOff();
        }
    }

    lastLevel_        = level;
    lastChargeStatus_ = chargeStatus;
}

void BatteryLedService::setLed(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    M5.Led.setBrightness(brightness);
    M5.Led.setAllColor(r, g, b);
    M5.Led.display();
}

void BatteryLedService::turnOff() {
    M5.Led.setBrightness(0);
    M5.Led.setAllColor(0, 0, 0);
    M5.Led.display();
}
```

- [ ] **Step 2: 验证编译**

```bash
pio test -e native -f test_battery_led_service
```
Expected: 链接错误（test_main.cpp 还不存在），但实现文件语法正确。

---

### Task 4: 编写单元测试

**Files:**
- Create: `test/native/test_battery_led_service/test_main.cpp`

- [ ] **Step 1: 编写测试文件**

```cpp
#include <unity.h>
#include <cstring>

#define private public
#include "../../../src/battery_led_service.h"
#include "../../../src/battery_led_service.cpp"
#undef private

#include "../support/fake_arduino/M5Unified.h"

namespace {
    BatteryLedService service;
}  // namespace

void setUp() {
    fakeM5Reset();
    service = BatteryLedService{};
    service.begin();
}

void tearDown() {
}

// 辅助函数：快进时间并调用 update
void advanceAndUpdate(unsigned long ms) {
#if !defined(UNIT_TEST)
    advanceTime(ms);
#else
    extern unsigned long fakeMillis;
    fakeMillis += ms;
#endif
    service.update();
}

void test_charging_not_full_shows_yellow() {
    M5.Power.chargeStatus = m5::Power_Class::is_charging;
    M5.Power.batteryLevel = 80;

    service.update();

    TEST_ASSERT_EQUAL_UINT8(255, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(255, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
    TEST_ASSERT_EQUAL_UINT8(BatteryLedService::kChargingBrightness, M5.Led.lastBrightness);
    TEST_ASSERT_GREATER_THAN(0, M5.Led.displayCallCount);
}

void test_charging_full_shows_green() {
    M5.Power.chargeStatus = m5::Power_Class::is_charging;
    M5.Power.batteryLevel = 100;

    service.update();

    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(255, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
}

void test_discharging_low_battery_blinks_red() {
    M5.Power.chargeStatus = m5::Power_Class::is_discharging;
    M5.Power.batteryLevel = 3;

    // 在闪烁 ON 阶段 (0-200ms)
    extern unsigned long fakeMillis;
    fakeMillis = 100;
    service.update();

    TEST_ASSERT_EQUAL_UINT8(255, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
}

void test_discharging_low_battery_off_phase() {
    M5.Power.chargeStatus = m5::Power_Class::is_discharging;
    M5.Power.batteryLevel = 3;

    // 在闪烁 OFF 阶段 (200ms-3000ms)
    extern unsigned long fakeMillis;
    fakeMillis = 300;
    service.update();

    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBrightness);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
}

void test_discharging_normal_battery_turns_off() {
    M5.Power.chargeStatus = m5::Power_Class::is_discharging;
    M5.Power.batteryLevel = 50;

    service.update();

    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBrightness);
}

void test_unknown_charge_status_treats_as_discharging() {
    M5.Power.chargeStatus = m5::Power_Class::charge_unknown;
    M5.Power.batteryLevel = 3;

    // 未知充电状态 + 低电量 -> 红灯闪烁
    extern unsigned long fakeMillis;
    fakeMillis = 50;
    service.update();

    TEST_ASSERT_EQUAL_UINT8(255, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
}

void test_update_throttled_to_1_second() {
    M5.Power.chargeStatus = m5::Power_Class::is_charging;
    M5.Power.batteryLevel = 80;

    extern unsigned long fakeMillis;
    fakeMillis = 0;
    service.update();
    int firstCallCount = M5.Led.displayCallCount;

    // 同秒内再调用，不执行
    fakeMillis = 500;
    M5.Power.chargeStatus = m5::Power_Class::is_charging;
    M5.Power.batteryLevel = 100;
    service.update();

    TEST_ASSERT_EQUAL_INT(firstCallCount, M5.Led.displayCallCount);
}

void test_begin_turns_off_led() {
    M5.Power.chargeStatus = m5::Power_Class::is_charging;
    M5.Power.batteryLevel = 80;

    // 先调用 update 点亮 LED
    service.update();
    TEST_ASSERT_GREATER_THAN_INT(0, M5.Led.displayCallCount);

    // 重新 begin 应该关灯
    service.begin();
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBrightness);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastRed);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastGreen);
    TEST_ASSERT_EQUAL_UINT8(0, M5.Led.lastBlue);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_charging_not_full_shows_yellow);
    RUN_TEST(test_charging_full_shows_green);
    RUN_TEST(test_discharging_low_battery_blinks_red);
    RUN_TEST(test_discharging_low_battery_off_phase);
    RUN_TEST(test_discharging_normal_battery_turns_off);
    RUN_TEST(test_unknown_charge_status_treats_as_discharging);
    RUN_TEST(test_update_throttled_to_1_second);
    RUN_TEST(test_begin_turns_off_led);
    return UNITY_END();
}
```

- [ ] **Step 2: 运行测试验证全部通过**

```bash
pio test -e native -f test_battery_led_service
```
Expected: ALL TESTS PASS (8 个)

---

### Task 5: 集成到 main.cpp

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: 修改 main.cpp**

```cpp
#include <Arduino.h>

#include "battery_led_service.h"
#include "boot_controller.h"

namespace {

BootController gBootController;
BatteryLedService gBatteryLed;

}  // namespace

void setup() {
  gBatteryLed.begin();
  gBootController.begin();
}

void loop() {
  gBatteryLed.update();
  gBootController.update();
  gBootController.enterDeepSleep();
}
```

- [ ] **Step 2: 验证完整编译**

```bash
pio run -e m5stack-papercolor
```
Expected: 编译成功。

---

### Task 6: 唤醒时低电量闪烁

**Files:**
- Modify: `src/boot_controller.cpp`

- [ ] **Step 1: 在 begin() 的快速唤醒路径中，M5 初始化后检查电量**

修改 `BootController::begin()`，在 `m5Begin` 和 `m5Update` 调用之后，判断唤醒场景后，添加低电量闪烁逻辑。

在 `if (isTimerWakeup)` 块之前（第 270 行附近），增加：

```cpp
  // 从深度睡眠唤醒时检查低电量，闪红灯提醒
  if (isTimerWakeup || wakeupCause == ESP_SLEEP_WAKEUP_TIMER) {
    // 此处走快速唤醒路径，在 enterHomeModeFast 之前闪烁
  }
```

实际上，只需修改 `begin()` 方法中 `m5Update` 调用后、进入快速/正常路径之前。在第 255-257 行之后插入：

```cpp
  // 仅定时器唤醒时检查（冷启动不检查，电池通常不低）
  if (isTimerWakeup) {
#if !defined(UNIT_TEST)
    if (M5.Power.isCharging() != m5::Power_Class::is_charging) {
      int level = M5.Power.getBatteryLevel();
      if (level < 5) {
        M5.Led.setBrightness(64);
        M5.Led.setAllColor(255, 0, 0);
        M5.Led.display();
        delay(200);
        M5.Led.setBrightness(0);
        M5.Led.setAllColor(0, 0, 0);
        M5.Led.display();
      }
    }
#endif
  }
```

位置：在 `if (isTimerWakeup) { enterHomeModeFast(config_); return; }` 之前插入。

完整 diff：

在 `const bool isTimerWakeup = (wakeupCause == ESP_SLEEP_WAKEUP_TIMER);` 之后、`if (isTimerWakeup) {` 之前插入上述代码。

- [ ] **Step 2: 验证编译**

```bash
pio run -e m5stack-papercolor
```
Expected: 编译成功。

---

### Task 7: 运行全部测试

- [ ] **Step 1: 运行所有测试确认无回归**

```bash
pio test -e native
```
Expected: 所有已有测试通过 + 新增 8 个测试通过。

---

### Task 8: 代码格式化

- [ ] **Step 1: 格式化新增文件**

```bash
# 如果项目有 .clang-format 配置
clang-format -i src/battery_led_service.h src/battery_led_service.cpp
```
Expected: 格式化后无 diff。

---

## 总结

| Task | 内容 | 验证方法 |
|------|------|---------|
| 1 | 扩展 fake M5Unified.h | 编译通过 |
| 2 | 创建 .h 头文件 | 编译通过 |
| 3 | 创建 .cpp 实现 | 编译通过 |
| 4 | 编写 8 个单元测试 | 全部通过 |
| 5 | 集成到 main.cpp | 硬件编译通过 |
| 6 | 唤醒闪烁 | 硬件编译通过 |
| 7 | 全量测试 | 无回归 |
| 8 | 格式化 | 无 diff |
