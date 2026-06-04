#pragma once

#include <ctime>
#include <string>
#include <cstdint>

namespace homedeck {

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

WeatherData makeWeatherData(const std::tm& localTime);
WeatherData makeCurrentWeatherData();

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

extern WeatherCache gWeatherCache;

void writeWeatherCache(const WeatherData& data);
bool applyCachedWeather(int year, int month, int day, WeatherData& data);

#ifdef UNIT_TEST
void resetWeatherCacheForTest();
#endif

class WeatherView {
 public:
  void render(const WeatherData& data);
  void renderSleep();
};

}  // namespace homedeck
