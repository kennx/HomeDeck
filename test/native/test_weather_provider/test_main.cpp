#include <unity.h>
#include "providers/weather_provider.h"

void setUp() {}
void tearDown() {}

void test_weather_description_mapping() {
  TEST_ASSERT_EQUAL_STRING("晴", homedeck::weatherDescription(0));
  TEST_ASSERT_EQUAL_STRING("多云", homedeck::weatherDescription(2));
  TEST_ASSERT_EQUAL_STRING("阴", homedeck::weatherDescription(3));
  TEST_ASSERT_EQUAL_STRING("雨", homedeck::weatherDescription(63));
  TEST_ASSERT_EQUAL_STRING("雷阵雨", homedeck::weatherDescription(95));
  TEST_ASSERT_EQUAL_STRING("未知天气", homedeck::weatherDescription(999));
}

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

void test_fetch_weather_wifi_fail() {
  homedeck::WeatherProviderDeps deps;
  bool wifiDisconnected = false;
  deps.connectWifi = [](const std::string&, const std::string&) { return false; };
  deps.disconnectWifi = [&]() { wifiDisconnected = true; };
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> { return {200, ""}; };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
  TEST_ASSERT_TRUE(wifiDisconnected);
}

void test_fetch_weather_http_fail() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> { return {500, "Error"}; };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_malformed_json() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> { return {200, "not json"}; };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_missing_current_key() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> {
    std::string mockJson = R"({"daily":{"weather_code":[1],"temperature_2m_max":[30],"temperature_2m_min":[20]}})";
    return {200, mockJson};
  };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_missing_current_temperature() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> {
    std::string mockJson = R"({"current":{"weather_code":1},"daily":{"weather_code":[1],"temperature_2m_max":[30],"temperature_2m_min":[20]}})";
    return {200, mockJson};
  };

  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_missing_current_weather_code() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> {
    std::string mockJson = R"({"current":{"temperature_2m":22},"daily":{"weather_code":[1],"temperature_2m_max":[30],"temperature_2m_min":[20]}})";
    return {200, mockJson};
  };

  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_empty_daily_arrays() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> {
    std::string mockJson = R"({"current":{"temperature_2m":22,"weather_code":1,"relative_humidity_2m":50,"apparent_temperature":21},"daily":{"weather_code":[],"temperature_2m_max":[],"temperature_2m_min":[]}})";
    return {200, mockJson};
  };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_FALSE(result.ok);
}

void test_fetch_weather_negative_temp() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  deps.httpGet = [](const std::string&) -> std::pair<int, std::string> {
    std::string mockJson = R"({
      "current":{"temperature_2m":-5.7,"weather_code":71,"relative_humidity_2m":80,"apparent_temperature":-8.2},
      "daily":{"weather_code":[71],"temperature_2m_max":[-2.1],"temperature_2m_min":[-10.3]}
    })";
    return {200, mockJson};
  };
  
  auto result = homedeck::fetchWeather(deps, "25.78", "113.02", "Asia/Shanghai", "SSID", "PASS");
  TEST_ASSERT_TRUE(result.ok);
  TEST_ASSERT_EQUAL(-5, result.currentTemp);
  TEST_ASSERT_EQUAL(71, result.weatherCode);
  TEST_ASSERT_EQUAL(-2, result.tempMax);
  TEST_ASSERT_EQUAL(-10, result.tempMin);
  TEST_ASSERT_EQUAL(80, result.relativeHumidity);
  TEST_ASSERT_EQUAL(-8, result.apparentTemperature);
}

void test_url_encode_special_chars() {
  homedeck::WeatherProviderDeps deps;
  deps.connectWifi = [](const std::string&, const std::string&) { return true; };
  deps.disconnectWifi = []() {};
  std::string capturedUrl;
  deps.httpGet = [&](const std::string& url) -> std::pair<int, std::string> {
    capturedUrl = url;
    return {200, "{}"};
  };
  
  homedeck::fetchWeather(deps, "25.78", "113.02", "Etc/GMT+1", "SSID", "PASS");
  TEST_ASSERT_TRUE(capturedUrl.find("Etc%2FGMT%2B1") != std::string::npos);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_weather_description_mapping);
  RUN_TEST(test_fetch_weather_success);
  RUN_TEST(test_fetch_weather_wifi_fail);
  RUN_TEST(test_fetch_weather_http_fail);
  RUN_TEST(test_fetch_weather_malformed_json);
  RUN_TEST(test_fetch_weather_missing_current_key);
  RUN_TEST(test_fetch_weather_missing_current_temperature);
  RUN_TEST(test_fetch_weather_missing_current_weather_code);
  RUN_TEST(test_fetch_weather_empty_daily_arrays);
  RUN_TEST(test_fetch_weather_negative_temp);
  RUN_TEST(test_url_encode_special_chars);
  return UNITY_END();
}
