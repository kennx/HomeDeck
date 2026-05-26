# 日历翻页功能设计文档

日期：2026-05-26

---

## 1. 需求概述

在日历视图下，用户可以通过物理按钮翻页浏览不同月份的日历：

- **G9 (BtnB)**：按一下上翻一个月（例如 2026 年 5 月 → 2026 年 4 月）
- **G10 (BtnA)**：按一下下翻一个月（例如 2026 年 5 月 → 2026 年 6 月）
- **双击 G1 (BtnC)**：回到本月

### 硬件按钮映射（M5PaperColor）

| GPIO | M5Unified 按钮 | 功能 |
|------|---------------|------|
| G1 | BtnC | 单击切换视图 / 双击回到本月 |
| G9 | BtnB | 日历上翻月 |
| G10 | BtnA | 日历下翻月 |

### 交互矩阵

| 当前视图 | 单击 G1 (BtnC) | 双击 G1 (BtnC) | G9 (BtnB) | G10 (BtnA) |
|---------|---------------|---------------|-----------|-----------|
| **黄历** | → 日历 | 忽略 | 忽略 | 忽略 |
| **日历** | → 黄历 | 回到本月 | 上翻月 | 下翻月 |

---

## 2. 设计决策

### 2.1 当天高亮规则

- **只高亮"真实今天"所在的月份中的当天日期**
- 翻到别的月份时，日期网格中**不显示任何高亮**
- 实现方式：`CalendarData.day` 在偏移为 0（本月）时设为真实今天日期，否则设为 0。`renderCalendar()` 中 `if (dayNumber == data.day)` 在 `data.day == 0` 时自然不匹配，因此不显示高亮。

### 2.2 Deep Sleep 唤醒后状态

- 每次从 deep sleep 唤醒后，日历视图**回到本月**
- 实现方式：`BootController::enterSystemMode()` 中重置 `calendarMonthOffset_ = 0`

### 2.3 翻页操作与 Deep Sleep 计时器

- 翻页操作（按 G9/G10）**重置 deep sleep 的 5 分钟无操作计时器**
- 实现方式：每次翻页后更新 `lastActivityMs_ = now`

### 2.4 按钮检测集中化

由于 M5Unified 的 `wasClicked()` 和 `wasDecideClickCount()` 是消耗性 API，不能同时消费同一个按钮事件，因此将 **BtnC 的检测从 `ViewManager` 移到 `BootController`** 集中管理：

- `ViewManager` 不再负责按钮检测，改为被动式（提供 `switchToNextView()` 方法由 BootController 调用）
- `BootController::update()` 中统一检测所有按钮并分发行为

---

## 3. 架构设计

### 3.1 组件变更

#### ViewManager

- **移除** `wasCalendarButtonClicked` 依赖
- **移除** `update()` 方法（按钮检测不再由 ViewManager 负责）
- **新增** `switchToNextView()` 方法：Almanac ↔ Calendar 循环切换
- `viewSwitched()` 仍保留，供 BootController 判断是否需要重置计时器

#### BootController

- **新增状态**：`int calendarMonthOffset_ = 0`（从本月的月份偏移量）
- **新增依赖**：
  - `std::function<int()> getCalendarButtonClickCount`（0=无, 1=单击, 2=双击）
  - `std::function<bool()> wasPrevMonthClicked`（BtnB.wasClicked()）
  - `std::function<bool()> wasNextMonthClicked`（BtnA.wasClicked()）
  - `std::function<void(int monthOffset)> renderCalendarWithOffset`
- **更新** `update()`：集中检测所有按钮
- **更新** `enterSystemMode()`：重置 `calendarMonthOffset_ = 0`

#### app_runtime.cpp

- **新增** `renderCalendarWithOffset(int monthOffset)`：计算目标年月并构造 `CalendarData`
- **更新** `makeBootDeps()`：提供新的按钮检测回调

#### FakeM5（测试桩）

- FakeButton 可能需要新增 `wasDecideClickCount()` 和 `getClickCount()` 支持

### 3.2 核心数据流

以"用户在日历视图双击 G1 回到本月"为例：

```
1. 用户快速按两次 G1 (BtnC)，间隔 < 300ms
   ↓
2. M5.update() 刷新按钮状态
   ↓
3. BootController::update()
   ├── getCalendarButtonClickCount()
   │   └── M5.BtnC.wasDecideClickCount() → true
   │   └── M5.BtnC.getClickCount() → 2
   ├── btnCClicks == 2 ✓
   ├── currentView() == Calendar ✓
   ├── calendarMonthOffset_ = 0
   ├── renderCalendarWithOffset(0)
   │   └── 计算目标年月 = 本月
   │   └── data.day = local->tm_mday（恢复高亮）
   │   └── gHomeRenderer.renderCalendar(data)
   └── lastActivityMs_ = now
```

以"用户按 G9 从 5 月翻到 4 月"为例：

```
BootController::update()
├── getCalendarButtonClickCount() → 0（无 BtnC 操作）
├── currentView() == Calendar ✓
├── wasPrevMonthClicked() → true（BtnB.wasClicked()）
├── calendarMonthOffset_ 从 0 变为 -1
├── renderCalendarWithOffset(-1)
│   └── targetMonth = 5 + (-1) = 4
│   └── data.day = 0（不显示高亮）
│   └── gHomeRenderer.renderCalendar(data)
└── lastActivityMs_ = now
```

### 3.3 月份计算逻辑

```cpp
void renderCalendarWithOffset(int monthOffset) {
  std::time_t now = time(nullptr);
  std::tm* local = now > 0 ? std::localtime(&now) : nullptr;
  if (local == nullptr) {
    gHomeRenderer.renderCalendar(makeCurrentCalendarData());
    return;
  }

  int targetYear = local->tm_year + 1900;
  int targetMonth = local->tm_mon + 1 + monthOffset;

  // 处理跨年
  while (targetMonth > 12) { targetMonth -= 12; targetYear++; }
  while (targetMonth < 1)  { targetMonth += 12; targetYear--; }

  CalendarData data;
  data.year = targetYear;
  data.month = targetMonth;
  data.day = (monthOffset == 0) ? local->tm_mday : 0;

  // 读取温湿度
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }

  gHomeRenderer.renderCalendar(data);
}
```

---

## 4. 边界情况与错误处理

| 场景 | 处理方式 |
|------|---------|
| **跨年翻月**（1月上翻→12月） | `renderCalendarWithOffset` 中用 `while` 循环处理月份越界 |
| **时间获取失败** | fallback 到 `makeCurrentCalendarData()`（渲染本月） |
| **在 Almanac 视图按 G9/G10** | `currentView() != Calendar`，忽略 |
| **在 Almanac 视图双击 G1** | `currentView() != Calendar`，忽略 |
| **连续快速翻月**（连按5次G9） | `calendarMonthOffset_` 累加为 -5，渲染 5 个月前 |
| **deep sleep 唤醒** | `enterSystemMode()` 中 `calendarMonthOffset_ = 0`，回到本月 |
| **单击和双击的延迟** | M5Unified 内部处理，通常 200~300ms 判定窗口，对电子墨水屏可接受 |

---

## 5. 测试策略

### test_view_manager（更新）

- `test_begins_with_almanac`：`begin()` 后为 Almanac
- `test_switch_to_next_view`：`switchToNextView()` 切换到 Calendar
- `test_switch_to_next_view_cycles`：两次 `switchToNextView()` 回到 Almanac
- `test_view_switched_flag`：switch 后 `viewSwitched()` 为 true

### test_boot_controller（新增）

- `test_single_click_switches_view`：单击 BtnC，切换 Almanac→Calendar
- `test_double_click_resets_to_today_in_calendar`：Calendar 视图双击 BtnC，`renderCalendarWithOffset(0)`
- `test_double_click_ignored_in_almanac`：Almanac 视图双击 BtnC，无操作
- `test_prev_month_click_in_calendar`：Calendar 视图中按 BtnB，`renderCalendarWithOffset(-1)`
- `test_next_month_click_in_calendar`：Calendar 视图中按 BtnA，`renderCalendarWithOffset(+1)`
- `test_month_click_ignored_in_almanac`：Almanac 视图中按 BtnB/BtnA，无操作
- `test_month_click_resets_sleep_timer`：翻页后 deep sleep 计时器重置
- `test_enter_system_mode_resets_month_offset`：`begin()` 后偏移为 0
- `test_continuous_prev_month_clicks`：连续按 BtnB 两次，偏移为 -2

### test_home_renderer（新增）

- `test_calendar_no_highlight_when_day_is_zero`：`CalendarData.day = 0` 时无高亮
- `test_calendar_renders_past_month`：验证偏移后的月份正确渲染（标题年份、月份正确）

---

## 6. 风险与注意事项

1. **M5Unified 双击检测兼容性**：`wasDecideClickCount()` 和 `getClickCount()` 的具体行为需要在 FakeM5 中准确模拟。如果 FakeM5 的实现与真实 M5Unified 不一致，可能导致硬件上行为异常。
2. **双击延迟**：用户单击后需要等待 ~300ms 才能确定不是双击。这对电子墨水屏的响应速度来说通常可以接受，但需要实际测试确认。
3. **ViewManager 测试变更**：移除 `update()` 方法后，现有 ViewManager 测试需要重写，但这是必要改动。
