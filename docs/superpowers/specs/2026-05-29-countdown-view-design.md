# 倒数日视图与统一视图接口重构设计

## 背景

HomeDeck 目前有两个视图：黄历（Almanac）和日历（Calendar），通过 G1（BtnC）循环切换。用户需要添加第三个视图——倒数日（Countdown），显示"今年还剩多少天"。

由于未来可能添加第四个视图，本次设计采用**统一视图接口重构**（方案 C），将现有两个视图一并纳入规范化的接口体系，使后续扩展只需"实现接口 + 注册到 ViewManager"即可。

## 目标

1. 添加倒数日视图，按 G1 切换顺序为：**黄历 → 日历 → 倒数日 → 黄历**。
2. 倒数日视图显示当年剩余天数，布局为三行居中文字 + 底部状态栏。
3. 重构视图管理层，使 `BootController` 不再直接感知各视图的具体差异。
4. Deep sleep 逻辑对所有视图统一处理。
5. 所有现有测试在重构后继续通过。

---

## 架构总览

核心变化是引入 **`ViewRenderer` 统一接口层**，让 `ViewManager` 和 `BootController` 都通过抽象接口操作视图。

```
┌─────────────────────────────────────────────────────────────┐
│                      BootController                         │
│  ┌─────────────┐   ┌─────────────────────────────────────┐  │
│  │ ViewManager │──▶│  AlmanacView / CalendarView /       │  │
│  │  (register  │   │  CountdownView  (ViewRenderer 接口) │  │
│  │   & switch) │   └─────────────────────────────────────┘  │
│  └─────────────┘                                           │
│         │                                                   │
│         └─ 按钮事件 ──▶ current()->onButtonA/B/reset()      │
│         └─ deep sleep ─▶ current()->renderSleep()           │
└─────────────────────────────────────────────────────────────┘
```

**关键改动点：**

1. **新建 `ViewRenderer` 接口**（`view_renderer.h`）：每个视图实现 `render()` / `renderSleep()` / `onButtonA()` / `onButtonB()` / `reset()` / `viewType()`。
2. **`ViewManager` 重构**：不再用 `std::function` 回调，改为持有 `std::vector<std::unique_ptr<ViewRenderer>>`，通过索引循环切换。
3. **`BootController` 解耦**：移除 `renderAlmanac` / `renderCalendar` / `renderCalendarWithOffset` / `renderAlmanacWithOffset` / `preSleepRender` 等 per-view 回调。按钮翻页、双击重置、deep sleep 前渲染全部通过 `ViewManager::current()` 统一转发。
4. **渲染工具提取**：`HomeRenderer` 中的共用工具（`sprite()`、`prepareScreen()`、`pushScreen()`、`drawEnvironmentReadings()` 等）提取到 `render_context.h/cpp`，供各视图共享。
5. **现有视图拆分**：黄历渲染 + 日偏移逻辑拆为 `AlmanacView`；日历渲染 + 月偏移逻辑拆为 `CalendarView`；新增 `CountdownView`。

---

## 视图接口与三个实现

### ViewRenderer 接口

```cpp
// view_renderer.h
namespace homedeck {

class ViewRenderer {
 public:
  virtual ~ViewRenderer() = default;
  virtual SystemView viewType() const = 0;
  virtual void render() = 0;        // 正常刷新
  virtual void renderSleep() = 0;   // deep sleep 前去活性画面
  virtual void onButtonA() {}       // BtnA（默认无操作）
  virtual void onButtonB() {}       // BtnB（默认无操作）
  virtual void reset() {}           // 双击 BtnC 重置偏移
};

}  // namespace homedeck
```

### 三个视图的职责划分

| 视图 | `viewType()` | 状态 | `onButtonA` | `onButtonB` | `reset()` |
|------|-------------|------|-------------|-------------|-----------|
| `AlmanacView` | `Almanac` | `dayOffset_`（当前显示日期偏移） | 上一天（`--`） | 下一天（`++`） | `dayOffset_ = 0`，重新渲染 |
| `CalendarView` | `Calendar` | `monthOffset_`（当前显示月份偏移） | 上月（`--`） | 下月（`++`） | `monthOffset_ = 0`，重新渲染 |
| `CountdownView` | `Countdown` | 无 | 空操作 | 空操作 | 空操作 |

### 数据流

- `AlmanacView::render()` 内部调用 `time(nullptr)` → `makeHomeCalendarData()`（含环境读数）→ 通过 `RenderContext` 绘制。
- `CalendarView::render()` 内部根据 `monthOffset_` 计算目标年月 → `makeCalendarData()` → 绘制。
- `CountdownView::render()` 计算当年剩余天数 → 绘制三行居中文字 + 底部状态栏。

每个视图自己持有偏移状态，`BootController` 不再关心"这个偏移是日还是月"。

---

## ViewManager 与 BootController 重构

### ViewManager

```cpp
// view_manager.h
class ViewManager {
 public:
  void addView(std::unique_ptr<ViewRenderer> view);
  void begin(SystemView initialView);
  void switchToNextView();
  SystemView currentView() const;
  ViewRenderer* current() const;

 private:
  std::vector<std::unique_ptr<ViewRenderer>> views_;
  size_t currentIndex_ = 0;
};
```

- `addView()` 按注册顺序决定切换循环：Almanac → Calendar → Countdown → Almanac。
- `switchToNextView()` 切换到 `views_[(currentIndex_ + 1) % views_.size()]` 并调用新视图的 `render()`。
- `current()` 返回裸指针，供 `BootController` 转发按钮和 deep sleep 事件。

### BootControllerDeps 简化

**移除以下回调：**
- `renderAlmanac`, `renderCalendar`
- `renderCalendarWithOffset`, `renderAlmanacWithOffset`
- `preSleepRender`

**保留（与配置、按钮硬件、时间、sleep 相关）：**
- `loadFlags`, `clearForceConfigOnNextBoot`, `setForceConfigOnNextBoot`
- `startConfigPortal`, `handleConfigPortalClient`
- `restoreSystemTimeFromRtc`
- `getCalendarButtonClickCount`, `wasPrevMonthClicked`, `wasNextMonthClicked`
- `updateButtons`, `areSetupButtonsPressed`, `millis`, `restart`, `currentTime`
- `loadSavedView`, `saveCurrentView`
- `enterDeepSleep`

### BootController::update() 中按钮处理简化

```cpp
// 1. BtnC 单击切换视图
if (btnCClicks == 1) {
  viewManager_->switchToNextView();
  deps_.saveCurrentView(viewManager_->currentView());
  lastActivityMs_ = now;
} else if (btnCClicks >= 2) {
  // 双击重置当前视图偏移
  if (auto* v = viewManager_->current()) {
    v->reset();
    v->render();
  }
  lastActivityMs_ = now;
}

// 2. BtnA/B 直接转发给当前视图（不再按视图类型判断）
if (auto* v = viewManager_->current()) {
  if (deps_.wasPrevMonthClicked()) v->onButtonA();
  if (deps_.wasNextMonthClicked()) v->onButtonB();
  // 若有渲染则刷新 sleep 计时器
}

// 3. deep sleep 前
if (auto* v = viewManager_->current()) {
  v->renderSleep();
}
deps_.enterDeepSleep(makeHomeSleepRequest());
```

`BootController` 内部不再需要 `calendarMonthOffset_` / `almanacDayOffset_` 成员变量。

---

## 倒数日视图（CountdownView） specifics

### 数据结构与计算

```cpp
// countdown_view.h
struct CountdownData {
  int year = 0;
  int daysRemaining = 0;
  bool temperatureAvailable = false;
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;
  float humidityPercent = 0.0f;
  std::string bottomCenterMessage;  // 时间 HH:MM
};

CountdownData makeCurrentCountdownData();
```

**计算逻辑（`makeCurrentCountdownData`）：**

1. 获取 `std::localtime()`；若返回 `nullptr`，使用 1970-01-01 fallback（与现有黄历/日历一致）。
2. 当年总天数 = 365（平年）或 366（闰年）。
3. `daysRemaining = totalDays - tm_yday - 1`。
   - 1月1日 → 364/365 天
   - 12月31日 → 0 天
4. 读取 SHT40 环境读数，填充温湿度。
5. `bottomCenterMessage` = 当前 `HH:MM`。

### 渲染布局

```
┌──────────────────────────────┐
│                              │
│      20xx年还剩               │  ← 第一行：kDeviceFontVlw（20px）
│                              │      textdatum_t::middle_center
│         xx                   │  ← 第二行：kDeviceLargeDateFontVlw（156px）
│                              │      textdatum_t::middle_center
│          天                   │  ← 第三行：kDeviceFontVlw（20px）
│                              │      textdatum_t::middle_center
│                              │
│  --.-°C      --:--      --.-% │  ← 底部状态栏（同黄历/日历）
└──────────────────────────────┘
```

三行整体水平垂直居中：先计算三行文字的总高度，以屏幕中心为基准均匀分布。

### CountdownView::renderSleep()

- 与 `render()` 相同布局。
- 但 `bottomCenterMessage = "--:--"`，温湿度显示为 `"--.-°C"` / `"--.-%"`（复用现有格式化逻辑）。

---

## Deep Sleep 与资源适配

### Deep Sleep 流程

- `BootController::updateHomeSleep()` 中不再调用 `deps_.preSleepRender(view)`，改为：
  ```cpp
  if (auto* v = viewManager_->current()) {
    v->renderSleep();
  }
  ```
- `gRtcSavedView` 类型已经是 `SystemView`，支持 `Countdown` 无需额外改动。
- `makeHomeSleepRequest()` 的"到次日 00:00"计算逻辑对所有视图通用，保持不变。

### 字体资源

- `tools/generate_device_font.py` 的 `EXTRA_TEXT` 追加"**剩**"字，确保 `20xx年还剩` 完整渲染。重新运行字体生成脚本后 `kDeviceFontVlw` 自动更新。
- `kDeviceFontVlw` 已包含"还"、数字和中文字形，第一行和第三行使用此字体。
- `kDeviceLargeDateFontVlw` 是 156px 数字字体，第二行天数使用此字体，无需修改。

---

## 测试策略

| 测试文件 | 内容 |
|---------|------|
| `test_view_manager` | Mock `ViewRenderer`（记录 `render()` / `viewType()` 调用），验证 `addView` 注册顺序、`begin` 初始视图、`switchToNextView` 三视图循环（Almanac→Calendar→Countdown→Almanac）。 |
| `test_boot_controller` | 简化 `BootControllerDeps`（移除 render 相关回调），注入配置好的 `ViewManager`。验证：BtnC 单击切换三视图、双击调用 `reset()`、BtnA/B 转发到当前视图、`renderSleep()` 在 deep sleep 前被调用。 |
| `test_countdown_view` | **新增**。验证 `makeCurrentCountdownData` 天数计算：平年 1月1日（364）、闰年 1月1日（365）、12月31日（0）、年中某日。不依赖硬件，纯日期计算单元测试。 |
| `test_almanac_view` / `test_calendar_view` | 由现有 `test_home_renderer` 拆分，分别测试数据生成和渲染逻辑（若现有测试覆盖）。 |

全部现有测试在重构后需继续通过。

---

## 文件变更清单

### 新增文件

| 文件 | 说明 |
|------|------|
| `src/view_renderer.h` | `ViewRenderer` 纯虚接口 |
| `src/render_context.h` / `.cpp` | 共用渲染工具（`sprite()`、`prepareScreen()`、`pushScreen()`、`drawEnvironmentReadings()` 等） |
| `src/almanac_view.h` / `.cpp` | 黄历视图：数据生成 + 渲染 + 日偏移 |
| `src/calendar_view.h` / `.cpp` | 日历视图：数据生成 + 渲染 + 月偏移 |
| `src/countdown_view.h` / `.cpp` | 倒数日视图：数据生成 + 渲染 |
| `test/native/test_countdown_view/test_main.cpp` | 倒数日天数计算单元测试 |

### 修改文件

| 文件 | 说明 |
|------|------|
| `src/view_manager.h` / `.cpp` | `SystemView` 枚举添加 `Countdown`；重构为基于 `ViewRenderer` 接口的注册/切换模式 |
| `src/boot_controller.h` / `.cpp` | 移除 per-view render 回调，改为通过 `ViewManager::current()` 转发 |
| `src/app_runtime.cpp` | `makeBootDeps()` 适配新 deps；创建 `ViewManager` 并注册三个视图；`preSleepRender` 移除 |
| `src/home_renderer.h` / `.cpp` | 黄历/日历渲染逻辑迁出到 `AlmanacView` / `CalendarView`，保留配置门户渲染或迁到独立文件 |
| `src/generated/device_font_vlw.h` / `.cpp` | 重新生成，包含"剩"字 |
| `tools/generate_device_font.py` | `EXTRA_TEXT` 追加"剩"字 |
| `test/native/test_view_manager/test_main.cpp` | 适配新 `ViewManager` 接口 |
| `test/native/test_boot_controller/test_main.cpp` | 适配新 `BootControllerDeps` |
| `test/native/test_home_renderer/test_main.cpp` | 视情况拆分为 `test_almanac_view` / `test_calendar_view` |

### 移除/重命名文件

- `src/home_renderer.h` / `.cpp` 中的黄历/日历渲染逻辑迁出后，若剩余内容仅为配置门户，可合并到 `setup_page` 相关文件或保留为简化版 `home_renderer`。

---

## 风险与回退

| 风险 | 缓解措施 |
|------|---------|
| 重构引入回归 bug | 所有现有测试（view_manager、boot_controller、home_renderer 等）必须在重构后通过；分步提交，先重构再添加 CountdownView。 |
| 字体生成后体积膨胀 | "剩"字为单个 glyph，对 7541 glyphs 的 `kDeviceFontVlw` 影响极小（~300 字节级）。 |
| 视图切换性能下降 | 接口虚函数调用开销可忽略；`switchToNextView` 仍只触发一次渲染。 |
