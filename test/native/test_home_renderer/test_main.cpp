#include <unity.h>

#include <string>

#include "../support/fake_arduino/M5Unified.h"

FakeM5Global M5;

#include "../../../src/device_font.cpp"
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

int codePointWidthFor(FakeFontKind fontKind, unsigned char leadByte) {
  return FakeDisplay::codePointWidthFor(fontKind, leadByte);
}

int renderedWidth(const FakePrintedText& entry) {
  int width = 0;
  for (unsigned char ch : entry.text) {
    if ((ch & 0xC0) != 0x80) {
      width += codePointWidthFor(entry.fontKind, ch) * entry.size;
    }
  }
  return width;
}

int rightEdgeOf(const FakePrintedText& entry) {
  return entry.x + renderedWidth(entry);
}

void assertPrintedFont(const std::string& text, FakeFontKind expected) {
  const FakePrintedText* entry = findPrintedText(text);
  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(entry->fontKind));
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
      renderedWidth(*holiday));

  const FakePrintedText* eventTitle = findPrintedText(112, 396);
  TEST_ASSERT_NOT_NULL(eventTitle);
  TEST_ASSERT_LESS_OR_EQUAL_INT(
      availableWidthFor(*eventTitle),
      renderedWidth(*eventTitle));
}

void test_render_truncates_holiday_text_using_device_font_metrics() {
  resetFakes();

  homedeck::HomeViewModel model;
  model.timeText = "09:30";
  model.dateText = "2026年5月21日 星期四";
  model.lunarText = "农历 四月初五";
  model.solarTermText = "节气 小满";
  model.holidayText = "一二三四五六七八九十一二三四五六七八九十";
  model.temperatureText = "23.7°C";
  model.humidityText = "56%";
  model.wifiConnected = true;
  model.timeSynced = true;
  model.calendarFresh = true;
  model.sensorAvailable = true;
  model.eventRows[0] = {"09:00", "开晨会"};
  model.eventCount = 1;

  HomeRenderer renderer;
  renderer.render(model);

  const FakePrintedText* holiday = findPrintedText(20, 204);
  TEST_ASSERT_NOT_NULL(holiday);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(FakeFontKind::kDeviceDefault),
      static_cast<int>(holiday->fontKind));
  TEST_ASSERT_TRUE(holiday->text.size() >= 3);
  TEST_ASSERT_EQUAL_STRING("...", holiday->text.substr(holiday->text.size() - 3).c_str());
  TEST_ASSERT_LESS_OR_EQUAL_INT(availableWidthFor(*holiday), renderedWidth(*holiday));
}

void test_render_uses_device_default_font_for_all_text_fields() {
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
  model.eventRows[0] = {"09:00", "开晨会"};
  model.eventCount = 1;

  HomeRenderer renderer;
  renderer.render(model);

  assertPrintedFont("09:30", FakeFontKind::kDeviceDefault);
  assertPrintedFont("2026年5月21日 星期四", FakeFontKind::kDeviceDefault);
  assertPrintedFont("今日日程", FakeFontKind::kDeviceDefault);
  assertPrintedFont("09:00", FakeFontKind::kDeviceDefault);
  assertPrintedFont("56%", FakeFontKind::kDeviceDefault);
  assertPrintedFont("开晨会", FakeFontKind::kDeviceDefault);
}

void test_render_draws_temperature_value_with_device_default_font() {
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
  model.eventRows[0] = {"09:00", "开晨会"};
  model.eventCount = 1;

  HomeRenderer renderer;
  renderer.render(model);

  const FakePrintedText* temperature = findPrintedText("23.7°C");
  TEST_ASSERT_NOT_NULL(temperature);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(FakeFontKind::kDeviceDefault),
      static_cast<int>(temperature->fontKind));
  TEST_ASSERT_LESS_OR_EQUAL_INT(32 + 146, rightEdgeOf(*temperature));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_render_shows_empty_event_message_when_event_count_is_zero);
  RUN_TEST(test_render_fits_long_holiday_and_event_text_within_screen);
  RUN_TEST(test_render_truncates_holiday_text_using_device_font_metrics);
  RUN_TEST(test_render_uses_device_default_font_for_all_text_fields);
  RUN_TEST(test_render_draws_temperature_value_with_device_default_font);
  return UNITY_END();
}
