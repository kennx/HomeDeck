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

}  // namespace

WeatherData makeWeatherData(const std::tm& localTime) {
  WeatherData data{};
  data.year = localTime.tm_year + 1900;
  data.month = localTime.tm_mon + 1;
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

void WeatherView::render() {
  WeatherData data = makeCurrentWeatherData();
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }
  data.bottomCenterMessage = formatCurrentTimeHHMM();
  render(data);
}

void WeatherView::render(const WeatherData& data) {
  M5Canvas& canvas = sprite();
  prepareScreen(canvas);

  // 1. 顶部状态栏：年 / 月份 / 星期几
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    canvas.setTextDatum(textdatum_t::top_left);
    canvas.drawString(formatYear(data.year).c_str(), kViewInsetX, kViewHeaderTopY);

    canvas.setTextDatum(textdatum_t::top_center);
    canvas.drawString(chineseMonthName(data.month - 1), kViewCenterX, kViewHeaderTopY);

    canvas.setTextDatum(textdatum_t::top_right);
    canvas.drawString(weekdayName(data.weekday), kViewRightX, kViewHeaderTopY);
    canvas.unloadFont();
  }

  const int centerX = canvas.width() / 2;
  const int centerY = canvas.height() / 2;
  constexpr int kTempFontHeight = 156;
  constexpr int kTempFontHalfHeight = static_cast<int>(kTempFontHeight * kGlyphHeightRatio / 2);

  // 2. 中间大字气温和 °C 符号
  std::string tempStr = data.valid ? std::to_string(data.currentTemp) : "--";
  std::string unitStr = "°C";
  
  int tempWidth = 0;
  int unitWidth = 0;
  
  if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {
    tempWidth = canvas.textWidth(tempStr.c_str());
    canvas.unloadFont();
  }
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    unitWidth = canvas.textWidth(unitStr.c_str());
    canvas.unloadFont();
  }
  
  int totalWidth = tempWidth + 4 + unitWidth;
  int startX = centerX - totalWidth / 2;
  
  // 绘制温度大数字
  if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    canvas.setTextDatum(textdatum_t::middle_left);
    canvas.drawString(tempStr.c_str(), startX, centerY);
    canvas.unloadFont();
  }
  
  // 绘制 °C 符号
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    canvas.setTextDatum(textdatum_t::top_left);
    canvas.drawString(unitStr.c_str(), startX + tempWidth + 4, centerY - kTempFontHalfHeight + 10);
    canvas.unloadFont();
  }

  // 3. 气温大字上方的天气描述
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    canvas.setTextDatum(textdatum_t::bottom_center);
    std::string desc = data.valid ? weatherDescription(data.weatherCode) : "";
    if (!desc.empty()) {
      canvas.drawString(desc.c_str(), centerX, centerY - kTempFontHalfHeight - 12);
    }
    canvas.unloadFont();
  }

  // 4. 气温大字下方的今日最高最低温
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    canvas.setTextDatum(textdatum_t::top_center);
    char highLowBuf[64] = {};
    if (data.valid) {
      std::snprintf(highLowBuf, sizeof(highLowBuf), "最高 %d° / 最低 %d°", data.tempMax, data.tempMin);
    } else {
      std::snprintf(highLowBuf, sizeof(highLowBuf), "最高 -- / 最低 --");
    }
    canvas.drawString(highLowBuf, centerX, centerY + kTempFontHalfHeight + 12);
    canvas.unloadFont();
  }

  // 5. 底部状态栏
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(kThemeColor, kBgColor);
    drawBottomStatusBar(canvas, {data.temperatureAvailable, data.temperatureCelsius,
                                 data.humidityAvailable, data.humidityPercent,
                                 data.bottomCenterMessage});
    canvas.unloadFont();
  }

  pushScreen(canvas);
}

void WeatherView::renderSleep() {
  WeatherData data = makeCurrentWeatherData();
  data.valid = false;
  data.temperatureAvailable = false;
  data.humidityAvailable = false;
  data.bottomCenterMessage = "--:--";
  render(data);
}

}  // namespace homedeck
