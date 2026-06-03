#include "providers/weather_provider.h"
#include <ArduinoJson.h>

namespace homedeck {

namespace {

std::string urlEncode(const std::string& value) {
  std::string result;
  result.reserve(value.size() * 3);
  for (char c : value) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      result += c;
    } else {
      char buf[4] = {};
      std::snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      result += buf;
    }
  }
  return result;
}

}  // namespace

WeatherResult fetchWeather(
    const WeatherProviderDeps& deps,
    const std::string& latitude,
    const std::string& longitude,
    const std::string& timezoneIana,
    const std::string& wifiSsid,
    const std::string& wifiPassword) {
  
  WeatherResult result{};
  if (!deps.connectWifi || !deps.disconnectWifi || !deps.httpGet) {
    return result;
  }
  
  if (!deps.connectWifi(wifiSsid, wifiPassword)) {
    deps.disconnectWifi();
    return result;
  }
  
  std::string encodedTz = urlEncode(timezoneIana);
  std::string url = "https://api.open-meteo.com/v1/forecast?latitude=" + latitude +
                    "&longitude=" + longitude +
                    "&current=temperature_2m,weather_code" +
                    "&daily=weather_code,temperature_2m_max,temperature_2m_min" +
                    "&forecast_days=1" +
                    "&timezone=" + encodedTz;
                    
  auto httpResponse = deps.httpGet(url);
  deps.disconnectWifi();
  
  if (httpResponse.first != 200 || httpResponse.second.empty()) {
    return result;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, httpResponse.second);
  if (error) {
    return result;
  }
  
  JsonObject current = doc["current"];
  JsonObject daily = doc["daily"];
  
  if (current.isNull() || daily.isNull()) {
    return result;
  }

  if (current["temperature_2m"].isNull() || current["weather_code"].isNull()) {
    return result;
  }
  
  result.currentTemp = static_cast<int>(current["temperature_2m"].as<float>());
  result.weatherCode = current["weather_code"].as<int>();
  
  JsonArray maxTemps = daily["temperature_2m_max"];
  JsonArray minTemps = daily["temperature_2m_min"];
  if (maxTemps.size() > 0 && minTemps.size() > 0) {
    result.tempMax = static_cast<int>(maxTemps[0].as<float>());
    result.tempMin = static_cast<int>(minTemps[0].as<float>());
    result.ok = true;
  }
  
  return result;
}

const char* weatherDescription(int weatherCode) {
  switch (weatherCode) {
    case 0: return "晴";
    case 1: return "基本晴";
    case 2: return "多云";
    case 3: return "阴";
    case 45:
    case 48: return "雾";
    case 51:
    case 53:
    case 55: return "毛毛雨";
    case 56:
    case 57: return "冻毛毛雨";
    case 61:
    case 63:
    case 65: return "雨";
    case 66:
    case 67: return "冻雨";
    case 71:
    case 73:
    case 75: return "雪";
    case 77: return "雪粒";
    case 80:
    case 81:
    case 82: return "阵雨";
    case 85:
    case 86: return "阵雪";
    case 95: return "雷阵雨";
    case 96:
    case 99: return "冰雹雷暴";
    default: return "未知天气";
  }
}

}  // namespace homedeck
