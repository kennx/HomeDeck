# Home Almanac Sleep Environment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add one-shot SHT40 temperature/humidity rendering to the almanac home screen and put the configured home screen into deep sleep after 60 seconds, waking at local midnight or Button C.

**Architecture:** Keep the approved方案 A boundaries: `HomeRenderer` only consumes `HomeCalendarData`, the SHT40 protocol reader is a small hardware helper, `BootController` owns the 60-second system-mode sleep state, and `app_runtime` wires hardware adapters to those seams. Tests drive each behavior through native Unity suites and fakes before production code is changed.

**Tech Stack:** PlatformIO, Arduino ESP32-S3, M5Unified/M5GFX, ESP-IDF sleep APIs through Arduino, Unity native tests, LittleFS almanac data.

---

## Scope And File Map

This plan implements the approved spec at `docs/superpowers/specs/2026-05-25-home-almanac-sleep-environment-design.md`.

Files to create:

- `src/sht40_reader.h`: SHT40 reading result and public `readSht40Environment()` API.
- `src/sht40_reader.cpp`: SHT40 high-precision I2C command, CRC validation, conversion, and humidity clamp.
- `test/native/test_sht40_reader/test_main.cpp`: native tests for successful reading, I2C failure, CRC failure, command address, and humidity clamping.

Files to modify:

- `src/home_renderer.h`: add optional temperature/humidity fields and expose `makeCurrentHomeCalendarData()`.
- `src/home_renderer.cpp`: cap `宜/忌` wrapping to two lines, draw bottom temperature/humidity, and let `render()` reuse `makeCurrentHomeCalendarData()`.
- `test/native/test_home_renderer/test_main.cpp`: add environment display and two-line truncation assertions; update layout expectations changed by capped rows.
- `src/boot_controller.h`: add `HomeSleepRequest`, sleep deps, and private state for the 60-second display window.
- `src/boot_controller.cpp`: compute sleep duration to next local midnight, fall back to one hour when time is not trusted, and prioritize A+B restart over sleep.
- `test/native/test_boot_controller/test_main.cpp`: cover configured sleep timing, midnight duration, one-hour fallback, config-mode exclusion, and A+B priority.
- `src/app_runtime.cpp`: read SHT40 once before home render; configure Button C GPIO wakeup, timer wakeup, display sleep, and deep sleep start.
- `test/native/support/fake_arduino/M5Unified.h`: add `BtnC`, display sleep recording, and richer `FakeI2C` observations.
- `test/native/support/fake_arduino/Arduino.h`: add `INPUT_PULLUP`, `pinMode()` fake state, and reset hooks.
- `test/native/support/fake_arduino/esp_sleep.h`: add deep-sleep GPIO wakeup mode and fake observations.

Constants locked by the spec:

- Home display window: `60000` ms.
- Button C wake pin: GPIO `1`.
- Button C wake level: low.
- Button C pin mode: `INPUT_PULLUP`.
- Fallback timer sleep when time is not trusted: `3600` seconds.
- Trusted modern Unix threshold: `1704067200` (`2024-01-01T00:00:00Z`), matching the existing NTP freshness check.
- Bottom text y top coordinate: `574` for the current 14px device font, giving `600 - 12 - 14`.

---

### Task 1: Renderer Data, Bottom Environment Text, And Two-Line Yi/Ji

**Files:**
- Modify: `src/home_renderer.h`
- Modify: `src/home_renderer.cpp`
- Modify: `test/native/test_home_renderer/test_main.cpp`

- [ ] **Step 1: Add failing renderer tests for environment text**

Add these constants near the other renderer test constants in `test/native/test_home_renderer/test_main.cpp`:

```cpp
constexpr int kEnvironmentTextTopY = 574;
constexpr std::uint32_t kWeekdayColor = 0x0449;
```

Add this test before `test_home_renderer_draws_config_portal_layout()`:

```cpp
void test_home_renderer_draws_environment_readings_at_bottom_edges() {
  auto data = figmaCalendarData();
  data.temperatureAvailable = true;
  data.temperatureCelsius = 30.04f;
  data.humidityAvailable = true;
  data.humidityPercent = 49.96f;
  homedeck::HomeRenderer renderer;

  renderer.render(data);

  bool foundTemperature = false;
  bool foundHumidity = false;
  for (const auto& print : M5.Display.prints) {
    if (print.text == "30.0°C") {
      TEST_ASSERT_EQUAL(12, print.x);
      TEST_ASSERT_EQUAL(kEnvironmentTextTopY, print.y);
      TEST_ASSERT_EQUAL_UINT32(kWeekdayColor, print.color);
      TEST_ASSERT_EQUAL(static_cast<int>(FakeFontKind::kDeviceDefault), static_cast<int>(print.fontKind));
      foundTemperature = true;
    }
    if (print.text == "50.0%") {
      TEST_ASSERT_EQUAL(388, print.x);
      TEST_ASSERT_EQUAL(kEnvironmentTextTopY, print.y);
      TEST_ASSERT_EQUAL_UINT32(kWeekdayColor, print.color);
      TEST_ASSERT_EQUAL(static_cast<int>(FakeFontKind::kDeviceDefault), static_cast<int>(print.fontKind));
      foundHumidity = true;
    }
  }

  TEST_ASSERT_TRUE(foundTemperature);
  TEST_ASSERT_TRUE(foundHumidity);
}
```

Add this test next:

```cpp
void test_home_renderer_draws_environment_placeholders_when_unavailable() {
  auto data = figmaCalendarData();
  homedeck::HomeRenderer renderer;

  renderer.render(data);

  bool foundTemperature = false;
  bool foundHumidity = false;
  for (const auto& print : M5.Display.prints) {
    if (print.text == "--.-°C") {
      TEST_ASSERT_EQUAL(12, print.x);
      TEST_ASSERT_EQUAL(kEnvironmentTextTopY, print.y);
      foundTemperature = true;
    }
    if (print.text == "--.-%") {
      TEST_ASSERT_EQUAL(388, print.x);
      TEST_ASSERT_EQUAL(kEnvironmentTextTopY, print.y);
      foundHumidity = true;
    }
  }

  TEST_ASSERT_TRUE(foundTemperature);
  TEST_ASSERT_TRUE(foundHumidity);
}
```

Register both tests in `main()`:

```cpp
RUN_TEST(test_home_renderer_draws_environment_readings_at_bottom_edges);
RUN_TEST(test_home_renderer_draws_environment_placeholders_when_unavailable);
```

- [ ] **Step 2: Run renderer tests and verify environment tests fail**

Run:

```bash
pio test -e native --filter native/test_home_renderer
```

Expected: FAIL because `HomeCalendarData` has no `temperatureAvailable`, `temperatureCelsius`, `humidityAvailable`, or `humidityPercent` fields yet.

- [ ] **Step 3: Add environment fields and current-data helper**

Modify `src/home_renderer.h`:

```cpp
struct HomeCalendarData {
  std::string year;
  std::string month;
  std::string day;
  std::string weekday;
  bool isHoliday = false;
  std::string lunarDate;
  std::string solarTerm;
  std::string ganzhi;
  std::string wuxing;
  std::string chongsha;
  std::string zhishen;
  std::string jianchu;
  std::string taishen;
  std::string yi;
  std::string ji;
  bool temperatureAvailable = false;
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;
  float humidityPercent = 0.0f;
};

HomeCalendarData makeHomeCalendarData(const std::tm& localTime);
HomeCalendarData makeCurrentHomeCalendarData();
```

Modify `HomeRenderer::render()` in `src/home_renderer.cpp`:

```cpp
HomeCalendarData makeCurrentHomeCalendarData() {
  const std::time_t now = std::time(nullptr);
  const std::tm* local = now > 0 ? std::localtime(&now) : nullptr;
  return makeHomeCalendarData(local != nullptr ? *local : fallbackLocalTime());
}

void HomeRenderer::render() {
  render(makeCurrentHomeCalendarData());
}
```

- [ ] **Step 4: Implement bottom environment drawing**

Add constants near the other calendar constants in `src/home_renderer.cpp`:

```cpp
constexpr int kEnvironmentTextTopY = 574;
constexpr int kEnvironmentTextLeftX = 12;
constexpr int kEnvironmentTextRightX = 388;
```

Add helpers inside the anonymous namespace:

```cpp
std::string formatTemperatureText(const HomeCalendarData& data) {
  if (!data.temperatureAvailable) {
    return "--.-°C";
  }
  char buffer[16] = {};
  std::snprintf(buffer, sizeof(buffer), "%.1f°C", data.temperatureCelsius);
  return buffer;
}

std::string formatHumidityText(const HomeCalendarData& data) {
  if (!data.humidityAvailable) {
    return "--.-%";
  }
  char buffer[16] = {};
  std::snprintf(buffer, sizeof(buffer), "%.1f%%", data.humidityPercent);
  return buffer;
}

void drawEnvironmentReadings(M5Canvas& canvas, const HomeCalendarData& data) {
  canvas.setTextDatum(textdatum_t::top_left);
  const std::string temperature = formatTemperatureText(data);
  canvas.drawString(temperature.c_str(), kEnvironmentTextLeftX, kEnvironmentTextTopY);

  canvas.setTextDatum(textdatum_t::top_right);
  const std::string humidity = formatHumidityText(data);
  canvas.drawString(humidity.c_str(), kEnvironmentTextRightX, kEnvironmentTextTopY);
}
```

Call it before `canvas.unloadFont()` in the `generated::kDeviceFontVlw` block:

```cpp
    drawEnvironmentReadings(canvas, data);

    canvas.unloadFont();
```

- [ ] **Step 5: Run renderer tests and verify environment tests pass**

Run:

```bash
pio test -e native --filter native/test_home_renderer
```

Expected: PASS for the new environment tests. Existing row-height tests may still fail until the two-line cap is implemented in later steps of this task.

- [ ] **Step 6: Add failing tests for two-line Yi/Ji truncation**

Add this helper inside the test namespace in `test/native/test_home_renderer/test_main.cpp`:

```cpp
int countPrintedGlyphsInRow(int y) {
  int count = 0;
  for (const auto& print : M5.Display.prints) {
    if (print.y == y && print.text.size() > 0 && print.text != "宜" && print.text != "忌") {
      ++count;
    }
  }
  return count;
}
```

Add this test before config portal tests:

```cpp
void test_home_renderer_limits_yi_and_ji_to_two_lines() {
  auto data = figmaCalendarData();
  data.yi = "甲乙丙丁戊己庚辛壬癸子丑寅卯辰巳午未申酉戌亥甲乙丙丁戊己庚辛壬癸子丑寅卯辰巳午未申酉戌亥";
  data.ji = data.yi;
  homedeck::HomeRenderer renderer;

  renderer.render(data);

  TEST_ASSERT_GREATER_THAN(0, countPrintedGlyphsInRow(397));
  TEST_ASSERT_GREATER_THAN(0, countPrintedGlyphsInRow(424));
  TEST_ASSERT_EQUAL(0, countPrintedGlyphsInRow(451));

  bool foundJiLabelAtCappedRow = false;
  for (const auto& print : M5.Display.prints) {
    if (print.text == "忌" && print.y == 471) {
      foundJiLabelAtCappedRow = true;
    }
    TEST_ASSERT_NOT_EQUAL(525, print.y);
    TEST_ASSERT_NOT_EQUAL(552, print.y);
  }
  TEST_ASSERT_TRUE(foundJiLabelAtCappedRow);
}
```

Register it:

```cpp
RUN_TEST(test_home_renderer_limits_yi_and_ji_to_two_lines);
```

- [ ] **Step 7: Run renderer tests and verify truncation test fails**

Run:

```bash
pio test -e native --filter native/test_home_renderer
```

Expected: FAIL because current `drawWrappedText()` keeps drawing every wrapped line and `dynamicActionRowHeight()` uses the uncapped line count.

- [ ] **Step 8: Cap action rows and wrapped drawing to two lines**

Add this constant in `src/home_renderer.cpp`:

```cpp
constexpr int kMaxActionLines = 2;
```

Replace `drawWrappedText()` with:

```cpp
void drawWrappedText(
    M5Canvas& canvas,
    const std::string& text,
    int startX,
    int startY,
    int maxWidth,
    int lineHeight,
    int maxLines) {
  int currentX = startX;
  int currentY = startY;
  int currentLine = 1;

  auto moveToNextLine = [&]() {
    if (currentLine >= maxLines) {
      return false;
    }
    ++currentLine;
    currentX = startX;
    currentY += lineHeight;
    return true;
  };

  for (std::size_t index = 0; index < text.size();) {
    std::size_t length = utf8CodePointLength(static_cast<unsigned char>(text[index]));
    if (index + length > text.size()) {
      length = 1;
    }

    std::string glyph = text.substr(index, length);
    index += length;

    if (glyph == "\n") {
      if (!moveToNextLine()) {
        return;
      }
      continue;
    }
    if (glyph == " " && currentX == startX) {
      continue;
    }

    const int glyphWidth = canvas.textWidth(glyph.c_str());
    if (currentX > startX && currentX + glyphWidth > startX + maxWidth) {
      if (!moveToNextLine()) {
        return;
      }
      if (glyph == " ") {
        continue;
      }
    }

    canvas.drawString(glyph.c_str(), currentX, currentY);
    currentX += glyphWidth;
  }
}
```

Replace `dynamicActionRowHeight()` with:

```cpp
int dynamicActionRowHeight(M5Canvas& canvas, const std::string& text) {
  int lineCount = wrappedLineCount(canvas, text, kTableContentWidth);
  if (lineCount > kMaxActionLines) {
    lineCount = kMaxActionLines;
  }
  const int contentHeight = lineCount * kTableLineHeight + kTableRowPaddingY * 2;
  return contentHeight > kTableFixedRowHeight ? contentHeight : kTableFixedRowHeight;
}
```

Update the two call sites:

```cpp
drawWrappedText(canvas, data.yi, kTableContentLeftX, kTableYiTextY, kTableContentWidth, kTableLineHeight, kMaxActionLines);
drawWrappedText(canvas, data.ji, kTableContentLeftX, jiTextY, kTableContentWidth, kTableLineHeight, kMaxActionLines);
```

- [ ] **Step 9: Update changed table-height expectations**

In `test_home_renderer_draws_lunar_calendar_portrait()`, adjust the table border and dynamic divider expectations to match capped rows:

```cpp
if (rect.x == 12 && rect.y == 293 && rect.w == 376 && rect.h == 215) {
  foundTableBorder = true;
} else if (rect.x == 12 && rect.w == 376 && rect.h == 1) {
  if (rect.y == 340 || rect.y == 387 || rect.y == 461) {
    internalLineCount++;
  }
}
```

Keep `test_home_renderer_shrinks_yi_ji_rows_when_content_is_single_line()` unchanged; single-line content should still use the compact 47px row height.

- [ ] **Step 10: Run renderer tests**

Run:

```bash
pio test -e native --filter native/test_home_renderer
```

Expected: PASS.

- [ ] **Step 11: Commit renderer changes**

Run:

```bash
git add src/home_renderer.h src/home_renderer.cpp test/native/test_home_renderer/test_main.cpp
git commit -m "feat: 显示首页温湿度并限制宜忌行数" \
  -m "- 为 HomeCalendarData 增加温湿度可用状态和读数字段" \
  -m "- 首页底部按 12px 边距显示温度、湿度和不可用占位" \
  -m "- 将宜忌内容限制为最多两行，防止挤压底部读数"
```

Expected: commit succeeds.

---

### Task 2: SHT40 Reader And I2C Fakes

**Files:**
- Create: `src/sht40_reader.h`
- Create: `src/sht40_reader.cpp`
- Create: `test/native/test_sht40_reader/test_main.cpp`
- Modify: `test/native/support/fake_arduino/M5Unified.h`

- [ ] **Step 1: Add richer FakeI2C observations**

Modify `FakeI2C` in `test/native/support/fake_arduino/M5Unified.h` by adding fields:

```cpp
std::uint8_t lastAddress = 0;
bool lastReadMode = false;
std::uint32_t lastFrequency = 0;
int startCalls = 0;
int stopCalls = 0;
std::vector<std::uint8_t> writtenBytes;
```

Replace `start()`, `write()`, and `stop()` with:

```cpp
bool start(std::uint8_t address, bool read, std::uint32_t frequency) {
  ++startCalls;
  lastAddress = address;
  lastReadMode = read;
  lastFrequency = frequency;
  return enabled;
}

bool write(std::uint8_t value) {
  if (!enabled) {
    return false;
  }
  writtenBytes.push_back(value);
  return true;
}

bool stop() {
  ++stopCalls;
  return enabled;
}
```

- [ ] **Step 2: Write failing SHT40 reader tests**

Create `test/native/test_sht40_reader/test_main.cpp`:

```cpp
#include <unity.h>

#include <M5Unified.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "sht40_reader.h"

namespace {

std::uint8_t sht40Crc(std::uint8_t msb, std::uint8_t lsb) {
  std::uint8_t crc = 0xFF;
  const std::uint8_t bytes[] = {msb, lsb};
  for (const std::uint8_t byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x31) : static_cast<std::uint8_t>(crc << 1);
    }
  }
  return crc;
}

std::array<std::uint8_t, 6> makeSht40Frame(std::uint16_t tempRaw, std::uint16_t humidityRaw) {
  const auto tMsb = static_cast<std::uint8_t>(tempRaw >> 8);
  const auto tLsb = static_cast<std::uint8_t>(tempRaw & 0xFF);
  const auto hMsb = static_cast<std::uint8_t>(humidityRaw >> 8);
  const auto hLsb = static_cast<std::uint8_t>(humidityRaw & 0xFF);
  return {tMsb, tLsb, sht40Crc(tMsb, tLsb), hMsb, hLsb, sht40Crc(hMsb, hLsb)};
}

void loadFrame(std::uint16_t tempRaw, std::uint16_t humidityRaw) {
  const auto frame = makeSht40Frame(tempRaw, humidityRaw);
  std::copy(frame.begin(), frame.end(), M5.In_I2C.nextReadBuffer.begin());
}

}  // namespace

void setUp() {
  M5 = FakeM5Global{};
  fakeArduinoResetClock();
}

void tearDown() {
}

void test_sht40_reader_reads_temperature_and_humidity() {
  M5.In_I2C.enabled = true;
  loadFrame(28086, 29360);

  const auto reading = homedeck::readSht40Environment();

  TEST_ASSERT_TRUE(reading.ok);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 30.0f, reading.temperatureCelsius);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, reading.humidityPercent);
  TEST_ASSERT_EQUAL(2, M5.In_I2C.startCalls);
  TEST_ASSERT_EQUAL(0x44, M5.In_I2C.lastAddress);
  TEST_ASSERT_TRUE(M5.In_I2C.lastReadMode);
  TEST_ASSERT_EQUAL(400000u, M5.In_I2C.lastFrequency);
  TEST_ASSERT_EQUAL(1, static_cast<int>(M5.In_I2C.writtenBytes.size()));
  TEST_ASSERT_EQUAL_HEX8(0xFD, M5.In_I2C.writtenBytes[0]);
  TEST_ASSERT_GREATER_OR_EQUAL(10u, gFakeMillis);
}

void test_sht40_reader_fails_when_i2c_is_disabled() {
  M5.In_I2C.enabled = false;

  const auto reading = homedeck::readSht40Environment();

  TEST_ASSERT_FALSE(reading.ok);
}

void test_sht40_reader_fails_on_crc_error() {
  M5.In_I2C.enabled = true;
  loadFrame(28086, 29360);
  M5.In_I2C.nextReadBuffer[2] ^= 0x01;

  const auto reading = homedeck::readSht40Environment();

  TEST_ASSERT_FALSE(reading.ok);
}

void test_sht40_reader_clamps_humidity_to_display_range() {
  M5.In_I2C.enabled = true;
  loadFrame(28086, 65535);

  const auto reading = homedeck::readSht40Environment();

  TEST_ASSERT_TRUE(reading.ok);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, reading.humidityPercent);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sht40_reader_reads_temperature_and_humidity);
  RUN_TEST(test_sht40_reader_fails_when_i2c_is_disabled);
  RUN_TEST(test_sht40_reader_fails_on_crc_error);
  RUN_TEST(test_sht40_reader_clamps_humidity_to_display_range);
  return UNITY_END();
}
```

- [ ] **Step 3: Run SHT40 reader tests and verify they fail**

Run:

```bash
pio test -e native --filter native/test_sht40_reader
```

Expected: FAIL because `sht40_reader.h` does not exist.

- [ ] **Step 4: Add SHT40 reader header**

Create `src/sht40_reader.h`:

```cpp
#pragma once

namespace homedeck {

struct EnvironmentReading {
  bool ok = false;
  float temperatureCelsius = 0.0f;
  float humidityPercent = 0.0f;
};

EnvironmentReading readSht40Environment();

}  // namespace homedeck
```

- [ ] **Step 5: Implement SHT40 reader**

Create `src/sht40_reader.cpp`:

```cpp
#include "sht40_reader.h"

#include <Arduino.h>
#include <M5Unified.h>

#include <algorithm>
#include <cstdint>

namespace homedeck {
namespace {

constexpr std::uint8_t kSht40Address = 0x44;
constexpr std::uint8_t kMeasureHighPrecisionCommand = 0xFD;
constexpr std::uint32_t kI2cFrequency = 400000;
constexpr unsigned long kSht40MeasurementDelayMs = 10;

std::uint8_t crc8(const std::uint8_t* data, int length) {
  std::uint8_t crc = 0xFF;
  for (int index = 0; index < length; ++index) {
    crc ^= data[index];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<std::uint8_t>((crc << 1) ^ 0x31) : static_cast<std::uint8_t>(crc << 1);
    }
  }
  return crc;
}

bool crcOk(const std::uint8_t* data) {
  return crc8(data, 2) == data[2];
}

float convertTemperature(std::uint16_t raw) {
  return -45.0f + 175.0f * static_cast<float>(raw) / 65535.0f;
}

float convertHumidity(std::uint16_t raw) {
  const float humidity = -6.0f + 125.0f * static_cast<float>(raw) / 65535.0f;
  return std::max(0.0f, std::min(100.0f, humidity));
}

std::uint16_t readRaw(const std::uint8_t msb, const std::uint8_t lsb) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(msb) << 8) | lsb);
}

}  // namespace

EnvironmentReading readSht40Environment() {
  if (!M5.In_I2C.start(kSht40Address, false, kI2cFrequency)) {
    return {};
  }
  if (!M5.In_I2C.write(kMeasureHighPrecisionCommand) || !M5.In_I2C.stop()) {
    M5.In_I2C.stop();
    return {};
  }

  delay(kSht40MeasurementDelayMs);

  std::uint8_t buffer[6] = {};
  if (!M5.In_I2C.start(kSht40Address, true, kI2cFrequency)) {
    return {};
  }
  const bool readOk = M5.In_I2C.read(buffer, sizeof(buffer), true);
  const bool stopOk = M5.In_I2C.stop();
  if (!readOk || !stopOk) {
    return {};
  }

  if (!crcOk(buffer) || !crcOk(buffer + 3)) {
    return {};
  }

  EnvironmentReading reading{};
  reading.ok = true;
  reading.temperatureCelsius = convertTemperature(readRaw(buffer[0], buffer[1]));
  reading.humidityPercent = convertHumidity(readRaw(buffer[3], buffer[4]));
  return reading;
}

}  // namespace homedeck
```

- [ ] **Step 6: Run SHT40 reader tests**

Run:

```bash
pio test -e native --filter native/test_sht40_reader
```

Expected: PASS.

- [ ] **Step 7: Commit SHT40 reader changes**

Run:

```bash
git add src/sht40_reader.h src/sht40_reader.cpp test/native/test_sht40_reader/test_main.cpp test/native/support/fake_arduino/M5Unified.h
git commit -m "feat: 读取 PaperColor SHT40 环境数据" \
  -m "- 通过 M5.In_I2C 发送 SHT40 高精度测量命令并校验 CRC" \
  -m "- 转换温度和湿度读数，湿度限制在 0 到 100%" \
  -m "- 补充 native fake 和 SHT40 成功、失败路径测试"
```

Expected: commit succeeds.

---

### Task 3: BootController Home Sleep Scheduling

**Files:**
- Modify: `src/boot_controller.h`
- Modify: `src/boot_controller.cpp`
- Modify: `test/native/test_boot_controller/test_main.cpp`

- [ ] **Step 1: Add failing BootController sleep tests**

Modify the `Fixture` in `test/native/test_boot_controller/test_main.cpp`:

```cpp
std::time_t currentUnix = 1704110400;
std::vector<homedeck::HomeSleepRequest> sleepRequests;
```

Add to `deps()`:

```cpp
deps.currentTime = [this]() { return currentUnix; };
deps.enterDeepSleep = [this](const homedeck::HomeSleepRequest& request) {
  sleepRequests.push_back(request);
};
```

Add `#include <ctime>` and `#include <vector>` at the top of the file.

Add these tests before `test_config_mode_update_handles_portal_client()`:

```cpp
void test_system_mode_does_not_sleep_before_home_display_window() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 59999;
  controller.update();

  TEST_ASSERT_EQUAL(0, static_cast<int>(f.sleepRequests.size()));
}

void test_system_mode_sleeps_to_next_midnight_after_home_display_window() {
  Fixture f{};
  f.configured = true;
  f.currentUnix = 1704110400;  // 2024-01-01 12:00:00 UTC
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 60000;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.sleepRequests.size()));
  TEST_ASSERT_EQUAL_UINT64(43200000000ULL, f.sleepRequests[0].timerWakeupUs);
  TEST_ASSERT_EQUAL(1, f.sleepRequests[0].wakeupGpio);
  TEST_ASSERT_TRUE(f.sleepRequests[0].wakeOnLow);
}

void test_system_mode_uses_one_hour_sleep_when_time_is_not_trusted() {
  Fixture f{};
  f.configured = true;
  f.currentUnix = 1000;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 60000;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.sleepRequests.size()));
  TEST_ASSERT_EQUAL_UINT64(3600000000ULL, f.sleepRequests[0].timerWakeupUs);
}

void test_system_mode_requests_sleep_only_once() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 60000;
  controller.update();
  f.now = 70000;
  controller.update();

  TEST_ASSERT_EQUAL(1, static_cast<int>(f.sleepRequests.size()));
}

void test_config_mode_does_not_request_home_sleep() {
  Fixture f{};
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 60000;
  controller.update();

  TEST_ASSERT_EQUAL(0, static_cast<int>(f.sleepRequests.size()));
  TEST_ASSERT_TRUE(f.portalHandled);
}

void test_ab_config_reboot_takes_priority_over_sleep() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.buttonsPressed = true;
  f.now = 55000;
  controller.update();
  f.now = 60000;
  controller.update();

  TEST_ASSERT_TRUE(f.forceFlagWritten);
  TEST_ASSERT_TRUE(f.restarted);
  TEST_ASSERT_EQUAL(0, static_cast<int>(f.sleepRequests.size()));
}
```

Register all six tests in `main()`.

- [ ] **Step 2: Run BootController tests and verify they fail**

Run:

```bash
pio test -e native --filter native/test_boot_controller
```

Expected: FAIL because `HomeSleepRequest`, `currentTime`, and `enterDeepSleep` do not exist yet.

- [ ] **Step 3: Add sleep request types and deps**

Modify `src/boot_controller.h`:

```cpp
#include <cstdint>
#include <ctime>
#include <functional>
```

Add before `BootControllerDeps`:

```cpp
struct HomeSleepRequest {
  std::uint64_t timerWakeupUs = 0;
  int wakeupGpio = 1;
  bool wakeOnLow = true;
};
```

Extend `BootControllerDeps`:

```cpp
std::function<std::time_t()> currentTime;
std::function<void(const HomeSleepRequest&)> enterDeepSleep;
```

Add private methods and fields:

```cpp
void updateHomeSleep(unsigned long now);
HomeSleepRequest makeHomeSleepRequest() const;

unsigned long systemModeStartedAtMs_ = 0;
bool homeSleepRequested_ = false;
```

- [ ] **Step 4: Implement home sleep scheduling**

Modify `src/boot_controller.cpp` includes:

```cpp
#include <cstdint>
#include <ctime>
#include <utility>
```

Add constants:

```cpp
constexpr unsigned long kSetupShortcutHoldMs = 5000;
constexpr unsigned long kHomeDisplayDurationMs = 60000;
constexpr std::time_t kTrustedUnixTimeThreshold = 1704067200;
constexpr std::uint64_t kMicrosPerSecond = 1000000ULL;
constexpr std::uint64_t kFallbackSleepSeconds = 3600ULL;
constexpr int kButtonCWakeupGpio = 1;
```

In `enterSystemMode()`, reset sleep state:

```cpp
  systemModeStartedAtMs_ = deps_.millis ? deps_.millis() : 0;
  homeSleepRequested_ = false;
```

In `update()`, after `updateSetupShortcut(now);`, add:

```cpp
  if (setupShortcutConsumed_) {
    return;
  }
  updateHomeSleep(now);
```

Add the helper implementations:

```cpp
void BootController::updateHomeSleep(unsigned long now) {
  if (homeSleepRequested_) {
    return;
  }
  if (now - systemModeStartedAtMs_ < kHomeDisplayDurationMs) {
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

  std::tm nextMidnight = *local;
  nextMidnight.tm_hour = 0;
  nextMidnight.tm_min = 0;
  nextMidnight.tm_sec = 0;
  nextMidnight.tm_mday += 1;
  nextMidnight.tm_isdst = -1;

  const std::time_t wakeAt = std::mktime(&nextMidnight);
  if (wakeAt <= now) {
    return request;
  }

  const auto seconds = static_cast<std::uint64_t>(wakeAt - now);
  request.timerWakeupUs = seconds * kMicrosPerSecond;
  return request;
}
```

- [ ] **Step 5: Run BootController tests**

Run:

```bash
pio test -e native --filter native/test_boot_controller
```

Expected: PASS.

- [ ] **Step 6: Commit BootController sleep scheduling**

Run:

```bash
git add src/boot_controller.h src/boot_controller.cpp test/native/test_boot_controller/test_main.cpp
git commit -m "feat: 首页显示一分钟后请求睡眠" \
  -m "- BootController 在系统模式渲染后等待 60 秒再请求 deep sleep" \
  -m "- 根据可信本地时间计算到下一个午夜的 timer wakeup，不可信时间兜底一小时" \
  -m "- 保留 AP 配置页常驻，并让 A+B 配置重启优先于睡眠"
```

Expected: commit succeeds.

---

### Task 4: Runtime Wiring For SHT40 And Deep Sleep Hardware

**Files:**
- Modify: `src/app_runtime.cpp`
- Modify: `test/native/support/fake_arduino/M5Unified.h`
- Modify: `test/native/support/fake_arduino/Arduino.h`
- Modify: `test/native/support/fake_arduino/esp_sleep.h`

- [ ] **Step 1: Add fake support for pin mode, BtnC, display sleep, and GPIO wakeup**

Modify `test/native/support/fake_arduino/Arduino.h`:

```cpp
constexpr std::uint8_t INPUT_PULLUP = 0x05;
inline int gFakeLastPinModePin = -1;
inline std::uint8_t gFakeLastPinModeMode = 0;
inline int gFakePinModeCalls = 0;

inline void pinMode(std::uint8_t pin, std::uint8_t mode) {
  ++gFakePinModeCalls;
  gFakeLastPinModePin = pin;
  gFakeLastPinModeMode = mode;
}
```

Update `fakeArduinoResetClock()`:

```cpp
inline void fakeArduinoResetClock() {
  gFakeMillis = 0;
  gFakeDelayCallback = nullptr;
  gFakeLastPinModePin = -1;
  gFakeLastPinModeMode = 0;
  gFakePinModeCalls = 0;
}
```

Modify `FakeDisplay` in `test/native/support/fake_arduino/M5Unified.h`:

```cpp
int sleepCount = 0;

void sleep() {
  ++sleepCount;
}
```

Modify `FakeM5Global`:

```cpp
FakeButton BtnC;
```

Modify `test/native/support/fake_arduino/esp_sleep.h`:

```cpp
enum esp_deepsleep_gpio_wake_up_mode_t {
  ESP_GPIO_WAKEUP_GPIO_LOW = 0,
  ESP_GPIO_WAKEUP_GPIO_HIGH = 1,
};

inline bool gFakeGpioWakeupConfigured = false;
inline uint64_t gFakeGpioWakeupMask = 0;
inline esp_deepsleep_gpio_wake_up_mode_t gFakeGpioWakeupMode = ESP_GPIO_WAKEUP_GPIO_LOW;
```

Update `fakeEspSleepReset()` and `fakeEspSleepReboot()` to reset those fields:

```cpp
gFakeGpioWakeupConfigured = false;
gFakeGpioWakeupMask = 0;
gFakeGpioWakeupMode = ESP_GPIO_WAKEUP_GPIO_LOW;
```

Add:

```cpp
inline void esp_deep_sleep_enable_gpio_wakeup(
    uint64_t gpio_pin_mask,
    esp_deepsleep_gpio_wake_up_mode_t mode) {
  gFakeGpioWakeupConfigured = true;
  gFakeGpioWakeupMask = gpio_pin_mask;
  gFakeGpioWakeupMode = mode;
}
```

- [ ] **Step 2: Wire SHT40 into home rendering**

Modify `src/app_runtime.cpp` includes:

```cpp
#include <esp_sleep.h>

#include "sht40_reader.h"
```

Add helper in the anonymous namespace:

```cpp
void renderHomeWithEnvironment() {
  HomeCalendarData data = makeCurrentHomeCalendarData();
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }
  gHomeRenderer.render(data);
}
```

Update `makeBootDeps()`:

```cpp
deps.renderHome = renderHomeWithEnvironment;
deps.currentTime = []() { return time(nullptr); };
```

- [ ] **Step 3: Wire Button C/timer deep sleep hardware**

Add helper in `src/app_runtime.cpp` anonymous namespace:

```cpp
void enterHomeDeepSleep(const HomeSleepRequest& request) {
  pinMode(static_cast<std::uint8_t>(request.wakeupGpio), INPUT_PULLUP);
  esp_sleep_enable_timer_wakeup(request.timerWakeupUs);
  esp_deep_sleep_enable_gpio_wakeup(
      1ULL << request.wakeupGpio,
      request.wakeOnLow ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH);
  M5.Display.sleep();
  M5.Display.waitDisplay();
  esp_deep_sleep_start();
}
```

Update `makeBootDeps()`:

```cpp
deps.enterDeepSleep = enterHomeDeepSleep;
```

Keep `deps.areSetupButtonsPressed` unchanged:

```cpp
deps.areSetupButtonsPressed = []() { return M5.BtnA.isPressed() && M5.BtnB.isPressed(); };
```

Button C is only a deep-sleep wake source, not part of the A+B config shortcut.

- [ ] **Step 4: Run targeted runtime-related tests**

Run:

```bash
pio test -e native --filter native/test_sht40_reader
pio test -e native --filter native/test_boot_controller
pio test -e native --filter native/test_home_renderer
```

Expected: all PASS.

- [ ] **Step 5: Build the device firmware**

Run:

```bash
pio run -e m5stack-papercolor
```

Expected: SUCCESS. This catches mismatches with real `esp_sleep.h`, `M5.Display.sleep()`, `INPUT_PULLUP`, and M5Unified I2C APIs.

- [ ] **Step 6: Commit runtime wiring**

Run:

```bash
git add src/app_runtime.cpp test/native/support/fake_arduino/M5Unified.h test/native/support/fake_arduino/Arduino.h test/native/support/fake_arduino/esp_sleep.h
git commit -m "feat: 接入首页环境采样和深度睡眠" \
  -m "- 首页渲染前读取一次 SHT40，失败时保留占位显示" \
  -m "- deep sleep 前配置午夜 timer wakeup 和 Button C GPIO1 低电平唤醒" \
  -m "- 进入 deep sleep 前让墨水屏 sleep 并等待刷新完成"
```

Expected: commit succeeds.

---

### Task 5: Full Verification And Cleanup

**Files:**
- Inspect all modified files.
- No new production behavior beyond the approved spec.

- [ ] **Step 1: Run full native test suite**

Run:

```bash
pio test -e native
```

Expected: PASS.

- [ ] **Step 2: Run device build**

Run:

```bash
pio run -e m5stack-papercolor
```

Expected: SUCCESS.

- [ ] **Step 3: Check whitespace**

Run:

```bash
git diff --check
```

Expected: no output.

- [ ] **Step 4: Review final diff**

Run:

```bash
git status --short --branch
git log --oneline --decorate -8
git diff --stat origin/main...HEAD
```

Expected:

- Branch is ahead of `origin/main` by the spec commit, plan commit if committed by the executor, and implementation commits from Tasks 1-4.
- Diff only touches the files listed in this plan.
- No unrelated formatting or generated-data churn.

- [ ] **Step 5: Prepare final implementation summary**

Write a concise summary with:

- Renderer: bottom temperature/humidity and two-line `宜/忌`.
- SHT40: one-shot read, CRC, conversion, failure placeholder.
- Sleep: 60-second display window, midnight timer wake, Button C wake, one-hour fallback.
- Verification commands and outcomes.

---

## Self-Review

Spec coverage:

- Bottom `30.0°C` and `50.0%` at 12px margins: Task 1.
- `--.-°C` and `--.-%` fallback: Task 1 and Task 2.
- `宜/忌` two-line cap: Task 1.
- One-shot SHT40 read: Task 2 and Task 4.
- System-mode-only 60-second sleep: Task 3.
- AP config page exclusion: Task 3.
- Midnight timer wake and one-hour fallback: Task 3 and Task 4.
- Button C / GPIO1 low wake: Task 3 and Task 4.
- Verification commands: Task 5.

Plan hygiene scan:

- Red-flag placeholder patterns were searched and are absent from task instructions.
- Every code-changing step includes concrete code snippets and exact commands.

Type consistency:

- `EnvironmentReading` is defined in `src/sht40_reader.h` and consumed by `src/app_runtime.cpp`.
- `HomeSleepRequest` is defined in `src/boot_controller.h`, inspected by `test_boot_controller`, and consumed by `src/app_runtime.cpp`.
- `makeCurrentHomeCalendarData()` is declared in `src/home_renderer.h` and used by both `HomeRenderer::render()` and runtime wiring.
