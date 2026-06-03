#pragma once

#include <string>
#include <functional>
#include <utility>

namespace homedeck {

struct WeatherResult {
  bool ok = false;
  int currentTemp = 0;
  int weatherCode = 0;
  int tempMax = 0;
  int tempMin = 0;
};

struct WeatherProviderDeps {
  std::function<bool(const std::string& ssid, const std::string& password)> connectWifi;
  std::function<void()> disconnectWifi;
  std::function<std::pair<int, std::string>(const std::string& url)> httpGet;
};

WeatherResult fetchWeather(
    const WeatherProviderDeps& deps,
    const std::string& latitude,
    const std::string& longitude,
    const std::string& timezoneIana,
    const std::string& wifiSsid,
    const std::string& wifiPassword);

const char* weatherDescription(int weatherCode);

}  // namespace homedeck
