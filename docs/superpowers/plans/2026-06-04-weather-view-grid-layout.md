# 天气视图网格化布局（变体 C-2）实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在天气视图（WeatherView）中添加从 API 获取的湿度、体感温度，以及数据最后更新时间，并采用变体 C-2 的两层网格布局对其进行水平、垂直居中渲染。

**Architecture:**
1. 修改天气数据提供者 `fetchWeather` API 请求及解析，新增体感温度与湿度。
2. 扩展天气数据结构 `WeatherData`、缓存结构 `WeatherCache` 以及结果结构 `WeatherResult`。
3. 调整 `WeatherView::render` 坐标，整体垂直居中，使用 `kDeviceFontVlw` 小字体分两行网格绘制高低温、体感、湿度和更新时间。
4. 补充和修正对应的单元测试以覆盖新增字段及新的布局渲染输出。

**Tech Stack:** C++, PlatformIO, M5Unified, ArduinoJson, Unity (单元测试框架)

---

### Task 1: 扩展天气提供者获取体感温度和湿度

**Files:**
- Modify: [weather_provider.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/providers/weather_provider.h)
- Modify: [weather_provider.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/providers/weather_provider.cpp)
- Test: [test_main.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_weather_provider/test_main.cpp)

- [ ] **Step 1: 修改测试用例以包含体感温度和湿度的断言**
  在 `test/native/test_weather_provider/test_main.cpp` 的 `test_fetch_weather_success` 用例中，修改模拟的 JSON 响应，并增加断言检测。
  修改 `test_fetch_weather_success` 代码如下：
  ```cpp
  void test_fetch_weather_success() {
    homedeck::WeatherProviderDeps deps;
    bool wifiConnected = false;
    bool wifiDisconnected = false;
    
    deps.connectWifi = [&](const std::string& ssid, const std::string& pass) {
      wifiConnected = (ssid == "MySSID" && pass == "MyPass");
      return wifiConnected;
    };
    deps.disconnectWifi = [&]() {
      wifiDisconnected = true;
    };
    deps.httpGet = [&](const std::string& url) -> std::pair<int, std::string> {
      if (url.find("latitude=25.78") != std::string::npos &&
          url.find("longitude=113.02") != std::string::npos &&
          url.find("timezone=Asia%2FShanghai") != std::string::npos &&
          url.find("relative_humidity_2m") != std::string::npos &&
          url.find("apparent_temperature") != std::string::npos) {
        std::string mockJson = R"({
          "current": {
            "temperature_2m": 33.6,
            "weather_code": 3,
            "relative_humidity_2m": 65,
            "apparent_temperature": 34.2
          },
          "daily": {
            "weather_code": [3],
            "temperature_2m_max": [35.2],
            "temperature_2m_min": [26.1]
          }
        })";
        return {200, mockJson};
      }
      return {404, ""};
    };
    
    auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "MySSID", "MyPass");
    
    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_TRUE(wifiConnected);
    TEST_ASSERT_TRUE(wifiDisconnected);
    TEST_ASSERT_EQUAL(33, result.currentTemp);
    TEST_ASSERT_EQUAL(3, result.weatherCode);
    TEST_ASSERT_EQUAL(35, result.tempMax);
    TEST_ASSERT_EQUAL(26, result.tempMin);
    TEST_ASSERT_EQUAL(65, result.relativeHumidity);
    TEST_ASSERT_EQUAL(34, result.apparentTemperature);
  }
  ```

- [ ] **Step 2: 运行测试并确保其编译失败（或断言失败）**
  运行: `pio test -e native -f test_weather_provider`
  预期: 编译失败，提示 `struct WeatherResult` 没有 `relativeHumidity` 成员。

- [ ] **Step 3: 修改 `weather_provider.h` 中的 `WeatherResult` 结构**
  在 `src/providers/weather_provider.h` 的 `WeatherResult` 中添加新字段：
  ```cpp
  struct WeatherResult {
    bool ok = false;
    int currentTemp = 0;
    int weatherCode = 0;
    int tempMax = 0;
    int tempMin = 0;
    int relativeHumidity = 0;      // 相对湿度 (%)
    int apparentTemperature = 0;   // 体感温度 (°C)
  };
  ```

- [ ] **Step 4: 修改 `weather_provider.cpp` 解析 API 参数**
  1. 在 `fetchWeather` 中的 URL 参数 current 里，追加 `relative_humidity_2m` 和 `apparent_temperature`：
  ```cpp
    std::string url = "https://api.open-meteo.com/v1/forecast?latitude=" + latitude +
                      "&longitude=" + longitude +
                      "&current=temperature_2m,weather_code,relative_humidity_2m,apparent_temperature" +
                      "&daily=weather_code,temperature_2m_max,temperature_2m_min" +
                      "&forecast_days=1" +
                      "&timezone=" + encodedTz;
  ```
  2. 在解析 JSON 时，校验并获取新增的字段：
  ```cpp
    if (current["temperature_2m"].isNull() || current["weather_code"].isNull() ||
        current["relative_humidity_2m"].isNull() || current["apparent_temperature"].isNull()) {
      return result;
    }
    
    result.currentTemp = static_cast<int>(current["temperature_2m"].as<float>());
    result.weatherCode = current["weather_code"].as<int>();
    result.relativeHumidity = static_cast<int>(current["relative_humidity_2m"].as<float>());
    result.apparentTemperature = static_cast<int>(current["apparent_temperature"].as<float>());
  ```

- [ ] **Step 5: 重新运行提供者测试，并确保通过**
  运行: `pio test -e native -f test_weather_provider`
  预期: 所有 `test_weather_provider` 的测试均通过 (PASS)。

- [ ] **Step 6: Git Commit**
  ```bash
  git add src/providers/weather_provider.h src/providers/weather_provider.cpp test/native/test_weather_provider/test_main.cpp
  git commit -m "feat(weather): retrieve relative humidity and apparent temperature from Open-Meteo API"
  ```

---

### Task 2: 扩展天气视图数据结构与缓存支持

**Files:**
- Modify: [weather_view.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/views/weather_view.h)
- Modify: [weather_view.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/views/weather_view.cpp:48-75)
- Test: [test_main.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_weather_view/test_main.cpp)

- [ ] **Step 1: 在天气视图单元测试中加入新增字段的缓存测试**
  在 `test/native/test_weather_view/test_main.cpp` 的 `test_weather_cache_write_and_apply` 用例中，增加对 `relativeHumidity`、`apparentTemperature`、`lastUpdate` 字段的读写测试：
  ```cpp
  void test_weather_cache_write_and_apply() {
    homedeck::WeatherData source{};
    source.valid = true;
    source.year = 2026;
    source.month = 6;
    source.day = 3;
    source.currentTemp = 28;
    source.weatherCode = 1;
    source.tempMax = 32;
    source.tempMin = 22;
    source.relativeHumidity = 65;
    source.apparentTemperature = 29;
    source.lastUpdate = 1780536954;
  
    homedeck::writeWeatherCache(source);
  
    homedeck::WeatherData target{};
    target.year = 2026;
    target.month = 6;
    target.day = 3;
    bool applied = homedeck::applyCachedWeather(2026, 6, 3, target);
  
    TEST_ASSERT_TRUE(applied);
    TEST_ASSERT_TRUE(target.valid);
    TEST_ASSERT_EQUAL(28, target.currentTemp);
    TEST_ASSERT_EQUAL(1, target.weatherCode);
    TEST_ASSERT_EQUAL(32, target.tempMax);
    TEST_ASSERT_EQUAL(22, target.tempMin);
    TEST_ASSERT_EQUAL(65, target.relativeHumidity);
    TEST_ASSERT_EQUAL(29, target.apparentTemperature);
    TEST_ASSERT_EQUAL(1780536954, target.lastUpdate);
  }
  ```

- [ ] **Step 2: 运行测试并确保编译失败**
  运行: `pio test -e native -f test_weather_view`
  预期: 编译失败，报错信息提示结构体没有新字段。

- [ ] **Step 3: 修改 `weather_view.h` 增加字段定义**
  在 `src/views/weather_view.h` 中修改 `WeatherData` 和 `WeatherCache` 结构，增加 `relativeHumidity`、`apparentTemperature`、`lastUpdate` 字段：
  ```cpp
  struct WeatherData {
    bool valid = false;
    int currentTemp = 0;
    int weatherCode = 0;
    int tempMax = 0;
    int tempMin = 0;
    int relativeHumidity = 0;      // API 相对湿度 (%)
    int apparentTemperature = 0;   // API 体感温度 (°C)
    uint32_t lastUpdate = 0;        // 数据更新时间戳
  
    int year = 0;
    int month = 0;
    int day = 0;
    int weekday = 0;
  
    bool temperatureAvailable = false;
    float temperatureCelsius = 0.0f;
    bool humidityAvailable = false;
    float humidityPercent = 0.0f;
    std::string bottomCenterMessage;
  };
  
  struct WeatherCache {
    bool valid = false;
    int year = 0;
    int month = 0;
    int day = 0;
    int currentTemp = 0;
    int weatherCode = 0;
    int tempMax = 0;
    int tempMin = 0;
    int relativeHumidity = 0;
    int apparentTemperature = 0;
    uint32_t lastUpdate = 0;
  };
  ```

- [ ] **Step 4: 修改 `weather_view.cpp` 的缓存读写逻辑**
  修改 `writeWeatherCache` 和 `applyCachedWeather` 两个函数以读写这些字段：
  ```cpp
  void writeWeatherCache(const WeatherData& data) {
    gWeatherCache.valid = data.valid;
    gWeatherCache.year = data.year;
    gWeatherCache.month = data.month;
    gWeatherCache.day = data.day;
    gWeatherCache.currentTemp = data.currentTemp;
    gWeatherCache.weatherCode = data.weatherCode;
    gWeatherCache.tempMax = data.tempMax;
    gWeatherCache.tempMin = data.tempMin;
    gWeatherCache.relativeHumidity = data.relativeHumidity;
    gWeatherCache.apparentTemperature = data.apparentTemperature;
    gWeatherCache.lastUpdate = data.lastUpdate;
  }
  
  bool applyCachedWeather(int year, int month, int day, WeatherData& data) {
    if (!gWeatherCache.valid || gWeatherCache.year != year || gWeatherCache.month != month || gWeatherCache.day != day) {
      return false;
    }
    data.valid = true;
    data.currentTemp = gWeatherCache.currentTemp;
    data.weatherCode = gWeatherCache.weatherCode;
    data.tempMax = gWeatherCache.tempMax;
    data.tempMin = gWeatherCache.tempMin;
    data.relativeHumidity = gWeatherCache.relativeHumidity;
    data.apparentTemperature = gWeatherCache.apparentTemperature;
    data.lastUpdate = gWeatherCache.lastUpdate;
    return true;
  }
  ```

- [ ] **Step 5: 运行视图测试，验证缓存部分通过**
  运行: `pio test -e native -f test_weather_view`
  预期: 缓存读取相关的测试用例通过 (PASS)。

- [ ] **Step 6: Git Commit**
  ```bash
  git add src/views/weather_view.h src/views/weather_view.cpp test/native/test_weather_view/test_main.cpp
  git commit -m "feat(weather): extend WeatherData and WeatherCache with apptemp, humidity and lastUpdate"
  ```

---

### Task 3: 实现天气视图的网格化渲染 (变体 C-2)

**Files:**
- Modify: [weather_view.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/views/weather_view.cpp:77-170)
- Test: [test_main.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_weather_view/test_main.cpp)

- [ ] **Step 1: 在天气视图单元测试中对网格内容进行断言**
  在 `test/native/test_weather_view/test_main.cpp` 的 `test_weather_view_render_success` 单元测试中，增加对新增网格中渲染文本的断言：
  ```cpp
  void test_weather_view_render_success() {
    homedeck::WeatherData data{};
    data.valid = true;
    data.currentTemp = 33;
    data.weatherCode = 2; // 多云
    data.tempMax = 35;
    data.tempMin = 26;
    data.year = 2026;
    data.month = 6;
    data.weekday = 3; // 星期三
    data.relativeHumidity = 65;
    data.apparentTemperature = 34;
    data.lastUpdate = 1780536954; // 格式化后为 09:35 或其他对应时间
    data.temperatureAvailable = true;
    data.temperatureCelsius = 25.3f;
    data.humidityAvailable = true;
    data.humidityPercent = 65.2f;
    data.bottomCenterMessage = "12:00";
    
    homedeck::WeatherView view;
    view.render(data);
    
    bool foundTemp = false;
    bool foundDescription = false;
    bool foundHighLow = false;
    bool foundGridLabels = false;
    bool foundGridValues = false;
    
    for (const auto& print : M5.Display.prints) {
      if (print.text == "33" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
        foundTemp = true;
      }
      if (print.text == "多云") {
        foundDescription = true;
      }
      if (print.text.find("最高") != std::string::npos && print.text.find("最低") != std::string::npos) {
        // C-2 方案里最高温和最低温分开了或合并在网格第一层
        foundHighLow = true;
      }
      if (print.text == "体感" || print.text == "相对湿度" || print.text == "数据更新") {
        foundGridLabels = true;
      }
      if (print.text.find("34") != std::string::npos || print.text.find("65%") != std::string::npos) {
        foundGridValues = true;
      }
    }
    
    TEST_ASSERT_TRUE(foundTemp);
    TEST_ASSERT_TRUE(foundDescription);
    TEST_ASSERT_TRUE(foundHighLow);
    TEST_ASSERT_TRUE(foundGridLabels);
    TEST_ASSERT_TRUE(foundGridValues);
  }
  ```

- [ ] **Step 2: 运行测试并确保编译失败**
  运行: `pio test -e native -f test_weather_view`
  预期: 测试中的渲染测试编译通过，但断言失败（因为还没有输出这些字段）。

- [ ] **Step 3: 修改 `weather_view.cpp` 里的渲染核心逻辑**
  1. 在匿名命名空间（或者 `weather_view.cpp` 顶部）增加时间格式化辅助函数：
  ```cpp
  namespace {
  std::string formatTimeHHMM(uint32_t timestamp) {
    if (timestamp == 0) return "--:--";
    std::time_t t = static_cast<std::time_t>(timestamp);
    std::tm buf{};
    std::tm* local = localtime_r(&t, &buf);
    if (!local) return "--:--";
    char out[16] = {};
    std::snprintf(out, sizeof(out), "%02d:%02d", local->tm_hour, local->tm_min);
    return out;
  }
  } // namespace
  ```
  2. 修改 `WeatherView::render` 方法，将渲染中心垂直偏移上移 25 像素，并添加网格化渲染流程：
  ```cpp
  void WeatherView::render(const WeatherData& data) {
    M5Canvas& canvas = sprite();
    prepareScreen(canvas);
  
    const int centerX = canvas.width() / 2;
    const int centerY = canvas.height() / 2;
    // 整体向上移动 25 像素以实现新的两行网格的垂直居中
    const int contentCenterY = centerY - 25; 
    constexpr int kTempFontHeight = 156;
    constexpr int kTempFontHalfHeight = static_cast<int>(kTempFontHeight * kGlyphHeightRatio / 2);
  
    std::string tempStr = data.valid ? std::to_string(data.currentTemp) : "--";
    std::string unitStr = "°C";
  
    int tempWidth = 0;
    int unitWidth = 0;
  
    // 1. 顶部状态栏 + 测量 unitWidth
    if (canvas.loadFont(generated::kDeviceFontVlw)) {
      canvas.setTextColor(kThemeColor, kBgColor);
      canvas.setTextDatum(textdatum_t::top_left);
      canvas.drawString(formatYear(data.year).c_str(), kViewInsetX, kViewHeaderTopY);
  
      canvas.setTextDatum(textdatum_t::top_center);
      canvas.drawString(chineseMonthName(data.month - 1), kViewCenterX, kViewHeaderTopY);
  
      canvas.setTextDatum(textdatum_t::top_right);
      canvas.drawString(weekdayName(data.weekday), kViewRightX, kViewHeaderTopY);
  
      unitWidth = canvas.textWidth(unitStr.c_str());
      canvas.unloadFont();
    }
  
    // 2. 测量 tempWidth
    if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {
      tempWidth = canvas.textWidth(tempStr.c_str());
      canvas.unloadFont();
    }
  
    int totalWidth = tempWidth + 4 + unitWidth;
    int startX = centerX - totalWidth / 2;
  
    // 3. 绘制温度大数字（在偏移后的 contentCenterY 上）
    if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {
      canvas.setTextColor(kThemeColor, kBgColor);
      canvas.setTextDatum(textdatum_t::middle_left);
      canvas.drawString(tempStr.c_str(), startX, contentCenterY);
      canvas.unloadFont();
    }
  
    // 4. 绘制 °C、天气描述、网格（一次性加载小字体）
    if (canvas.loadFont(generated::kDeviceFontVlw)) {
      canvas.setTextColor(kThemeColor, kBgColor);
  
      // °C 符号
      canvas.setTextDatum(textdatum_t::top_left);
      canvas.drawString(unitStr.c_str(), startX + tempWidth + 4, contentCenterY - kTempFontHalfHeight + 10);
  
      // 天气描述
      canvas.setTextDatum(textdatum_t::bottom_center);
      std::string desc = data.valid ? weatherDescription(data.weatherCode) : "";
      if (!desc.empty()) {
        canvas.drawString(desc.c_str(), centerX, contentCenterY - kTempFontHalfHeight - 12);
      }
  
      // 分隔水平虚线
      const int dividerY = contentCenterY + kTempFontHalfHeight + 16;
      for (int x = 40; x < 360; x += 8) {
        canvas.drawFastHLine(x, dividerY, 4, kThemeColor);
      }
  
      // 网格第一行：最高温 / 最低温 (2列)
      const int row1Y = dividerY + 12;
      canvas.setTextDatum(textdatum_t::top_center);
      char maxBuf[32] = {};
      char minBuf[32] = {};
      if (data.valid) {
        std::snprintf(maxBuf, sizeof(maxBuf), "最高 %d°", data.tempMax);
        std::snprintf(minBuf, sizeof(minBuf), "最低 %d°", data.tempMin);
      } else {
        std::snprintf(maxBuf, sizeof(maxBuf), "最高 --");
        std::snprintf(minBuf, sizeof(minBuf), "最低 --");
      }
      canvas.drawString(maxBuf, centerX - 70, row1Y);
      canvas.drawString(minBuf, centerX + 70, row1Y);
      // 中间垂直分隔短线
      canvas.drawFastVLine(centerX, row1Y + 2, 16, kThemeColor);
  
      // 网格第二行：体感、相对湿度、最后更新 (3列，双子行，上行为标签，下行为数值)
      const int row2aY = row1Y + 34; // 标签行
      const int row2bY = row2aY + 22; // 数值行
  
      canvas.drawString("体感", centerX - 110, row2aY);
      canvas.drawString("相对湿度", centerX, row2aY);
      canvas.drawString("数据更新", centerX + 110, row2aY);
  
      char appTempBuf[32] = {};
      char humidityBuf[32] = {};
      std::string updateTimeStr = "--:--";
      if (data.valid) {
        std::snprintf(appTempBuf, sizeof(appTempBuf), "%d°C", data.apparentTemperature);
        std::snprintf(humidityBuf, sizeof(humidityBuf), "%d%%", data.relativeHumidity);
        updateTimeStr = formatTimeHHMM(data.lastUpdate);
      } else {
        std::snprintf(appTempBuf, sizeof(appTempBuf), "--");
        std::snprintf(humidityBuf, sizeof(humidityBuf), "--");
      }
  
      canvas.drawString(appTempBuf, centerX - 110, row2bY);
      canvas.drawString(humidityBuf, centerX, row2bY);
      canvas.drawString(updateTimeStr.c_str(), centerX + 110, row2bY);
  
      // 第二列网格垂直分隔线
      canvas.drawFastVLine(centerX - 55, row2aY + 4, 30, kThemeColor);
      canvas.drawFastVLine(centerX + 55, row2aY + 4, 30, kThemeColor);
  
      // 底部状态栏 (原样)
      drawBottomStatusBar(canvas, {data.temperatureAvailable, data.temperatureCelsius,
                                   data.humidityAvailable, data.humidityPercent,
                                   data.bottomCenterMessage});
      canvas.unloadFont();
    }
  
    pushScreen(canvas);
  }
  ```

- [ ] **Step 4: 运行视图测试确保全部通过**
  运行: `pio test -e native -f test_weather_view`
  预期: 所有 test_weather_view 测试案例顺利通过 (PASS)。

- [ ] **Step 5: Git Commit**
  ```bash
  git add src/views/weather_view.cpp test/native/test_weather_view/test_main.cpp
  git commit -m "feat(weather): implement variant C-2 grid layout for WeatherView"
  ```

---

### Task 4: 在运行时串联数据并更新缓存

**Files:**
- Modify: [app_runtime.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/app/app_runtime.cpp:337-362)

- [ ] **Step 1: 在 `app_runtime.cpp` 获取天气成功时传递和存储新增字段**
  1. 在 `renderWeatherWithEnvironment()` 内，更新 `fetchWeather` 成功后的赋值逻辑，写入 `relativeHumidity`、`apparentTemperature`、`lastUpdate` 到 `data` 结构中：
  ```cpp
    WeatherData data = makeCurrentWeatherData();
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
  ```

- [ ] **Step 2: 使用 PlatformIO 编译工程，并在 native 下跑全量单元测试进行验证**
  运行: `pio run -e m5stack-papercolor`
  运行: `pio test -e native`
  预期: 固件顺利编译无 warning/error，且全量单元测试均成功 (PASS)。

- [ ] **Step 3: Git Commit**
  ```bash
  git add src/app/app_runtime.cpp
  git commit -m "feat(weather): feed humidity, apparent temp, and fetch timestamp to WeatherData in app runtime"
  ```
