#include <unity.h>
#include "views/weather_view.h"
#include "M5Unified.h"
#include "system/render_context.h"
#include <algorithm>

void setUp() {
  M5 = FakeM5Global{};
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
  data.temperatureAvailable = true;
  data.temperatureCelsius = 25.3f;
  data.humidityAvailable = true;
  data.humidityPercent = 65.2f;
  data.bottomCenterMessage = "12:00";
  
  homedeck::WeatherView view;
  view.render(data);
  
  // 检查是否绘制了 33（大字气温）
  bool foundTemp = false;
  bool foundDescription = false;
  bool foundHighLow = false;
  bool foundStatus = false;
  
  for (const auto& print : M5.Display.prints) {
    if (print.text == "33" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundTemp = true;
    }
    if (print.text == "多云") {
      foundDescription = true;
    }
    if (print.text.find("最高 35") != std::string::npos && print.text.find("最低 26") != std::string::npos) {
      foundHighLow = true;
    }
    if (print.text == "星期三") {
      foundStatus = true;
    }
  }
  
  TEST_ASSERT_TRUE(foundTemp);
  TEST_ASSERT_TRUE(foundDescription);
  TEST_ASSERT_TRUE(foundHighLow);
  TEST_ASSERT_TRUE(foundStatus);
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
  bool foundDegradedHighLow = false;
  
  for (const auto& print : M5.Display.prints) {
    if (print.text == "--" && print.fontKind == FakeFontKind::kDeviceLargeDate) {
      foundDegradedTemp = true;
    }
    if (print.text.find("最高 -- / 最低 --") != std::string::npos) {
      foundDegradedHighLow = true;
    }
  }
  
  TEST_ASSERT_TRUE(foundDegradedTemp);
  TEST_ASSERT_TRUE(foundDegradedHighLow);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_weather_view_render_success);
  RUN_TEST(test_weather_view_render_invalid);
  return UNITY_END();
}
