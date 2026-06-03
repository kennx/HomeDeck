#pragma once

#include <ctime>
#include <string>

namespace homedeck {

struct WeatherData {
  bool valid = false;
  int currentTemp = 0;
  int weatherCode = 0;
  int tempMax = 0;
  int tempMin = 0;
  
  int year = 0;
  int month = 0;
  int weekday = 0;
  
  bool temperatureAvailable = false;
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;
  float humidityPercent = 0.0f;
  std::string bottomCenterMessage;
};

WeatherData makeWeatherData(const std::tm& localTime);
WeatherData makeCurrentWeatherData();

class WeatherView {
 public:
  void render(const WeatherData& data);
  void renderSleep();
};

}  // namespace homedeck
