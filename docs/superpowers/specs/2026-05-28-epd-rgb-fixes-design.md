# EPD 启动刷新与 RGB 断电修复设计

## 背景

对 `feature/calendar-ui` 上尚未推送的 4 个 commit 做 code review 后，确认有两个需要修复的真实问题：

1. `appSetup()` 中虽然先设置了 `epd_quality`，但没有实际触发一次完整高质量刷新，导致提交说明中的“修复 E-Ink 冷启动显示异常”在实现上并未真正成立。
2. RGB LED 在进入 deep sleep 前只做了像素清零，没有关闭 `M5PM1` 的 LDO 供电，导致“避免睡眠耗电”的修复不完整。

本次目标是以最小范围修正这两个问题，并补上对应测试，避免后续再次出现“提交说明与真实行为不一致”。

## 目标

- 冷启动或从 deep sleep 唤醒后，真正执行一次 `epd_quality` 基线刷新
- 进入 deep sleep 前，RGB LED 既灭灯也断供电
- 拆出职责明确的小函数，避免显示初始化时序和 LED 电源控制继续混杂在业务流程中
- 补充最小必要的原生测试，覆盖这次修复点

## 范围

仅修改以下范围：

- `src/app_runtime.cpp`
- 与本次修复直接相关的 fake / native test 支撑代码
- 对应的 `test_app_runtime` 测试

本次不修改：

- 日历/黄历数据逻辑
- `HomeRenderer` 页面布局
- `BootController` 状态机
- 其它与 deep sleep 无关的渲染路径

## 架构设计

采用“小函数拆分 + 原路径接入”的方式，不新增类，不引入新的抽象层。

### 1. EPD 启动时序边界

从 `appSetup()` 中抽出一个专门负责唤醒后基线刷新的辅助函数，命名为 `prepareEpdAfterWakeup()`。

该函数只负责：

- 设置 `epd_quality`
- 唤醒显示
- 执行一次白底完整刷新
- 等待刷新完成
- 切回 `epd_fast`

`appSetup()` 仍然负责整体初始化顺序，但不再直接堆叠多条 EPD 控制语句。

### 2. RGB LED 电源控制边界

保留现有 `initRgbLed()` 作为启动入口，但把“清灯”和“断供电”拆开，避免 `turnOffRgbLed()` 这种名字掩盖真实行为。

建议拆分为三个职责明确的函数：

- `clearRgbLedPixels()`：只负责 `gPixels.clear()` + `gPixels.show()`
- `disableRgbLedPower()`：只负责在 `gPm1Ready` 时调用 `gPm1.setLdoEnable(false)`
- `shutdownRgbLedForSleep()`：组合调用前两者，作为 deep sleep 前的业务入口

这样可以保证“灭灯”和“断电”语义明确，后续如果要单独控制其中一步，也不会再误用。

## 详细设计

### EPD 启动时序

`appSetup()` 中与 EPD 相关的流程调整为：

1. `M5.begin(cfg)`
2. `M5.Display.setRotation(0)`
3. 调用 `prepareEpdAfterWakeup()`
4. 继续 `gConfigStore.begin()`、`BootController` 初始化等现有流程

`prepareEpdAfterWakeup()` 的内部顺序固定为：

1. `M5.Display.setEpdMode(epd_mode_t::epd_quality)`
2. `M5.Display.wakeup()`
3. `M5.Display.clear(TFT_WHITE)`
4. `M5.Display.waitDisplay()`
5. `M5.Display.setEpdMode(epd_mode_t::epd_fast)`

这样可以确保：

- `epd_quality` 不只是“设置过”，而是真的参与了一次实际刷新
- 后续业务渲染继续走 `epd_fast`，不影响平时刷新速度
- 修复当前首帧前没有高质量基线刷新的问题

### RGB deep sleep 时序

启动时保留当前语义：

1. 初始化 `M5PM1`
2. 初始化成功则开启 RGB 对应 LDO
3. 初始化 NeoPixel 对象
4. 默认清灯

deep sleep 前流程调整为：

1. 调用 `shutdownRgbLedForSleep()`
2. 配置 RTC/GPIO/timer 唤醒源
3. 执行 `M5.Display.sleep()` 和后续 deep sleep 逻辑

`shutdownRgbLedForSleep()` 固定顺序为：

1. `clearRgbLedPixels()`
2. `disableRgbLedPower()`

这样可以保证：

- 从用户可见效果上，灯会先灭掉
- 从功耗上，RGB 供电轨被真正关闭
- 与项目文档中 `LDO3V3_EN_PP` 作为 RGB 供电开关的硬件定义保持一致

## 测试策略

只补与本次修复直接相关的最小测试能力。

### fake / 支撑层需要记录的行为

#### Display 侧

- `setEpdMode()` 调用顺序
- `wakeup()` 调用次数
- `clear()` 调用次数和颜色
- `waitDisplay()` 调用次数

#### PM1 侧

- `setLdoEnable(true/false)` 调用记录

#### NeoPixel 侧

如果现有 fake 已能观察 `clear()` / `show()`，则直接复用；否则只补最小计数能力，不扩展无关接口。

### 需要新增或更新的测试

#### `test/native/test_app_runtime/test_main.cpp`

1. `appSetup` 的 EPD 初始化时序测试
   - 验证先切到 `epd_quality`
   - 验证随后调用 `wakeup()`
   - 验证执行一次 `clear(TFT_WHITE)`
   - 验证执行一次 `waitDisplay()`
   - 验证最后切回 `epd_fast`

2. `enterHomeDeepSleep` 的 RGB 关闭测试
   - 验证 deep sleep 前会清像素并 `show()`
   - 验证 `gPm1Ready == true` 时调用 `setLdoEnable(false)`

3. `initRgbLed` 初始化测试
   - 验证 `M5PM1` 初始化成功时调用 `setLdoEnable(true)`
   - 验证初始化完成后默认灯为熄灭状态

## 成功标准

1. `EPD` 高质量基线刷新被封装为独立小函数
2. `appSetup()` 中首帧业务渲染前，已经执行过一次 `epd_quality` 白底刷新并等待完成
3. deep sleep 前 RGB LED 既清像素又关闭 LDO 供电
4. 新增/更新的 native 测试真实执行并覆盖上述行为
5. 修复范围保持在 `app_runtime` 与直接相关测试支撑层，不扩散到无关模块

## 变更文件清单

| 文件 | 改动类型 | 说明 |
|------|---------|------|
| `src/app_runtime.cpp` | 修改 | 拆分 EPD 启动刷新函数与 RGB 关闭函数，并接入现有流程 |
| `test/native/support/fake_arduino/M5Unified.h` | 修改 | 增加 Display 调用记录能力 |
| `test/native/support/fake_arduino/M5PM1.h` | 修改 | 增加 `setLdoEnable` 调用记录能力 |
| `test/native/support/fake_arduino/Adafruit_NeoPixel.h` | 视现有能力决定 | 若当前 fake 不能观察 `clear()` / `show()`，则补最小调用计数 |
| `test/native/test_app_runtime/test_main.cpp` | 修改 | 增加/更新 EPD 初始化与 RGB 断电测试 |
