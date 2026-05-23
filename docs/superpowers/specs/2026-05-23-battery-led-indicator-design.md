# 电池充电指示灯 设计文档

> 日期：2026-05-23
> 硬件：M5Stack PaperColor (C151)，G21 控制 2× RGB LED (WS2812C-2020)

---

## 需求概述

通过 G21 RGB LED 显示电池和充电状态，提供直观的视觉反馈。

## 状态转换逻辑

| 充电中 | 电量 | LED 行为 |
|--------|------|---------|
| 是 | < 100% | 黄灯常亮 (255,255,0) |
| 是 | 100% | 绿灯常亮 (0,255,0) |
| 否 | < 5% | 红灯闪烁 (255,0,0)，200ms 亮 / 2800ms 灭 |
| 否 | >= 5% | 熄灭 |
| 未知 | — | 熄灭 |

充电状态通过 `M5.Power.isCharging()` 获取，返回值为：
- `m5::Power_Class::is_charging` — 充电中
- `m5::Power_Class::is_discharging` — 放电中
- `m5::Power_Class::charge_unknown` — 未知，按放电处理

深度睡眠唤醒时：如果电量 < 5%，在 `BootController::begin()` 中闪一下红灯（亮 200ms → 灭），然后继续正常流程。

## 架构

### 文件

| 文件 | 说明 |
|------|------|
| `src/battery_led_service.h` | 类声明 |
| `src/battery_led_service.cpp` | 实现，约 80 行 |
| `src/main.cpp` | 增加实例化和调用 |
| `src/boot_controller.cpp` | 唤醒时检查低电量并闪烁 |

### BatteryLedService 接口

```cpp
class BatteryLedService {
public:
  void begin();   // 初始化，关闭 LED
  void update();  // 每帧调用，内部限频 1 秒检查一次
private:
  void setLed(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
  void turnOff();

  unsigned long lastCheckMs_ = 0;
  int lastLevel_ = -1;
  m5::Power_Class::charge_status_t lastChargeStatus_;
};
```

### 集成方式

`main.cpp` 中：

```cpp
namespace {
BootController gBootController;
BatteryLedService gBatteryLed;
}

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

`boot_controller.cpp` 中，在 `begin()` 初始化完成后、正常流程之前，检查电量并闪烁。

## API 依赖

| API | 用途 | 来源 |
|-----|------|------|
| `M5.Power.getBatteryLevel()` | 获取电量 0-100% | M5Unified |
| `M5.Power.isCharging()` | 检测充电状态 | M5Unified |
| `M5.Led.setBrightness(0-255)` | 设置 LED 亮度 | M5Unified |
| `M5.Led.setAllColor(r,g,b)` | 设置 RGB 颜色 | M5Unified |
| `M5.Led.display()` | 刷新 LED 显示 | M5Unified |

所有 API 已在 M5Unified 库中可用，无需额外依赖。

## 实现细节

### 限频策略

`update()` 每帧调用，但内部通过 `millis()` 限频：每 **1 秒** 才重新读取电量并更新 LED。闪烁效果通过计算当前 millis 来决定亮/灭状态。

### 闪烁实现

红灯闪烁时，不使用 `delay()` 阻塞主循环。在 `update()` 中根据 `(millis() / 100) % 30` 判断当前在 3 秒周期内的位置：
- 0-200ms：亮
- 200ms-3000ms：灭

### 唤醒闪烁

在 `BootController::begin()` 中，初始化完成后：
1. 检查 `M5.Power.isCharging()` → 如果正在充电，跳过
2. 检查 `M5.Power.getBatteryLevel()` → 如果 >= 5%，跳过
3. 设置红灯亮 200ms → 关灯 → 继续

此闪烁通过 `M5.delay()` 阻塞实现（仅唤醒时发生一次，不影响主循环）。

## 测试验证

### 单元测试

在 `test/native/` 下新增 `test_battery_led_service/`：
- Fake M5.Power 和 M5.Led 的 mock 实现
- 测试 4 种状态转换的 LED 颜色和亮度输出

### 硬件验证

1. USB 供电（充电中），电量不满 → 观察黄灯常亮
2. USB 供电，充满 → 观察绿灯常亮
3. 拔掉 USB，电量 >= 5% → LED 熄灭
4. 拔掉 USB，电量 < 5% → 红灯每 3 秒闪一次
5. 唤醒场景：深度睡眠醒来，电量 < 5% → 闪一下红灯
