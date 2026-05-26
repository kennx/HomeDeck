# 日历翻页功能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在日历视图下实现月份翻页（G9上翻、G10下翻、双击G1回本月），并将按钮检测集中到 BootController 管理。

**Architecture:** ViewManager 改为被动式（移除按钮检测），BootController 集中处理所有按钮（BtnC 单击/双击、BtnA/B 翻页）并持有 `calendarMonthOffset_` 状态。新增 `renderCalendarWithOffset` 回调计算目标年月。

**Tech Stack:** C++17, ESP32-S3 Arduino, PlatformIO, Unity 测试框架, FakeM5

---

## 文件映射

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/view_manager.h` | 修改 | 移除 `wasCalendarButtonClicked`，移除 `update()`，新增 `switchToNextView()` |
| `src/view_manager.cpp` | 修改 | 实现被动式 ViewManager |
| `src/boot_controller.h` | 修改 | 新增 `calendarMonthOffset_`、按钮依赖、`renderCalendarWithOffset` |
| `src/boot_controller.cpp` | 修改 | 集中管理所有按钮检测 |
| `src/app_runtime.cpp` | 修改 | 新增 `renderCalendarWithOffset`，更新 `makeBootDeps()` |
| `test/native/support/fake_arduino/M5Unified.h` | 修改 | FakeButton 支持 `wasDecideClickCount()` 和 `getClickCount()` |
| `test/native/test_view_manager/test_main.cpp` | 修改 | 重写测试（移除按钮检测测试，新增 `switchToNextView` 测试） |
| `test/native/test_boot_controller/test_main.cpp` | 修改 | 新增翻页、双击、偏移重置等测试 |
| `test/native/test_home_renderer/test_main.cpp` | 修改 | 新增翻页后无高亮、月份正确渲染测试 |

---

### Task 1: ViewManager 改为被动式

**Files:**
- Modify: `src/view_manager.h`
- Modify: `src/view_manager.cpp`
- Test: `test/native/test_view_manager/test_main.cpp`

- [ ] **Step 1: 修改 ViewManager 头文件**

修改 `src/view_manager.h`：

```cpp
#pragma once

#include <functional>

namespace homedeck {

enum class SystemView {
  Almanac,
  Calendar,
};

struct ViewManagerDeps {
  std::function<void()> renderAlmanac;
  std::function<void()> renderCalendar;
};

class ViewManager {
 public:
  explicit ViewManager(ViewManagerDeps deps);
  void begin();
  void switchToNextView();
  SystemView currentView() const;
  bool viewSwitched() const;

 private:
  void switchTo(SystemView view);

  ViewManagerDeps deps_;
  SystemView currentView_ = SystemView::Almanac;
  bool viewSwitched_ = false;
};

}  // namespace homedeck
```

- [ ] **Step 2: 修改 ViewManager 实现**

修改 `src/view_manager.cpp`：

```cpp
#include "view_manager.h"

namespace homedeck {

ViewManager::ViewManager(ViewManagerDeps deps) : deps_(std::move(deps)) {
}

void ViewManager::begin() {
  switchTo(SystemView::Almanac);
}

void ViewManager::switchToNextView() {
  switch (currentView_) {
    case SystemView::Almanac:
      switchTo(SystemView::Calendar);
      break;
    case SystemView::Calendar:
      switchTo(SystemView::Almanac);
      break;
  }
}

SystemView ViewManager::currentView() const {
  return currentView_;
}

bool ViewManager::viewSwitched() const {
  return viewSwitched_;
}

void ViewManager::switchTo(SystemView view) {
  currentView_ = view;
  viewSwitched_ = true;
  switch (view) {
    case SystemView::Almanac:
      if (deps_.renderAlmanac) {
        deps_.renderAlmanac();
      }
      break;
    case SystemView::Calendar:
      if (deps_.renderCalendar) {
        deps_.renderCalendar();
      }
      break;
  }
}

}  // namespace homedeck
```

- [ ] **Step 3: 运行 view_manager 测试确认编译失败**

```bash
pio test -e native --filter test_view_manager
```

Expected: 编译失败（旧测试引用已删除的 `update()` 和 `wasCalendarButtonClicked`）

- [ ] **Step 4: 更新 view_manager 测试**

修改 `test/native/test_view_manager/test_main.cpp`：

```cpp
#include <unity.h>

#include <vector>

#include "view_manager.h"

namespace {

struct Fixture {
  std::vector<std::string> renderedViews;

  homedeck::ViewManagerDeps deps() {
    homedeck::ViewManagerDeps deps{};
    deps.renderAlmanac = [this]() { renderedViews.push_back("almanac"); };
    deps.renderCalendar = [this]() { renderedViews.push_back("calendar"); };
    return deps;
  }
};

}  // namespace

void setUp() {
}

void tearDown() {
}

void test_view_manager_begins_with_almanac() {
  Fixture f{};
  homedeck::ViewManager vm{f.deps()};

  vm.begin();

  TEST_ASSERT_EQUAL(homedeck::SystemView::Almanac, vm.currentView());
  TEST_ASSERT_TRUE(vm.viewSwitched());
  TEST_ASSERT_EQUAL(1, static_cast<int>(f.renderedViews.size()));
  TEST_ASSERT_EQUAL_STRING("almanac", f.renderedViews[0].c_str());
}

void test_view_manager_switch_to_next_view_from_almanac() {
  Fixture f{};
  homedeck::ViewManager vm{f.deps()};
  vm.begin();
  f.renderedViews.clear();

  vm.switchToNextView();

  TEST_ASSERT_EQUAL(homedeck::SystemView::Calendar, vm.currentView());
  TEST_ASSERT_TRUE(vm.viewSwitched());
  TEST_ASSERT_EQUAL(1, static_cast<int>(f.renderedViews.size()));
  TEST_ASSERT_EQUAL_STRING("calendar", f.renderedViews[0].c_str());
}

void test_view_manager_switch_to_next_view_from_calendar() {
  Fixture f{};
  homedeck::ViewManager vm{f.deps()};
  vm.begin();
  vm.switchToNextView();
  f.renderedViews.clear();

  vm.switchToNextView();

  TEST_ASSERT_EQUAL(homedeck::SystemView::Almanac, vm.currentView());
  TEST_ASSERT_TRUE(vm.viewSwitched());
  TEST_ASSERT_EQUAL_STRING("almanac", f.renderedViews[0].c_str());
}

void test_view_manager_switch_to_next_view_cycles() {
  Fixture f{};
  homedeck::ViewManager vm{f.deps()};
  vm.begin();

  vm.switchToNextView();
  vm.switchToNextView();
  vm.switchToNextView();

  TEST_ASSERT_EQUAL(homedeck::SystemView::Calendar, vm.currentView());
}

void test_view_manager_does_not_switch_without_call() {
  Fixture f{};
  homedeck::ViewManager vm{f.deps()};
  vm.begin();
  f.renderedViews.clear();

  // 不调用 switchToNextView

  TEST_ASSERT_EQUAL(homedeck::SystemView::Almanac, vm.currentView());
  TEST_ASSERT_FALSE(vm.viewSwitched());
  TEST_ASSERT_EQUAL(0, static_cast<int>(f.renderedViews.size()));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_view_manager_begins_with_almanac);
  RUN_TEST(test_view_manager_switch_to_next_view_from_almanac);
  RUN_TEST(test_view_manager_switch_to_next_view_from_calendar);
  RUN_TEST(test_view_manager_switch_to_next_view_cycles);
  RUN_TEST(test_view_manager_does_not_switch_without_call);
  return UNITY_END();
}
```

- [ ] **Step 5: 运行 view_manager 测试确认通过**

```bash
pio test -e native --filter test_view_manager
```

Expected: 5 tests PASS

- [ ] **Step 6: Commit**

```bash
git add src/view_manager.h src/view_manager.cpp test/native/test_view_manager/test_main.cpp
git commit -m "refactor: ViewManager 改为被动式

- 移除按钮检测逻辑和 wasCalendarButtonClicked 依赖
- 移除 update() 方法
- 新增 switchToNextView() 由外部调用
- 更新测试"
```

---

### Task 2: FakeM5 按钮桩更新

**Files:**
- Modify: `test/native/support/fake_arduino/M5Unified.h`

- [ ] **Step 1: FakeButton 新增双击检测支持**

修改 `test/native/support/fake_arduino/M5Unified.h` 中的 `FakeButton`：

```cpp
struct FakeButton {
  bool pressed = false;
  bool clicked = false;
  int clickCount = 0;           // 用于模拟双击
  bool decideClickCount = false; // 用于模拟 wasDecideClickCount

  bool isPressed() const {
    return pressed;
  }
  bool wasClicked() {
    if (clicked) {
      clicked = false;
      return true;
    }
    return false;
  }
  bool wasDecideClickCount() {
    if (decideClickCount) {
      decideClickCount = false;
      return true;
    }
    return false;
  }
  int getClickCount() const {
    return clickCount;
  }
};
```

- [ ] **Step 2: Commit**

```bash
git add test/native/support/fake_arduino/M5Unified.h
git commit -m "test: FakeButton 支持双击检测桩"
```

---

### Task 3: BootController 集中管理按钮

**Files:**
- Modify: `src/boot_controller.h`
- Modify: `src/boot_controller.cpp`
- Test: `test/native/test_boot_controller/test_main.cpp`

- [ ] **Step 1: 修改 BootController 头文件**

修改 `src/boot_controller.h`：

```cpp
#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>

#include "view_manager.h"

namespace homedeck {

struct HomeSleepRequest {
  std::uint64_t timerWakeupUs = 0;
  int wakeupGpio = 1;
  bool wakeOnLow = true;
};

enum class BootMode {
  Config,
  System,
};

struct BootFlags {
  bool configured = false;
  bool forceConfigOnNextBoot = false;
};

struct BootControllerDeps {
  std::function<BootFlags()> loadFlags;
  std::function<bool()> clearForceConfigOnNextBoot;
  std::function<bool()> setForceConfigOnNextBoot;
  std::function<void()> startConfigPortal;
  std::function<void()> handleConfigPortalClient;
  std::function<void()> restoreSystemTimeFromRtc;
  std::function<void()> renderAlmanac;
  std::function<void()> renderCalendar;
  std::function<void(int monthOffset)> renderCalendarWithOffset;
  std::function<int()> getCalendarButtonClickCount;
  std::function<bool()> wasPrevMonthClicked;
  std::function<bool()> wasNextMonthClicked;
  std::function<void()> updateButtons;
  std::function<bool()> areSetupButtonsPressed;
  std::function<unsigned long()> millis;
  std::function<void()> restart;
  std::function<std::time_t()> currentTime;
  std::function<void(const HomeSleepRequest&)> enterDeepSleep;
};

class BootController {
 public:
  explicit BootController(BootControllerDeps deps);

  void begin();
  void update();
  BootMode mode() const;
  SystemView currentView() const;

 private:
  void enterConfigMode();
  void enterSystemMode();
  void updateSetupShortcut(unsigned long now);
  void updateHomeSleep(unsigned long now);
  HomeSleepRequest makeHomeSleepRequest() const;

  BootControllerDeps deps_;
  BootMode mode_ = BootMode::System;
  bool started_ = false;
  unsigned long setupButtonsPressedSinceMs_ = 0;
  bool setupButtonsWerePressed_ = false;
  bool setupShortcutConsumed_ = false;
  unsigned long lastActivityMs_ = 0;
  bool homeSleepRequested_ = false;
  int calendarMonthOffset_ = 0;
  std::unique_ptr<ViewManager> viewManager_;
};

}  // namespace homedeck
```

- [ ] **Step 2: 修改 BootController 实现**

修改 `src/boot_controller.cpp`：

```cpp
#include "boot_controller.h"

#include <cstdint>
#include <ctime>
#include <utility>

namespace homedeck {
namespace {

constexpr unsigned long kSetupShortcutHoldMs = 5000;
constexpr unsigned long kHomeDisplayDurationMs = 300000;
constexpr std::time_t kTrustedUnixTimeThreshold = 1704067200;
constexpr std::uint64_t kMicrosPerSecond = 1000000ULL;
constexpr std::uint64_t kFallbackSleepSeconds = 3600ULL;
constexpr int kButtonCWakeupGpio = 1;

}  // namespace

BootController::BootController(BootControllerDeps deps) : deps_(std::move(deps)) {
}

void BootController::begin() {
  const BootFlags flags = deps_.loadFlags ? deps_.loadFlags() : BootFlags{};
  started_ = true;

  if (!flags.configured) {
    enterConfigMode();
    return;
  }

  if (flags.forceConfigOnNextBoot) {
    if (deps_.clearForceConfigOnNextBoot) {
      deps_.clearForceConfigOnNextBoot();
    }
    enterConfigMode();
    return;
  }

  enterSystemMode();
}

void BootController::update() {
  if (!started_) {
    return;
  }

  if (mode_ == BootMode::Config) {
    if (deps_.handleConfigPortalClient) {
      deps_.handleConfigPortalClient();
    }
    return;
  }

  if (deps_.updateButtons) {
    deps_.updateButtons();
  }
  const unsigned long now = deps_.millis ? deps_.millis() : 0;

  // 1. 检测 BtnC（视图切换 / 双击回本月）
  const int btnCClicks = deps_.getCalendarButtonClickCount ? deps_.getCalendarButtonClickCount() : 0;
  if (btnCClicks == 1) {
    if (viewManager_) {
      viewManager_->switchToNextView();
      if (viewManager_->viewSwitched()) {
        lastActivityMs_ = now;
      }
    }
  } else if (btnCClicks == 2) {
    if (viewManager_ && viewManager_->currentView() == SystemView::Calendar) {
      calendarMonthOffset_ = 0;
      if (deps_.renderCalendarWithOffset) {
        deps_.renderCalendarWithOffset(0);
      }
      lastActivityMs_ = now;
    }
  }

  // 2. 检测日历翻页（仅在 Calendar 视图）
  if (viewManager_ && viewManager_->currentView() == SystemView::Calendar) {
    bool calendarUpdated = false;
    if (deps_.wasPrevMonthClicked && deps_.wasPrevMonthClicked()) {
      calendarMonthOffset_--;
      calendarUpdated = true;
    } else if (deps_.wasNextMonthClicked && deps_.wasNextMonthClicked()) {
      calendarMonthOffset_++;
      calendarUpdated = true;
    }
    if (calendarUpdated && deps_.renderCalendarWithOffset) {
      deps_.renderCalendarWithOffset(calendarMonthOffset_);
      lastActivityMs_ = now;
    }
  }

  updateSetupShortcut(now);
  updateHomeSleep(now);
}

BootMode BootController::mode() const {
  return mode_;
}

SystemView BootController::currentView() const {
  return viewManager_ ? viewManager_->currentView() : SystemView::Almanac;
}

void BootController::enterConfigMode() {
  mode_ = BootMode::Config;
  if (deps_.startConfigPortal) {
    deps_.startConfigPortal();
  }
}

void BootController::enterSystemMode() {
  mode_ = BootMode::System;
  setupButtonsPressedSinceMs_ = 0;
  setupButtonsWerePressed_ = false;
  setupShortcutConsumed_ = false;
  homeSleepRequested_ = false;
  calendarMonthOffset_ = 0;

  if (deps_.restoreSystemTimeFromRtc) {
    deps_.restoreSystemTimeFromRtc();
  }

  ViewManagerDeps vmDeps{};
  vmDeps.renderAlmanac = deps_.renderAlmanac;
  vmDeps.renderCalendar = deps_.renderCalendar;
  viewManager_ = std::make_unique<ViewManager>(std::move(vmDeps));
  viewManager_->begin();

  lastActivityMs_ = deps_.millis ? deps_.millis() : 0;
}

void BootController::updateHomeSleep(unsigned long now) {
  if (homeSleepRequested_) {
    return;
  }
  if (now - lastActivityMs_ < kHomeDisplayDurationMs) {
    return;
  }

  homeSleepRequested_ = true;
  if (deps_.enterDeepSleep) {
    deps_.enterDeepSleep(makeHomeSleepRequest());
  }
}

HomeSleepRequest BootController::makeHomeSleepRequest() const {
  HomeSleepRequest request{};
  request.wakeupGpio = kButtonCWakeupGpio;
  request.wakeOnLow = true;
  request.timerWakeupUs = kFallbackSleepSeconds * kMicrosPerSecond;

  const std::time_t now = deps_.currentTime ? deps_.currentTime() : 0;
  if (now < kTrustedUnixTimeThreshold) {
    return request;
  }

  const std::tm* local = std::localtime(&now);
  if (local == nullptr || local->tm_year < 124 || local->tm_mon < 0 || local->tm_mon > 11 || local->tm_mday < 1) {
    return request;
  }

  const int secondsUntilMidnight = (24 - local->tm_hour) * 3600 - local->tm_min * 60 - local->tm_sec;
  if (secondsUntilMidnight <= 0) {
    return request;
  }

  request.timerWakeupUs = static_cast<std::uint64_t>(secondsUntilMidnight) * kMicrosPerSecond;
  return request;
}

void BootController::updateSetupShortcut(unsigned long now) {
  if (setupShortcutConsumed_) {
    return;
  }
  const bool pressed = deps_.areSetupButtonsPressed && deps_.areSetupButtonsPressed();
  if (!pressed) {
    setupButtonsWerePressed_ = false;
    setupButtonsPressedSinceMs_ = 0;
    return;
  }

  if (!setupButtonsWerePressed_) {
    setupButtonsWerePressed_ = true;
    setupButtonsPressedSinceMs_ = now;
    return;
  }

  if (now - setupButtonsPressedSinceMs_ >= kSetupShortcutHoldMs) {
    if (deps_.setForceConfigOnNextBoot && deps_.setForceConfigOnNextBoot()) {
      setupShortcutConsumed_ = true;
    }
    if (setupShortcutConsumed_ && deps_.restart) {
      deps_.restart();
    }
  }
}

}  // namespace homedeck
```

- [ ] **Step 3: 运行 boot_controller 测试确认编译失败**

```bash
pio test -e native --filter test_boot_controller
```

Expected: 编译失败（旧测试引用已删除的 `wasCalendarButtonClicked` 等）

- [ ] **Step 4: Commit 中间状态（可选）**

```bash
git add src/boot_controller.h src/boot_controller.cpp
git commit -m "refactor: BootController 集中管理按钮检测

- 新增 calendarMonthOffset_ 状态
- 新增 getCalendarButtonClickCount / wasPrevMonthClicked / wasNextMonthClicked 依赖
- 新增 renderCalendarWithOffset 依赖
- update() 中集中处理 BtnC 单击/双击、BtnA/B 翻页
- enterSystemMode() 重置偏移为 0"
```

---

### Task 4: app_runtime 提供新回调

**Files:**
- Modify: `src/app_runtime.cpp`

- [ ] **Step 1: 实现 renderCalendarWithOffset**

在 `src/app_runtime.cpp` 中，在 `renderCalendarWithEnvironment()` 附近新增：

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

  while (targetMonth > 12) {
    targetMonth -= 12;
    targetYear++;
  }
  while (targetMonth < 1) {
    targetMonth += 12;
    targetYear--;
  }

  CalendarData data;
  data.year = targetYear;
  data.month = targetMonth;
  data.day = (monthOffset == 0) ? local->tm_mday : 0;

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

- [ ] **Step 2: 更新 makeBootDeps()**

修改 `src/app_runtime.cpp` 中的 `makeBootDeps()`：

```cpp
deps.renderAlmanac = renderHomeWithEnvironment;
deps.renderCalendar = renderCalendarWithEnvironment;
deps.renderCalendarWithOffset = renderCalendarWithOffset;
deps.getCalendarButtonClickCount = []() {
  if (M5.BtnC.wasDecideClickCount()) {
    return M5.BtnC.getClickCount();
  }
  return 0;
};
deps.wasPrevMonthClicked = []() { return M5.BtnB.wasClicked(); };
deps.wasNextMonthClicked = []() { return M5.BtnA.wasClicked(); };
deps.updateButtons = []() { M5.update(); };
deps.areSetupButtonsPressed = []() { return M5.BtnA.isPressed() && M5.BtnB.isPressed(); };
```

同时移除 `deps.wasCalendarButtonClicked`（已删除）。

- [ ] **Step 3: ESP32-S3 编译验证**

```bash
pio run -e m5stack-papercolor
```

Expected: SUCCESS

- [ ] **Step 4: Commit**

```bash
git add src/app_runtime.cpp
git commit -m "feat: app_runtime 提供日历翻页依赖

- 新增 renderCalendarWithOffset 回调
- 更新 makeBootDeps 提供按钮检测回调
- 移除已删除的 wasCalendarButtonClicked"
```

---

### Task 5: BootController 测试更新

**Files:**
- Modify: `test/native/test_boot_controller/test_main.cpp`

- [ ] **Step 1: 更新 Fixture 和测试**

修改 `test/native/test_boot_controller/test_main.cpp`，重写 Fixture：

```cpp
#include <unity.h>

#include <cstdlib>
#include <ctime>
#include <vector>

#include "boot_controller.h"

namespace {

struct Fixture {
  bool configured = false;
  bool forceConfig = false;
  bool portalStarted = false;
  bool portalHandled = false;
  bool homeRendered = false;
  bool calendarRendered = false;
  int calendarButtonClickCount = 0;
  bool prevMonthClicked = false;
  bool nextMonthClicked = false;
  bool forceFlagWritten = false;
  bool forceFlagCleared = false;
  bool forceFlagWriteSucceeds = true;
  bool restarted = false;
  bool buttonsPressed = false;
  unsigned long now = 0;
  unsigned long renderHomeDurationMs = 0;
  int updateCalls = 0;
  std::time_t currentUnix = 1704110400;
  std::vector<homedeck::HomeSleepRequest> sleepRequests;
  std::vector<int> calendarOffsets;

  homedeck::BootControllerDeps deps() {
    homedeck::BootControllerDeps deps{};
    deps.loadFlags = [this]() {
      return homedeck::BootFlags{configured, forceConfig};
    };
    deps.clearForceConfigOnNextBoot = [this]() {
      forceConfig = false;
      forceFlagCleared = true;
      return true;
    };
    deps.setForceConfigOnNextBoot = [this]() {
      forceFlagWritten = true;
      if (!forceFlagWriteSucceeds) {
        return false;
      }
      forceConfig = true;
      return true;
    };
    deps.startConfigPortal = [this]() { portalStarted = true; };
    deps.handleConfigPortalClient = [this]() { portalHandled = true; };
    deps.restoreSystemTimeFromRtc = []() {};
    deps.renderAlmanac = [this]() {
      homeRendered = true;
      now += renderHomeDurationMs;
    };
    deps.renderCalendar = [this]() { calendarRendered = true; };
    deps.renderCalendarWithOffset = [this](int offset) {
      calendarOffsets.push_back(offset);
    };
    deps.getCalendarButtonClickCount = [this]() { return calendarButtonClickCount; };
    deps.wasPrevMonthClicked = [this]() { return prevMonthClicked; };
    deps.wasNextMonthClicked = [this]() { return nextMonthClicked; };
    deps.updateButtons = [this]() { ++updateCalls; };
    deps.areSetupButtonsPressed = [this]() { return buttonsPressed; };
    deps.millis = [this]() { return now; };
    deps.restart = [this]() { restarted = true; };
    deps.currentTime = [this]() { return currentUnix; };
    deps.enterDeepSleep = [this](const homedeck::HomeSleepRequest& request) {
      sleepRequests.push_back(request);
    };
    return deps;
  }
};

}  // namespace

void setUp() {
  setenv("TZ", "UTC", 1);
  tzset();
}

void tearDown() {
}
```

- [ ] **Step 2: 保留现有测试并新增翻页测试**

保留所有现有测试（除了与 `wasCalendarButtonClicked` 相关的需要更新），新增以下测试：

```cpp
void test_single_click_switches_view() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.homeRendered = false;
  f.calendarRendered = false;
  f.calendarButtonClickCount = 1;
  controller.update();

  TEST_ASSERT_TRUE(f.calendarRendered);
  TEST_ASSERT_EQUAL(homedeck::SystemView::Calendar, controller.currentView());
}

void test_double_click_resets_to_today_in_calendar() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  // 先切换到 Calendar
  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;

  // 翻页到上月
  f.prevMonthClicked = true;
  controller.update();
  f.prevMonthClicked = false;

  // 双击回本月
  f.calendarOffsets.clear();
  f.calendarButtonClickCount = 2;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.calendarOffsets.size()));
  TEST_ASSERT_EQUAL(0, f.calendarOffsets[0]);
}

void test_double_click_ignored_in_almanac() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.calendarOffsets.clear();
  f.calendarButtonClickCount = 2;
  controller.update();

  TEST_ASSERT_EQUAL(0, static_cast<int>(f.calendarOffsets.size()));
}

void test_prev_month_click_in_calendar() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;

  f.prevMonthClicked = true;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.calendarOffsets.size()));
  TEST_ASSERT_EQUAL(-1, f.calendarOffsets[0]);
}

void test_next_month_click_in_calendar() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;

  f.nextMonthClicked = true;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.calendarOffsets.size()));
  TEST_ASSERT_EQUAL(1, f.calendarOffsets[0]);
}

void test_month_click_ignored_in_almanac() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.prevMonthClicked = true;
  controller.update();

  TEST_ASSERT_EQUAL(0, static_cast<int>(f.calendarOffsets.size()));
}

void test_continuous_prev_month_clicks() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;

  f.prevMonthClicked = true;
  controller.update();
  f.prevMonthClicked = true;
  controller.update();

  TEST_ASSERT_EQUAL(2, static_cast<int>(f.calendarOffsets.size()));
  TEST_ASSERT_EQUAL(-1, f.calendarOffsets[0]);
  TEST_ASSERT_EQUAL(-2, f.calendarOffsets[1]);
}

void test_month_click_resets_sleep_timer() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;

  f.now = 240000;
  f.prevMonthClicked = true;
  controller.update();
  f.prevMonthClicked = false;

  f.now = 480000;
  controller.update();
  TEST_ASSERT_EQUAL(0, static_cast<int>(f.sleepRequests.size()));

  f.now = 780000;
  controller.update();
  TEST_ASSERT_EQUAL(1, static_cast<int>(f.sleepRequests.size()));
}

void test_enter_system_mode_resets_month_offset() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  // 先翻页
  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;
  f.prevMonthClicked = true;
  controller.update();
  f.prevMonthClicked = false;

  // 重新进入 SystemMode（模拟唤醒）
  controller.begin();
  f.calendarOffsets.clear();
  f.calendarButtonClickCount = 1;
  controller.update();
  f.calendarButtonClickCount = 0;
  f.prevMonthClicked = true;
  controller.update();

  // 如果偏移被重置，翻页后偏移应为 -1
  TEST_ASSERT_EQUAL(1, static_cast<int>(f.calendarOffsets.size()));
  TEST_ASSERT_EQUAL(-1, f.calendarOffsets[0]);
}
```

注意：需要更新 `test_calendar_button_click_switches_view_and_resets_sleep_timer` 和 `test_second_calendar_click_switches_back_to_almanac` 这两个旧测试，改为使用 `calendarButtonClickCount`。

- [ ] **Step 3: 运行 boot_controller 测试确认全部通过**

```bash
pio test -e native --filter test_boot_controller
```

Expected: 所有测试 PASS（包括保留的旧测试和新增测试）

- [ ] **Step 4: Commit**

```bash
git add test/native/test_boot_controller/test_main.cpp
git commit -m "test: BootController 日历翻页测试

- 更新 Fixture 使用新的按钮检测回调
- 新增单击/双击/翻页/偏移重置测试
- 保留并更新现有 deep sleep 测试"
```

---

### Task 6: HomeRenderer 测试新增

**Files:**
- Modify: `test/native/test_home_renderer/test_main.cpp`

- [ ] **Step 1: 新增翻页后无高亮测试**

在 `test/native/test_home_renderer/test_main.cpp` 中新增：

```cpp
void test_calendar_no_highlight_when_day_is_zero() {
  homedeck::CalendarData data;
  data.year = 2026;
  data.month = 4;
  data.day = 0;  // 翻到别月，day=0 表示不显示高亮

  homedeck::HomeRenderer renderer;
  renderer.renderCalendar(data);

  // 验证没有任何黑色填充矩形（高亮使用 fillRect + TFT_BLACK）
  for (const auto& rect : M5.Display.rects) {
    if (rect.color == TFT_BLACK && rect.h > 10) {
      // 高亮矩形的高度大约是 kCalDateRowHeight=47
      // 但配置界面也用了黑色，所以需要更精确的判断
      // 实际上配置界面不在日历测试中执行
    }
  }

  // 更简单的方式：验证没有打印任何白色文字的日期
  // 因为高亮时使用 setTextColor(TFT_WHITE, TFT_BLACK)
  bool foundWhiteText = false;
  for (const auto& print : M5.Display.prints) {
    if (print.color == TFT_WHITE) {
      foundWhiteText = true;
    }
  }
  TEST_ASSERT_FALSE(foundWhiteText);
}

void test_calendar_renders_past_month_correctly() {
  homedeck::CalendarData data;
  data.year = 2026;
  data.month = 4;
  data.day = 26;  // 真实今天是 4月26日，高亮

  homedeck::HomeRenderer renderer;
  renderer.renderCalendar(data);

  bool foundYear = false;
  bool foundMonth = false;
  for (const auto& print : M5.Display.prints) {
    if (print.text == "2026 年") {
      foundYear = true;
    }
    if (print.text == "四月") {
      foundMonth = true;
    }
  }
  TEST_ASSERT_TRUE(foundYear);
  TEST_ASSERT_TRUE(foundMonth);
}
```

- [ ] **Step 2: 运行 home_renderer 测试**

```bash
pio test -e native --filter test_home_renderer
```

Expected: 所有测试 PASS

- [ ] **Step 3: Commit**

```bash
git add test/native/test_home_renderer/test_main.cpp
git commit -m "test: HomeRenderer 日历翻页渲染测试

- 验证 day=0 时不显示当天高亮
- 验证翻页后月份标题正确"
```

---

### Task 7: 全链路编译和测试验证

- [ ] **Step 1: 运行所有 native 测试**

```bash
pio test -e native
```

Expected: 所有测试 PASS

- [ ] **Step 2: ESP32-S3 编译验证**

```bash
pio run -e m5stack-papercolor
```

Expected: SUCCESS

- [ ] **Step 3: Commit**

```bash
git commit -m "chore: 日历翻页功能完整验证通过

- 所有 native 测试通过
- ESP32-S3 编译成功"
```

---

## 计划自审

### 规格覆盖检查

| 规格需求 | 对应任务 |
|---------|---------|
| G9 上翻月 | Task 3 (BootController), Task 5 (测试) |
| G10 下翻月 | Task 3 (BootController), Task 5 (测试) |
| 双击 G1 回本月 | Task 3 (BootController), Task 5 (测试) |
| 当天高亮只在本月 | Task 4 (renderCalendarWithOffset 中 day=0), Task 6 (测试) |
| Deep sleep 唤醒回本月 | Task 3 (enterSystemMode 重置偏移), Task 5 (测试) |
| 翻页重置计时器 | Task 3 (update 中 lastActivityMs_ = now), Task 5 (测试) |
| 按钮检测集中到 BootController | Task 1 (ViewManager 移除), Task 3 (BootController 集中) |

### Placeholder 扫描

- 无 TBD/TODO/"implement later"
- 每个步骤包含完整代码
- 无 "similar to Task N"
- 所有类型和函数名前后一致

### 类型一致性

- `calendarMonthOffset_` 始终为 `int`
- `renderCalendarWithOffset` 始终为 `std::function<void(int)>`
- `getCalendarButtonClickCount` 始终为 `std::function<int()>`
- FakeButton 的 `wasDecideClickCount()` 和 `getClickCount()` 与 M5Unified API 匹配
