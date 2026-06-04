#include <unity.h>
#include "views/weather_view.h"
#include "M5Unified.h"
#include "system/render_context.h"
#include <algorithm>

void setUp() {
  M5 = FakeM5Global{};
  homedeck::resetWeatherCacheForTest();
}
void tearDown() {}

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
  data.lastUpdate = 1780536954; // 包含对应的更新时间戳
  data.temperatureAvailable = true;
  data.temperatureCelsius = 25.3f;
  data.humidityAvailable = true;
  data.humidityPercent = 65.2f;
  data.bottomCenterMessage = "12:00";
  
  homedeck::WeatherView view;
  view.render(data);
  
  bool foundTemp = false;
  bool foundDescription = false;
  bool foundHighMax = false;
  bool foundHighMin = false;
  bool foundGridLabels = false;
  bool foundGridValues = false;
  
  for (const auto& print : M5.Display.prints) {
    if (print.text == "33" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundTemp = true;
    }
    if (print.text == "多云") {
      foundDescription = true;
    }
    if (print.text == "最高 35°") {
      foundHighMax = true;
    }
    if (print.text == "最低 26°") {
      foundHighMin = true;
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
  TEST_ASSERT_TRUE(foundHighMax);
  TEST_ASSERT_TRUE(foundHighMin);
  TEST_ASSERT_TRUE(foundGridLabels);
  TEST_ASSERT_TRUE(foundGridValues);
}

void test_weather_view_render_invalid() {
  homedeck::WeatherData data{};
  data.valid = false;
  data.year = 2026;
  data.month = 6;
  data.weekday = 3;

  homedeck::WeatherView view;
  view.render(data);

  bool foundDegradedTemp = false;
  bool foundDegradedMax = false;
  bool foundDegradedMin = false;

  for (const auto& print : M5.Display.prints) {
    if (print.text == "--" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundDegradedTemp = true;
    }
    if (print.text == "最高 --") {
      foundDegradedMax = true;
    }
    if (print.text == "最低 --") {
      foundDegradedMin = true;
    }
  }

  TEST_ASSERT_TRUE(foundDegradedTemp);
  TEST_ASSERT_TRUE(foundDegradedMax);
  TEST_ASSERT_TRUE(foundDegradedMin);
}

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

void test_weather_cache_date_mismatch() {
  homedeck::WeatherData source{};
  source.valid = true;
  source.year = 2026;
  source.month = 6;
  source.day = 3;
  source.currentTemp = 28;
  source.weatherCode = 1;
  source.tempMax = 32;
  source.tempMin = 22;

  homedeck::writeWeatherCache(source);

  homedeck::WeatherData target{};
  target.year = 2026;
  target.month = 6;
  target.day = 4;
  bool applied = homedeck::applyCachedWeather(2026, 6, 4, target);

  TEST_ASSERT_FALSE(applied);
}

void test_weather_view_render_sleep_with_cache() {
  // 使用真实当前日期，因为 makeCurrentWeatherData 内部调用 time(nullptr)
  const std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = localtime_r(&now, &buf);
  TEST_ASSERT_NOT_NULL(local);

  homedeck::WeatherData source{};
  source.valid = true;
  source.year = local->tm_year + 1900;
  source.month = local->tm_mon + 1;
  source.day = local->tm_mday;
  source.currentTemp = 28;
  source.weatherCode = 1;
  source.tempMax = 32;
  source.tempMin = 22;
  homedeck::writeWeatherCache(source);

  homedeck::WeatherView view;
  view.renderSleep();

  bool foundTemp = false;
  bool foundHighMax = false;
  bool foundHighMin = false;

  for (const auto& print : M5.Display.prints) {
    if (print.text == "28" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundTemp = true;
    }
    if (print.text == "最高 32°") {
      foundHighMax = true;
    }
    if (print.text == "最低 22°") {
      foundHighMin = true;
    }
  }

  TEST_ASSERT_TRUE(foundTemp);
  TEST_ASSERT_TRUE(foundHighMax);
  TEST_ASSERT_TRUE(foundHighMin);
}

void test_weather_view_render_sleep_without_cache() {
  // 确保缓存为空
  homedeck::resetWeatherCacheForTest();

  homedeck::WeatherView view;
  view.renderSleep();

  bool foundDegradedTemp = false;
  bool foundDegradedMax = false;
  bool foundDegradedMin = false;

  for (const auto& print : M5.Display.prints) {
    if (print.text == "--" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundDegradedTemp = true;
    }
    if (print.text == "最高 --") {
      foundDegradedMax = true;
    }
    if (print.text == "最低 --") {
      foundDegradedMin = true;
    }
  }

  TEST_ASSERT_TRUE(foundDegradedTemp);
  TEST_ASSERT_TRUE(foundDegradedMax);
  TEST_ASSERT_TRUE(foundDegradedMin);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_weather_view_render_success);
  RUN_TEST(test_weather_view_render_invalid);
  RUN_TEST(test_weather_cache_write_and_apply);
  RUN_TEST(test_weather_cache_date_mismatch);
  RUN_TEST(test_weather_view_render_sleep_with_cache);
  RUN_TEST(test_weather_view_render_sleep_without_cache);
  return UNITY_END();
}
