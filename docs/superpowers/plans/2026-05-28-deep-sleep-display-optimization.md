# Deep Sleep 显示优化实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 进入 deep sleep 前去掉 "DEEP SLEEP" 文字，改为显示 HH:MM 时间；日历视图下 deep sleep 显示本月日历。

**Architecture:** 通过 `BootControllerDeps` 注入 `preSleepRender` 回调，让 `BootController` 在触发 deep sleep 前调用它完成最后一帧渲染。`BootController` 负责重置 offset，回调负责根据当前视图渲染对应内容（带 HH:MM 时间）。

**Tech Stack:** PlatformIO (Arduino/ESP32-S3), M5Unified + M5GFX, Unity native testing

---

## 文件结构

| 文件 | 责任 |
|------|------|
| `src/boot_controller.h` | `BootControllerDeps` 新增 `preSleepRender` 回调声明 |
| `src/boot_controller.cpp` | `updateHomeSleep()` 重置 offset + 调用 `preSleepRender(currentView())` |
| `src/home_renderer.h` | `CalendarData` 新增 `bottomCenterMessage` 字段 |
| `src/home_renderer.cpp` | `drawEnvironmentReadings()` 通用化；`renderCalendar()` 传递底部文字 |
| `src/app_runtime.cpp` | 绑定 `preSleepRender` lambda；删除 `renderHomeWithDeepSleepMessage()`；简化 `enterHomeDeepSleep()` |
| `test/native/test_boot_controller/test_main.cpp` | Fixture mock 桩 + 3 个新测试 |
| `test/native/test_home_renderer/test_main.cpp` | 更新现有测试数据 + 新增日历底部文字测试 |

---

## Task 1: BootController 新增 preSleepRender 回调

**Files:**
- Modify: `src/boot_controller.h`
- Modify: `src/boot_controller.cpp`

- [ ] **Step 1: 在 `BootControllerDeps` 中新增 `preSleepRender`**

  在 `src/boot_controller.h` 的 `BootControllerDeps` 结构体中，在 `enterDeepSleep` 上方添加：

  ```cpp
  std::function<void(SystemView)> preSleepRender;
  ```

- [ ] **Step 2: 在 `updateHomeSleep()` 中重置 offset 并调用 `preSleepRender`**

  修改 `src/boot_controller.cpp` 的 `updateHomeSleep()` 方法。在 `homeSleepRequested_ = true;` 之后、`deps_.enterDeepSleep(...)` 之前插入：

  ```cpp
  homeSleepRequested_ = true;
  calendarMonthOffset_ = 0;
  almanacDayOffset_ = 0;
  if (deps_.preSleepRender) {
    deps_.preSleepRender(currentView());
  }
  if (deps_.enterDeepSleep) {
    deps_.enterDeepSleep(makeHomeSleepRequest());
  }
  ```

- [ ] **Step 3: 编译验证**

  Run: `pio run --environment m5stack-cores3`
  Expected: 编译成功，无错误无警告

- [ ] **Step 4: Commit**

  ```bash
  git add src/boot_controller.h src/boot_controller.cpp
  git commit -m "feat: BootController 新增 preSleepRender 回调

- BootControllerDeps 新增 preSleepRender 字段
- updateHomeSleep() 在 enterDeepSleep 前先重置 offset 并调用 preSleepRender"
  ```

---

## Task 2: BootController 测试

**Files:**
- Modify: `test/native/test_boot_controller/test_main.cpp`

- [ ] **Step 1: Fixture 新增 `preSleepRender` mock 桩**

  在 Fixture 结构体中新增以下字段（在 `almanacOffsets` 下方）：

  ```cpp
  std::vector<homedeck::SystemView> preSleepRenderViews;
  bool preSleepRenderCalled = false;
  ```

  在 `deps()` 方法中 `enterDeepSleep` 之前添加：

  ```cpp
  deps.preSleepRender = [this](homedeck::SystemView view) {
    preSleepRenderCalled = true;
    preSleepRenderViews.push_back(view);
  };
  ```

- [ ] **Step 2: 新增 `test_preSleepRender_called_before_deep_sleep`**

  ```cpp
  void test_preSleepRender_called_before_deep_sleep() {
    Fixture f{};
    f.configured = true;
    homedeck::BootController controller{f.deps()};
    controller.begin();

    f.now = 300000;
    controller.update();

    TEST_ASSERT_TRUE(f.preSleepRenderCalled);
    TEST_ASSERT_EQUAL(1, static_cast<int>(f.preSleepRenderViews.size()));
    TEST_ASSERT_EQUAL(homedeck::SystemView::Almanac, f.preSleepRenderViews[0]);
    TEST_ASSERT_EQUAL(1, static_cast<int>(f.sleepRequests.size()));
  }
  ```

- [ ] **Step 3: 新增 `test_offsets_reset_before_deep_sleep`**

  ```cpp
  void test_offsets_reset_before_deep_sleep() {
    Fixture f{};
    f.configured = true;
    homedeck::BootController controller{f.deps()};
    controller.begin();

    // 翻页到上月
    f.calendarButtonClickCount = 1;
    controller.update();
    f.calendarButtonClickCount = 0;
    f.prevMonthClicked = true;
    controller.update();
    f.prevMonthClicked = false;

    // 黄历翻页到昨天
    f.prevMonthClicked = true;
    controller.update();
    f.prevMonthClicked = false;

    f.calendarOffsets.clear();
    f.almanacOffsets.clear();

    // 进入 deep sleep
    f.now = 300000;
    controller.update();

    // offset 已重置，preSleepRender 不会触发翻页渲染
    TEST_ASSERT_TRUE(f.preSleepRenderCalled);
    TEST_ASSERT_EQUAL(0, static_cast<int>(f.calendarOffsets.size()));
    TEST_ASSERT_EQUAL(0, static_cast<int>(f.almanacOffsets.size()));
  }
  ```

- [ ] **Step 4: 新增 `test_preSleepRender_receives_calendar_view`**

  ```cpp
  void test_preSleepRender_receives_calendar_view() {
    Fixture f{};
    f.configured = true;
    homedeck::BootController controller{f.deps()};
    controller.begin();

    // 切换到日历视图
    f.calendarButtonClickCount = 1;
    controller.update();
    f.calendarButtonClickCount = 0;

    f.now = 300000;
    controller.update();

    TEST_ASSERT_TRUE(f.preSleepRenderCalled);
    TEST_ASSERT_EQUAL(1, static_cast<int>(f.preSleepRenderViews.size()));
    TEST_ASSERT_EQUAL(homedeck::SystemView::Calendar, f.preSleepRenderViews[0]);
  }
  ```

- [ ] **Step 5: 注册新测试到 `main()`**

  在 `main()` 的 `UNITY_BEGIN()` 之后添加：

  ```cpp
  RUN_TEST(test_preSleepRender_called_before_deep_sleep);
  RUN_TEST(test_offsets_reset_before_deep_sleep);
  RUN_TEST(test_preSleepRender_receives_calendar_view);
  ```

- [ ] **Step 6: 运行测试**

  Run: `pio test --environment native --filter test_boot_controller`
  Expected: 所有测试通过（包括新增的 3 个）

- [ ] **Step 7: Commit**

  ```bash
  git add test/native/test_boot_controller/test_main.cpp
  git commit -m "test: BootController preSleepRender 测试

- 验证 deep sleep 前调用 preSleepRender 并传入正确视图
- 验证 offset 在 deep sleep 前被重置"
  ```

---

## Task 3: HomeRenderer 数据模型和渲染改动

**Files:**
- Modify: `src/home_renderer.h`
- Modify: `src/home_renderer.cpp`

- [ ] **Step 1: `CalendarData` 新增 `bottomCenterMessage`**

  在 `src/home_renderer.h` 的 `CalendarData` 结构体末尾添加：

  ```cpp
  std::string bottomCenterMessage;
  ```

- [ ] **Step 2: `drawEnvironmentReadings()` 通用化**

  修改 `src/home_renderer.cpp` 中的 `drawEnvironmentReadings()` 函数。把：

  ```cpp
  if (data.bottomCenterMessage == "DEEP SLEEP") {
    canvas.setTextDatum(textdatum_t::bottom_center);
    canvas.drawString(data.bottomCenterMessage.c_str(), kCalendarCenterX, bottomY);
  }
  ```

  改为：

  ```cpp
  if (!data.bottomCenterMessage.empty()) {
    canvas.setTextDatum(textdatum_t::bottom_center);
    canvas.drawString(data.bottomCenterMessage.c_str(), kCalendarCenterX, bottomY);
  }
  ```

- [ ] **Step 3: `renderCalendar()` 传递 `bottomCenterMessage`**

  在 `src/home_renderer.cpp` 的 `renderCalendar()` 方法中，构造临时 `HomeCalendarData` 时添加：

  ```cpp
  HomeCalendarData envData{};
  envData.temperatureAvailable = data.temperatureAvailable;
  envData.temperatureCelsius = data.temperatureCelsius;
  envData.humidityAvailable = data.humidityAvailable;
  envData.humidityPercent = data.humidityPercent;
  envData.bottomCenterMessage = data.bottomCenterMessage;  // 新增
  drawEnvironmentReadings(canvas, envData);
  ```

- [ ] **Step 4: 编译验证**

  Run: `pio run --environment m5stack-cores3`
  Expected: 编译成功

- [ ] **Step 5: Commit**

  ```bash
  git add src/home_renderer.h src/home_renderer.cpp
  git commit -m "feat: CalendarData 支持底部中间文字

- CalendarData 新增 bottomCenterMessage 字段
- drawEnvironmentReadings() 从硬判断 DEEP SLEEP 改为非空即显示
- renderCalendar() 传递 bottomCenterMessage 给底部渲染"
  ```

---

## Task 4: HomeRenderer 测试更新

**Files:**
- Modify: `test/native/test_home_renderer/test_main.cpp`

- [ ] **Step 1: 更新 `test_home_renderer_draws_bottom_center_message_when_present`**

  把测试数据从 `"DEEP SLEEP"` 改为 `"14:30"`，验证通用化后的逻辑：

  ```cpp
  void test_home_renderer_draws_bottom_center_message_when_present() {
    auto data = figmaCalendarData();
    data.bottomCenterMessage = "14:30";
    data.temperatureAvailable = true;
    data.temperatureCelsius = 30.04f;
    data.humidityAvailable = true;
    data.humidityPercent = 49.96f;
    homedeck::HomeRenderer renderer;

    renderer.render(data);

    bool foundMessage = false;
    bool foundTemperature = false;
    bool foundHumidity = false;
    for (const auto& print : M5.Display.prints) {
      if (print.text == "14:30") {
        TEST_ASSERT_EQUAL(200, print.x);
        TEST_ASSERT_EQUAL(kEnvironmentTextBottomY, print.y);
        TEST_ASSERT_EQUAL(static_cast<int>(textdatum_t::bottom_center), print.datum);
        TEST_ASSERT_EQUAL(static_cast<int>(FakeFontKind::kDeviceDefault), static_cast<int>(print.fontKind));
        TEST_ASSERT_EQUAL_UINT32(TFT_BLACK, print.color);
        foundMessage = true;
      }
      if (print.text == "30.0°C") {
        foundTemperature = true;
      }
      if (print.text == "50.0%") {
        foundHumidity = true;
      }
    }

    TEST_ASSERT_TRUE(foundMessage);
    TEST_ASSERT_TRUE(foundTemperature);
    TEST_ASSERT_TRUE(foundHumidity);
  }
  ```

- [ ] **Step 2: 更新 `test_home_renderer_does_not_draw_bottom_center_message_for_other_text`**

  重命名为 `test_home_renderer_does_not_draw_bottom_center_message_when_empty`，断言改为验证空字符串不绘制：

  ```cpp
  void test_home_renderer_does_not_draw_bottom_center_message_when_empty() {
    auto data = figmaCalendarData();
    data.bottomCenterMessage = "";
    homedeck::HomeRenderer renderer;

    renderer.render(data);

    for (const auto& print : M5.Display.prints) {
      TEST_ASSERT_FALSE(print.text == "14:30");
    }
  }
  ```

- [ ] **Step 3: 新增 `test_calendar_draws_bottom_center_message_when_present`**

  验证日历视图下底部中间文字也能正确显示：

  ```cpp
  void test_calendar_draws_bottom_center_message_when_present() {
    homedeck::CalendarData data;
    data.year = 2026;
    data.month = 5;
    data.day = 28;
    data.bottomCenterMessage = "09:15";
    data.temperatureAvailable = true;
    data.temperatureCelsius = 25.5f;
    data.humidityAvailable = true;
    data.humidityPercent = 60.0f;

    homedeck::HomeRenderer renderer;
    renderer.renderCalendar(data);

    bool foundMessage = false;
    bool foundTemperature = false;
    bool foundHumidity = false;
    for (const auto& print : M5.Display.prints) {
      if (print.text == "09:15") {
        TEST_ASSERT_EQUAL(200, print.x);
        TEST_ASSERT_EQUAL(kEnvironmentTextBottomY, print.y);
        TEST_ASSERT_EQUAL(static_cast<int>(textdatum_t::bottom_center), print.datum);
        foundMessage = true;
      }
      if (print.text == "25.5°C") {
        foundTemperature = true;
      }
      if (print.text == "60.0%") {
        foundHumidity = true;
      }
    }

    TEST_ASSERT_TRUE(foundMessage);
    TEST_ASSERT_TRUE(foundTemperature);
    TEST_ASSERT_TRUE(foundHumidity);
  }
  ```

- [ ] **Step 4: 注册新测试到 `main()`**

  更新 `main()`：
  - 保留 `RUN_TEST(test_home_renderer_draws_bottom_center_message_when_present);`（内容已更新）
  - 把 `RUN_TEST(test_home_renderer_does_not_draw_bottom_center_message_for_other_text);` 替换为 `RUN_TEST(test_home_renderer_does_not_draw_bottom_center_message_when_empty);`
  - 新增 `RUN_TEST(test_calendar_draws_bottom_center_message_when_present);`

- [ ] **Step 5: 运行测试**

  Run: `pio test --environment native --filter test_home_renderer`
  Expected: 所有测试通过

- [ ] **Step 6: Commit**

  ```bash
  git add test/native/test_home_renderer/test_main.cpp
  git commit -m "test: HomeRenderer 底部文字通用化测试

- 更新 bottom_center_message 测试数据为 14:30
- 新增日历视图底部中间文字测试"
  ```

---

## Task 5: app_runtime 集成 — preSleepRender 绑定 + enterHomeDeepSleep 简化

**Files:**
- Modify: `src/app_runtime.cpp`

- [ ] **Step 1: 删除 `renderHomeWithDeepSleepMessage()` 函数**

  在 `src/app_runtime.cpp` 中找到并删除整个 `renderHomeWithDeepSleepMessage()` 函数：

  ```cpp
  void renderHomeWithDeepSleepMessage() {
    HomeCalendarData data = makeCurrentHomeCalendarData();
    data.bottomCenterMessage = "DEEP SLEEP";
    gHomeRenderer.render(data);
  }
  ```

- [ ] **Step 2: 简化 `enterHomeDeepSleep()`**

  删除 `enterHomeDeepSleep()` 中的 `renderHomeWithDeepSleepMessage();` 调用：

  修改前（第303行附近）：
  ```cpp
  renderHomeWithDeepSleepMessage();
  M5.Display.sleep();
  ```

  修改后：
  ```cpp
  M5.Display.sleep();
  ```

- [ ] **Step 3: 在 `makeBootDeps()` 中绑定 `preSleepRender`**

  在 `makeBootDeps()` 中 `deps.enterDeepSleep = enterHomeDeepSleep;` 之前添加：

  ```cpp
  deps.preSleepRender = [](homedeck::SystemView view) {
    time_t now = time(nullptr);
    tm* local = localtime(&now);
    char timeStr[6] = {};
    if (local != nullptr) {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d", local->tm_hour, local->tm_min);
    }

    if (view == homedeck::SystemView::Almanac) {
      HomeCalendarData data = makeCurrentHomeCalendarDataWithEnvironment();
      data.bottomCenterMessage = timeStr;
      gHomeRenderer.render(data);
    } else {
      CalendarData data = makeCurrentCalendarData();
      data.bottomCenterMessage = timeStr;
      gHomeRenderer.renderCalendar(data);
    }
  };
  ```

- [ ] **Step 4: 编译验证**

  Run: `pio run --environment m5stack-cores3`
  Expected: 编译成功，无错误无警告

- [ ] **Step 5: Commit**

  ```bash
  git add src/app_runtime.cpp
  git commit -m "feat: app_runtime 集成 preSleepRender + 简化 deep sleep

- makeBootDeps() 绑定 preSleepRender：根据视图渲染黄历/日历+HH:MM
- 删除 renderHomeWithDeepSleepMessage()
- enterHomeDeepSleep() 不再负责渲染"
  ```

---

## Task 6: 全量验证

**Files:** 无新增/修改，仅运行验证命令

- [ ] **Step 1: 运行全部 native 测试**

  Run: `pio test --environment native`
  Expected: 所有测试通过

- [ ] **Step 2: ESP32-S3 编译验证**

  Run: `pio run --environment m5stack-cores3`
  Expected: 编译成功，无错误无警告

- [ ] **Step 3: Commit（如测试通过）**

  ```bash
  git commit --allow-empty -m "chore: 全量测试通过验证"
  ```

---

## Task 7: 最终提交检查

- [ ] **Step 1: 检查 git status**

  Run: `git status`
  Expected: 工作区干净，所有变更已提交

- [ ] **Step 2: 查看提交历史**

  Run: `git log --oneline -7`
  Expected: 看到以下提交（或类似）：
  1. `feat: BootController 新增 preSleepRender 回调`
  2. `test: BootController preSleepRender 测试`
  3. `feat: CalendarData 支持底部中间文字`
  4. `test: HomeRenderer 底部文字通用化测试`
  5. `feat: app_runtime 集成 preSleepRender + 简化 deep sleep`
  6. `chore: 全量测试通过验证`

---

## Self-Review Checklist

### Spec Coverage

| 设计文档需求 | 对应 Task |
|-------------|----------|
| `BootControllerDeps` 新增 `preSleepRender` | Task 1 |
| `updateHomeSleep()` 重置 offset + 调用 `preSleepRender` | Task 1 |
| `enterHomeDeepSleep()` 不再渲染 | Task 5 |
| `CalendarData` 新增 `bottomCenterMessage` | Task 3 |
| `drawEnvironmentReadings()` 通用化 | Task 3 |
| `renderCalendar()` 传递底部文字 | Task 3 |
| `preSleepRender` lambda 实现（HH:MM + 视图分支） | Task 5 |
| 删除 `renderHomeWithDeepSleepMessage()` | Task 5 |
| BootController 测试（调用顺序 + offset 重置 + 视图传递） | Task 2 |
| HomeRenderer 测试（通用化 + 日历底部文字） | Task 4 |

### Placeholder Scan

- [x] 无 TBD / TODO / "implement later"
- [x] 无 "add appropriate error handling" 等模糊描述
- [x] 每步都有完整代码或精确命令
- [x] 测试代码包含断言和预期输出

### Type Consistency

- [x] `preSleepRender` 签名：`std::function<void(SystemView)>` — 在各处一致
- [x] `bottomCenterMessage` 字段名在 `HomeCalendarData` 和 `CalendarData` 中一致
- [x] `SystemView` 枚举值 `Almanac` / `Calendar` 在各处使用正确
