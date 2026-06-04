#include "views/weather_view.h"
#include <M5Unified.h>
#include <cstdio>
#include <string>

#include "generated/device_font_vlw.h"
#include "system/render_context.h"
#include "system/sht40_reader.h"
#include "views/view_common.h"
#include "providers/weather_provider.h"

namespace homedeck {

namespace {

constexpr int kThemeColor = TFT_BLACK;
constexpr int kBgColor = TFT_WHITE;
constexpr float kGlyphHeightRatio = 13.0f / 16.0f;

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

}  // namespace

WeatherData makeWeatherData(const std::tm& localTime) {
  WeatherData data{};
  data.year = localTime.tm_year + 1900;
  data.month = localTime.tm_mon + 1;
  data.day = localTime.tm_mday;
  data.weekday = localTime.tm_wday;
  return data;
}

WeatherData makeCurrentWeatherData() {
  const std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = now > 0 ? localtime_r(&now, &buf) : nullptr;
  if (local == nullptr) {
    std::tm fallback = fallbackLocalTime();
    return makeWeatherData(fallback);
  }
  return makeWeatherData(*local);
}

#ifdef UNIT_TEST
WeatherCache gWeatherCache;
#else
RTC_DATA_ATTR WeatherCache gWeatherCache;
#endif

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

#ifdef UNIT_TEST
void resetWeatherCacheForTest() {
  gWeatherCache = WeatherCache{};
}
#endif

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
    // 中间垂直分隔 short 线
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

void WeatherView::renderSleep() {
  WeatherData data = makeCurrentWeatherData();
  if (!applyCachedWeather(data.year, data.month, data.day, data)) {
    data.valid = false;
  }
  data.temperatureAvailable = false;
  data.humidityAvailable = false;
  data.bottomCenterMessage = "--:--";
  render(data);
}

}  // namespace homedeck
