# AP 配置屏幕实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 AP 配置模式下的新屏幕布局，包含 logo、AP 名字、IP 地址和 WiFi 二维码，所有元素水平居中。

**Architecture:** 直接屏幕绘制方案。使用 M5GFX API 在 E-ink 屏幕上按精确坐标绘制各个元素。新增 20px MiSans-Bold VLW 字体，通过 LittleFS 加载 logo PNG，使用 QRCode 库生成并绘制 WiFi 格式二维码。

**Tech Stack:** Arduino, M5Unified, M5GFX, QRCode, LittleFS, PlatformIO, Unity (测试)

---

## 文件结构

| 文件 | 操作 | 说明 |
|------|------|------|
| `tools/generate_device_font.py` | 修改 | 添加第四个 FontResource（MiSans-Bold 20px） |
| `src/generated/device_font_vlw.h` | 重新生成 | 包含 `kConfigPortalFontVlwSize` 等声明 |
| `src/generated/device_font_vlw.cpp` | 重新生成 | 包含 `kConfigPortalFontVlw` PROGMEM 数组 |
| `platformio.ini` | 修改 | 添加 `board_build.filesystem = littlefs` |
| `data/logo.png` | 创建（复制） | 从 `assets/images/logo.png` 复制 |
| `src/home_renderer.h` | 修改 | 添加 `#include` 和字体头文件引用 |
| `src/home_renderer.cpp` | 修改 | 重写 `renderConfigPortal` 方法 |
| `test/native/support/fake_arduino/M5Unified.h` | 修改 | 扩展 `FakeDisplay` 支持 `drawPngFile`、`loadFont` 新字体识别 |
| `test/native/support/fake_arduino/qrcode.h` | 修改 | 扩展 `qrcode_initText` 记录调用参数 |
| `test/native/test_home_renderer/test_main.cpp` | 修改 | 更新测试断言匹配新布局 |

---

## Task 1: 添加配置屏幕字体生成

**Files:**
- Modify: `tools/generate_device_font.py`

- [ ] **Step 1: 在 `generate_device_font.py` 中添加配置屏幕字体资源**

在文件第 12 行附近（METRIC_FONT 下方）添加：

```python
CONFIG_FONT = ROOT / "fonts" / "misans" / "MiSans-Bold.ttf"
CONFIG_PIXEL_SIZE = 20
```

- [ ] **Step 2: 在 `main()` 函数中添加配置字体资源**

在 `main()` 函数中，将 `CONFIG_FONT` 加入存在性检查：

```python
    for font_path in (BODY_FONT, METRIC_FONT, TIME_FONT, CONFIG_FONT):
        if not font_path.exists():
            raise SystemExit(f"source font missing: {font_path}")
```

在 `resources` 列表末尾添加第四个 `FontResource`：

```python
        FontResource(
            "config_portal_font",
            "kConfigPortal",
            CONFIG_PIXEL_SIZE,
            sorted(numeric_codepoints),
            CONFIG_FONT,
        ),
```

- [ ] **Step 3: 提交字体生成工具修改**

```bash
git add tools/generate_device_font.py
git commit -m "feat: 添加配置屏幕字体生成

- 新增 MiSans-Bold 20px 字体用于 AP 配置屏幕
- 复用 NUMERIC_TEXT 字符集（AP 名字和 IP 均为 ASCII）"
```

---

## Task 2: 重新生成字体文件

**Files:**
- Regenerate: `src/generated/device_font_vlw.h`
- Regenerate: `src/generated/device_font_vlw.cpp`

- [ ] **Step 1: 运行字体生成脚本**

```bash
python3 tools/generate_device_font.py
```

Expected: 脚本成功执行，生成 `src/generated/device_font_vlw.h` 和 `.cpp`，输出包含 `config_portal_font` 的信息。

- [ ] **Step 2: 验证生成的头文件**

检查 `src/generated/device_font_vlw.h` 中是否包含：

```cpp
extern const std::uint8_t kConfigPortalFontVlw[];
inline constexpr std::size_t kConfigPortalFontVlwSize = ...U;
inline constexpr std::uint32_t kConfigPortalFontGlyphCount = ...U;
inline constexpr std::uint32_t kConfigPortalFontPixelSize = 20U;
```

- [ ] **Step 3: 提交生成的字体文件**

```bash
git add src/generated/
git commit -m "feat: 生成配置屏幕 VLW 字体

- 新增 kConfigPortalFontVlw（MiSans-Bold 20px）
- 包含 ASCII 数字和符号字符"
```

---

## Task 3: 添加 LittleFS 支持

**Files:**
- Modify: `platformio.ini`
- Create: `data/logo.png`（复制）

- [ ] **Step 1: 在 `platformio.ini` 中添加 LittleFS 配置**

在 `[env:m5stack-papercolor]` 段末尾添加：

```ini
board_build.filesystem = littlefs
```

- [ ] **Step 2: 创建 data 目录并复制 logo**

```bash
mkdir -p data
cp assets/images/logo.png data/logo.png
```

- [ ] **Step 3: 验证 data 目录结构**

```bash
ls -la data/
```

Expected: 显示 `logo.png` 文件，大小约 3137 字节。

- [ ] **Step 4: 提交 LittleFS 配置**

```bash
git add platformio.ini data/logo.png
git commit -m "chore: 添加 LittleFS 文件系统支持

- platformio.ini 启用 littlefs 文件系统
- data/logo.png 用于配置屏幕显示"
```

---

## Task 4: 实现新的 renderConfigPortal

**Files:**
- Modify: `src/home_renderer.h`
- Modify: `src/home_renderer.cpp`

- [ ] **Step 1: 在 `home_renderer.h` 中添加必要的 include**

```cpp
#pragma once

#include <cstdint>
#include <string>

// Forward declaration for M5GFX
class M5GFX;

namespace homedeck {

class HomeRenderer {
 public:
  void render();
  void renderConfigPortal(const std::string& apSsid, const std::string& ipAddress);
  
 private:
  void drawQrCode(const std::string& text, int centerX, int centerY, int size);
};

}  // namespace homedeck
```

- [ ] **Step 2: 实现 `home_renderer.cpp` 中的 `renderConfigPortal`**

```cpp
#include "home_renderer.h"

#include <M5Unified.h>
#include <LittleFS.h>
#include <qrcode.h>

#include <cstdio>
#include <string>

#include "generated/device_font_vlw.h"

namespace homedeck {

namespace {

constexpr int kLogoTopY = 86;
constexpr int kLogoHeight = 40;
constexpr int kSpacing = 26;
constexpr int kQrCodeSize = 256;
constexpr int kFontSize = 20;

}  // namespace

void HomeRenderer::render() {
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(textdatum_t::middle_center);
  M5.Display.drawString("HomeDeck", M5.Display.width() / 2, M5.Display.height() / 2);
}

void HomeRenderer::renderConfigPortal(const std::string& apSsid, const std::string& ipAddress) {
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(textdatum_t::middle_center);

  const int centerX = M5.Display.width() / 2;
  
  // Load custom font
  M5.Display.loadFont(homedeck::generated::kConfigPortalFontVlw);

  // Calculate Y positions
  const int logoCenterY = kLogoTopY + kLogoHeight / 2;
  const int apCenterY = logoCenterY + kLogoHeight / 2 + kSpacing + kFontSize / 2;
  const int ipCenterY = apCenterY + kFontSize / 2 + kSpacing + kFontSize / 2;
  const int qrCenterY = ipCenterY + kFontSize / 2 + kSpacing + kQrCodeSize / 2;

  // 1. Draw logo
  if (LittleFS.begin()) {
    M5.Display.drawPngFile(LittleFS, "/logo.png", centerX, logoCenterY, 0, 0, 0, 0, 1.0f, 1.0f, datum_t::middle_center);
    LittleFS.end();
  }

  // 2. Draw AP name
  M5.Display.drawString(apSsid.c_str(), centerX, apCenterY);

  // 3. Draw IP address
  M5.Display.drawString(ipAddress.c_str(), centerX, ipCenterY);

  // 4. Draw QR code
  const std::string qrText = std::string("WIFI:T:nopass;S:") + apSsid + ";;";
  drawQrCode(qrText, centerX, qrCenterY, kQrCodeSize);

  // Restore default font
  M5.Display.unloadFont();
}

void HomeRenderer::drawQrCode(const std::string& text, int centerX, int centerY, int size) {
  QRCode qrcode;
  const int bufferSize = qrcode_getBufferSize(3);
  std::vector<std::uint8_t> qrcodeBuffer(bufferSize);
  
  qrcode_initText(&qrcode, qrcodeBuffer.data(), 3, ECC_LOW, text.c_str());
  
  const int moduleSize = size / qrcode.size;
  const int totalSize = moduleSize * qrcode.size;
  const int startX = centerX - totalSize / 2;
  const int startY = centerY - totalSize / 2;
  
  for (int y = 0; y < qrcode.size; ++y) {
    for (int x = 0; x < qrcode.size; ++x) {
      if (qrcode_getModule(&qrcode, x, y)) {
        M5.Display.fillRect(
            startX + x * moduleSize,
            startY + y * moduleSize,
            moduleSize,
            moduleSize,
            TFT_BLACK);
      }
    }
  }
}

}  // namespace homedeck
```

- [ ] **Step 3: 提交 home_renderer 修改**

```bash
git add src/home_renderer.h src/home_renderer.cpp
git commit -m "feat: 实现新的 AP 配置屏幕布局

- 使用 MiSans-Bold 20px 字体显示 AP 名字和 IP
- 通过 LittleFS 加载并居中显示 logo.png
- 生成 WiFi 格式二维码（WIFI:T:nopass;S:...）
- 所有元素严格水平居中，间距 26px"
```

---

## Task 5: 更新测试 Fake

**Files:**
- Modify: `test/native/support/fake_arduino/M5Unified.h`
- Modify: `test/native/support/fake_arduino/qrcode.h`

- [ ] **Step 1: 在 `FakeDisplay` 中添加 `drawPngFile` 支持**

在 `FakeDisplay` struct 中添加：

```cpp
struct FakePngDraw {
  std::string path;
  int x = 0;
  int y = 0;
  int datum = 0;
};

std::vector<FakePngDraw> pngDraws;

void drawPngFile(auto& fs, const char* path, int x, int y, int, int, int, int, float, float, int datum) {
  pngDraws.push_back({path != nullptr ? path : "", x, y, datum});
}
```

同时更新 `FakeCanvas`，添加相同的 `drawPngFile` 方法（delegate 到 parent）。

- [ ] **Step 2: 在 `FakeDisplay` 中扩展 `loadFont` 以识别新字体**

修改 `loadFont` 方法：

```cpp
  bool loadFont(const std::uint8_t* font) {
    if (!loadFontSucceeds) {
      fontKind = FakeFontKind::kDefault;
      return false;
    }
    if (font == nullptr) {
      fontKind = FakeFontKind::kDefault;
    } else if (font[0] == 28) {
      fontKind = FakeFontKind::kDeviceMetric;
    } else if (font[0] == 42) {
      fontKind = FakeFontKind::kDeviceTime;
    } else if (font[0] == 20) {
      fontKind = FakeFontKind::kConfigPortal;
    } else {
      fontKind = FakeFontKind::kDeviceDefault;
    }
    return true;
  }
```

并在 `FakeFontKind` enum 中添加 `kConfigPortal = 5`。

- [ ] **Step 3: 在 `FakeDisplay` 中添加 `unloadFont` 方法**

```cpp
  void unloadFont() {
    fontKind = FakeFontKind::kDefault;
  }
```

- [ ] **Step 4: 更新 `qrcode.h` fake 以记录文本内容**

```cpp
#pragma once

#include <cstdint>
#include <string>

constexpr int ECC_LOW = 0;

struct QRCode {
  int size = 21;
};

inline std::string lastQrCodeText;

inline int qrcode_getBufferSize(int) {
  return 64;
}

inline void qrcode_initText(QRCode* qrcode, std::uint8_t*, int, int, const char* text) {
  if (text != nullptr) {
    lastQrCodeText = text;
  }
  if (qrcode != nullptr) {
    qrcode->size = 21;
  }
}

inline bool qrcode_getModule(const QRCode*, int x, int y) {
  return ((x + y) % 3) == 0;
}
```

- [ ] **Step 5: 提交 fake 更新**

```bash
git add test/native/support/fake_arduino/M5Unified.h test/native/support/fake_arduino/qrcode.h
git commit -m "test: 扩展 fake 支持配置屏幕测试

- FakeDisplay 添加 drawPngFile 和 unloadFont 方法
- FakeFontKind 添加 kConfigPortal（20px 字体）
- qrcode fake 记录 lastQrCodeText 用于断言验证"
```

---

## Task 6: 更新测试

**Files:**
- Modify: `test/native/test_home_renderer/test_main.cpp`

- [ ] **Step 1: 重写测试以验证新布局**

```cpp
#include <unity.h>

#include <M5Unified.h>

#include "home_renderer.h"

void setUp() {
  M5 = FakeM5Global{};
}

void tearDown() {
}

void test_home_renderer_draws_centered_home_deck_in_portrait() {
  homedeck::HomeRenderer renderer;

  renderer.render();

  TEST_ASSERT_EQUAL(0, M5.Display.rotation);
  TEST_ASSERT_EQUAL(TFT_WHITE, M5.Display.fillScreenColor);
  TEST_ASSERT_EQUAL(
      static_cast<int>(textdatum_t::middle_center),
      static_cast<int>(M5.Display.textDatum));
  TEST_ASSERT_EQUAL(1, static_cast<int>(M5.Display.prints.size()));
  TEST_ASSERT_EQUAL_STRING("HomeDeck", M5.Display.prints[0].text.c_str());
  TEST_ASSERT_EQUAL(M5.Display.width() / 2, M5.Display.prints[0].x);
  TEST_ASSERT_EQUAL(M5.Display.height() / 2, M5.Display.prints[0].y);
}

void test_home_renderer_draws_config_portal_with_logo_ap_ip_qr() {
  homedeck::HomeRenderer renderer;

  renderer.renderConfigPortal("HomeDeck-ABCD", "192.168.4.1");

  TEST_ASSERT_EQUAL(0, M5.Display.rotation);
  TEST_ASSERT_EQUAL(TFT_WHITE, M5.Display.fillScreenColor);
  TEST_ASSERT_EQUAL(
      static_cast<int>(textdatum_t::middle_center),
      static_cast<int>(M5.Display.textDatum));
  
  // Verify logo was drawn
  TEST_ASSERT_EQUAL(1, static_cast<int>(M5.Display.pngDraws.size()));
  TEST_ASSERT_EQUAL_STRING("/logo.png", M5.Display.pngDraws[0].path.c_str());
  TEST_ASSERT_EQUAL(M5.Display.width() / 2, M5.Display.pngDraws[0].x);
  
  // Verify font was loaded (kConfigPortal)
  TEST_ASSERT_EQUAL(static_cast<int>(FakeFontKind::kConfigPortal), static_cast<int>(M5.Display.fontKind));
  
  // Verify AP name and IP are printed with correct font
  TEST_ASSERT_EQUAL(2, static_cast<int>(M5.Display.prints.size()));
  TEST_ASSERT_EQUAL_STRING("HomeDeck-ABCD", M5.Display.prints[0].text.c_str());
  TEST_ASSERT_EQUAL(M5.Display.width() / 2, M5.Display.prints[0].x);
  TEST_ASSERT_EQUAL_STRING("192.168.4.1", M5.Display.prints[1].text.c_str());
  TEST_ASSERT_EQUAL(M5.Display.width() / 2, M5.Display.prints[1].x);
  
  // Verify QR code text
  TEST_ASSERT_EQUAL_STRING("WIFI:T:nopass;S:HomeDeck-ABCD;;", lastQrCodeText.c_str());
  
  // Verify QR code modules were drawn (some rects should exist)
  TEST_ASSERT_GREATER_THAN(0, static_cast<int>(M5.Display.rects.size()));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_home_renderer_draws_centered_home_deck_in_portrait);
  RUN_TEST(test_home_renderer_draws_config_portal_with_logo_ap_ip_qr);
  return UNITY_END();
}
```

- [ ] **Step 2: 运行测试**

```bash
pio test -e native
```

Expected: 所有测试通过。

- [ ] **Step 3: 提交测试更新**

```bash
git add test/native/test_home_renderer/test_main.cpp
git commit -m "test: 更新 home_renderer 测试匹配新布局

- 验证 logo.png 绘制、AP/IP 文本、二维码生成
- 断言字体加载为 kConfigPortal
- 验证 WiFi 二维码内容格式"
```

---

## Task 7: 构建验证

**Files:**
- Build: `src/` 所有文件

- [ ] **Step 1: 运行 native 测试**

```bash
pio test -e native
```

Expected: 所有测试通过，无编译错误。

- [ ] **Step 2: 构建目标固件**

```bash
pio run -e m5stack-papercolor
```

Expected: 编译成功，生成 `.pio/build/m5stack-papercolor/firmware.bin`。

- [ ] **Step 3: 验证 LittleFS 镜像构建**

检查 PlatformIO 是否自动构建 LittleFS 镜像：

```bash
ls -la .pio/build/m5stack-papercolor/littlefs.bin 2>/dev/null || echo "LittleFS image not found, may need manual upload"
```

如果未自动生成，运行：

```bash
pio run -t buildfs -e m5stack-papercolor
```

- [ ] **Step 4: 提交最终验证**

```bash
git log --oneline -5
```

Expected: 显示所有提交记录。

---

## 自检

### Spec 覆盖检查

| Spec 要求 | 对应任务 |
|-----------|----------|
| Logo 水平居中，顶部 86px | Task 4 |
| AP 名字 MiSans-Bold 20px，水平居中，距 Logo 26px | Task 1, Task 4 |
| IP 地址 MiSans-Bold 20px，水平居中，距 AP 26px | Task 1, Task 4 |
| 二维码 256×256，WiFi 格式，水平居中，距 IP 26px | Task 4 |
| 使用 LittleFS 加载 logo | Task 3 |
| 使用 MiSans-Bold.ttf 生成 VLW | Task 1, Task 2 |
| 参考 main 分支实现 | Task 4（遵循现有代码风格） |
| PlatformIO 方式放置 logo | Task 3 |

### 占位符检查

- 无 TBD/TODO
- 无 "add appropriate error handling"
- 所有步骤包含实际代码
- 无 "Similar to Task N"

### 类型一致性

- `kConfigPortalFontVlw` 在所有任务中命名一致
- `FakeFontKind::kConfigPortal` 在 Task 5 和 Task 6 中一致
- `drawPngFile` 签名在 fake 和真实代码中一致

---

## 执行交接

Plan complete and saved to `docs/superpowers/plans/2026-05-24-ap-config-screen-plan.md`. Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?
