#include "app/app_runtime.h"

#include <Arduino.h>
#include <ESP.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_sntp.h>
#include <esp_sleep.h>
#include <time.h>

#include <Adafruit_NeoPixel.h>
#include <M5PM1.h>
#ifndef UNIT_TEST
#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <esp_attr.h>
#endif

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include "app/boot_controller.h"
#include "config/config_portal.h"
#include "config/config_store.h"
#include "views/almanac_view.h"
#include "views/calendar_view.h"
#include "providers/webcal_provider.h"
#include "views/countdown_view.h"
#include "config/config_portal_renderer.h"
#include "views/weather_view.h"
#include "views/view_common.h"
#include "generated/device_font_vlw.h"
#include "providers/weather_provider.h"
#include "system/render_context.h"
#ifndef UNIT_TEST
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif
#include "system/sht40_reader.h"
#include "system/time_service.h"
#include "providers/timezone_catalog.h"
#include "system/wifi_connection.h"

#ifndef UNIT_TEST
#define WEATHER_LOG(...) Serial.printf(__VA_ARGS__)
#else
#define WEATHER_LOG(...)
#endif

namespace homedeck {

#ifdef UNIT_TEST
SystemView gRtcSavedView = SystemView::Almanac;
#else
RTC_DATA_ATTR SystemView gRtcSavedView = SystemView::Almanac;
#endif

namespace {

#ifndef UNIT_TEST
void i2cBusRecovery() {
  constexpr gpio_num_t kSclPin = GPIO_NUM_2;
  constexpr gpio_num_t kSdaPin = GPIO_NUM_3;

  gpio_config_t cfg;
  cfg.pin_bit_mask = (1ULL << kSclPin) | (1ULL << kSdaPin);
  cfg.mode = GPIO_MODE_INPUT_OUTPUT_OD;
  cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);

  gpio_set_level(kSdaPin, 1);
  gpio_set_level(kSclPin, 1);
  esp_rom_delay_us(10);

  for (int i = 0; i < 9; ++i) {
    gpio_set_level(kSclPin, 0);
    esp_rom_delay_us(5);
    gpio_set_level(kSclPin, 1);
    esp_rom_delay_us(5);
    if (gpio_get_level(kSdaPin) == 1) {
      break;
    }
  }

  gpio_set_level(kSdaPin, 0);
  esp_rom_delay_us(5);
  gpio_set_level(kSclPin, 1);
  esp_rom_delay_us(5);
  gpio_set_level(kSdaPin, 1);
  esp_rom_delay_us(5);

  gpio_reset_pin(kSclPin);
  gpio_reset_pin(kSdaPin);
}
#endif

constexpr uint8_t kRgbLedPin = 21;
constexpr uint8_t kRgbLedCount = 2;

M5PM1 gPm1;
Adafruit_NeoPixel gPixels(kRgbLedCount, kRgbLedPin, NEO_GRB + NEO_KHZ800);
bool gPm1Ready = false;

void clearRgbLedPixels() {
  gPixels.clear();
  gPixels.show();
}

void disableRgbLedPower() {
  if (gPm1Ready) {
    gPm1.setLdoEnable(false);
  }
}

void shutdownRgbLedForSleep() {
  clearRgbLedPixels();
  disableRgbLedPower();
}

void initRgbLed() {
  m5pm1_err_t err = gPm1.begin(&M5.In_I2C, M5PM1_DEFAULT_ADDR, M5PM1_I2C_FREQ_100K);
  gPm1Ready = (err == M5PM1_OK);
  if (gPm1Ready) {
    gPm1.setLdoEnable(true);
  }
  gPixels.begin();
  clearRgbLedPixels();
}

#ifndef UNIT_TEST
// EPD 状态诊断日志：esp_log 输出在 USB 重连后仍可被捕获，
// 用于排查墨水屏不刷新类问题（busy 电平、board 识别、供电时序）。
#define EPDDBG(...) ESP_LOGI("epddbg", __VA_ARGS__)
#endif

// ED2208 数据手册：退出 Deep Sleep 必须发送 HWRESET（RES# 低电平有效，GPIO12）。
// 旧固件的 M5.Display.sleep() 会把面板送入深睡；M5GFX 首轮初始化不带硬件复位，
// 深睡中的面板无视所有 SPI 命令并保持 BUSY_N 拉低，每次 busy 等待都烧满 20s 超时。
// 因此在 M5.begin 之前先脉冲 RES#，让面板在初始化前就退出深睡。
constexpr int kEpdResetPin = 12;
constexpr unsigned long kEpdResetLowMs = 10;
constexpr unsigned long kEpdResetHighMs = 10;

void resetEpdController() {
  pinMode(kEpdResetPin, OUTPUT);
  digitalWrite(kEpdResetPin, HIGH);
  delay(2);
  digitalWrite(kEpdResetPin, LOW);
  delay(kEpdResetLowMs);
  digitalWrite(kEpdResetPin, HIGH);
  delay(kEpdResetHighMs);
}

// 冷启动冲刷次数：Spectra 6 面板对长时间停留的画面存在物理烧入（粒子卡住），
// 单方向白刷无法冲开，需要黑白交替的极性驱动（白→黑→白共 3 次）。
constexpr int kEpdColdBootClearPasses = 3;

void prepareEpdAfterWakeup() {
  // 冷启动基线清屏：墨水屏断电后仍保留旧画面，fast 刷新盖不住会产生残影。
  // 白→黑→白交替全量刷新：把长时间烧入的粒子在两个极性间来回冲刷，
  // 残余残影随后续每次全量刷新（每日唤醒渲染）继续自然消退。
  // deep sleep 唤醒走 prepareEpdAfterDeepSleep()，不做此清屏。
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
#ifndef UNIT_TEST
  EPDDBG("before wakeup busy=%d", gpio_get_level(GPIO_NUM_11));
#endif
  M5.Display.wakeup();
#ifndef UNIT_TEST
  EPDDBG("after wakeup busy=%d", gpio_get_level(GPIO_NUM_11));
#endif
  for (int pass = 0; pass < kEpdColdBootClearPasses; ++pass) {
    const std::uint32_t clearColor = (pass % 2 == 0) ? TFT_WHITE : TFT_BLACK;
    M5.Display.clear(clearColor);
#ifndef UNIT_TEST
    EPDDBG("after clear pass=%d color=%s busy=%d", pass + 1,
           clearColor == TFT_WHITE ? "white" : "black", gpio_get_level(GPIO_NUM_11));
#endif
    M5.Display.waitDisplay();
#ifndef UNIT_TEST
    EPDDBG("after waitDisplay pass=%d busy=%d", pass + 1, gpio_get_level(GPIO_NUM_11));
#endif
  }
  M5.Display.setEpdMode(epd_mode_t::epd_fast);
}

#ifndef UNIT_TEST
void prepareEpdAfterDeepSleep() {
  M5.Display.setEpdMode(epd_mode_t::epd_fast);
  M5.Display.wakeup();
}
#endif

// M5PM1 GPIO0 = PY_EPD_EN，墨水屏供电使能（见 docs/PaperColor.md 管脚映射）。
constexpr std::uint8_t kPm1EpdEnableGpio = 0;
constexpr unsigned long kEpdPowerCycleOffMs = 100;
constexpr unsigned long kEpdPowerCycleOnMs = 100;

void disableEpdPower() {
  // 墨水屏是双稳态的：控制器断电不丢失画面。
  // 直接断电而不是发送 DEEP_SLEEP 命令——ED2208 深度睡眠后缺少硬件复位恢复路径，
  // 面板卡死后所有后续刷新都会失效（屏幕停留在旧画面，表现为"刷了固件也没变化"）。
  if (!gPm1Ready) {
    return;
  }
  gPm1.pinMode(kPm1EpdEnableGpio, OUTPUT);
  gPm1.digitalWrite(kPm1EpdEnableGpio, LOW);
}

void powerCycleEpd() {
  // 每次启动都断电重启墨水屏控制器，再重发初始化序列：
  // M5GFX 对 PaperColor 的首轮初始化不带硬件复位（_pin_reset 的 use_reset=false），
  // 面板可能残留上一次固件 DEEP_SLEEP 后的卡死状态，断电是最可靠的恢复手段。
  if (!gPm1Ready) {
    // PM1 不可用时退化为仅重发初始化序列。
    M5.Display.wakeup();
    return;
  }
  gPm1.pinMode(kPm1EpdEnableGpio, OUTPUT);
  gPm1.digitalWrite(kPm1EpdEnableGpio, LOW);
  delay(kEpdPowerCycleOffMs);
  gPm1.digitalWrite(kPm1EpdEnableGpio, HIGH);
  delay(kEpdPowerCycleOnMs);
  M5.Display.wakeup();
}

ConfigStore gConfigStore;
ConfigPortalRenderer gConfigPortalRenderer;
AlmanacView gAlmanacView;
CalendarView gCalendarView;
CountdownView gCountdownView;
WeatherView gWeatherView;
ConfigPortal gConfigPortal;
std::unique_ptr<TimeService> gTimeService;
std::unique_ptr<BootController> gBootController;

constexpr time_t kTrustedUnixTimeThreshold = 1704067200;
constexpr unsigned long kNtpSyncTimeoutMs = 10000;
constexpr unsigned long kNtpPollIntervalMs = 250;

std::string makeApSsid() {
  const std::uint64_t mac = ESP.getEfuseMac();
  char suffix[5] = {};
  std::snprintf(suffix, sizeof(suffix), "%04X", static_cast<unsigned int>(mac & 0xFFFF));
  return std::string("HomeDeck-") + suffix;
}

std::string softApIpAddress() {
  return WiFi.softAPIP().toString().c_str();
}

bool syncNtp(const std::string& posixTimezone, const std::string& ntpServer, time_t* syncedUnix) {
  if (syncedUnix == nullptr) {
    return false;
  }
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
  configTzTime(posixTimezone.c_str(), ntpServer.c_str());
  const unsigned long startedAt = millis();
  while (millis() - startedAt < kNtpSyncTimeoutMs) {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      const time_t now = time(nullptr);
      if (now >= kTrustedUnixTimeThreshold) {
        *syncedUnix = now;
        return true;
      }
    }
    delay(kNtpPollIntervalMs);
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

  // 验证写入：立即读取 RTC 并比对，防止 I2C 静默失败
  m5::rtc_datetime_t dt;
  if (M5.Rtc.getDateTime(&dt)) {
    std::tm readBack{};
    readBack.tm_year = dt.date.year - 1900;
    readBack.tm_mon = dt.date.month - 1;
    readBack.tm_mday = dt.date.date;
    readBack.tm_hour = dt.time.hours;
    readBack.tm_min = dt.time.minutes;
    readBack.tm_sec = dt.time.seconds;
    readBack.tm_isdst = 0;
    const char* previousTimezone = std::getenv("TZ");
    const std::string savedTimezone = previousTimezone != nullptr ? previousTimezone : "";
    setenv("TZ", "UTC0", 1);
    tzset();
    const time_t readBackUnix = std::mktime(&readBack);
    if (previousTimezone != nullptr) {
      setenv("TZ", savedTimezone.c_str(), 1);
    } else {
      unsetenv("TZ");
    }
    tzset();
    if (readBackUnix >= 0 && std::llabs(static_cast<long long>(readBackUnix) - static_cast<long long>(unixTime)) <= 1) {
      return true;
    }
  }
  return false;
}

TimeServiceDeps makeTimeDeps() {
  TimeServiceDeps deps{};
  deps.connectWifi = [](const std::string& ssid, const std::string& password) {
    return connectWifiPreservingAccessPoint(ssid, password);
  };
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
  // 配置成功后固定回到默认黄历视图，避免沿用 RTC 中残留的历史视图，
  // 保证设备重启后直接进入黄历界面。
  gRtcSavedView = SystemView::Almanac;
  return ConfigValidationResult{};
}

void renderHomeWithEnvironment() {
  gAlmanacView.render();
}

void renderCalendarWithEnvironment() {
  gCalendarView.render();
}

void renderCalendarWithOffset(int weekOffset) {
  gCalendarView.renderWithOffset(weekOffset);
}

void renderAlmanacWithOffset(int dayOffset) {
  gAlmanacView.renderWithOffset(dayOffset);
}

void renderWeatherWithEnvironment() {
  SetupConfig config = gConfigStore.loadSetupConfig();
  
  WEATHER_LOG("[Weather] Config values: Lat=%s, Lon=%s, SSID=%s\n", 
              config.latitude.c_str(), config.longitude.c_str(), config.wifiSsid.c_str());

  WeatherData data = makeCurrentWeatherData();
  bool hasConfig = !config.wifiSsid.empty() && !config.latitude.empty() && 
                   !config.longitude.empty() && !config.timezoneIana.empty();
  
  bool cacheIsFresh = false;
  if (applyCachedWeather(data.year, data.month, data.day, data)) {
    const time_t now = time(nullptr);
    if (now >= data.lastUpdate && (now - data.lastUpdate) < 600) { // 10 minutes cache freshness
      cacheIsFresh = true;
    }
  }

  if (!hasConfig || cacheIsFresh) {
    WEATHER_LOG("[Weather] Skip network request: hasConfig=%d, cacheIsFresh=%d\n", hasConfig, cacheIsFresh);
    const EnvironmentReading reading = readSht40Environment();
    if (reading.ok) {
      data.temperatureAvailable = true;
      data.temperatureCelsius = reading.temperatureCelsius;
      data.humidityAvailable = true;
      data.humidityPercent = reading.humidityPercent;
    }
    data.bottomCenterMessage = formatCurrentTimeHHMM();
    gWeatherView.render(data);
    return;
  }

  WeatherProviderDeps providerDeps;
  providerDeps.connectWifi = [](const std::string& ssid, const std::string& pass) {
    WEATHER_LOG("[Weather] Connecting to WiFi SSID: %s...\n", ssid.c_str());
    bool connected = connectWifiPreservingAccessPoint(ssid, pass, 10000);
    WEATHER_LOG("[Weather] WiFi connect result: %d\n", connected);
    return connected;
  };
  providerDeps.disconnectWifi = []() {
    WEATHER_LOG("[Weather] Disconnecting WiFi...\n");
#ifndef UNIT_TEST
    WiFi.disconnect(true);
#endif
  };
  providerDeps.httpGet = [](const std::string& url) -> std::pair<int, std::string> {
    WEATHER_LOG("[Weather] Performing HTTP GET on URL: %s\n", url.c_str());
#ifndef UNIT_TEST
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url.c_str());
    http.setConnectTimeout(5000);
    http.setTimeout(10000);
    int code = http.GET();
    WEATHER_LOG("[Weather] HTTP Code: %d\n", code);
    std::string payload = "";
    if (code == 200) {
      payload = http.getString().c_str();
      WEATHER_LOG("[Weather] Payload length: %d\n", (int)payload.length());
    } else {
      WEATHER_LOG("[Weather] HTTP failed, error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
    return {code, payload};
#else
    (void)url;
    return {500, ""};
#endif
  };

  WeatherResult result = fetchWeather(providerDeps, config.latitude, config.longitude, config.timezoneIana, config.wifiSsid, config.wifiPassword);
  
  WEATHER_LOG("[Weather] fetchWeather result: ok=%d, temp=%d, code=%d, max=%d, min=%d, rh=%d, app=%d\n",
              result.ok, result.currentTemp, result.weatherCode, result.tempMax, result.tempMin,
              result.relativeHumidity, result.apparentTemperature);

  data = makeCurrentWeatherData();
  if (result.ok) {
    data.valid = true;
    data.currentTemp = result.currentTemp;
    data.weatherCode = result.weatherCode;
    data.tempMax = result.tempMax;
    data.tempMin = result.tempMin;
    data.relativeHumidity = result.relativeHumidity;
    data.apparentTemperature = result.apparentTemperature;
    data.lastUpdate = static_cast<uint32_t>(time(nullptr));
    writeWeatherCache(data);
  } else {
    if (!applyCachedWeather(data.year, data.month, data.day, data)) {
      data.valid = false;
    }
  }
  
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }
  data.bottomCenterMessage = formatCurrentTimeHHMM();
  
  gWeatherView.render(data);
}

}  // namespace

#ifdef UNIT_TEST
bool syncNtpForTest(
    const std::string& posixTimezone,
    const std::string& ntpServer,
    time_t* syncedUnix) {
  return syncNtp(posixTimezone, ntpServer, syncedUnix);
}

bool writeRtcUtcForTest(time_t unixTime) {
  return writeRtcUtc(unixTime);
}

ConfigValidationResult saveSubmittedConfigForTest(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime) {
  return saveSubmittedConfig(config, manualDateTime);
}

void prepareEpdAfterWakeupForTest() {
  prepareEpdAfterWakeup();
}

void powerCycleEpdForTest() {
  powerCycleEpd();
}

void resetEpdControllerForTest() {
  resetEpdController();
}

void initRgbLedForTest() {
  initRgbLed();
}

void shutdownRgbLedForSleepForTest() {
  shutdownRgbLedForSleep();
}
#endif

void enterHomeDeepSleep(const HomeSleepRequest& request) {
  shutdownRgbLedForSleep();
  const auto wakeupGpio = static_cast<gpio_num_t>(request.wakeupGpio);
  pinMode(static_cast<std::uint8_t>(request.wakeupGpio), INPUT_PULLUP);
  if (esp_sleep_enable_timer_wakeup(request.timerWakeupUs) != ESP_OK) {
    return;
  }
  if (rtc_gpio_pullup_en(wakeupGpio) != ESP_OK) {
    return;
  }
  if (rtc_gpio_pulldown_dis(wakeupGpio) != ESP_OK) {
    return;
  }
  if (rtc_gpio_set_direction_in_sleep(wakeupGpio, RTC_GPIO_MODE_INPUT_ONLY) != ESP_OK) {
    return;
  }
  if (esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON) != ESP_OK) {
    return;
  }
  if (esp_sleep_enable_ext0_wakeup(wakeupGpio, request.wakeOnLow ? 0 : 1) != ESP_OK) {
    return;
  }
  // 等待睡眠画面（preSleepRender）刷新完成后直接断电墨水屏控制器。
  // 不调用 M5.Display.sleep()：ED2208 的 DEEP_SLEEP 命令缺少可靠的硬件复位唤醒路径，
  // 面板卡死后所有后续刷新都会失效（屏幕停留在旧画面）。
  M5.Display.waitDisplay();
  disableEpdPower();
  M5.Power.deepSleep(request.timerWakeupUs, false);
}

namespace {

void syncAllNetworkResources() {
  SetupConfig config = gConfigStore.loadSetupConfig();
  if (config.wifiSsid.empty()) {
    return;
  }

  if (!connectWifiPreservingAccessPoint(config.wifiSsid, config.wifiPassword, 10000)) {
    return;
  }

  // 1. 同步时间 (NTP) — 强制刷新时无条件执行，不依赖 autoRtcCorrection 设置
  time_t syncedUnix = 0;
  if (!config.ntpServer.empty()) {
    if (syncNtp(config.timezoneIana, config.ntpServer, &syncedUnix)) {
      writeRtcUtc(syncedUnix);
      if (gTimeService) {
        gTimeService->restoreSystemTimeFromRtc();
        gTimeService->applyTimezone(config.timezoneIana);
      }
    }
  }

  // 2. 同步天气 (fetchWeather)
  if (!config.latitude.empty() && !config.longitude.empty()) {
    WeatherProviderDeps providerDeps;
    providerDeps.connectWifi = [](const std::string&, const std::string&) {
      return true;
    };
    providerDeps.disconnectWifi = []() {
      // 保持连接，由外层统一断开
    };
    providerDeps.httpGet = [](const std::string& url) -> std::pair<int, std::string> {
#ifndef UNIT_TEST
      WiFiClientSecure client;
      client.setInsecure();  // 用户自托管设备，接受自签名/用户自定义 Webcal 地址的安全权衡
      HTTPClient http;
      http.begin(client, url.c_str());
      http.setConnectTimeout(5000);
      http.setTimeout(10000);
      int code = http.GET();
      std::string payload = "";
      if (code == 200) {
        payload = http.getString().c_str();
      }
      http.end();
      return {code, payload};
#else
      (void)url;
      return {500, ""};
#endif
    };

    WeatherResult result = fetchWeather(providerDeps, config.latitude, config.longitude, config.timezoneIana, config.wifiSsid, config.wifiPassword);
    if (result.ok) {
      WeatherData data = makeCurrentWeatherData();
      data.valid = true;
      data.currentTemp = result.currentTemp;
      data.weatherCode = result.weatherCode;
      data.tempMax = result.tempMax;
      data.tempMin = result.tempMin;
      data.relativeHumidity = result.relativeHumidity;
      data.apparentTemperature = result.apparentTemperature;
      data.lastUpdate = static_cast<uint32_t>(time(nullptr));
      writeWeatherCache(data);
    }
  }

  // 3. 同步 Webcal 节日数据 (ICS)
  if (!config.webcalUrl.empty()) {
    syncWebcalFestivals(config.webcalUrl, config.wifiSsid, config.wifiPassword);
  }

  // 4. 断开 WiFi
#ifndef UNIT_TEST
  WiFi.disconnect(true);
#endif
}

BootControllerDeps makeBootDeps() {
  BootControllerDeps deps{};
  deps.loadFlags = []() { return gConfigStore.loadBootFlags(); };
  deps.clearForceConfigOnNextBoot = []() { return gConfigStore.clearForceConfigOnNextBoot(); };
  deps.setForceConfigOnNextBoot = []() { return gConfigStore.setForceConfigOnNextBoot(true); };
  deps.startConfigPortal = []() {
    const std::string apSsid = makeApSsid();
    auto batteryProvider = []() -> std::string {
      const std::int32_t level = M5.Power.getBatteryLevel();
      if (level < 0) {
        return "";
      }
      return std::string("电量: ") + std::to_string(level) + "%";
    };
    gConfigPortal.begin(apSsid, gConfigStore.loadSetupConfig(), saveSubmittedConfig, batteryProvider);
    gConfigPortalRenderer.renderConfigPortal(apSsid, softApIpAddress());
#ifndef UNIT_TEST
    EPDDBG("config portal rendered busy=%d", gpio_get_level(GPIO_NUM_11));
#endif
  };
  deps.handleConfigPortalClient = []() { gConfigPortal.handleClient(); };
  deps.restoreSystemTimeFromRtc = []() {
    const SetupConfig config = gConfigStore.loadSetupConfig();
    if (!gTimeService->applyTimezone(config.timezoneIana)) {
      gTimeService->applyTimezone(defaultTimezone()->iana);
    }
    gTimeService->restoreSystemTimeFromRtc();
    // M5Unified 的 setSystemTimeFromRtc() 有 getenv/setenv 悬空指针 bug，
    // 会将 TZ 错误地恢复为 GMT0。必须在调用后重新应用时区。
    if (!gTimeService->applyTimezone(config.timezoneIana)) {
      gTimeService->applyTimezone(defaultTimezone()->iana);
    }
  };
  deps.renderAlmanac = renderHomeWithEnvironment;
  deps.renderCalendar = renderCalendarWithEnvironment;
  deps.renderCountdown = []() {
    gCountdownView.render();
  };
  deps.renderWeather = renderWeatherWithEnvironment;
  deps.renderCalendarWithOffset = renderCalendarWithOffset;
  deps.renderAlmanacWithOffset = renderAlmanacWithOffset;
  deps.resetCalendarView = []() { gCalendarView.resetOffset(); };
  deps.resetAlmanacView = []() { gAlmanacView.resetOffset(); };
  deps.getCalendarButtonClickCount = []() -> int {
    if (M5.BtnC.wasDecideClickCount()) {
      return static_cast<int>(M5.BtnC.getClickCount());
    }
    return 0;
  };
  deps.wasPrevWeekClicked = []() { return M5.BtnB.wasClicked(); };
  deps.wasNextWeekClicked = []() { return M5.BtnA.wasClicked(); };
  deps.updateButtons = []() { M5.update(); };
  deps.areSetupButtonsPressed = []() { return M5.BtnA.isPressed() && M5.BtnB.isPressed(); };
  deps.millis = []() { return millis(); };
  deps.restart = []() { ESP.restart(); };
  deps.currentTime = []() { return time(nullptr); };
  deps.loadSavedView = []() { return gRtcSavedView; };
  deps.saveCurrentView = [](homedeck::SystemView view) { gRtcSavedView = view; };
  deps.preSleepRender = [](homedeck::SystemView view) {
    if (view == homedeck::SystemView::Almanac) {
      gAlmanacView.renderSleep();
    } else if (view == homedeck::SystemView::Calendar) {
      gCalendarView.renderSleep();
    } else if (view == homedeck::SystemView::Countdown) {
      gCountdownView.renderSleep();
    } else if (view == homedeck::SystemView::Weather) {
      gWeatherView.renderSleep();
    }
  };
  deps.enterDeepSleep = enterHomeDeepSleep;

#ifndef UNIT_TEST
  deps.isWakeUpFromDeepSleep = []() { return esp_reset_reason() == ESP_RST_DEEPSLEEP; };
#else
  deps.isWakeUpFromDeepSleep = []() { return false; };
#endif
  deps.isBtnCPressed = []() { return M5.BtnC.isPressed(); };
  deps.syncNetworkResources = []() {
    // 绘制临时刷新提示，给用户强制刷新的视觉反馈
    M5Canvas& canvas = sprite();
    prepareScreen(canvas);
    if (canvas.loadFont(generated::kDeviceFontVlw)) {
      canvas.setTextColor(TFT_BLACK, TFT_WHITE);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.drawString("正在刷新数据...", kViewCenterX, kViewCenterY);
      canvas.unloadFont();
    }
    pushScreen(canvas);

    syncAllNetworkResources();
  };

  return deps;
}

}  // namespace

void appSetup() {
#ifndef UNIT_TEST
  EPDDBG("appSetup enter reset=%d", static_cast<int>(esp_reset_reason()));
  if (esp_reset_reason() != ESP_RST_POWERON) {
    i2cBusRecovery();
  }
#endif
  // 在 M5.begin 之前先复位墨水屏控制器：ED2208 退出深睡必须走 HWRESET，
  // 否则面板保持 BUSY_N 拉低，后续每次 busy 等待都会烧满 20s 超时。
  resetEpdController();
#ifndef UNIT_TEST
  EPDDBG("epd hwreset done busy=%d", gpio_get_level(GPIO_NUM_11));
#endif
  auto cfg = M5.config();
  cfg.clear_display = false;
  // HomeDeck 不使用板载音频；禁用可避免 M5.begin 在共享 I2C 总线上探测
  // 未供电的编解码芯片（ES8311/ES7210），产生大量 i2c write NACK。
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  M5.begin(cfg);
  M5.Display.setRotation(0);
#ifndef UNIT_TEST
  EPDDBG("M5.begin done board=%d gfxBoard=%d busy=%d",
         static_cast<int>(M5.getBoard()),
         static_cast<int>(M5.Display.getBoard()),
         gpio_get_level(GPIO_NUM_11));
#endif
  // PM1 必须在墨水屏供电控制之前就绪。
  initRgbLed();
#ifndef UNIT_TEST
  EPDDBG("pm1 ready=%d", gPm1Ready ? 1 : 0);
#endif
  powerCycleEpd();
#ifndef UNIT_TEST
  EPDDBG("epd power cycle done busy=%d", gpio_get_level(GPIO_NUM_11));
  if (esp_reset_reason() == ESP_RST_DEEPSLEEP) {
    prepareEpdAfterDeepSleep();
  } else {
    prepareEpdAfterWakeup();
  }
  EPDDBG("epd prep done busy=%d", gpio_get_level(GPIO_NUM_11));
#else
  prepareEpdAfterWakeup();
#endif
  gConfigStore.begin();
  gTimeService = std::make_unique<TimeService>(makeTimeDeps());
  gBootController = std::make_unique<BootController>(makeBootDeps());
#ifndef UNIT_TEST
  EPDDBG("before controller begin");
#endif
  gBootController->begin();
#ifndef UNIT_TEST
  EPDDBG("controller begin done");
#endif
}

void appLoop() {
  if (gBootController) {
    gBootController->update();
  }
}

}  // namespace homedeck
