# 周日历视图与 G1 强制刷新实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 HomeDeck 日历视图变更为周视图（4排显示，使用 12px 农历/节日专用字库，移除旧底部卡片），加入 Webcal 节日流式解析与本地 LittleFS 缓存，支持 G9/G10 上下翻周（范围限制 ±5周），并在日历/天气视图下支持 G1（BtnC）按键长按 3 秒进行强制联网刷新。

**Architecture:** 
1. 字体工具扩展：通过 Python 脚本从 `MiSans-Normal.ttf` 生成 12px 版本的 VLW 字库 `kDeviceLunarFontVlw`。
2. 配置参数扩展：在 NVS 及设置页新增 `webcalUrl` 字段，通过 `sync_preview.py` 自动注入 HTML 模板。
3. Webcal 解析库：实现基于 `Stream` / `WiFiClient` 逐行扫描的流式解析器，过滤出当前日期 ±40 天的事件，并在 LittleFS 中保存为极简 JSON `/webcal_cache.json`。
4. 交互层重构：利用 `M5.BtnC.isPressed()` 配合时间戳实现长按 3 秒强制刷新；将按月翻页重构为限制在 `[-5, 5]` 的按周翻页。

**Tech Stack:** C++17, PlatformIO, ESP32-S3, Arduino, M5Unified, LittleFS, ArduinoJson, Python3.

---

### Task 1: VLW 农历字体生成

**Files:**
- Modify: [generate_device_font.py](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/generate_device_font.py)

- [ ] **Step 1: 在生成脚本中新增 12px 字体资源**
  修改 `tools/generate_device_font.py`，新增名为 `device_lunar_font` 的 `FontResource`，属性前缀为 `kDeviceLunar`，字号为 `12px`，字体路径为 `MiSans-Normal.ttf`。
  
  在 `tools/generate_device_font.py` 约 275 行处，将：
  ```python
      resources = [
          FontResource("device_font", "kDevice", BODY_PIXEL_SIZE, body_codepoints, BODY_FONT),
  ```
  修改为：
  ```python
      resources = [
          FontResource("device_font", "kDevice", BODY_PIXEL_SIZE, body_codepoints, BODY_FONT),
          FontResource(
              "device_lunar_font",
              "kDeviceLunar",
              12,
              body_codepoints,
              ROOT / "fonts" / "misans" / "MiSans-Normal.ttf",
          ),
  ```

- [ ] **Step 2: 运行字体生成脚本**
  运行：`python3 tools/generate_device_font.py`
  验证：脚本成功生成 `src/generated/device_font_vlw.h` 和 `device_font_vlw.cpp`，并且其中包含 `kDeviceLunarFontVlw` 字体资源数组及 `kDeviceLunarFontPixelSize = 12` 的字号定义。

- [ ] **Step 3: Commit**
  ```bash
  git add tools/generate_device_font.py src/generated/device_font_vlw.h src/generated/device_font_vlw.cpp
  git commit -m "feat: add 12px MiSans-Normal vlw font for lunar calendar"
  ```

---

### Task 2: Web Portal 增加 Webcal 配置

**Files:**
- Modify: [preview.html](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/scratch/preview.html)
- Modify: [sync_preview.py](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/sync_preview.py)

- [ ] **Step 1: 在 HTML 模板中新增 Webcal 地址输入框**
  在 `scratch/preview.html` 的 `地理位置坐标` 这一 section 之前（约 490 行），增加“节日 Webcal 配置”。
  修改 `scratch/preview.html` 如下：
  ```html
        <div class="form-group">
          <label for="webcal_url">节日 Webcal 地址</label>
          <input id="webcal_url" name="webcal_url" value="" placeholder="例如: webcal://example.com/calendar.ics (留空不显示)">
        </div>
      </section>

      <section class="card">
        <h2>
          <svg class="section-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z" />
  ```

- [ ] **Step 2: 修改同步脚本注册占位符**
  修改 `tools/sync_preview.py`，注册 `{{WEBCAL_URL}}` 占位符。
  在 `tools/sync_preview.py` 约 44 行后插入：
  ```python
      content = re.sub(r'id="webcal_url"\s+name="webcal_url"\s+value=""', 'id="webcal_url" name="webcal_url" value="{{WEBCAL_URL}}"', content)
  ```
  同时在 `required_placeholders` 列表中增加 `'{{WEBCAL_URL}}'`：
  ```python
      required_placeholders = [
          '{{AP_SSID}}', '{{WIFI_GRID_ITEMS}}', '{{TIMEZONE_OPTION_ITEMS}}',
          '{{ERROR_CONTAINER_STYLE}}', '{{ERROR_MESSAGE}}', '{{WIFI_SSID}}',
          '{{WIFI_PASSWORD}}', '{{NTP_SERVER}}', '{{LATITUDE}}', '{{LONGITUDE}}',
          '{{AUTO_RTC_CHECKED}}', '{{AUTO_RTC_DISABLED}}',
          '{{BATTERY_INFO_STYLE}}', '{{BATTERY_INFO}}', '{{WEBCAL_URL}}'
      ]
  ```

- [ ] **Step 3: 运行 HTML 同步脚本**
  运行：`python3 tools/sync_preview.py`
  验证：脚本运行成功，且 `src/generated/setup_page_html.h` 中已正确包含了 webcal_url 输入框及其对应的占位符。

- [ ] **Step 4: Commit**
  ```bash
  git add scratch/preview.html tools/sync_preview.py src/generated/setup_page_html.h
  git commit -m "feat: add webcalUrl input form on configuration web page"
  ```

---

### Task 3: 后端配置存储与加载支持

**Files:**
- Modify: [config_types.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/config_types.h)
- Modify: [config_store.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/config_store.cpp)
- Modify: [setup_page.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/setup_page.cpp)
- Modify: [config_portal.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/config_portal.cpp)

- [ ] **Step 1: 在 SetupConfig 结构体中增加字段**
  修改 `src/config/config_types.h`：
  ```cpp
  struct SetupConfig {
    std::string wifiSsid;
    std::string wifiPassword;
    std::string timezoneIana = "Asia/Shanghai";
    bool autoRtcCorrection = false;
    std::string ntpServer = "pool.ntp.org";
    std::string latitude;
    std::string longitude;
    std::string webcalUrl; // 新增
  };
  ```

- [ ] **Step 2: 在 ConfigStore 存取逻辑中加入 webcal_url**
  修改 `src/config/config_store.cpp` 的 `loadSetupConfig` 和 `saveSetupConfig`：
  在 `kWifiPassword = "wifi_pass";` 附近添加：
  ```cpp
  constexpr const char* kWebcalUrl = "webcal_url";
  ```
  在 `loadSetupConfig()` 中：
  ```cpp
  config.webcalUrl = prefs_.getString(kWebcalUrl, "").c_str();
  ```
  在 `saveSetupConfig()` 中：
  ```cpp
  const bool webcalOk = prefs_.putString(kWebcalUrl, config.webcalUrl.c_str()) > 0 || config.webcalUrl.empty();
  return stringsOk && passwordOk && timezoneOk && ntpOk && boolOk && latOk && lonOk && webcalOk;
  ```

- [ ] **Step 3: 设置页 HTML 替换 webcal 占位符**
  修改 `src/config/setup_page.cpp` 的 `buildSetupPageHtml`：
  在 `placeholder == "{{LONGITUDE}}"` 的分支后插入：
  ```cpp
  } else if (placeholder == "{{WEBCAL_URL}}") {
    out << htmlEscape(values.webcalUrl);
  ```

- [ ] **Step 4: 配置保存接口读取 POST 数据**
  修改 `src/config/config_portal.cpp` 的解析逻辑：
  在 `setup_form` 处理 POST 请求的地方（约 85 行）：
  ```cpp
  if (server_.hasArg("webcal_url")) {
    config.webcalUrl = server_.arg("webcal_url").c_str();
  }
  ```

- [ ] **Step 5: 编写配置验证的 native 测试**
  修改 `test/native/test_config_store/test_main.cpp` (如果该文件存在)，或者在现有 native 测试中进行验证。先用 `pio test -e native` 验证原测试是否依然编译通过。

- [ ] **Step 6: Commit**
  ```bash
  git add src/config/config_types.h src/config/config_store.cpp src/config/setup_page.cpp src/config/config_portal.cpp
  git commit -m "feat: add config store and portal support for webcalUrl"
  ```

---

### Task 4: Webcal ics 流式解析器核心类

**Files:**
- Create: [webcal_provider.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/providers/webcal_provider.h)
- Create: [webcal_provider.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/providers/webcal_provider.cpp)
- Create: [test_webcal_provider.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_webcal_provider/test_main.cpp)

- [ ] **Step 1: 创建 webcal_provider.h**
  编写 `src/providers/webcal_provider.h`：
  ```cpp
  #pragma once
  
  #include <string>
  #include <map>
  #include <ctime>
  
  #if defined(ARDUINO)
  #include <Stream.h>
  #else
  // Native 宿主机环境下的 Stream 模拟类
  #include <sstream>
  class Stream {
   public:
    virtual ~Stream() = default;
    virtual std::string readStringUntil(char terminator) = 0;
    virtual bool available() = 0;
    virtual int peek() = 0;
  };
  
  class StringStream : public Stream {
   public:
    explicit StringStream(const std::string& data) : ss_(data) {}
    std::string readStringUntil(char terminator) override {
      std::string line;
      std::getline(ss_, line, terminator);
      return line;
    }
    bool available() override { return !ss_.eof(); }
    int peek() override { return ss_.peek(); }
   private:
    std::stringstream ss_;
  };
  #endif
  
  namespace homedeck {
  
  // 在给定时间 localNow 前后 40 天范围内，解析 Stream 里的 ICS 节日信息
  bool parseIcsStream(Stream& stream, const std::tm& localNow, std::map<std::string, std::string>& outFestivals);
  
  // 联网同步 webcal 节日数据并存入 LittleFS 缓存文件
  bool syncWebcalFestivals(const std::string& webcalUrl, const std::string& wifiSsid, const std::string& wifiPassword);
  
  // 从 LittleFS 读取缓存的节日并填入 map
  bool loadCachedFestivals(std::map<std::string, std::string>& outFestivals);
  
  }  // namespace homedeck
  ```

- [ ] **Step 2: 创建 webcal_provider.cpp**
  编写 `src/providers/webcal_provider.cpp`：
  ```cpp
  #include "providers/webcal_provider.h"
  #include <ArduinoJson.h>
  #include <LittleFS.h>
  #include <algorithm>
  #include <cstdio>
  
  #if !defined(ARDUINO)
  // 宿主机环境下模拟 LittleFS
  #endif
  
  namespace homedeck {
  namespace {
  
  constexpr const char* kWebcalCachePath = "/webcal_cache.json";
  
  // 去除 \r\n 并 trim
  std::string trimString(const std::string& str) {
    if (str.empty()) return "";
    size_t first = 0;
    while (first < str.size() && (std::isspace(str[first]) || str[first] == '\r')) {
      first++;
    }
    size_t last = str.size();
    while (last > first && (std::isspace(str[last - 1]) || str[last - 1] == '\r')) {
      last--;
    }
    return str.substr(first, last - first);
  }
  
  // 计算日期差（简易）
  int daysDiff(int y1, int m1, int d1, int y2, int m2, int d2) {
    std::tm t1{};
    t1.tm_year = y1 - 1900;
    t1.tm_mon = m1 - 1;
    t1.tm_mday = d1;
    std::tm t2{};
    t2.tm_year = y2 - 1900;
    t2.tm_mon = m2 - 1;
    t2.tm_mday = d2;
    std::time_t time1 = std::mktime(&t1);
    std::time_t time2 = std::mktime(&t2);
    return static_cast<int>(std::difftime(time1, time2) / (24 * 3600));
  }
  
  }  // namespace
  
  bool parseIcsStream(Stream& stream, const std::tm& localNow, std::map<std::string, std::string>& outFestivals) {
    std::string currentSummary;
    std::string currentStart;
    bool inVevent = false;
    
    // 用较小的数据段读取，防止 OOM
    while (stream.available() || stream.peek() != -1) {
      std::string rawLine = stream.readStringUntil('\n');
      std::string line = trimString(rawLine);
      if (line.empty()) continue;
      
      if (line == "BEGIN:VEVENT") {
        inVevent = true;
        currentSummary.clear();
        currentStart.clear();
      } else if (line == "END:VEVENT") {
        if (inVevent && !currentStart.empty() && !currentSummary.empty()) {
          // currentStart 格式应为 YYYYMMDD，转换为 YYYY-MM-DD
          if (currentStart.length() >= 8) {
            int evYear = std::stoi(currentStart.substr(0, 4));
            int evMonth = std::stoi(currentStart.substr(4, 2));
            int evDay = std::stoi(currentStart.substr(6, 2));
            
            // 过滤前后 40 天
            int diff = daysDiff(evYear, evMonth, evDay, localNow.tm_year + 1900, localNow.tm_mon + 1, localNow.tm_mday);
            if (diff >= -40 && diff <= 40) {
              char dateStr[11];
              std::snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", evYear, evMonth, evDay);
              outFestivals[dateStr] = currentSummary;
            }
          }
        }
        inVevent = false;
      } else if (inVevent) {
        if (line.rfind("DTSTART", 0) == 0) {
          size_t pos = line.find(':');
          if (pos != std::string::npos) {
            currentStart = line.substr(pos + 1);
            if (currentStart.length() >= 8) {
              currentStart = currentStart.substr(0, 8);
            }
          }
        } else if (line.rfind("SUMMARY", 0) == 0) {
          size_t pos = line.find(':');
          if (pos != std::string::npos) {
            currentSummary = line.substr(pos + 1);
          }
        }
      }
    }
    return true;
  }
  
  // 联网拉取并在 LittleFS 中保存 JSON 缓存
  bool syncWebcalFestivals(const std::string& webcalUrl, const std::string& wifiSsid, const std::string& wifiPassword) {
  #if defined(ARDUINO)
    if (webcalUrl.empty()) {
      LittleFS.begin();
      LittleFS.remove(kWebcalCachePath);
      LittleFS.end();
      return true;
    }
    
    // 连接 WiFi (复用 connectWifiPreservingAccessPoint)
    extern bool connectWifiPreservingAccessPoint(const std::string& ssid, const std::string& password, unsigned long timeoutMs = 10000);
    if (!connectWifiPreservingAccessPoint(wifiSsid, wifiPassword)) {
      return false;
    }
    
    // 替换 webcal:// 协议头为 https:// 或 http://
    std::string url = webcalUrl;
    if (url.rfind("webcal://", 0) == 0) {
      url = "https://" + url.substr(9);
    }
    
    #include <WiFiClientSecure.h>
    #include <HTTPClient.h>
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url.c_str());
    int code = http.GET();
    if (code != 200) {
      http.end();
      WiFi.disconnect(true);
      return false;
    }
    
    std::time_t now = std::time(nullptr);
    std::tm buf{};
    std::tm* local = localtime_r(&now, &buf);
    std::map<std::string, std::string> festivals;
    
    WiFiClient* stream = http.getStreamPtr();
    // 由于 Arduino Stream 和 std::string 交互接口，使用自定义包装类或者直接读 Stream
    struct ArduinoStreamWrapper : public Stream {
      WiFiClient* s;
      explicit ArduinoStreamWrapper(WiFiClient* ws) : s(ws) {}
      std::string readStringUntil(char terminator) override {
        return s->readStringUntil(terminator).c_str();
      }
      bool available() override { return s->available() > 0; }
      int peek() override { return s->peek(); }
    } wrapper(stream);
    
    parseIcsStream(wrapper, *local, festivals);
    http.end();
    WiFi.disconnect(true);
    
    // 写入 LittleFS JSON
    if (!LittleFS.begin()) {
      return false;
    }
    File f = LittleFS.open(kWebcalCachePath, "w");
    if (!f) {
      LittleFS.end();
      return false;
    }
    JsonDocument doc;
    for (const auto& [k, v] : festivals) {
      doc[k] = v;
    }
    serializeJson(doc, f);
    f.close();
    LittleFS.end();
    return true;
  #else
    (void)webcalUrl; (void)wifiSsid; (void)wifiPassword;
    return false;
  #endif
  }
  
  bool loadCachedFestivals(std::map<std::string, std::string>& outFestivals) {
    outFestivals.clear();
  #if defined(ARDUINO)
    if (!LittleFS.begin()) {
      return false;
    }
    if (!LittleFS.exists(kWebcalCachePath)) {
      LittleFS.end();
      return true; // 允许无缓存
    }
    File f = LittleFS.open(kWebcalCachePath, "r");
    if (!f) {
      LittleFS.end();
      return false;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
      f.close();
      LittleFS.end();
      return false;
    }
    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair p : obj) {
      outFestivals[p.key().c_str()] = p.value().as<std::string>();
    }
    f.close();
    LittleFS.end();
    return true;
  #else
    return false;
  #endif
  }
  
  }  // namespace homedeck
  ```

- [ ] **Step 3: 编写 Native 单元测试验证 ICS 解析**
  创建目录 `test/native/test_webcal_provider` 并编写 `test/native/test_webcal_provider/test_main.cpp`：
  ```cpp
  #include <unity.h>
  #include "providers/webcal_provider.h"
  
  void test_parse_ics_stream() {
    std::string ics_data = 
      "BEGIN:VCALENDAR\n"
      "VERSION:2.0\n"
      "BEGIN:VEVENT\n"
      "DTSTART;VALUE=DATE:20260601\n"
      "SUMMARY:儿童节\n"
      "END:VEVENT\n"
      "BEGIN:VEVENT\n"
      "DTSTART:20260715T120000Z\n"
      "SUMMARY:期中考\n"
      "END:VEVENT\n"
      "END:VCALENDAR\n";
      
    StringStream stream(ics_data);
    std::tm localNow{};
    localNow.tm_year = 2026 - 1900;
    localNow.tm_mon = 6 - 1; // 6月
    localNow.tm_mday = 4; // 6月4日
    
    std::map<std::string, std::string> festivals;
    bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
    
    TEST_ASSERT_TRUE(ok);
    // 6月1日 (相差 3 天，在 ±40 天内) 应该被解析出
    TEST_ASSERT_EQUAL_STRING("儿童节", festivals["2026-06-01"].c_str());
    // 7月15日 (相差 41 天，在 ±40 天外) 应该被过滤掉
    TEST_ASSERT_TRUE(festivals.find("2026-07-15") == festivals.end());
  }
  
  int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_ics_stream);
    return UNITY_END();
  }
  ```

- [ ] **Step 4: 运行 Native 测试**
  运行：`pio test -e native -f test_webcal_provider`
  验证：测试编译通过，单元测试 `test_parse_ics_stream` 顺利通过 (PASS)。

- [ ] **Step 5: Commit**
  ```bash
  git add src/providers/webcal_provider.h src/providers/webcal_provider.cpp test/native/test_webcal_provider/test_main.cpp
  git commit -m "feat: implement streaming ICS webcal parser and native tests"
  ```

---

### Task 5: 整合后台静默同步与长按刷新按键机制

**Files:**
- Modify: [boot_controller.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/app/boot_controller.h)
- Modify: [boot_controller.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/app/boot_controller.cpp)
- Modify: [app_runtime.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/app/app_runtime.cpp)

- [ ] **Step 1: 在 BootControllerDeps 中注入强制手动刷新和长按判断依赖**
  修改 `src/app/boot_controller.h`，在 `BootControllerDeps` 结构体中添加：
  ```cpp
  std::function<bool()> wasBtnCLongPressed; // 返回是否长按 G1 超过了 3 秒
  std::function<void()> forceNetworkSync;   // 强制联网刷新时间、天气、Webcal
  ```
  同时将 `renderCalendarWithOffset` 修改为使用 `weekOffset` 的含义。

- [ ] **Step 2: 在 BootController::update() 中捕捉长按刷新逻辑**
  修改 `src/app/boot_controller.cpp` 的 `update()` 循环：
  在 `updateSetupShortcut(now);` 之后、BtnC 单/双击检测之前，插入：
  ```cpp
  if (deps_.wasBtnCLongPressed && deps_.wasBtnCLongPressed()) {
    if (viewManager_) {
      SystemView current = viewManager_->currentView();
      if (current == SystemView::Calendar || current == SystemView::Weather) {
        // 重置偏移量为 0，防止刷新后停在偏离页面
        calendarMonthOffset_ = 0; 
        almanacDayOffset_ = 0;
        if (deps_.resetCalendarView) deps_.resetCalendarView();
        if (deps_.resetAlmanacView) deps_.resetAlmanacView();
        
        if (deps_.forceNetworkSync) {
          deps_.forceNetworkSync();
        }
        lastActivityMs_ = now;
        return; // 手动刷新并重绘后立即返回
      }
    }
  }
  ```

- [ ] **Step 3: 在 app_runtime.cpp 中实现 G1 长按检测与 forceNetworkSync 动作**
  修改 `src/app/app_runtime.cpp` 的 `makeBootDeps()` 方法：
  ```cpp
  deps.wasBtnCLongPressed = []() -> bool {
    static bool triggered = false;
    if (M5.BtnC.isPressed()) {
      if (M5.BtnC.pressedFor(3000)) {
        if (!triggered) {
          triggered = true;
          return true;
        }
      }
    } else {
      triggered = false;
    }
    return false;
  };
  
  deps.forceNetworkSync = []() {
    SetupConfig config = gConfigStore.loadSetupConfig();
    if (config.wifiSsid.empty()) return;
    
    // 绘制临时提示
    M5Canvas& canvas = sprite();
    prepareScreen(canvas);
    if (canvas.loadFont(generated::kDeviceFontVlw)) {
      canvas.setTextColor(TFT_BLACK, TFT_WHITE);
      canvas.setTextDatum(textdatum_t::middle_center);
      canvas.drawString("正在强制刷新数据...", kViewCenterX, kViewCenterY);
      canvas.unloadFont();
    }
    pushScreen(canvas);
    
    // 强制同步时间 (NTP)
    time_t synced = 0;
    if (connectWifiPreservingAccessPoint(config.wifiSsid, config.wifiPassword)) {
      syncNtp(config.timezoneIana, config.ntpServer, &synced);
      if (synced > 0) {
        writeRtcUtc(synced);
        M5.Rtc.setSystemTimeFromRtc();
      }
      WiFi.disconnect(true);
    }
    
    // 强制同步 Webcal 节日
    syncWebcalFestivals(config.webcalUrl, config.wifiSsid, config.wifiPassword);
    
    // 天气同步由 renderWeather 自动触发
    renderWeatherWithEnvironment();
  };
  ```

- [ ] **Step 4: 在深夜休眠醒来（NTP同步）的地方加入 Webcal 自动更新**
  （因为系统每天深夜会通过 timer 唤醒同步 NTP，此时也顺便拉取节日数据）
  修改 `src/app/app_runtime.cpp` 中执行时间纠正或 NTP 同步的逻辑（大约在 `saveSubmittedConfig` 或者唤醒自动校验时间处），在成功联网后调用 `syncWebcalFestivals` 刷新数据。

- [ ] **Step 5: 验证编译**
  运行：`pio run -e m5stack-papercolor`
  验证：项目编译无误。

- [ ] **Step 6: Commit**
  ```bash
  git add src/app/boot_controller.h src/app/boot_controller.cpp src/app/app_runtime.cpp
  git commit -m "feat: integrate G1 long press 3s force refresh event loop"
  ```

---

### Task 6: 重构为周历视图渲染逻辑

**Files:**
- Modify: [calendar_view.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/views/calendar_view.h)
- Modify: [calendar_view.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/views/calendar_view.cpp)

- [ ] **Step 1: 更新 CalendarData 结构体**
  修改 `src/views/calendar_view.h`：
  移除无用的未来节日和特别日期字段，新增代表周内 7 天的每日详细数据。
  ```cpp
  struct WeeklyDayData {
    int day = 0;
    int month = 0;
    int year = 0;
    std::string lunarDate;
    std::string festivalOrTerm;
    bool isToday = false;
  };

  struct CalendarData {
    int year = 0;
    int month = 0;
    int currentWeekOfYear = 0;
    WeeklyDayData days[7]; // 周一至周日
    bool temperatureAvailable = false;
    float temperatureCelsius = 0.0f;
    bool humidityAvailable = false;
    float humidityPercent = 0.0f;
    std::string bottomCenterMessage;
  };
  ```

- [ ] **Step 2: 重构 makeCalendarData 方法为周日历计算**
  在 `src/views/calendar_view.cpp` 中重写 `makeCalendarData`：
  根据 `localTime` 和 `weekOffset_` 计算周首（星期一）的 Unix 时间戳，并累加 86400 秒得出周内这 7 天具体的年月日及星期。然后对每一天读取年鉴农历，并从 `/webcal_cache.json` 读取缓存的 Webcal 节日。
  
  ```cpp
  CalendarData makeCalendarData(const std::tm& localTime, int weekOffset) {
    CalendarData data{};
    std::time_t baseTime = std::mktime(const_cast<std::tm*>(&localTime));
    
    // 计算 localTime 是星期几 (0-6, 0是周日)
    int weekday = localTime.tm_wday;
    int daysToMonday = (weekday == 0) ? -6 : (1 - weekday);
    
    // 偏移到目标周的周一
    baseTime += (daysToMonday + weekOffset * 7) * 24 * 3600;
    
    std::map<std::string, std::string> webcalFestivals;
    loadCachedFestivals(webcalFestivals);
    AlmanacProvider almanacProvider;
    
    std::tm todayBuf{};
    std::time_t todayUnix = std::time(nullptr);
    std::tm* todayTm = localtime_r(&todayUnix, &todayBuf);
    
    for (int i = 0; i < 7; ++i) {
      std::time_t dayTime = baseTime + i * 24 * 3600;
      std::tm dayTm{};
      localtime_r(&dayTime, &dayTm);
      
      data.days[i].year = dayTm.tm_year + 1900;
      data.days[i].month = dayTm.tm_mon + 1;
      data.days[i].day = dayTm.tm_mday;
      data.days[i].isToday = (todayTm && todayTm->tm_year == dayTm.tm_year &&
                              todayTm->tm_mon == dayTm.tm_mon &&
                              todayTm->tm_mday == dayTm.tm_mday);
      
      // 读取农历
      AlmanacDayData almanac{};
      if (almanacProvider.lookup(data.days[i].year, data.days[i].month, data.days[i].day, &almanac)) {
        data.days[i].lunarDate = almanac.lunarDate;
        data.days[i].festivalOrTerm = almanac.solarTerm; // 备用节气
      }
      
      // 读取 Webcal 节日
      char dateStr[11];
      std::snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", data.days[i].year, data.days[i].month, data.days[i].day);
      if (webcalFestivals.find(dateStr) != webcalFestivals.end()) {
        data.days[i].festivalOrTerm = webcalFestivals[dateStr];
      }
    }
    
    // 填充顶部年份/月份/当前第几周信息
    std::tm mondayTm{};
    localtime_r(&baseTime, &mondayTm);
    data.year = mondayTm.tm_year + 1900;
    data.month = mondayTm.tm_mon + 1;
    
    char weekBuf[8];
    std::strftime(weekBuf, sizeof(weekBuf), "%W", &mondayTm);
    data.currentWeekOfYear = std::stoi(weekBuf) + 1;
    
    return data;
  }
  ```

- [ ] **Step 3: 实现周历 7 列网格渲染绘制**
  重构 `CalendarView::render` 渲染算法：
  *   顶部显示：`2026年6月`，中间显示 `第23周`，右侧显示当天星期。
  *   使用 7 列计算每一列的位置：
      `int colWidth = 376 / 7;`
  *   第一排：星期文字（使用原 `kDevice` 20px 字体绘制：“一”、“二”……）。
  *   第二排：日期数字。如果该天是当天，用 `canvas.fillSmoothCircle(cx, cy, 18, TFT_BLACK)` 并将文字设为 `TFT_WHITE`。
  *   第三排：农历日期。使用 `kDeviceLunarFontVlw` (12px 字体) 进行中文字绘制。
  *   第四排：节日（或节气）字符串。使用 `kDeviceLunarFontVlw` (12px 字体) 绘制。如果字串长度超出单列容纳（利用 `canvas.textWidth()` 检测），可以使用截断逻辑或折行展示。
  *   底部：完全删除原先绘制在 sepY 附近的未来特别日子相关组件，直接画 sep 栏并调用 `drawBottomStatusBar`。

- [ ] **Step 4: 编译并修复错误**
  运行：`pio run -e m5stack-papercolor`
  验证：代码构建顺利通过，无重绘或字体未定义报错。

- [ ] **Step 5: Commit**
  ```bash
  git add src/views/calendar_view.h src/views/calendar_view.cpp
  git commit -m "feat: complete redraw of calendar view to 4-row weekly grid layout"
  ```

---

### Task 7: 按周翻页限制与双击归零归并

**Files:**
- Modify: [boot_controller.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/app/boot_controller.cpp)

- [ ] **Step 1: 替换为按周翻页的偏移度**
  将 `boot_controller.cpp` 中 `kCalendarMonthOffsetMin = -120` 和 `kCalendarMonthOffsetMax = 120` 替换为周翻页限制：
  ```cpp
  constexpr int kCalendarWeekOffsetMin = -5;
  constexpr int kCalendarWeekOffsetMax = 5;
  ```
  在 `BootController::update()` 里面的日历翻页检测中：
  ```cpp
  // 2. 检测日历翻页（仅在 Calendar 视图）
  if (viewManager_ && viewManager_->currentView() == SystemView::Calendar) {
    bool calendarUpdated = false;
    if (deps_.wasPrevMonthClicked && deps_.wasPrevMonthClicked()) {
      if (calendarMonthOffset_ > kCalendarWeekOffsetMin) {
        calendarMonthOffset_--; // 这里复用 calendarMonthOffset_ 变量表示 weekOffset_
        calendarUpdated = true;
      }
    } else if (deps_.wasNextMonthClicked && deps_.wasNextMonthClicked()) {
      if (calendarMonthOffset_ < kCalendarWeekOffsetMax) {
        calendarMonthOffset_++;
        calendarUpdated = true;
      }
    }
    if (calendarUpdated && deps_.renderCalendarWithOffset) {
      deps_.renderCalendarWithOffset(calendarMonthOffset_);
      lastActivityMs_ = now;
    }
  }
  ```

- [ ] **Step 2: 验证 Native 单元测试**
  运行：`pio test -e native`
  验证：跑通全部 BootController 和时间相关的 Native 测试。如果有因为 Offset 边界定义变更导致原测试失败的，按需更新原测试用例中的边界断言值。

- [ ] **Step 3: Commit**
  ```bash
  git add src/app/boot_controller.cpp
  git commit -m "feat: restrict weekly paging to +/- 5 weeks and bind double click home"
  ```
