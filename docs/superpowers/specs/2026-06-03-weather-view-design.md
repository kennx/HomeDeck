# 天气预报视图设计

## 概述

为 HomeDeck 添加「天气预报」视图，通过 Open-Meteo API 获取天气数据，在墨水屏中间用 156px 大字体显示当前气温，辅以天气描述和最高/最低温信息。

## 架构：Provider + View 分离

遵循项目中 `almanac_provider` / `almanac_view` 的模式：

- **WeatherProvider**：负责 HTTP 请求和 JSON 解析，输出 `WeatherData` 结构体
- **WeatherView**：负责接收 `WeatherData` 并渲染到墨水屏

## 数据结构

```cpp
struct WeatherData {
  bool valid = false;              // API 请求是否成功

  // 当前天气
  int currentTemp = 0;             // 当前气温（整数，截断小数）
  int weatherCode = 0;             // WMO 天气代码

  // 每日预报
  int tempMax = 0;                 // 今日最高温（整数）
  int tempMin = 0;                 // 今日最低温（整数）

  // 状态栏共享数据
  int month = 0;                   // 1-12
  int weekday = 0;                 // 0=周日
  int year = 0;
  bool temperatureAvailable = false;  // SHT40 传感器温度
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;     // SHT40 传感器湿度
  float humidityPercent = 0.0f;
  std::string bottomCenterMessage;    // 时间 "HH:MM"
};
```

## API 接口

### 请求 URL

```
https://api.open-meteo.com/v1/forecast
  ?latitude={config.latitude}
  &longitude={config.longitude}
  &current=temperature_2m,weather_code
  &daily=weather_code,temperature_2m_max,temperature_2m_min
  &forecast_days=1
  &timezone={config.timezoneIana}
```

只请求当前气温、天气代码和每日最高/最低温。用户原始 URL 中的 hourly 和其他 current 字段暂不使用，精简请求以减少内存和解析开销。

### 响应格式（示例）

```json
{
  "current": {
    "temperature_2m": 33.0,
    "weather_code": 3
  },
  "daily": {
    "weather_code": [95],
    "temperature_2m_max": [34.3],
    "temperature_2m_min": [26.8]
  }
}
```

### JSON 解析

添加 `ArduinoJson` 库依赖（platformio.ini）。项目当前没有 JSON 解析能力，`ArduinoJson` 是 ESP32 生态最成熟的选择。

## WMO 天气代码映射

将 WMO weather code 映射为中文简短描述：

| Code | 描述 |
|------|------|
| 0 | 晴 |
| 1 | 基本晴 |
| 2 | 多云 |
| 3 | 阴 |
| 45, 48 | 雾 |
| 51, 53, 55 | 毛毛雨 |
| 56, 57 | 冻毛毛雨 |
| 61, 63, 65 | 雨 |
| 66, 67 | 冻雨 |
| 71, 73, 75 | 雪 |
| 77 | 雪粒 |
| 80, 81, 82 | 阵雨 |
| 85, 86 | 阵雪 |
| 95 | 雷阵雨 |
| 96, 99 | 冰雹雷暴 |

## 屏幕布局

屏幕尺寸：400 × 540 像素。

```
┌─────────────────────────────────────┐
│ 2026 年        六月        星期三    │  ← 顶部状态栏 (20px, y=12)
│                                     │
│                                     │
│                                     │
│              多云                    │  ← 天气描述 (20px, 居中, y≈centerY - 大字半高 - 12)
│                                     │
│              33°C                   │  ← 当前气温 (156px 数字 + 20px °C, 垂直水平居中)
│                                     │
│          最高 34° / 最低 27°         │  ← 高低温 (20px, 居中, y≈centerY + 大字半高 + 12)
│                                     │
│                                     │
│ 25.3°C       15:30         65.2%    │  ← 底部状态栏 (20px, 与其他视图一致)
└─────────────────────────────────────┘
```

### 温度大字排版细节

- 数字部分使用 `kDeviceLargeDateFontVlw` (156px) 绘制，仅显示整数（截断小数）
- `°C` 符号使用 `kDeviceFontVlw` (20px) 绘制，紧贴数字右侧偏上
- 温度居中策略：先用 156px 字体 `textWidth()` 获取数字宽度 W，计算总宽 = W + °C符号宽度，以此居中放置
- 负温度直接显示负号（156px 字体已包含 `-` 字形）

### 降级显示

- 请求失败：数字区域显示 `--`（使用 156px 字体），天气描述显示空白，高低温显示 `-- / --`
- 睡眠前：与请求失败相同的降级界面

## 数据获取策略

### render()（切换到天气视图 / 唤醒时）

```
1. 读取 ConfigStore 获取 WiFi 凭据和经纬度/时区
2. 连接 WiFi（使用现有的 connectWifiPreservingAccessPoint）
3. HTTP GET 请求 Open-Meteo API
4. 断开 WiFi（WiFi.disconnect(true)）
5. 解析 JSON → WeatherData
6. 读取 SHT40 传感器 → 填充 temperatureAvailable/humidityAvailable
7. 渲染
```

### renderSleep()（进入深度睡眠前）

```
1. 渲染降级界面（-- / 无天气描述 / -- 高低温）
2. 底部状态栏温度/湿度显示 "--"，时间显示 "--:--"
```

### HTTP 超时

- 连接超时：5 秒
- 响应超时：10 秒
- 超时后视为请求失败，渲染降级界面

## WeatherProvider 接口

```cpp
struct WeatherResult {
  bool ok = false;
  int currentTemp = 0;
  int weatherCode = 0;
  int tempMax = 0;
  int tempMin = 0;
};

// deps 注入 WiFi 连接和 HTTP 能力
struct WeatherProviderDeps {
  std::function<bool(const std::string&, const std::string&)> connectWifi;
  std::function<void()> disconnectWifi;
};

WeatherResult fetchWeather(
    const WeatherProviderDeps& deps,
    const std::string& latitude,
    const std::string& longitude,
    const std::string& timezoneIana,
    const std::string& wifiSsid,
    const std::string& wifiPassword);
```

注意：考虑到 `AlmanacProvider` 是一个无状态类，而天气获取更自然地表达为一个自由函数（一次 HTTP 往返），这里使用自由函数而非类。

## 视图轮转集成

新顺序：`Almanac → Calendar → Countdown → Weather → Almanac`

### 需要修改的文件

#### [NEW] `src/providers/weather_provider.h`
- `WeatherResult` 结构体
- `WeatherProviderDeps` 结构体
- `fetchWeather()` 函数声明
- `weatherDescription()` WMO 代码 → 中文描述函数

#### [NEW] `src/providers/weather_provider.cpp`
- HTTP GET 实现（使用 ESP32 内置 `HTTPClient`）
- ArduinoJson 解析
- WMO 天气代码映射表

#### [NEW] `src/views/weather_view.h`
- `WeatherData` 结构体
- `makeWeatherData()` / `makeCurrentWeatherData()` 工厂函数
- `WeatherView` 类（`render()` / `render(data)` / `renderSleep()`）

#### [NEW] `src/views/weather_view.cpp`
- 渲染逻辑：顶部状态栏 → 天气描述 → 大字温度 → 高低温 → 底部状态栏

#### [MODIFY] `src/app/view_manager.h`
- `SystemView` 枚举加 `Weather`
- `ViewManagerDeps` 加 `renderWeather`

#### [MODIFY] `src/app/view_manager.cpp`
- `switchToNextView()`: Countdown → Weather, Weather → Almanac
- `switchTo()`: 加 Weather case

#### [MODIFY] `src/app/boot_controller.h`
- `BootControllerDeps` 加 `renderWeather`

#### [MODIFY] `src/app/boot_controller.cpp`
- 双击 BtnC 刷新天气视图
- `preSleepRender` 加 Weather case（在 app_runtime.cpp 中）

#### [MODIFY] `src/app/app_runtime.cpp`
- `#include` 新头文件
- 实例化 `WeatherView gWeatherView`
- `makeBootDeps()` 绑定 `renderWeather` / `preSleepRender` Weather case
- 实现 `renderWeather()` 包装函数（读 config → fetchWeather → 传感器 → 渲染）

#### [MODIFY] `platformio.ini`
- `lib_deps` 添加 `bblanchon/ArduinoJson`

## 测试计划

- `test/native/` 下新增 `test_weather_provider.cpp` 或并入现有测试文件
- 测试 `weatherDescription()` WMO 代码映射
- 测试 `WeatherData` 构造（makeWeatherData）
- HTTP 和 WiFi 部分通过 deps 注入 mock
- 构建验证：`pio run -e m5stack-papercolor` + `pio test -e native`
