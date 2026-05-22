#include <unity.h>

#include <string>

#include "../support/fake_arduino/M5Unified.h"

FakeM5Global M5;

#include "../../../src/home_renderer.cpp"

namespace {

void resetFakes() {
  M5 = FakeM5Global{};
}

const FakePrintedText* findPrintedText(int x, int y) {
  for (const auto& entry : M5.Display.prints) {
    if (entry.x == x && entry.y == y) {
      return &entry;
    }
  }

  return nullptr;
}

const FakePrintedText* findPrintedText(const std::string& text) {
  for (const auto& entry : M5.Display.prints) {
    if (entry.text == text) {
      return &entry;
    }
  }

  return nullptr;
}

int availableWidthFor(const FakePrintedText& entry) {
  return M5.Display.width() - entry.x - 20;
}

}  // namespace

void test_render_shows_empty_event_message_when_event_count_is_zero() {
  resetFakes();

  homedeck::HomeViewModel model;
  model.timeText = "09:30";
  model.dateText = "2026年5月21日 星期四";
  model.lunarText = "农历 四月初五";
  model.solarTermText = "节气 小满";
  model.holidayText = "节假日 无";
  model.temperatureText = "23.7°C";
  model.humidityText = "56%";
  model.wifiConnected = true;
  model.timeSynced = true;
  model.calendarFresh = true;
  model.sensorAvailable = true;
  model.eventRows[0] = {"", "今日日程为空"};
  model.eventCount = 0;

  HomeRenderer renderer;
  renderer.render(model);

  TEST_ASSERT_NOT_NULL(findPrintedText("今日日程为空"));
}

void test_render_fits_long_holiday_and_event_text_within_screen() {
  resetFakes();

  homedeck::HomeViewModel model;
  model.timeText = "09:30";
  model.dateText = "2026年5月21日 星期四";
  model.lunarText = "农历 四月初五";
  model.solarTermText = "节气 小满";
  model.holidayText = "节假日 这是一个非常非常非常非常非常非常长的说明文本用于测试截断";
  model.temperatureText = "23.7°C";
  model.humidityText = "56%";
  model.wifiConnected = true;
  model.timeSynced = true;
  model.calendarFresh = true;
  model.sensorAvailable = true;
  model.eventRows[0] = {
      "09:00",
      "这是一个非常非常非常非常非常非常长的日程标题用于测试截断是否生效",
  };
  model.eventCount = 1;

  HomeRenderer renderer;
  renderer.render(model);

  const FakePrintedText* holiday = findPrintedText(20, 204);
  TEST_ASSERT_NOT_NULL(holiday);
  TEST_ASSERT_LESS_OR_EQUAL_INT(
      availableWidthFor(*holiday),
      M5.Display.textWidth(holiday->text.c_str()));

  const FakePrintedText* eventTitle = findPrintedText(112, 396);
  TEST_ASSERT_NOT_NULL(eventTitle);
  TEST_ASSERT_LESS_OR_EQUAL_INT(
      availableWidthFor(*eventTitle),
      M5.Display.textWidth(eventTitle->text.c_str()));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_render_shows_empty_event_message_when_event_count_is_zero);
  RUN_TEST(test_render_fits_long_holiday_and_event_text_within_screen);
  return UNITY_END();
}
