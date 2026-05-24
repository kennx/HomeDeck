# Config Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build HomeDeck's first-run/configuration mode, RTC calibration flow, A+B re-entry trigger, and temporary portrait home screen.

**Architecture:** Use a small state-machine controller with dependency injection for testability. Keep pure validation, time decisions, HTML generation, and Wi-Fi list selection separate from Arduino hardware adapters so native tests cover the behavior without a board.

**Tech Stack:** PlatformIO, Arduino ESP32, M5Unified, M5GFX, Unity native tests, `Preferences`, `WiFi`, `WebServer`, `ESP.restart`.

---

## File Structure

- Create `src/config_types.h`: shared data types for setup config, manual datetime, Wi-Fi scan entries, and validation/calibration results.
- Create `src/timezone_catalog.h` and `src/timezone_catalog.cpp`: minimal IANA-to-POSIX timezone mapping, initially `Asia/Shanghai` and `UTC`.
- Create `src/config_validator.h` and `src/config_validator.cpp`: parse `datetime-local` values and validate setup submissions.
- Create `src/boot_controller.h` and `src/boot_controller.cpp`: boot/config/system state machine and A+B timing window.
- Create `src/time_service.h` and `src/time_service.cpp`: RTC restore and save-time calibration policy.
- Create `src/home_renderer.h` and `src/home_renderer.cpp`: portrait centered `HomeDeck` rendering.
- Create `src/setup_page.h` and `src/setup_page.cpp`: pure HTML rendering for the configuration page.
- Create `src/config_store.h` and `src/config_store.cpp`: `Preferences` storage for setup config and boot flags.
- Create `src/config_portal.h` and `src/config_portal.cpp`: AP + HTTP portal wiring.
- Create `src/app_runtime.h` and `src/app_runtime.cpp`: production dependency assembly.
- Modify `src/main.cpp`: delegate to `appSetup()` and `appLoop()`.
- Modify `test/native/support/fake_arduino/M5Unified.h`: add display datum/drawString support and update-count tracking.
- Modify `test/native/support/fake_arduino/Arduino.h`: add native `configTzTime` recording for runtime compile.
- Create `test/native/support/fake_arduino/Preferences.h`: native fake for key/value NVS.
- Create `test/native/support/fake_arduino/WiFi.h`: native fake for Wi-Fi status, scan results, AP, and STA connect calls.
- Create `test/native/support/fake_arduino/WebServer.h`: native fake sufficient for compile-time coverage.
- Create `test/native/support/fake_arduino/ESP.h`: native fake for restart requests.
- Create tests under `test/native/test_config_validation/`, `test/native/test_boot_controller/`, `test/native/test_time_service/`, `test/native/test_home_renderer/`, `test/native/test_config_store/`, and `test/native/test_setup_page/`.

## Task 1: Verify External APIs Before Coding

**Files:**
- Read: `platformio.ini`
- Read: `docs/PaperColor.md`
- Read: Context7 docs for M5Unified, M5GFX, Arduino-ESP32

- [ ] **Step 1: Confirm M5Unified button and RTC APIs**

Use Context7 with library `/m5stack/m5unified` and this query:

```text
Arduino M5Unified Button A Button B isPressed pressedFor M5.update RTC setDateTime getDateTime setSystemTimeFromRtc getVoltLow examples
```

Expected APIs to confirm:

```cpp
M5.update();
M5.BtnA.isPressed();
M5.BtnB.isPressed();
M5.Rtc.isEnabled();
M5.Rtc.getVoltLow();
M5.Rtc.setSystemTimeFromRtc();
M5.Rtc.setDateTime(const std::tm*);
```

- [ ] **Step 2: Confirm M5GFX centered text APIs**

Use Context7 with library `/m5stack/m5gfx` and this query:

```text
M5GFX setRotation setTextDatum middle_center drawString width height text rendering centered
```

Expected APIs to confirm:

```cpp
M5.Display.setRotation(0);
M5.Display.setTextDatum(textdatum_t::middle_center);
M5.Display.drawString("HomeDeck", M5.Display.width() / 2, M5.Display.height() / 2);
```

- [ ] **Step 3: Confirm Arduino-ESP32 AP, Wi-Fi scan, NTP, Preferences, and restart APIs**

Use Context7 with library `/espressif/arduino-esp32` and this query:

```text
Arduino ESP32 WiFi softAP softAPIP scanNetworks SSID RSSI scanDelete begin status configTzTime Preferences putString getString putBool getBool WebServer on arg send ESP.restart
```

Expected APIs to confirm:

```cpp
WiFi.softAP(apSsid);
WiFi.softAPIP();
WiFi.scanNetworks();
WiFi.SSID(i);
WiFi.RSSI(i);
WiFi.scanDelete();
WiFi.begin(ssid, password);
WiFi.status();
configTzTime(posixTimezone, ntpServer);
Preferences::begin("homedeck", false);
Preferences::putString("ssid", value);
Preferences::getString("ssid", "");
Preferences::putBool("configured", true);
Preferences::getBool("configured", false);
WebServer::on("/", HTTP_GET, handler);
WebServer::on("/save", HTTP_POST, handler);
WebServer::arg("field");
WebServer::send(status, "text/html; charset=utf-8", body);
ESP.restart();
```

- [ ] **Step 4: Commit nothing**

This task reads docs only. If any expected API is unavailable, update this plan and the design doc before writing code.

## Task 2: Config Types, Timezone Catalog, and Validation

**Files:**
- Create: `src/config_types.h`
- Create: `src/timezone_catalog.h`
- Create: `src/timezone_catalog.cpp`
- Create: `src/config_validator.h`
- Create: `src/config_validator.cpp`
- Create: `test/native/test_config_validation/test_main.cpp`

- [ ] **Step 1: Write failing validation tests**

Create `test/native/test_config_validation/test_main.cpp`:

```cpp
#include <unity.h>

#include "config_validator.h"
#include "timezone_catalog.h"

void test_empty_wifi_requires_manual_time() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.ntpServer = "pool.ntp.org";
  config.autoRtcCorrection = false;
  homedeck::ManualDateTime manual{};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::MissingManualDateTime, result.error);
}

void test_empty_wifi_accepts_manual_time_and_empty_password() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiPassword = "";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 30, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::None, result.error);
}

void test_wifi_auto_correction_requires_ntp_server() {
  homedeck::SetupConfig config{};
  config.wifiSsid = "Home";
  config.timezoneIana = "Asia/Shanghai";
  config.autoRtcCorrection = true;
  homedeck::ManualDateTime manual{};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::MissingNtpServer, result.error);
}

void test_wifi_without_auto_correction_requires_manual_time() {
  homedeck::SetupConfig config{};
  config.wifiSsid = "Home";
  config.timezoneIana = "Asia/Shanghai";
  config.autoRtcCorrection = false;
  config.ntpServer = "pool.ntp.org";
  homedeck::ManualDateTime manual{};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::MissingManualDateTime, result.error);
}

void test_parse_datetime_local_value() {
  homedeck::ManualDateTime manual{};

  const bool ok = homedeck::parseManualDateTime("2026-05-24T09:08", &manual);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(manual.present);
  TEST_ASSERT_EQUAL(2026, manual.year);
  TEST_ASSERT_EQUAL(5, manual.month);
  TEST_ASSERT_EQUAL(24, manual.day);
  TEST_ASSERT_EQUAL(9, manual.hour);
  TEST_ASSERT_EQUAL(8, manual.minute);
  TEST_ASSERT_EQUAL(0, manual.second);
}

void test_invalid_timezone_is_rejected() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Mars/Base";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::InvalidTimezone, result.error);
}

void test_timezone_catalog_maps_asia_shanghai() {
  const homedeck::TimezoneInfo* info = homedeck::findTimezoneByIana("Asia/Shanghai");

  TEST_ASSERT_NOT_NULL(info);
  TEST_ASSERT_EQUAL_STRING("CST-8", info->posix);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_wifi_requires_manual_time);
  RUN_TEST(test_empty_wifi_accepts_manual_time_and_empty_password);
  RUN_TEST(test_wifi_auto_correction_requires_ntp_server);
  RUN_TEST(test_wifi_without_auto_correction_requires_manual_time);
  RUN_TEST(test_parse_datetime_local_value);
  RUN_TEST(test_invalid_timezone_is_rejected);
  RUN_TEST(test_timezone_catalog_maps_asia_shanghai);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the validation test to verify it fails**

Run:

```bash
pio test -e native -f test_config_validation
```

Expected: compile failure because `config_validator.h` and related types do not exist.

- [ ] **Step 3: Add shared config and validation code**

Create `src/config_types.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace homedeck {

struct SetupConfig {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezoneIana = "Asia/Shanghai";
  bool autoRtcCorrection = false;
  std::string ntpServer = "pool.ntp.org";
};

struct ManualDateTime {
  bool present = false;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

struct WifiNetwork {
  std::string ssid;
  int32_t rssi = 0;
};

enum class ConfigValidationError {
  None,
  MissingManualDateTime,
  MissingNtpServer,
  InvalidManualDateTime,
  InvalidTimezone,
};

struct ConfigValidationResult {
  ConfigValidationError error = ConfigValidationError::None;
  const char* message = "";
  bool ok() const { return error == ConfigValidationError::None; }
};

}  // namespace homedeck
```

Create `src/timezone_catalog.h`:

```cpp
#pragma once

#include <cstddef>
#include <string_view>

namespace homedeck {

struct TimezoneInfo {
  const char* iana;
  const char* posix;
  const char* label;
};

const TimezoneInfo* findTimezoneByIana(std::string_view iana);
const TimezoneInfo* defaultTimezone();
const TimezoneInfo* timezoneCatalog(std::size_t* count);

}  // namespace homedeck
```

Create `src/timezone_catalog.cpp`:

```cpp
#include "timezone_catalog.h"

namespace homedeck {
namespace {

constexpr TimezoneInfo kTimezones[] = {
    {"Asia/Shanghai", "CST-8", "Asia/Shanghai"},
    {"UTC", "UTC0", "UTC"},
};

}  // namespace

const TimezoneInfo* findTimezoneByIana(std::string_view iana) {
  for (const auto& timezone : kTimezones) {
    if (iana == timezone.iana) {
      return &timezone;
    }
  }
  return nullptr;
}

const TimezoneInfo* defaultTimezone() {
  return &kTimezones[0];
}

const TimezoneInfo* timezoneCatalog(std::size_t* count) {
  if (count != nullptr) {
    *count = sizeof(kTimezones) / sizeof(kTimezones[0]);
  }
  return kTimezones;
}

}  // namespace homedeck
```

Create `src/config_validator.h`:

```cpp
#pragma once

#include <string_view>

#include "config_types.h"

namespace homedeck {

bool parseManualDateTime(std::string_view value, ManualDateTime* out);
bool isManualDateTimeValid(const ManualDateTime& value);
ConfigValidationResult validateSetupSubmission(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime);

}  // namespace homedeck
```

Create `src/config_validator.cpp`:

```cpp
#include "config_validator.h"

#include <cstdio>

#include "timezone_catalog.h"

namespace homedeck {
namespace {

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
  switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    case 2:
      return isLeapYear(year) ? 29 : 28;
    default:
      return 0;
  }
}

bool isBlank(std::string_view value) {
  for (const unsigned char ch : value) {
    if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
      return false;
    }
  }
  return true;
}

ConfigValidationResult makeError(ConfigValidationError error, const char* message) {
  return ConfigValidationResult{error, message};
}

}  // namespace

bool parseManualDateTime(std::string_view value, ManualDateTime* out) {
  if (out == nullptr) {
    return false;
  }

  *out = ManualDateTime{};
  if (value.empty()) {
    return true;
  }

  ManualDateTime parsed{};
  parsed.present = true;
  const int matched = std::sscanf(
      std::string(value).c_str(),
      "%d-%d-%dT%d:%d",
      &parsed.year,
      &parsed.month,
      &parsed.day,
      &parsed.hour,
      &parsed.minute);

  if (matched != 5 || !isManualDateTimeValid(parsed)) {
    return false;
  }

  *out = parsed;
  return true;
}

bool isManualDateTimeValid(const ManualDateTime& value) {
  if (!value.present) {
    return true;
  }
  if (value.year < 2024 || value.year > 2099) {
    return false;
  }
  if (value.month < 1 || value.month > 12) {
    return false;
  }
  if (value.day < 1 || value.day > daysInMonth(value.year, value.month)) {
    return false;
  }
  if (value.hour < 0 || value.hour > 23) {
    return false;
  }
  if (value.minute < 0 || value.minute > 59) {
    return false;
  }
  return value.second >= 0 && value.second <= 59;
}

ConfigValidationResult validateSetupSubmission(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime) {
  if (findTimezoneByIana(config.timezoneIana) == nullptr) {
    return makeError(ConfigValidationError::InvalidTimezone, "时区不受支持。");
  }

  if (!isManualDateTimeValid(manualDateTime)) {
    return makeError(ConfigValidationError::InvalidManualDateTime, "手动时间格式无效。");
  }

  const bool hasWifi = !isBlank(config.wifiSsid);
  if (!hasWifi && !manualDateTime.present) {
    return makeError(ConfigValidationError::MissingManualDateTime, "离线配置必须填写手动时间。");
  }

  if (hasWifi && config.autoRtcCorrection && isBlank(config.ntpServer)) {
    return makeError(ConfigValidationError::MissingNtpServer, "自动纠正 RTC 时必须填写 NTP 服务器。");
  }

  if (hasWifi && !config.autoRtcCorrection && !manualDateTime.present) {
    return makeError(ConfigValidationError::MissingManualDateTime, "关闭自动纠正时必须填写手动时间。");
  }

  return ConfigValidationResult{};
}

}  // namespace homedeck
```

- [ ] **Step 4: Run the validation test to verify it passes**

Run:

```bash
pio test -e native -f test_config_validation
```

Expected: PASS for all 7 validation tests.

- [ ] **Step 5: Commit validation foundation**

Run:

```bash
git add src/config_types.h src/timezone_catalog.h src/timezone_catalog.cpp src/config_validator.h src/config_validator.cpp test/native/test_config_validation/test_main.cpp
git commit -m "feat: 添加配置校验基础" -m "定义首次配置字段、手动时间解析、时区映射和提交校验规则。"
```

## Task 3: Boot Controller State Machine

**Files:**
- Create: `src/boot_controller.h`
- Create: `src/boot_controller.cpp`
- Create: `test/native/test_boot_controller/test_main.cpp`

- [ ] **Step 1: Write failing boot controller tests**

Create `test/native/test_boot_controller/test_main.cpp`:

```cpp
#include <unity.h>

#include "boot_controller.h"

namespace {

struct Fixture {
  bool configured = false;
  bool forceConfig = false;
  bool portalStarted = false;
  bool portalHandled = false;
  bool homeRendered = false;
  bool forceFlagWritten = false;
  bool forceFlagCleared = false;
  bool restarted = false;
  bool buttonsPressed = false;
  unsigned long now = 0;
  int updateCalls = 0;

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
      forceConfig = true;
      forceFlagWritten = true;
      return true;
    };
    deps.startConfigPortal = [this]() { portalStarted = true; };
    deps.handleConfigPortalClient = [this]() { portalHandled = true; };
    deps.restoreSystemTimeFromRtc = []() {};
    deps.renderHome = [this]() { homeRendered = true; };
    deps.updateButtons = [this]() { ++updateCalls; };
    deps.areSetupButtonsPressed = [this]() { return buttonsPressed; };
    deps.millis = [this]() { return now; };
    deps.restart = [this]() { restarted = true; };
    return deps;
  }
};

}  // namespace

void test_first_boot_enters_config_mode() {
  Fixture f{};
  homedeck::BootController controller{f.deps()};

  controller.begin();

  TEST_ASSERT_TRUE(f.portalStarted);
  TEST_ASSERT_FALSE(f.homeRendered);
  TEST_ASSERT_EQUAL(homedeck::BootMode::Config, controller.mode());
}

void test_force_config_flag_is_consumed_once() {
  Fixture f{};
  f.configured = true;
  f.forceConfig = true;
  homedeck::BootController controller{f.deps()};

  controller.begin();

  TEST_ASSERT_TRUE(f.portalStarted);
  TEST_ASSERT_TRUE(f.forceFlagCleared);
  TEST_ASSERT_FALSE(f.forceConfig);
  TEST_ASSERT_EQUAL(homedeck::BootMode::Config, controller.mode());
}

void test_configured_boot_renders_home() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};

  controller.begin();

  TEST_ASSERT_TRUE(f.homeRendered);
  TEST_ASSERT_FALSE(f.portalStarted);
  TEST_ASSERT_EQUAL(homedeck::BootMode::System, controller.mode());
}

void test_ab_held_for_three_seconds_requests_config_reboot() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.buttonsPressed = true;
  f.now = 100;
  controller.update();
  f.now = 3100;
  controller.update();

  TEST_ASSERT_TRUE(f.forceFlagWritten);
  TEST_ASSERT_TRUE(f.restarted);
}

void test_ab_release_resets_hold_timer() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.buttonsPressed = true;
  f.now = 100;
  controller.update();
  f.buttonsPressed = false;
  f.now = 2000;
  controller.update();
  f.buttonsPressed = true;
  f.now = 3200;
  controller.update();

  TEST_ASSERT_FALSE(f.forceFlagWritten);
  TEST_ASSERT_FALSE(f.restarted);
}

void test_ab_after_five_second_window_does_not_trigger() {
  Fixture f{};
  f.configured = true;
  homedeck::BootController controller{f.deps()};
  controller.begin();

  f.now = 6000;
  f.buttonsPressed = true;
  controller.update();
  f.now = 9000;
  controller.update();

  TEST_ASSERT_FALSE(f.forceFlagWritten);
  TEST_ASSERT_FALSE(f.restarted);
}

void test_config_mode_update_handles_portal_client() {
  Fixture f{};
  homedeck::BootController controller{f.deps()};
  controller.begin();

  controller.update();

  TEST_ASSERT_TRUE(f.portalHandled);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_first_boot_enters_config_mode);
  RUN_TEST(test_force_config_flag_is_consumed_once);
  RUN_TEST(test_configured_boot_renders_home);
  RUN_TEST(test_ab_held_for_three_seconds_requests_config_reboot);
  RUN_TEST(test_ab_release_resets_hold_timer);
  RUN_TEST(test_ab_after_five_second_window_does_not_trigger);
  RUN_TEST(test_config_mode_update_handles_portal_client);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the boot controller test to verify it fails**

Run:

```bash
pio test -e native -f test_boot_controller
```

Expected: compile failure because `boot_controller.h` does not exist.

- [ ] **Step 3: Add the boot controller**

Create `src/boot_controller.h`:

```cpp
#pragma once

#include <functional>

namespace homedeck {

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
  std::function<void()> renderHome;
  std::function<void()> updateButtons;
  std::function<bool()> areSetupButtonsPressed;
  std::function<unsigned long()> millis;
  std::function<void()> restart;
};

class BootController {
 public:
  explicit BootController(BootControllerDeps deps);

  void begin();
  void update();
  BootMode mode() const;

 private:
  void enterConfigMode();
  void enterSystemMode();
  void updateSetupShortcut(unsigned long now);

  BootControllerDeps deps_;
  BootMode mode_ = BootMode::System;
  bool started_ = false;
  unsigned long systemEnteredAtMs_ = 0;
  unsigned long setupButtonsPressedSinceMs_ = 0;
  bool setupButtonsWerePressed_ = false;
  bool setupShortcutConsumed_ = false;
};

}  // namespace homedeck
```

Create `src/boot_controller.cpp`:

```cpp
#include "boot_controller.h"

#include <utility>

namespace homedeck {
namespace {

constexpr unsigned long kSetupShortcutWindowMs = 5000;
constexpr unsigned long kSetupShortcutHoldMs = 3000;

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
  updateSetupShortcut(now);
}

BootMode BootController::mode() const {
  return mode_;
}

void BootController::enterConfigMode() {
  mode_ = BootMode::Config;
  if (deps_.startConfigPortal) {
    deps_.startConfigPortal();
  }
}

void BootController::enterSystemMode() {
  mode_ = BootMode::System;
  systemEnteredAtMs_ = deps_.millis ? deps_.millis() : 0;
  setupButtonsPressedSinceMs_ = 0;
  setupButtonsWerePressed_ = false;
  setupShortcutConsumed_ = false;

  if (deps_.restoreSystemTimeFromRtc) {
    deps_.restoreSystemTimeFromRtc();
  }
  if (deps_.renderHome) {
    deps_.renderHome();
  }
}

void BootController::updateSetupShortcut(unsigned long now) {
  if (setupShortcutConsumed_) {
    return;
  }
  if (now - systemEnteredAtMs_ > kSetupShortcutWindowMs) {
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
    setupShortcutConsumed_ = true;
    if (deps_.setForceConfigOnNextBoot) {
      deps_.setForceConfigOnNextBoot();
    }
    if (deps_.restart) {
      deps_.restart();
    }
  }
}

}  // namespace homedeck
```

- [ ] **Step 4: Run the boot controller test to verify it passes**

Run:

```bash
pio test -e native -f test_boot_controller
```

Expected: PASS for all 7 boot controller tests.

- [ ] **Step 5: Commit boot controller**

Run:

```bash
git add src/boot_controller.h src/boot_controller.cpp test/native/test_boot_controller/test_main.cpp
git commit -m "feat: 添加启动模式控制器" -m "实现首次配置、一次性配置标记、系统首页渲染入口和 A+B 三秒重启进入配置模式。"
```

## Task 4: Time Service Calibration Policy

**Files:**
- Create: `src/time_service.h`
- Create: `src/time_service.cpp`
- Create: `test/native/test_time_service/test_main.cpp`

- [ ] **Step 1: Write failing time service tests**

Create `test/native/test_time_service/test_main.cpp`:

```cpp
#include <ctime>

#include <unity.h>

#include "time_service.h"

namespace {

struct Fixture {
  bool wifiConnected = true;
  bool ntpSynced = true;
  bool rtcAvailable = true;
  bool rtcVoltLow = false;
  time_t ntpUnix = 1779573600;
  bool connectCalled = false;
  bool ntpCalled = false;
  bool writeRtcCalled = false;
  bool restoreCalled = false;
  time_t writtenUnix = 0;

  homedeck::TimeServiceDeps deps() {
    homedeck::TimeServiceDeps deps{};
    deps.connectWifi = [this](const std::string&, const std::string&) {
      connectCalled = true;
      return wifiConnected;
    };
    deps.syncNtp = [this](const std::string&, const std::string&, time_t* syncedUnix) {
      ntpCalled = true;
      if (!ntpSynced || syncedUnix == nullptr) {
        return false;
      }
      *syncedUnix = ntpUnix;
      return true;
    };
    deps.writeRtcUtc = [this](time_t unixTime) {
      writeRtcCalled = true;
      writtenUnix = unixTime;
      return true;
    };
    deps.rtcAvailable = [this]() { return rtcAvailable; };
    deps.rtcVoltLow = [this]() { return rtcVoltLow; };
    deps.restoreSystemTimeFromRtc = [this]() { restoreCalled = true; };
    return deps;
  }
};

homedeck::SetupConfig wifiAutoConfig() {
  homedeck::SetupConfig config{};
  config.wifiSsid = "Home";
  config.wifiPassword = "";
  config.timezoneIana = "Asia/Shanghai";
  config.autoRtcCorrection = true;
  config.ntpServer = "pool.ntp.org";
  return config;
}

}  // namespace

void test_ntp_success_writes_ntp_time_to_rtc() {
  Fixture f{};
  homedeck::TimeService service{f.deps()};

  const auto result = service.calibrateOnSave(wifiAutoConfig(), homedeck::ManualDateTime{});

  TEST_ASSERT_EQUAL(homedeck::TimeCalibrationStatus::SuccessNtp, result.status);
  TEST_ASSERT_TRUE(f.connectCalled);
  TEST_ASSERT_TRUE(f.ntpCalled);
  TEST_ASSERT_TRUE(f.writeRtcCalled);
  TEST_ASSERT_EQUAL_INT64(f.ntpUnix, f.writtenUnix);
}

void test_ntp_failure_uses_manual_fallback() {
  Fixture f{};
  f.ntpSynced = false;
  homedeck::TimeService service{f.deps()};
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = service.calibrateOnSave(wifiAutoConfig(), manual);

  TEST_ASSERT_EQUAL(homedeck::TimeCalibrationStatus::SuccessManualFallback, result.status);
  TEST_ASSERT_TRUE(f.writeRtcCalled);
}

void test_ntp_failure_without_manual_time_returns_error() {
  Fixture f{};
  f.ntpSynced = false;
  homedeck::TimeService service{f.deps()};

  const auto result = service.calibrateOnSave(wifiAutoConfig(), homedeck::ManualDateTime{});

  TEST_ASSERT_EQUAL(homedeck::TimeCalibrationStatus::FailedNeedsManualTime, result.status);
  TEST_ASSERT_FALSE(f.writeRtcCalled);
}

void test_offline_config_writes_manual_time() {
  Fixture f{};
  homedeck::TimeService service{f.deps()};
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = service.calibrateOnSave(config, manual);

  TEST_ASSERT_EQUAL(homedeck::TimeCalibrationStatus::SuccessManual, result.status);
  TEST_ASSERT_FALSE(f.connectCalled);
  TEST_ASSERT_TRUE(f.writeRtcCalled);
}

void test_restore_uses_rtc_when_available_and_not_low_voltage() {
  Fixture f{};
  homedeck::TimeService service{f.deps()};

  service.restoreSystemTimeFromRtc();

  TEST_ASSERT_TRUE(f.restoreCalled);
}

void test_restore_skips_low_voltage_rtc() {
  Fixture f{};
  f.rtcVoltLow = true;
  homedeck::TimeService service{f.deps()};

  service.restoreSystemTimeFromRtc();

  TEST_ASSERT_FALSE(f.restoreCalled);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ntp_success_writes_ntp_time_to_rtc);
  RUN_TEST(test_ntp_failure_uses_manual_fallback);
  RUN_TEST(test_ntp_failure_without_manual_time_returns_error);
  RUN_TEST(test_offline_config_writes_manual_time);
  RUN_TEST(test_restore_uses_rtc_when_available_and_not_low_voltage);
  RUN_TEST(test_restore_skips_low_voltage_rtc);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the time service test to verify it fails**

Run:

```bash
pio test -e native -f test_time_service
```

Expected: compile failure because `time_service.h` does not exist.

- [ ] **Step 3: Add time service**

Create `src/time_service.h`:

```cpp
#pragma once

#include <ctime>
#include <functional>
#include <string>

#include "config_types.h"

namespace homedeck {

enum class TimeCalibrationStatus {
  SuccessNtp,
  SuccessManual,
  SuccessManualFallback,
  FailedWifi,
  FailedNeedsManualTime,
  FailedRtcWrite,
  FailedTimezone,
};

struct TimeCalibrationResult {
  TimeCalibrationStatus status = TimeCalibrationStatus::SuccessManual;
  const char* message = "";
  bool ok() const {
    return status == TimeCalibrationStatus::SuccessNtp ||
           status == TimeCalibrationStatus::SuccessManual ||
           status == TimeCalibrationStatus::SuccessManualFallback;
  }
};

struct TimeServiceDeps {
  std::function<bool(const std::string& ssid, const std::string& password)> connectWifi;
  std::function<bool(const std::string& posixTimezone, const std::string& ntpServer, time_t* syncedUnix)> syncNtp;
  std::function<bool(time_t utcUnix)> writeRtcUtc;
  std::function<bool()> rtcAvailable;
  std::function<bool()> rtcVoltLow;
  std::function<void()> restoreSystemTimeFromRtc;
};

class TimeService {
 public:
  explicit TimeService(TimeServiceDeps deps);

  TimeCalibrationResult calibrateOnSave(
      const SetupConfig& config,
      const ManualDateTime& manualDateTime);
  void restoreSystemTimeFromRtc();

 private:
  bool manualDateTimeToUnix(
      const ManualDateTime& manualDateTime,
      const std::string& timezoneIana,
      time_t* unixTime) const;
  TimeCalibrationResult writeManual(
      const ManualDateTime& manualDateTime,
      const std::string& timezoneIana,
      TimeCalibrationStatus successStatus);

  TimeServiceDeps deps_;
};

}  // namespace homedeck
```

Create `src/time_service.cpp` with the status policy and timezone conversion:

```cpp
#include "time_service.h"

#include <cstdlib>
#include <cstring>
#include <utility>

#include "timezone_catalog.h"

namespace homedeck {
namespace {

bool isBlank(const std::string& value) {
  for (const unsigned char ch : value) {
    if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
      return false;
    }
  }
  return true;
}

TimeCalibrationResult result(TimeCalibrationStatus status, const char* message) {
  return TimeCalibrationResult{status, message};
}

}  // namespace

TimeService::TimeService(TimeServiceDeps deps) : deps_(std::move(deps)) {
}

TimeCalibrationResult TimeService::calibrateOnSave(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime) {
  const TimezoneInfo* timezone = findTimezoneByIana(config.timezoneIana);
  if (timezone == nullptr) {
    return result(TimeCalibrationStatus::FailedTimezone, "时区不受支持。");
  }

  const bool hasWifi = !isBlank(config.wifiSsid);
  if (hasWifi && config.autoRtcCorrection) {
    if (deps_.connectWifi && !deps_.connectWifi(config.wifiSsid, config.wifiPassword)) {
      if (manualDateTime.present) {
        return writeManual(manualDateTime, config.timezoneIana, TimeCalibrationStatus::SuccessManualFallback);
      }
      return result(TimeCalibrationStatus::FailedWifi, "无法连接 Wi-Fi，请填写手动时间。");
    }

    time_t syncedUnix = 0;
    if (deps_.syncNtp && deps_.syncNtp(timezone->posix, config.ntpServer, &syncedUnix)) {
      if (deps_.writeRtcUtc && deps_.writeRtcUtc(syncedUnix)) {
        return result(TimeCalibrationStatus::SuccessNtp, "");
      }
      return result(TimeCalibrationStatus::FailedRtcWrite, "RTC 写入失败。");
    }

    if (manualDateTime.present) {
      return writeManual(manualDateTime, config.timezoneIana, TimeCalibrationStatus::SuccessManualFallback);
    }
    return result(TimeCalibrationStatus::FailedNeedsManualTime, "无法自动校准，请填写手动时间。");
  }

  if (manualDateTime.present) {
    return writeManual(manualDateTime, config.timezoneIana, TimeCalibrationStatus::SuccessManual);
  }

  return result(TimeCalibrationStatus::FailedNeedsManualTime, "请填写手动时间。");
}

void TimeService::restoreSystemTimeFromRtc() {
  if (deps_.rtcAvailable && !deps_.rtcAvailable()) {
    return;
  }
  if (deps_.rtcVoltLow && deps_.rtcVoltLow()) {
    return;
  }
  if (deps_.restoreSystemTimeFromRtc) {
    deps_.restoreSystemTimeFromRtc();
  }
}

bool TimeService::manualDateTimeToUnix(
    const ManualDateTime& manualDateTime,
    const std::string& timezoneIana,
    time_t* unixTime) const {
  if (unixTime == nullptr || !manualDateTime.present) {
    return false;
  }

  const TimezoneInfo* timezone = findTimezoneByIana(timezoneIana);
  if (timezone == nullptr) {
    return false;
  }

  char* oldTimezone = std::getenv("TZ");
  const std::string savedTimezone = oldTimezone != nullptr ? oldTimezone : "";
  setenv("TZ", timezone->posix, 1);
  tzset();

  std::tm local{};
  local.tm_year = manualDateTime.year - 1900;
  local.tm_mon = manualDateTime.month - 1;
  local.tm_mday = manualDateTime.day;
  local.tm_hour = manualDateTime.hour;
  local.tm_min = manualDateTime.minute;
  local.tm_sec = manualDateTime.second;
  local.tm_isdst = -1;
  const time_t converted = std::mktime(&local);

  if (oldTimezone != nullptr) {
    setenv("TZ", savedTimezone.c_str(), 1);
  } else {
    unsetenv("TZ");
  }
  tzset();

  if (converted < 0) {
    return false;
  }

  *unixTime = converted;
  return true;
}

TimeCalibrationResult TimeService::writeManual(
    const ManualDateTime& manualDateTime,
    const std::string& timezoneIana,
    TimeCalibrationStatus successStatus) {
  time_t unixTime = 0;
  if (!manualDateTimeToUnix(manualDateTime, timezoneIana, &unixTime)) {
    return result(TimeCalibrationStatus::FailedTimezone, "手动时间无法按时区转换。");
  }
  if (deps_.writeRtcUtc && deps_.writeRtcUtc(unixTime)) {
    return result(successStatus, "");
  }
  return result(TimeCalibrationStatus::FailedRtcWrite, "RTC 写入失败。");
}

}  // namespace homedeck
```

- [ ] **Step 4: Run the time service test to verify it passes**

Run:

```bash
pio test -e native -f test_time_service
```

Expected: PASS for all 6 time service tests.

- [ ] **Step 5: Commit time service**

Run:

```bash
git add src/time_service.h src/time_service.cpp test/native/test_time_service/test_main.cpp
git commit -m "feat: 添加 RTC 校准服务" -m "实现 Wi-Fi/NTP 自动纠正、手动时间兜底和 RTC 恢复策略。"
```

## Task 5: Home Renderer and Display Fake Support

**Files:**
- Modify: `test/native/support/fake_arduino/M5Unified.h`
- Create: `src/home_renderer.h`
- Create: `src/home_renderer.cpp`
- Create: `test/native/test_home_renderer/test_main.cpp`

- [ ] **Step 1: Write failing home renderer test**

Create `test/native/test_home_renderer/test_main.cpp`:

```cpp
#include <unity.h>

#include <M5Unified.h>

#include "home_renderer.h"

FakeM5Global M5;

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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_home_renderer_draws_centered_home_deck_in_portrait);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the home renderer test to verify it fails**

Run:

```bash
pio test -e native -f test_home_renderer
```

Expected: compile failure because `home_renderer.h`, `textdatum_t`, and `drawString` support do not exist.

- [ ] **Step 3: Extend the display fake**

Modify `test/native/support/fake_arduino/M5Unified.h`:

```cpp
enum class textdatum_t {
  top_left = 0,
  middle_center = 1,
};
```

Add `textdatum_t textDatum = textdatum_t::top_left;` to `FakeDisplay`.

Add these methods to `FakeDisplay`:

```cpp
void setTextDatum(textdatum_t value) {
  textDatum = value;
}

void drawString(const char* text, int x, int y) {
  cursorX = x;
  cursorY = y;
  prints.push_back({x, y, textSize, fontKind, text != nullptr ? text : ""});
}
```

- [ ] **Step 4: Add home renderer**

Create `src/home_renderer.h`:

```cpp
#pragma once

namespace homedeck {

class HomeRenderer {
 public:
  void render();
};

}  // namespace homedeck
```

Create `src/home_renderer.cpp`:

```cpp
#include "home_renderer.h"

#include <M5Unified.h>

namespace homedeck {

void HomeRenderer::render() {
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(textdatum_t::middle_center);
  M5.Display.drawString("HomeDeck", M5.Display.width() / 2, M5.Display.height() / 2);
}

}  // namespace homedeck
```

- [ ] **Step 5: Run the home renderer test to verify it passes**

Run:

```bash
pio test -e native -f test_home_renderer
```

Expected: PASS for the centered portrait rendering test.

- [ ] **Step 6: Commit home renderer**

Run:

```bash
git add test/native/support/fake_arduino/M5Unified.h src/home_renderer.h src/home_renderer.cpp test/native/test_home_renderer/test_main.cpp
git commit -m "feat: 添加临时首页渲染" -m "使用 portrait 模式绘制水平和垂直居中的 HomeDeck 文本。"
```

## Task 6: Config Store and Native Preferences Fake

**Files:**
- Create: `test/native/support/fake_arduino/Preferences.h`
- Create: `src/config_store.h`
- Create: `src/config_store.cpp`
- Create: `test/native/test_config_store/test_main.cpp`

- [ ] **Step 1: Write failing config store test**

Create `test/native/test_config_store/test_main.cpp`:

```cpp
#include <unity.h>

#include <Preferences.h>

#include "config_store.h"

void setUp() {
  fakePreferencesReset();
}

void tearDown() {
}

void test_load_defaults_when_empty() {
  homedeck::ConfigStore store;
  TEST_ASSERT_TRUE(store.begin());

  const auto config = store.loadSetupConfig();
  const auto flags = store.loadBootFlags();

  TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", config.timezoneIana.c_str());
  TEST_ASSERT_EQUAL_STRING("pool.ntp.org", config.ntpServer.c_str());
  TEST_ASSERT_FALSE(flags.configured);
  TEST_ASSERT_FALSE(flags.forceConfigOnNextBoot);
}

void test_save_and_load_config_and_flags() {
  homedeck::ConfigStore store;
  TEST_ASSERT_TRUE(store.begin());
  homedeck::SetupConfig config{};
  config.wifiSsid = "Cafe";
  config.wifiPassword = "";
  config.timezoneIana = "Asia/Shanghai";
  config.autoRtcCorrection = true;
  config.ntpServer = "time.cloudflare.com";

  TEST_ASSERT_TRUE(store.saveSetupConfig(config));
  TEST_ASSERT_TRUE(store.saveConfigured(true));
  TEST_ASSERT_TRUE(store.setForceConfigOnNextBoot(true));

  const auto loaded = store.loadSetupConfig();
  const auto flags = store.loadBootFlags();

  TEST_ASSERT_EQUAL_STRING("Cafe", loaded.wifiSsid.c_str());
  TEST_ASSERT_EQUAL_STRING("", loaded.wifiPassword.c_str());
  TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", loaded.timezoneIana.c_str());
  TEST_ASSERT_TRUE(loaded.autoRtcCorrection);
  TEST_ASSERT_EQUAL_STRING("time.cloudflare.com", loaded.ntpServer.c_str());
  TEST_ASSERT_TRUE(flags.configured);
  TEST_ASSERT_TRUE(flags.forceConfigOnNextBoot);
}

void test_clear_force_config_flag() {
  homedeck::ConfigStore store;
  TEST_ASSERT_TRUE(store.begin());
  TEST_ASSERT_TRUE(store.setForceConfigOnNextBoot(true));
  TEST_ASSERT_TRUE(store.clearForceConfigOnNextBoot());

  TEST_ASSERT_FALSE(store.loadBootFlags().forceConfigOnNextBoot);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_load_defaults_when_empty);
  RUN_TEST(test_save_and_load_config_and_flags);
  RUN_TEST(test_clear_force_config_flag);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the config store test to verify it fails**

Run:

```bash
pio test -e native -f test_config_store
```

Expected: compile failure because `Preferences.h` fake and `config_store.h` do not exist.

- [ ] **Step 3: Add native Preferences fake**

Create `test/native/support/fake_arduino/Preferences.h`:

```cpp
#pragma once

#include <map>
#include <string>

inline std::map<std::string, std::string> gFakePreferenceStrings;
inline std::map<std::string, bool> gFakePreferenceBools;
inline bool gFakePreferencesBeginResult = true;

inline void fakePreferencesReset() {
  gFakePreferenceStrings.clear();
  gFakePreferenceBools.clear();
  gFakePreferencesBeginResult = true;
}

class Preferences {
 public:
  bool begin(const char*, bool = false) {
    started_ = gFakePreferencesBeginResult;
    return started_;
  }

  void end() {
    started_ = false;
  }

  std::string getString(const char* key, const char* defaultValue = "") const {
    const auto found = gFakePreferenceStrings.find(key);
    return found == gFakePreferenceStrings.end() ? std::string(defaultValue) : found->second;
  }

  std::size_t putString(const char* key, const char* value) {
    if (!started_) {
      return 0;
    }
    gFakePreferenceStrings[key] = value != nullptr ? value : "";
    return gFakePreferenceStrings[key].size();
  }

  bool getBool(const char* key, bool defaultValue = false) const {
    const auto found = gFakePreferenceBools.find(key);
    return found == gFakePreferenceBools.end() ? defaultValue : found->second;
  }

  bool putBool(const char* key, bool value) {
    if (!started_) {
      return false;
    }
    gFakePreferenceBools[key] = value;
    return true;
  }

 private:
  bool started_ = false;
};
```

- [ ] **Step 4: Add config store**

Create `src/config_store.h`:

```cpp
#pragma once

#include <Preferences.h>

#include "boot_controller.h"
#include "config_types.h"

namespace homedeck {

class ConfigStore {
 public:
  bool begin();
  SetupConfig loadSetupConfig() const;
  bool saveSetupConfig(const SetupConfig& config);
  BootFlags loadBootFlags() const;
  bool saveConfigured(bool configured);
  bool setForceConfigOnNextBoot(bool enabled);
  bool clearForceConfigOnNextBoot();

 private:
  mutable Preferences prefs_;
  bool started_ = false;
};

}  // namespace homedeck
```

Create `src/config_store.cpp`:

```cpp
#include "config_store.h"

namespace homedeck {
namespace {

constexpr const char* kNamespace = "homedeck";
constexpr const char* kWifiSsid = "wifi_ssid";
constexpr const char* kWifiPassword = "wifi_pass";
constexpr const char* kTimezoneIana = "tz";
constexpr const char* kAutoRtc = "auto_rtc";
constexpr const char* kNtpServer = "ntp";
constexpr const char* kConfigured = "configured";
constexpr const char* kForceConfig = "force_cfg";

}  // namespace

bool ConfigStore::begin() {
  started_ = prefs_.begin(kNamespace, false);
  return started_;
}

SetupConfig ConfigStore::loadSetupConfig() const {
  SetupConfig config{};
  config.wifiSsid = prefs_.getString(kWifiSsid, "").c_str();
  config.wifiPassword = prefs_.getString(kWifiPassword, "").c_str();
  config.timezoneIana = prefs_.getString(kTimezoneIana, "Asia/Shanghai").c_str();
  config.autoRtcCorrection = prefs_.getBool(kAutoRtc, false);
  config.ntpServer = prefs_.getString(kNtpServer, "pool.ntp.org").c_str();
  return config;
}

bool ConfigStore::saveSetupConfig(const SetupConfig& config) {
  if (!started_) {
    return false;
  }
  const bool stringsOk =
      prefs_.putString(kWifiSsid, config.wifiSsid.c_str()) > 0 || config.wifiSsid.empty();
  const bool passwordOk =
      prefs_.putString(kWifiPassword, config.wifiPassword.c_str()) > 0 || config.wifiPassword.empty();
  const bool timezoneOk = prefs_.putString(kTimezoneIana, config.timezoneIana.c_str()) > 0;
  const bool ntpOk = prefs_.putString(kNtpServer, config.ntpServer.c_str()) > 0 || config.ntpServer.empty();
  const bool boolOk = prefs_.putBool(kAutoRtc, config.autoRtcCorrection);
  return stringsOk && passwordOk && timezoneOk && ntpOk && boolOk;
}

BootFlags ConfigStore::loadBootFlags() const {
  return BootFlags{
      prefs_.getBool(kConfigured, false),
      prefs_.getBool(kForceConfig, false),
  };
}

bool ConfigStore::saveConfigured(bool configured) {
  return started_ && prefs_.putBool(kConfigured, configured);
}

bool ConfigStore::setForceConfigOnNextBoot(bool enabled) {
  return started_ && prefs_.putBool(kForceConfig, enabled);
}

bool ConfigStore::clearForceConfigOnNextBoot() {
  return setForceConfigOnNextBoot(false);
}

}  // namespace homedeck
```

- [ ] **Step 5: Run the config store test to verify it passes**

Run:

```bash
pio test -e native -f test_config_store
```

Expected: PASS for all 3 config store tests.

- [ ] **Step 6: Commit config store**

Run:

```bash
git add test/native/support/fake_arduino/Preferences.h src/config_store.h src/config_store.cpp test/native/test_config_store/test_main.cpp
git commit -m "feat: 添加配置持久化" -m "使用 Preferences 保存 Wi-Fi、时区、NTP、已配置状态和一次性配置标记。"
```

## Task 7: Setup Page, Wi-Fi List Selection, and Config Portal

**Files:**
- Create: `src/setup_page.h`
- Create: `src/setup_page.cpp`
- Create: `src/config_portal.h`
- Create: `src/config_portal.cpp`
- Create: `test/native/support/fake_arduino/WiFi.h`
- Create: `test/native/support/fake_arduino/WebServer.h`
- Create: `test/native/support/fake_arduino/ESP.h`
- Create: `test/native/test_setup_page/test_main.cpp`

- [ ] **Step 1: Write failing setup page tests**

Create `test/native/test_setup_page/test_main.cpp`:

```cpp
#include <unity.h>

#include "setup_page.h"

void test_select_top_five_wifi_networks_by_rssi() {
  std::vector<homedeck::WifiNetwork> networks = {
      {"weak", -90},
      {"best", -20},
      {"mid", -60},
      {"ok", -50},
      {"fine", -40},
      {"last", -70},
  };

  const auto selected = homedeck::selectTopWifiNetworks(networks, 5);

  TEST_ASSERT_EQUAL(5, static_cast<int>(selected.size()));
  TEST_ASSERT_EQUAL_STRING("best", selected[0].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("fine", selected[1].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("ok", selected[2].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("mid", selected[3].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("last", selected[4].ssid.c_str());
}

void test_setup_page_contains_wifi_list_timezone_and_disabled_auto_when_ssid_empty() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.ntpServer = "pool.ntp.org";
  std::vector<homedeck::WifiNetwork> networks = {{"Home", -30}, {"Cafe", -50}};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "");

  TEST_ASSERT_NOT_EQUAL(-1, html.find("HomeDeck-ABCD"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("Home"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("Cafe"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("Asia/Shanghai"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("name=\"auto_rtc\""));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("disabled"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("name=\"manual_datetime\""));
}

void test_setup_page_shows_error_message() {
  homedeck::SetupConfig config{};
  std::vector<homedeck::WifiNetwork> networks{};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "请填写手动时间。");

  TEST_ASSERT_NOT_EQUAL(-1, html.find("请填写手动时间。"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_select_top_five_wifi_networks_by_rssi);
  RUN_TEST(test_setup_page_contains_wifi_list_timezone_and_disabled_auto_when_ssid_empty);
  RUN_TEST(test_setup_page_shows_error_message);
  return UNITY_END();
}
```

- [ ] **Step 2: Run the setup page test to verify it fails**

Run:

```bash
pio test -e native -f test_setup_page
```

Expected: compile failure because `setup_page.h` does not exist.

- [ ] **Step 3: Add setup page pure helpers**

Create `src/setup_page.h`:

```cpp
#pragma once

#include <string>
#include <vector>

#include "config_types.h"

namespace homedeck {

std::vector<WifiNetwork> selectTopWifiNetworks(
    const std::vector<WifiNetwork>& networks,
    std::size_t limit);
std::string htmlEscape(const std::string& value);
std::string buildSetupPageHtml(
    const std::string& apSsid,
    const SetupConfig& values,
    const std::vector<WifiNetwork>& networks,
    const std::string& message);

}  // namespace homedeck
```

Create `src/setup_page.cpp` with simple HTML:

```cpp
#include "setup_page.h"

#include <algorithm>
#include <sstream>

#include "timezone_catalog.h"

namespace homedeck {

std::vector<WifiNetwork> selectTopWifiNetworks(
    const std::vector<WifiNetwork>& networks,
    std::size_t limit) {
  std::vector<WifiNetwork> selected = networks;
  std::sort(selected.begin(), selected.end(), [](const WifiNetwork& left, const WifiNetwork& right) {
    return left.rssi > right.rssi;
  });
  if (selected.size() > limit) {
    selected.resize(limit);
  }
  return selected;
}

std::string htmlEscape(const std::string& value) {
  std::string escaped;
  for (const char ch : value) {
    switch (ch) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

std::string buildSetupPageHtml(
    const std::string& apSsid,
    const SetupConfig& values,
    const std::vector<WifiNetwork>& networks,
    const std::string& message) {
  std::ostringstream html;
  html << "<!doctype html><html><head><meta charset=\"utf-8\">";
  html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html << "<title>HomeDeck Setup</title>";
  html << "<style>body{font-family:sans-serif;margin:24px;max-width:680px}label{display:block;margin-top:14px}input,select,button{font-size:16px;padding:8px;width:100%;box-sizing:border-box}.wifi button{margin:4px 0}.msg{color:#b00020}</style>";
  html << "</head><body><h1>HomeDeck Setup</h1>";
  html << "<p>AP: " << htmlEscape(apSsid) << " / 192.168.4.1</p>";
  if (!message.empty()) {
    html << "<p class=\"msg\">" << htmlEscape(message) << "</p>";
  }
  html << "<div class=\"wifi\"><strong>Wi-Fi 列表</strong>";
  for (const auto& network : networks) {
    html << "<button type=\"button\" onclick=\"pickSsid('" << htmlEscape(network.ssid) << "')\">";
    html << htmlEscape(network.ssid) << " (" << network.rssi << " dBm)</button>";
  }
  html << "</div><form method=\"post\" action=\"/save\">";
  html << "<label>Wi-Fi SSID<input id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" << htmlEscape(values.wifiSsid) << "\"></label>";
  html << "<label>Wi-Fi 密码<input name=\"wifi_password\" type=\"password\" value=\"" << htmlEscape(values.wifiPassword) << "\"></label>";
  html << "<label>时区<select name=\"timezone\">";
  std::size_t timezoneCount = 0;
  const auto* timezones = timezoneCatalog(&timezoneCount);
  for (std::size_t i = 0; i < timezoneCount; ++i) {
    html << "<option value=\"" << timezones[i].iana << "\"";
    if (values.timezoneIana == timezones[i].iana) {
      html << " selected";
    }
    html << ">" << timezones[i].label << "</option>";
  }
  html << "</select></label>";
  html << "<label><input id=\"auto_rtc\" name=\"auto_rtc\" type=\"checkbox\" value=\"1\"";
  if (values.autoRtcCorrection) {
    html << " checked";
  }
  if (values.wifiSsid.empty()) {
    html << " disabled";
  }
  html << "> 自动纠正 RTC</label>";
  html << "<label>NTP 服务器<input name=\"ntp_server\" value=\"" << htmlEscape(values.ntpServer) << "\"></label>";
  html << "<label>手动日期时间<input name=\"manual_datetime\" type=\"datetime-local\"></label>";
  html << "<button type=\"submit\">保存</button></form>";
  html << "<script>const ssid=document.getElementById('wifi_ssid');const auto=document.getElementById('auto_rtc');function sync(){auto.disabled=ssid.value.trim()==='';if(auto.disabled)auto.checked=false;}function pickSsid(v){ssid.value=v;sync();}ssid.addEventListener('input',sync);sync();</script>";
  html << "</body></html>";
  return html.str();
}

}  // namespace homedeck
```

- [ ] **Step 4: Add fake Arduino web headers for native compile**

Create `test/native/support/fake_arduino/WiFi.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

constexpr int WIFI_AP = 2;
constexpr int WIFI_STA = 1;
constexpr int WL_CONNECTED = 3;

struct IPAddress {
  int a = 192;
  int b = 168;
  int c = 4;
  int d = 1;
  std::string toString() const { return "192.168.4.1"; }
};

struct FakeWifiNetwork {
  std::string ssid;
  int32_t rssi = 0;
};

struct FakeWiFiClass {
  int modeValue = 0;
  bool softApStarted = false;
  std::string apSsid;
  std::string staSsid;
  std::string staPassword;
  int statusValue = WL_CONNECTED;
  std::vector<FakeWifiNetwork> scanResults;

  void mode(int value) { modeValue = value; }
  bool softAP(const char* ssid, const char* = nullptr) {
    softApStarted = true;
    apSsid = ssid != nullptr ? ssid : "";
    return true;
  }
  IPAddress softAPIP() const { return IPAddress{}; }
  int scanNetworks() { return static_cast<int>(scanResults.size()); }
  std::string SSID(int index) const { return scanResults[index].ssid; }
  int32_t RSSI(int index) const { return scanResults[index].rssi; }
  void scanDelete() { scanResults.clear(); }
  void begin(const char* ssid, const char* password) {
    staSsid = ssid != nullptr ? ssid : "";
    staPassword = password != nullptr ? password : "";
  }
  int status() const { return statusValue; }
};

inline FakeWiFiClass WiFi;
```

Create `test/native/support/fake_arduino/WebServer.h`:

```cpp
#pragma once

#include <functional>
#include <map>
#include <string>

constexpr int HTTP_GET = 0;
constexpr int HTTP_POST = 1;

class WebServer {
 public:
  explicit WebServer(int port) : port_(port) {}

  void on(const char* path, int, std::function<void()> handler) {
    handlers_[path] = std::move(handler);
  }

  void begin() { started_ = true; }
  void handleClient() {}
  std::string arg(const char* name) const {
    const auto found = args_.find(name);
    return found == args_.end() ? std::string{} : found->second;
  }
  void send(int status, const char* type, const char* body) {
    lastStatus = status;
    lastType = type != nullptr ? type : "";
    lastBody = body != nullptr ? body : "";
  }

  int port_ = 80;
  bool started_ = false;
  int lastStatus = 0;
  std::string lastType;
  std::string lastBody;
  std::map<std::string, std::string> args_;
  std::map<std::string, std::function<void()>> handlers_;
};
```

Create `test/native/support/fake_arduino/ESP.h`:

```cpp
#pragma once

#include <cstdint>

struct FakeESPClass {
  bool restartCalled = false;
  std::uint64_t efuseMac = 0x1234ABCD;
  void restart() { restartCalled = true; }
  std::uint64_t getEfuseMac() const { return efuseMac; }
};

inline FakeESPClass ESP;
```

- [ ] **Step 5: Add config portal shell**

Create `src/config_portal.h`:

```cpp
#pragma once

#include <WebServer.h>

#include <functional>
#include <string>
#include <vector>

#include "config_types.h"

namespace homedeck {

using ConfigPortalSaveCallback = std::function<ConfigValidationResult(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime)>;

class ConfigPortal {
 public:
  void begin(
      const std::string& apSsid,
      const SetupConfig& defaults,
      ConfigPortalSaveCallback onSave);
  void handleClient();

 private:
  std::vector<WifiNetwork> scanWifiNetworks();
  SetupConfig readConfigFromRequest();
  ManualDateTime readManualTimeFromRequest();
  void handleIndex();
  void handleSave();
  void sendPage(int status, const SetupConfig& values, const std::string& message);

  WebServer server_{80};
  std::string apSsid_;
  SetupConfig defaults_{};
  std::vector<WifiNetwork> networks_;
  ConfigPortalSaveCallback onSave_;
  bool restartScheduled_ = false;
  unsigned long restartAtMs_ = 0;
};

}  // namespace homedeck
```

Create `src/config_portal.cpp` with AP, scan, save, and restart wiring:

```cpp
#include "config_portal.h"

#include <Arduino.h>
#include <ESP.h>
#include <WiFi.h>

#include "config_validator.h"
#include "setup_page.h"

namespace homedeck {

void ConfigPortal::begin(
    const std::string& apSsid,
    const SetupConfig& defaults,
    ConfigPortalSaveCallback onSave) {
  apSsid_ = apSsid;
  defaults_ = defaults;
  onSave_ = std::move(onSave);
  restartScheduled_ = false;
  restartAtMs_ = 0;

  networks_ = selectTopWifiNetworks(scanWifiNetworks(), 5);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid_.c_str());
  delay(100);

  server_.on("/", HTTP_GET, [this]() { handleIndex(); });
  server_.on("/save", HTTP_POST, [this]() { handleSave(); });
  server_.begin();
}

void ConfigPortal::handleClient() {
  server_.handleClient();
  if (restartScheduled_ && static_cast<long>(millis() - restartAtMs_) >= 0) {
    ESP.restart();
  }
}

std::vector<WifiNetwork> ConfigPortal::scanWifiNetworks() {
  std::vector<WifiNetwork> networks;
  const int count = WiFi.scanNetworks();
  for (int i = 0; i < count; ++i) {
    networks.push_back(WifiNetwork{WiFi.SSID(i).c_str(), WiFi.RSSI(i)});
  }
  WiFi.scanDelete();
  return networks;
}

SetupConfig ConfigPortal::readConfigFromRequest() {
  SetupConfig config{};
  config.wifiSsid = server_.arg("wifi_ssid").c_str();
  config.wifiPassword = server_.arg("wifi_password").c_str();
  config.timezoneIana = server_.arg("timezone").c_str();
  config.autoRtcCorrection = server_.arg("auto_rtc") == "1" && !config.wifiSsid.empty();
  config.ntpServer = server_.arg("ntp_server").c_str();
  return config;
}

ManualDateTime ConfigPortal::readManualTimeFromRequest() {
  ManualDateTime manual{};
  parseManualDateTime(server_.arg("manual_datetime").c_str(), &manual);
  return manual;
}

void ConfigPortal::handleIndex() {
  sendPage(200, defaults_, "");
}

void ConfigPortal::handleSave() {
  const SetupConfig submitted = readConfigFromRequest();
  ManualDateTime manual{};
  if (!parseManualDateTime(server_.arg("manual_datetime").c_str(), &manual)) {
    sendPage(400, submitted, "手动时间格式无效。");
    return;
  }

  const ConfigValidationResult validation = validateSetupSubmission(submitted, manual);
  if (!validation.ok()) {
    sendPage(400, submitted, validation.message);
    return;
  }

  ConfigValidationResult saveResult{};
  if (onSave_) {
    saveResult = onSave_(submitted, manual);
  }
  if (!saveResult.ok()) {
    sendPage(500, submitted, saveResult.message);
    return;
  }

  defaults_ = submitted;
  restartScheduled_ = true;
  restartAtMs_ = millis() + 1000;
  sendPage(200, submitted, "设置已保存，设备将在 1 秒后重启。");
}

void ConfigPortal::sendPage(int status, const SetupConfig& values, const std::string& message) {
  const std::string html = buildSetupPageHtml(apSsid_, values, networks_, message);
  server_.send(status, "text/html; charset=utf-8", html.c_str());
}

}  // namespace homedeck
```

- [ ] **Step 6: Run the setup page test to verify it passes**

Run:

```bash
pio test -e native -f test_setup_page
```

Expected: PASS for all 3 setup page tests.

- [ ] **Step 7: Commit setup page and portal**

Run:

```bash
git add src/setup_page.h src/setup_page.cpp src/config_portal.h src/config_portal.cpp test/native/support/fake_arduino/WiFi.h test/native/support/fake_arduino/WebServer.h test/native/support/fake_arduino/ESP.h test/native/test_setup_page/test_main.cpp
git commit -m "feat: 添加配置门户页面" -m "实现开放 AP 页面、Wi-Fi 前五条列表、时区字段、RTC 自动纠正开关和保存后重启流程。"
```

## Task 8: Production Runtime Wiring

**Files:**
- Create: `src/app_runtime.h`
- Create: `src/app_runtime.cpp`
- Modify: `src/main.cpp`
- Modify: `test/native/support/fake_arduino/M5Unified.h`
- Modify: `test/native/support/fake_arduino/Arduino.h`

- [ ] **Step 1: Add update-call tracking to the M5 fake**

Modify `FakeM5Global` in `test/native/support/fake_arduino/M5Unified.h`:

```cpp
int updateCount = 0;

void update() {
  ++updateCount;
}
```

- [ ] **Step 2: Add fake configTzTime for native runtime compile**

Modify `test/native/support/fake_arduino/Arduino.h`:

```cpp
#include <string>

inline std::string gFakeTimezone;
inline std::string gFakeNtpServer;

inline void configTzTime(const char* timezone, const char* server) {
  gFakeTimezone = timezone != nullptr ? timezone : "";
  gFakeNtpServer = server != nullptr ? server : "";
}
```

- [ ] **Step 3: Add production runtime header**

Create `src/app_runtime.h`:

```cpp
#pragma once

namespace homedeck {

void appSetup();
void appLoop();

}  // namespace homedeck
```

- [ ] **Step 4: Add production runtime implementation**

Create `src/app_runtime.cpp`:

```cpp
#include "app_runtime.h"

#include <Arduino.h>
#include <ESP.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>

#include <cstdio>
#include <cstdint>
#include <memory>

#include "boot_controller.h"
#include "config_portal.h"
#include "config_store.h"
#include "home_renderer.h"
#include "time_service.h"
#include "timezone_catalog.h"

namespace homedeck {
namespace {

ConfigStore gConfigStore;
HomeRenderer gHomeRenderer;
ConfigPortal gConfigPortal;
std::unique_ptr<TimeService> gTimeService;
std::unique_ptr<BootController> gBootController;

std::string makeApSsid() {
  const std::uint64_t mac = ESP.getEfuseMac();
  char suffix[5] = {};
  std::snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned int>(mac & 0xFFFF));
  return std::string("HomeDeck-") + suffix;
}

bool connectWifi(const std::string& ssid, const std::string& password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 10000) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool syncNtp(const std::string& posixTimezone, const std::string& ntpServer, time_t* syncedUnix) {
  if (syncedUnix == nullptr) {
    return false;
  }
  configTzTime(posixTimezone.c_str(), ntpServer.c_str());
  const unsigned long startedAt = millis();
  while (millis() - startedAt < 10000) {
    const time_t now = time(nullptr);
    if (now >= 1704067200) {
      *syncedUnix = now;
      return true;
    }
    delay(250);
  }
  return false;
}

bool writeRtcUtc(time_t unixTime) {
  if (!M5.Rtc.isEnabled()) {
    return false;
  }
  const std::tm* utc = gmtime(&unixTime);
  if (utc == nullptr) {
    return false;
  }
  M5.Rtc.setDateTime(utc);
  return true;
}

TimeServiceDeps makeTimeDeps() {
  TimeServiceDeps deps{};
  deps.connectWifi = connectWifi;
  deps.syncNtp = syncNtp;
  deps.writeRtcUtc = writeRtcUtc;
  deps.rtcAvailable = []() { return M5.Rtc.isEnabled(); };
  deps.rtcVoltLow = []() { return M5.Rtc.getVoltLow(); };
  deps.restoreSystemTimeFromRtc = []() { M5.Rtc.setSystemTimeFromRtc(); };
  return deps;
}

ConfigValidationResult saveSubmittedConfig(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime) {
  const TimeCalibrationResult timeResult = gTimeService->calibrateOnSave(config, manualDateTime);
  if (!timeResult.ok()) {
    return ConfigValidationResult{ConfigValidationError::InvalidManualDateTime, timeResult.message};
  }
  if (!gConfigStore.saveSetupConfig(config) || !gConfigStore.saveConfigured(true)) {
    return ConfigValidationResult{ConfigValidationError::InvalidManualDateTime, "保存配置失败。"};
  }
  return ConfigValidationResult{};
}

BootControllerDeps makeBootDeps() {
  BootControllerDeps deps{};
  deps.loadFlags = []() { return gConfigStore.loadBootFlags(); };
  deps.clearForceConfigOnNextBoot = []() { return gConfigStore.clearForceConfigOnNextBoot(); };
  deps.setForceConfigOnNextBoot = []() { return gConfigStore.setForceConfigOnNextBoot(true); };
  deps.startConfigPortal = []() {
    gConfigPortal.begin(makeApSsid(), gConfigStore.loadSetupConfig(), saveSubmittedConfig);
  };
  deps.handleConfigPortalClient = []() { gConfigPortal.handleClient(); };
  deps.restoreSystemTimeFromRtc = []() { gTimeService->restoreSystemTimeFromRtc(); };
  deps.renderHome = []() { gHomeRenderer.render(); };
  deps.updateButtons = []() { M5.update(); };
  deps.areSetupButtonsPressed = []() { return M5.BtnA.isPressed() && M5.BtnB.isPressed(); };
  deps.millis = []() { return millis(); };
  deps.restart = []() { ESP.restart(); };
  return deps;
}

}  // namespace

void appSetup() {
  M5.begin();
  gConfigStore.begin();
  gTimeService = std::make_unique<TimeService>(makeTimeDeps());
  gBootController = std::make_unique<BootController>(makeBootDeps());
  gBootController->begin();
}

void appLoop() {
  if (gBootController) {
    gBootController->update();
  }
}

}  // namespace homedeck
```

- [ ] **Step 5: Replace main with runtime delegation**

Replace `src/main.cpp`:

```cpp
#include "app_runtime.h"

void setup() {
    homedeck::appSetup();
}

void loop() {
    homedeck::appLoop();
}
```

- [ ] **Step 6: Run focused native tests**

Run:

```bash
pio test -e native -f test_boot_controller -f test_time_service -f test_config_store -f test_home_renderer -f test_setup_page -f test_config_validation
```

Expected: PASS for all focused native tests.

- [ ] **Step 7: Commit runtime wiring**

Run:

```bash
git add src/app_runtime.h src/app_runtime.cpp src/main.cpp test/native/support/fake_arduino/M5Unified.h test/native/support/fake_arduino/Arduino.h
git commit -m "feat: 接入配置模式运行时" -m "将启动控制、配置门户、时间服务、配置存储和临时首页接入 Arduino setup/loop。"
```

## Task 9: Full Verification and Final Cleanup

**Files:**
- Read: all changed files
- Modify: only files required by failing checks

- [ ] **Step 1: Run native test suite**

Run:

```bash
pio test -e native
```

Expected: all native tests pass.

- [ ] **Step 2: Run device build**

Run:

```bash
pio run -e m5stack-papercolor
```

Expected: build succeeds for the PaperColor target.

- [ ] **Step 3: Inspect git diff**

Run:

```bash
git status --short --branch
git diff --stat HEAD
```

Expected: only files from this plan are modified or created.

- [ ] **Step 4: Run whitespace check**

Run:

```bash
git diff --check
```

Expected: no output.

- [ ] **Step 5: Commit verification fixes if any were needed**

If Step 1, Step 2, or Step 4 required edits, commit only those edits:

```bash
git add <files changed to fix verification>
git commit -m "fix: 修复配置模式回归" -m "根据 native 测试和 PaperColor 构建结果修正配置模式实现。"
```

If no edits were needed after Task 8, skip this commit.

## Self-Review

- Spec coverage: tasks cover first-run config mode, `HomeDeck-XXXX` open AP, `192.168.4.1` portal, top-5 Wi-Fi scan, empty SSID/password, timezone, auto RTC switch, NTP/manual RTC calibration, save-then-restart, portrait centered `HomeDeck`, A+B 5-second window and 3-second hold, and verification commands.
- Placeholder scan: the plan contains concrete file paths, code snippets, commands, and expected results for every task.
- Type consistency: `SetupConfig`, `ManualDateTime`, `BootFlags`, `BootControllerDeps`, `TimeServiceDeps`, and validation/result enums are defined before later tasks consume them.
