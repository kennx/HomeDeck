# Migrate HomeRenderer to ConfigPortalRenderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `HomeRenderer` from `views/` to `config/` as `ConfigPortalRenderer`, delete dead code (`render()` and `renderCalendar()`), and update all references.

**Architecture:** `HomeRenderer` currently lives in `views/` but its only active method is `renderConfigPortal()` — a configuration portal page renderer. Its `render()` and `renderCalendar()` methods are dead code that delegate to `AlmanacView` and `CalendarView`. This refactor moves the config portal renderer to `config/` where it belongs (alongside `ConfigPortal`), removes the dead code, and cleans up naming.

**Tech Stack:** C++17, PlatformIO native + m5stack-papercolor, M5Unified, LittleFS, QRCode

---

## File Structure

| File | Action | Responsibility |
|------|--------|--------------|
| `src/config/config_portal_renderer.h` | Create | `ConfigPortalRenderer` class declaration — single method `renderConfigPortal()` |
| `src/config/config_portal_renderer.cpp` | Create | Implementation moved from `views/home_renderer.cpp`, dead code removed |
| `src/views/home_renderer.h` | Delete | Old `HomeRenderer` class with misleading name and dead methods |
| `src/views/home_renderer.cpp` | Delete | Old implementation with dead `render()`/`renderCalendar()` methods |
| `src/app/app_runtime.cpp` | Modify | Update include, variable name (`gHomeRenderer` → `gConfigPortalRenderer`), call site |
| `AGENTS.md` | Modify | Update project map to reflect new file locations and responsibilities |

---

### Task 1: Create `src/config/config_portal_renderer.h`

**Files:**
- Create: `src/config/config_portal_renderer.h`

- [ ] **Step 1: Write the header file**

Copy the single active method from `HomeRenderer`, rename class to `ConfigPortalRenderer`:

```cpp
#pragma once

#include <string>

namespace homedeck {

class ConfigPortalRenderer {
 public:
  void renderConfigPortal(const std::string& apSsid, const std::string& ipAddress);
};

}  // namespace homedeck
```

- [ ] **Step 2: Commit**

```bash
git add src/config/config_portal_renderer.h
git commit -m "refactor: add ConfigPortalRenderer header in config/"
```

---

### Task 2: Create `src/config/config_portal_renderer.cpp`

**Files:**
- Create: `src/config/config_portal_renderer.cpp`

- [ ] **Step 1: Write the implementation**

Move the active implementation from `views/home_renderer.cpp`, stripping dead code. The implementation includes:
- Logo drawing from LittleFS `/logo.png`
- AP SSID and IP address text rendering
- WiFi QR code generation and drawing

```cpp
#include "config/config_portal_renderer.h"

#include <LittleFS.h>
#include <M5Unified.h>
#include <qrcode.h>

#include <string>
#include <vector>
#include <cstdint>

#include "generated/device_font_vlw.h"
#include "system/render_context.h"

namespace homedeck {
namespace {

constexpr int kLogoTopY = 86;
constexpr int kLogoWidth = 297;
constexpr int kLogoHeight = 40;
constexpr int kTextFrameHeight = 27;
constexpr int kApTextTopY = kLogoTopY + kLogoHeight + 26;
constexpr int kApTextCenterY = kApTextTopY + kTextFrameHeight / 2;
constexpr int kIpTextTopY = kApTextTopY + kTextFrameHeight + 26;
constexpr int kIpTextCenterY = kIpTextTopY + kTextFrameHeight / 2;
constexpr int kQrLeftX = 72;
constexpr int kQrTopY = kIpTextTopY + kTextFrameHeight + 26;
constexpr int kQrSize = 256;

int centerX() {
  return M5.Display.width() / 2;
}

int logoLeftX() {
  return (M5.Display.width() - kLogoWidth + 1) / 2;
}

void drawLogo(M5Canvas& canvas, int top) {
  if (!LittleFS.begin()) {
    return;
  }
  canvas.drawPngFile(
      LittleFS,
      "/logo.png",
      logoLeftX(),
      top,
      kLogoWidth,
      kLogoHeight,
      0,
      0,
      1.0f,
      1.0f,
      datum_t::top_left);
  LittleFS.end();
}

void loadConfigPortalFont(M5Canvas& canvas) {
  if (!canvas.loadFont(generated::kConfigPortalFontVlw)) {
    canvas.setFont(nullptr);
  }
  canvas.setTextSize(1);
}

void drawQrCode(M5Canvas& canvas, const std::string& text, int left, int top, int size) {
  QRCode qrcode;
  std::vector<std::uint8_t> qrcodeBuffer(qrcode_getBufferSize(3));
  qrcode_initText(&qrcode, qrcodeBuffer.data(), 3, ECC_LOW, text.c_str());

  for (int y = 0; y < qrcode.size; ++y) {
    for (int x = 0; x < qrcode.size; ++x) {
      if (!qrcode_getModule(&qrcode, x, y)) {
        continue;
      }

      const int moduleLeft = left + x * size / qrcode.size;
      const int moduleTop = top + y * size / qrcode.size;
      const int moduleRight = left + (x + 1) * size / qrcode.size;
      const int moduleBottom = top + (y + 1) * size / qrcode.size;
      canvas.fillRect(
          moduleLeft,
          moduleTop,
          moduleRight - moduleLeft,
          moduleBottom - moduleTop,
          TFT_BLACK);
    }
  }
}

}  // namespace

void ConfigPortalRenderer::renderConfigPortal(const std::string& apSsid, const std::string& ipAddress) {
  M5Canvas& canvas = sprite();
  prepareScreen(canvas);

  drawLogo(canvas, kLogoTopY);

  loadConfigPortalFont(canvas);
  canvas.drawString(apSsid.c_str(), centerX(), kApTextCenterY);
  canvas.drawString(ipAddress.c_str(), centerX(), kIpTextCenterY);
  canvas.unloadFont();

  const std::string qrText = std::string("WIFI:T:nopass;S:") + apSsid + ";;";
  drawQrCode(canvas, qrText, kQrLeftX, kQrTopY, kQrSize);
  pushScreen(canvas);
}

}  // namespace homedeck
```

- [ ] **Step 2: Commit**

```bash
git add src/config/config_portal_renderer.cpp
git commit -m "refactor: add ConfigPortalRenderer implementation in config/"
```

---

### Task 3: Update `src/app/app_runtime.cpp`

**Files:**
- Modify: `src/app/app_runtime.cpp`

- [ ] **Step 1: Update include**

Replace `#include "views/home_renderer.h"` with `#include "config/config_portal_renderer.h"`:

```cpp
// Remove this line:
// #include "views/home_renderer.h"
// Add this line:
#include "config/config_portal_renderer.h"
```

- [ ] **Step 2: Update global variable declaration**

Replace `HomeRenderer gHomeRenderer;` with `ConfigPortalRenderer gConfigPortalRenderer;`:

```cpp
// Change this:
// HomeRenderer gHomeRenderer;
// To this:
ConfigPortalRenderer gConfigPortalRenderer;
```

- [ ] **Step 3: Update call site in `makeBootDeps()`**

Replace `gHomeRenderer.renderConfigPortal(...)` with `gConfigPortalRenderer.renderConfigPortal(...)`:

```cpp
// In deps.startConfigPortal lambda, change:
// gHomeRenderer.renderConfigPortal(apSsid, softApIpAddress());
// To:
gConfigPortalRenderer.renderConfigPortal(apSsid, softApIpAddress());
```

- [ ] **Step 4: Commit**

```bash
git add src/app/app_runtime.cpp
git commit -m "refactor: update app_runtime to use ConfigPortalRenderer"
```

---

### Task 4: Delete `views/home_renderer.*`

**Files:**
- Delete: `src/views/home_renderer.h`
- Delete: `src/views/home_renderer.cpp`

- [ ] **Step 1: Delete the files**

```bash
git rm src/views/home_renderer.h src/views/home_renderer.cpp
```

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor: remove HomeRenderer from views/"
```

---

### Task 5: Update `AGENTS.md`

**Files:**
- Modify: `AGENTS.md`

- [ ] **Step 1: Update project map**

Replace the `home_renderer` line with `config_portal_renderer` in the `config/` section description. Since `config_portal_renderer` is now in `config/`, update both the `config/` and `views/` lines:

Current text:
```
- `src/views/home_renderer.cpp/h`：配置门户页面渲染（二维码、AP 信息）。`render()`/`renderCalendar()` 为向后兼容的委托包装，实际实现已分别在 `AlmanacView`/`CalendarView` 中。
- `src/views/`：视图组件。`almanac_view` 为主 UI；`calendar_view`、`countdown_view`、`weather_view` 为其他视图；`view_common` 为共享渲染工具（状态栏、时间格式化等）。
```

Replace with:
```
- `src/config/config_portal_renderer.cpp/h`：配置门户页面渲染器（二维码、AP 信息、Logo）。与 `ConfigPortal` 同属配置子系统。
- `src/views/`：视图组件。`almanac_view` 为主 UI；`calendar_view`、`countdown_view`、`weather_view` 为其他视图；`view_common` 为共享渲染工具（状态栏、时间格式化等）。
```

- [ ] **Step 2: Commit**

```bash
git add AGENTS.md
git commit -m "docs: update AGENTS.md project map for ConfigPortalRenderer migration"
```

---

### Task 6: Verify Build and Tests

**Files:**
- None (verification only)

- [ ] **Step 1: Run embedded build**

```bash
pio run -e m5stack-papercolor
```

Expected: SUCCESS, RAM and Flash usage unchanged (or slightly reduced due to deleted dead code).

- [ ] **Step 2: Run native tests**

```bash
pio test -e native
```

Expected: All tests pass (no tests reference `HomeRenderer`, so this is purely a compilation check).

- [ ] **Step 3: Commit verification result (if all pass)**

If both pass, no additional commit needed — the previous commits already contain the changes.

---

## Self-Review

**1. Spec coverage:**
- ✅ Create `config/config_portal_renderer.h` — Task 1
- ✅ Create `config/config_portal_renderer.cpp` — Task 2
- ✅ Update `app_runtime.cpp` references — Task 3
- ✅ Delete `views/home_renderer.*` — Task 4
- ✅ Update `AGENTS.md` — Task 5
- ✅ Verify build and tests — Task 6

**2. Placeholder scan:**
- ✅ No "TBD", "TODO", "implement later"
- ✅ All code blocks contain complete code
- ✅ All file paths are exact
- ✅ All commands have expected output

**3. Type consistency:**
- ✅ `ConfigPortalRenderer` used consistently in header, cpp, and app_runtime
- ✅ `renderConfigPortal` signature unchanged: `(const std::string&, const std::string&)`
- ✅ Namespace `homedeck` used consistently

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-06-03-migrate-home-renderer-to-config.md`.**

Two execution options:

**1. Subagent-Driven (recommended)** — Dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** — Execute tasks in this session, batch execution with checkpoints

Which approach?