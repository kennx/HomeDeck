# 天气视图新增指标与网格化布局设计说明

本文档定义了在 M5Stack PaperColor 的天气视图中新增湿度、体感温度、最后更新时间指标，并将其调整为变体 C-2 样式的网格布局设计规格。

## 背景与目标
目前的天气视图仅显示大字当前温度、天气描述、以及最高/最低温度。
用户希望追加：
1. **相对湿度**（API 获取）
2. **体感温度**（API 获取）
3. **数据最后更新时间**（发起 API 请求成功时的本地时间，精确到分）
4. 使用 VLW 20px（即 `generated::kDeviceFontVlw`）字体。
5. 整体布局在屏幕中水平、垂直居中。

## 布局设计 (变体 C-2)

布局排版将原有的“最高/最低温”下移并与新增的体感温度、湿度、更新时间整合，形成两行网格布局。

```text
+------------------------------------------+
|  2026年 6月                     星期三   |  <- 顶部状态栏 (原样)
+------------------------------------------+
|                                          |
|                  多云                    |  <- 天气描述 (原样)
|                                          |
|                 33 °C                    |  <- 气温大数字 (原样)
|                                          |
|        --------------------------        |  <- 虚线分隔线
|            最高 35°  |  最低 26°         |  <- 第一行 (2列网格)
|                                          |
|          体感    |   湿度   |   更新     |  <- 第二行 (3列网格)
|          34°C    |   65%    |  09:34     |  
|                                          |
+------------------------------------------+
|  室温 25.3°C    湿度 65.2%      12:00    |  <- 底部状态栏 (原样)
+------------------------------------------+
```

### 渲染细节与坐标计算
为了保证在 **400x600** 屏幕中水平、垂直居中，我们需要将整个中心内容区域（从“天气描述”到“网格第二行”）作为一个整体包围盒（Bounding Box）计算高度，并使其中心对齐屏幕的垂直中心。

1. **整体包围盒垂直尺寸计算**：
   - 气温大数字高度：约 `126px`（`kTempFontHeight * kGlyphHeightRatio = 156 * 13 / 16`）
   - 天气描述高度及间距：约 `20px`，向上偏移 `12px`
   - 虚线高度及上下边距：约 `20px`
   - 网格第一行（高低温）高度：`20px`（20px VLW字体），行距约 `8px`
   - 网格第二行（体感/湿度/更新）高度：`32px`（标签 + 数值），行距约 `8px`
   - 总高度约为：`20 (天气描述) + 12 (间距) + 126 (大字温度) + 20 (虚线) + 20 (第一行) + 8 (间距) + 32 (第二行) = 238px`。
   - 包围盒的中心应定位在屏幕的 `centerY`（约 `300px` 处），从而求出包围盒的起始 `y` 坐标。各部分的 `y` 坐标均基于该包围盒起始 `y` 轴进行相对偏移渲染。

2. **第一行网格（2列）**：
   - 虚线下方约 `10px` 开始。
   - 列1中心为 `kViewCenterX - 70` (130px 处)，右对齐或居中对齐渲染“最高 XX°”。
   - 列2中心为 `kViewCenterX + 70` (270px 处)，左对齐或居中对齐渲染“最低 XX°”。
   - 中间可用竖线 `|` 隔开。

3. **第二行网格（3列）**：
   - 第一行下方约 `32px` 开始。
   - 列1（体感）：中心位于 `kViewCenterX - 110` (90px)，上边显示“体感”，下边显示“XX°C”。
   - 列2（湿度）：中心位于 `kViewCenterX` (200px)，上边显示“相对湿度”或“湿度”，下边显示“XX%”。
   - 列3（更新时间）：中心位于 `kViewCenterX + 110` (310px)，上边显示“数据更新”或“更新”，下边显示“XX:XX”。
   - 两两列之间可用灰度竖线隔开。

4. **字体使用**：
   - 统一采用小字体 `generated::kDeviceFontVlw`。

## 数据流与结构变更

### 1. 天气服务接口 (`providers/weather_provider.h` / `weather_provider.cpp`)
- Open-Meteo API 请求 URL 修改：
  - 增加参数：`&current=temperature_2m,weather_code,relative_humidity_2m,apparent_temperature`
- `WeatherResult` 结构体：
  ```cpp
  struct WeatherResult {
    bool ok = false;
    int currentTemp = 0;
    int weatherCode = 0;
    int tempMax = 0;
    int tempMin = 0;
    int relativeHumidity = 0;      // 新增相对湿度 (%)
    int apparentTemperature = 0;   // 新增体感温度 (°C)
    uint32_t lastUpdate = 0;        // 新增请求成功的本地时间戳 (UNIX)
  };
  ```

### 2. 天气视图与缓存结构 (`views/weather_view.h` / `weather_view.cpp`)
- `WeatherData` 结构体：
  ```cpp
  struct WeatherData {
    bool valid = false;
    int currentTemp = 0;
    int weatherCode = 0;
    int tempMax = 0;
    int tempMin = 0;
    int relativeHumidity = 0;
    int apparentTemperature = 0;
    uint32_t lastUpdate = 0;        // 本地更新时间戳
    
    int year = 0;
    int month = 0;
    int day = 0;
    int weekday = 0;
    // ... 原有的传感器温湿度保留
  };
  ```
- `WeatherCache` 结构体 (RTC 备份保存，确保睡眠唤醒时可用)：
  ```cpp
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

### 3. 数据更新流 (`app/app_runtime.cpp`)
- 在 `renderWeatherWithEnvironment()` 内：
  - 调用 `fetchWeather`。
  - 如果 API 成功获取数据：
    - 将 `result.relativeHumidity` 和 `result.apparentTemperature` 存入 `WeatherData`。
    - 将 `result.lastUpdate` 赋值为当前时间（即 `time(nullptr)`）。
    - 写入缓存。
  - 如果 API 失败：
    - 从缓存中加载数据，包含新增字段和 `lastUpdate` 属性。
- 数据降级：
  - 如果数据无效（!valid），则在界面相应指标处展示 `--` 或空。

## 单元测试扩展
1. **`test_weather_provider`**：
   - 更新 Mock 出来的 HTTP 返回 JSON，增加对应的 `relative_humidity_2m` 和 `apparent_temperature`。
   - 验证 `fetchWeather` 后输出结果中，这两个字段被正确赋值。
2. **`test_weather_view`**：
   - 增加对新增字段 `relativeHumidity`、`apparentTemperature`、`lastUpdate` 的读写缓存测试。
   - 验证渲染输出里是否包含对应的网格文字（例如“体感”、“湿度”等）。
