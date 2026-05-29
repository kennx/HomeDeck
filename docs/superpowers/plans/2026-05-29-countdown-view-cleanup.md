# CountdownView 代码审查修复计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修复代码审查报告中指出的 Important 和 Minor 级别问题，不引入行为变更，所有现有测试继续通过。

**Architecture:** 将 `formatCurrentTimeHHMM()` 从 4 个文件提取到 `render_context.h/cpp` 作为公共工具；统一 `localtime()` 为 `localtime_r()` 消除线程安全隐患；清理未使用的 include、风格不一致和硬编码问题。

**Tech Stack:** PlatformIO (native test), C++17, M5Unified, Unity test framework

---

## 文件结构

### 修改文件

| 文件 | 变更 |
|------|------|
| `src/render_context.h` | 添加 `formatCurrentTimeHHMM()` 声明 |
| `src/render_context.cpp` | 添加 `formatCurrentTimeHHMM()` 实现 |
| `src/countdown_view.cpp` | 删除未使用的 `sht40_reader.h` include；使用 `formatCurrentTimeHHMM()`；使用 `localtime_r`；统一 `std::tm`/`std::localtime` 风格；缩小 `desc` 缓冲区；fallback 加注释 |
| `src/calendar_view.cpp` | 删除 `formatCurrentTimeHHMM()` 实现；使用公共版本；统一 `std::tm`/`std::localtime` 风格 |
| `src/almanac_view.cpp` | 删除 `formatCurrentTimeHHMM()` 实现；使用公共版本；统一 `std::tm`/`std::localtime` 风格 |
| `src/app_runtime.cpp` | 删除 `formatCurrentTimeHHMM()` 实现；使用公共版本；统一 `std::tm`/`std::localtime` 风格 |
| `src/home_renderer.cpp` | 使用全局实例 `gAlmanacView`/`gCalendarView` 替代临时对象 |
| `src/boot_controller.cpp` | 双击重置逻辑添加 Countdown 分支的显式注释 |

---

## Task 1: 提取 formatCurrentTimeHHMM 到 render_context

**Files:**
- Modify: `src/render_context.h`
- Modify: `src/render_context.cpp`
- Modify: `src/countdown_view.cpp`
- Modify: `src/calendar_view.cpp`
- Modify: `src/almanac_view.cpp`
- Modify: `src/app_runtime.cpp`

- [ ] **Step 1: 在 render_context.h 添加声明**

```cpp
// 在 namespace homedeck { 内添加
std::string formatCurrentTimeHHMM();
```

- [ ] **Step 2: 在 render_context.cpp 添加实现**

替换现有文件中的空实现（或追加到文件末尾）：

```cpp
std::string formatCurrentTimeHHMM() {
  std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = localtime_r(&now, &buf);
  char timeStr[6] = {};
  if (local != nullptr) {
    std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d", local->tm_hour, local->tm_min);
  }
  return std::string(timeStr);
}
```

- [ ] **Step 3: 从 countdown_view.cpp 删除本地实现并改用公共版本**

删除 `countdown_view.cpp` 匿名命名空间中的 `formatCurrentTimeHHMM()` 函数（约第 29-37 行）。

在 `countdown_view.cpp` 顶部添加 `#include "render_context.h"`（如果已有则确认包含）。

确保 `CountdownView::render()` 中调用的 `formatCurrentTimeHHMM()` 能解析到 `render_context.h` 中的声明。

- [ ] **Step 4: 从 calendar_view.cpp 删除本地实现并改用公共版本**

删除 `calendar_view.cpp` 匿名命名空间中的 `formatCurrentTimeHHMM()` 函数（约第 89-97 行）。

确认 `calendar_view.cpp` 已包含 `#include "render_context.h"`。

- [ ] **Step 5: 从 almanac_view.cpp 删除本地实现并改用公共版本**

删除 `almanac_view.cpp` 匿名命名空间中的 `formatCurrentTimeHHMM()` 函数（约第 480-488 行）。

确认 `almanac_view.cpp` 已包含 `#include "render_context.h"`。

- [ ] **Step 6: 从 app_runtime.cpp 删除本地实现并改用公共版本**

删除 `app_runtime.cpp` 中的 `formatCurrentTimeHHMM()` 函数（约第 242-250 行）。

确认 `app_runtime.cpp` 已包含 `#include "render_context.h"`。

- [ ] **Step 7: 编译并运行全部测试**

```bash
pio test -e native
```

预期：141 个测试全部通过。

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "refactor: 提取 formatCurrentTimeHHMM 到 render_context，消除 4 处重复

将 formatCurrentTimeHHMM() 从 app_runtime.cpp、almanac_view.cpp、
calendar_view.cpp、countdown_view.cpp 提取到 render_context.h/cpp。
同时统一使用 localtime_r 替代 localtime，消除线程安全隐患。"
```

---

## Task 2: 清理 countdown_view.cpp 杂项问题

**Files:**
- Modify: `src/countdown_view.cpp`

- [ ] **Step 1: 删除未使用的 sht40_reader.h include**

删除第 8 行的 `#include "sht40_reader.h"`。

- [ ] **Step 2: 统一 std::tm/std::localtime 风格并改用 localtime_r**

修改 `makeCurrentCountdownData()`（约第 68-83 行）：

```cpp
CountdownData makeCurrentCountdownData() {
  const std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = now > 0 ? localtime_r(&now, &buf) : nullptr;
  if (local == nullptr) {
    std::tm fallback{};
    // 设备时钟完全失效时的降级显示，固定为 2026-01-01
    fallback.tm_year = 126;
    fallback.tm_mon = 0;
    fallback.tm_mday = 1;
    fallback.tm_hour = 0;
    fallback.tm_min = 0;
    fallback.tm_sec = 0;
    fallback.tm_isdst = -1;
    return makeCountdownData(fallback);
  }
  return makeCountdownData(*local);
}
```

- [ ] **Step 3: 缩小 desc 缓冲区**

修改 `CountdownView::render()` 中（约第 101 行）：

```cpp
char desc[32] = {};
```

- [ ] **Step 4: 编译并运行全部测试**

```bash
pio test -e native
```

预期：141 个测试全部通过。

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "chore: 清理 countdown_view.cpp 代码风格问题

- 删除未使用的 sht40_reader.h include
- 统一使用 std::tm/localtime_r 风格
- 缩小 desc 缓冲区 64->32
- fallback 日期添加注释说明"
```

---

## Task 3: 统一 calendar_view.cpp 和 almanac_view.cpp 中的 localtime 风格

**Files:**
- Modify: `src/calendar_view.cpp`
- Modify: `src/almanac_view.cpp`

- [ ] **Step 1: 统一 calendar_view.cpp 中的 localtime 调用**

搜索所有 `localtime(` 和 `tm*` 的使用，统一替换为 `localtime_r` + `std::tm*` 风格。

涉及的函数：
- `makeCurrentCalendarData()`（约第 371-383 行）
- `makeCalendarData()` 中可能的其他调用
- `CalendarView::render()`（约第 424 行附近）

示例修改：
```cpp
// 修改前
std::tm* local = localtime(&now);

// 修改后
std::tm buf{};
std::tm* local = localtime_r(&now, &buf);
```

- [ ] **Step 2: 统一 almanac_view.cpp 中的 localtime 调用**

搜索所有 `localtime(` 和 `tm*` 的使用，统一替换。

涉及的函数：
- `AlmanacView::render()`（约第 576 行）
- `AlmanacView::renderSleep()`（约第 606 行）
- `makeCurrentHomeCalendarData()`（约第 468 行）
- `formatCurrentTimeHHMM()` 已在 Task 1 中删除

- [ ] **Step 3: 编译并运行全部测试**

```bash
pio test -e native
```

预期：141 个测试全部通过。

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "refactor: 统一 calendar_view 和 almanac_view 的 localtime_r 使用

将 localtime() 替换为线程安全的 localtime_r()，
统一使用 std::tm* 替代 tm*。"
```

---

## Task 4: 修复 BootController 双击重置代码不对称问题

**Files:**
- Modify: `src/boot_controller.cpp`

- [ ] **Step 1: 添加 Countdown 分支的显式注释**

在 `boot_controller.cpp` 第 82-96 行的双击处理逻辑中，为 Countdown 视图添加显式的空处理注释：

```cpp
} else if (btnCClicks >= 2) {
  if (viewManager_) {
    if (viewManager_->currentView() == SystemView::Calendar) {
      calendarMonthOffset_ = 0;
      if (deps_.renderCalendarWithOffset) {
        deps_.renderCalendarWithOffset(0);
      }
    } else if (viewManager_->currentView() == SystemView::Almanac) {
      almanacDayOffset_ = 0;
      if (deps_.renderAlmanacWithOffset) {
        deps_.renderAlmanacWithOffset(0);
      }
    }
    // CountdownView 无偏移状态，双击无需重置
    lastActivityMs_ = now;
  }
}
```

- [ ] **Step 2: 编译并运行全部测试**

```bash
pio test -e native
```

预期：141 个测试全部通过。

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "chore: BootController 双击重置添加 Countdown 分支注释

CountdownView 无偏移状态，双击无需操作。添加显式注释
消除代码阅读者的困惑。"
```

---

## Task 5: 修复 HomeRenderer 临时视图对象问题

**Files:**
- Modify: `src/home_renderer.cpp`

- [ ] **Step 1: 改用全局实例**

修改 `src/home_renderer.cpp`：

```cpp
// 添加前置声明（或在头文件中包含）
namespace homedeck {
extern AlmanacView gAlmanacView;
extern CalendarView gCalendarView;
}

// 修改 render 方法
void HomeRenderer::render(const HomeCalendarData& data) {
  gAlmanacView.render(data);
}

void HomeRenderer::renderCalendar(const CalendarData& data) {
  gCalendarView.render(data);
}
```

或者更简洁：在 `home_renderer.cpp` 顶部添加 `extern` 声明，直接调用全局实例。

注意：`gAlmanacView` 和 `gCalendarView` 定义在 `app_runtime.cpp` 中（匿名命名空间内），外部不可见。需要将它们移到 `namespace homedeck` 中，或改为在 `app_runtime.cpp` 的 `namespace homedeck` 中定义。

**更简单的替代方案：** 保留临时对象但添加注释说明这是无状态的向后兼容委托：

```cpp
void HomeRenderer::render(const HomeCalendarData& data) {
  // 无状态临时对象，仅为向后兼容保留的委托包装
  AlmanacView view;
  view.render(data);
}
```

选择替代方案（添加注释），因为移动全局实例的可见性改动较大，超出本次清理的范围。

- [ ] **Step 2: 编译并运行全部测试**

```bash
pio test -e native
```

预期：141 个测试全部通过。

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "chore: HomeRenderer 委托方法添加无状态注释

AlmanacView/CalendarView 为无状态类，临时对象构造开销
可忽略。添加注释说明这是向后兼容的委托包装。"
```

---

## Task 6: 最终验证

- [ ] **Step 1: 运行完整测试套件**

```bash
pio test -e native
```

预期：141 个测试全部通过，无编译警告。

- [ ] **Step 2: 检查 git diff 确认无意外变更**

```bash
git diff --stat HEAD~5..HEAD
```

确认每个 commit 的修改范围符合预期。

- [ ] **Step 3: 更新 TODO 列表状态**

所有 Task 标记为 done。
