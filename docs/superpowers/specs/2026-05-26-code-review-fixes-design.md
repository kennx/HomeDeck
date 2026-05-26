# 代码审查修复设计

**日期**: 2026-05-26
**分支**: feature/calendar-ui
**范围**: 日历翻页功能 code review 中发现的问题修复

## 修复清单

### 1. 移除 ViewManager 死代码

**问题**: `resetViewSwitched()` 在生产代码中从未被调用。`viewSwitched_` 标志在 `begin()` 后永久为 true，`viewSwitched()` 在 BootController 中的检查是冗余的（因为 `switchToNextView()` 总是成功执行切换）。

**修复**:
- 从 ViewManager 中移除 `viewSwitched_` 成员、`viewSwitched()` 和 `resetViewSwitched()` 方法
- 从 `view_manager.h` 和 `view_manager.cpp` 删除对应声明和实现
- 在 `boot_controller.cpp` 中，`switchToNextView()` 后直接设置 `lastActivityMs_`（去掉 `viewSwitched()` 守卫）
- 更新 `test_view_manager/test_main.cpp`：移除对 `viewSwitched()` 的断言，改为通过 `currentView()` 和 `renderedViews` 验证视图切换

**影响**: ViewManager 公共 API 缩小 2 个方法，BootController 逻辑简化 1 行。测试更新后仍验证相同的行为。

### 2. 提取 SHT40 环境数据填充函数

**问题**: 以下模式在 3 处重复（2 处操作 CalendarData，1 处操作 HomeCalendarData）：
```cpp
const EnvironmentReading reading = readSht40Environment();
if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
}
```

**修复**:
- 在 `home_renderer.h` 中声明 `void applySht40ToCalendar(CalendarData& data);`
- 在 `home_renderer.cpp` 中实现
- `makeCurrentCalendarData()` 和 `renderCalendarWithOffset()` 改为调用此函数
- `makeCurrentHomeCalendarDataWithEnvironment()` 中保留原样（操作不同结构体 `HomeCalendarData`）

**影响**: 减少 ~10 行重复代码。2 处调用点改为单行函数调用。

### 3. calendarMonthOffset_ 添加边界

**问题**: 用户可以无限翻页，offset 无上限。虽然月份包裹逻辑正确，但无界增长不整洁。

**修复**:
- 定义 `kCalendarMonthOffsetMin = -120` 和 `kCalendarMonthOffsetMax = +120`（±10 年）
- 在 `boot_controller.cpp` 的 `update()` 中，翻页（递增/递减）前检查边界
- 在 `enterSystemMode()` 中重置为 0（已有此逻辑，不变）

**影响**: 翻页操作最多前后各 10 年，对桌面设备绰绰有余。

### 4. 三击及以上等同于双击

**问题**: `btnCClicks == 2` 只匹配双击，三击及以上无任何反馈。

**修复**:
- 将 `boot_controller.cpp:72` 的 `btnCClicks == 2` 改为 `btnCClicks >= 2`

**影响**: 三击及以上在 Calendar 视图执行回本月操作，在 Almanac 视图忽略（双击已有此逻辑）。

### 5. 日历测试精确化

**问题**: `test_calendar_no_highlight_when_day_is_zero` 通过检查全屏无 TFT_WHITE 文本来验证不高亮，可能因其他 UI 元素颜色变更而误报。

**修复**:
- 改为只检查日期网格区域内的 TFT_WHITE 输出
- 使用 `kCalDateStartY` (~108) 到 `kCalDateStartY + 6 * (kCalDateRowHeight + kCalDateRowGap)` (~450) 作为 Y 范围
- 利用 FakeDisplay 中每个 draw 操作的 X/Y 坐标进行过滤

**影响**: 测试更精确，不会因标题行或星期行颜色变更而误报。

## 不变内容

- BootController 的按键逻辑和视图切换行为不变
- 日历翻页和渲染行为不变
- 所有现有测试的语义保持不变（可能更新断言方式）
- 公共 API 的变更仅影响 ViewManager 的 2 个死方法
