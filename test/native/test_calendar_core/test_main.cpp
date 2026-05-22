#include <array>

#include <unity.h>

#include "homedeck/home_screen.h"

using homedeck::EventRow;
using homedeck::VisibleEvents;
using homedeck::clampTodayEvents;
using homedeck::makeEmptyEventMessage;
using homedeck::makeHolidayText;

void test_clamp_today_events_truncates_and_reports_footer() {
  const std::array<EventRow, 5> source{{
      {"08:00", "早餐"},
      {"09:00", "站会"},
      {"12:00", "午饭"},
      {"15:00", "散步"},
      {"20:00", "阅读"},
  }};

  const VisibleEvents result = clampTodayEvents(source, 5, 4);

  TEST_ASSERT_EQUAL_UINT32(4, result.visibleCount);
  TEST_ASSERT_EQUAL_STRING("08:00", result.visible[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("早餐", result.visible[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("09:00", result.visible[1].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("站会", result.visible[1].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("12:00", result.visible[2].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("午饭", result.visible[2].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("15:00", result.visible[3].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("散步", result.visible[3].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("还有 1 项", result.footerText.c_str());
}

void test_clamp_today_events_uses_source_count_for_footer_when_cache_is_truncated() {
  const std::array<EventRow, 5> source{{
      {"08:00", "早餐"},
      {"09:00", "站会"},
      {"12:00", "午饭"},
      {"15:00", "散步"},
      {"20:00", "阅读"},
  }};

  const VisibleEvents result = clampTodayEvents(source, 7, 4);

  TEST_ASSERT_EQUAL_UINT32(4, result.visibleCount);
  TEST_ASSERT_EQUAL_STRING("08:00", result.visible[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("散步", result.visible[3].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("还有 3 项", result.footerText.c_str());
}

void test_clamp_today_events_without_truncation_keeps_footer_empty() {
  const std::array<EventRow, 5> source{{
      {"08:00", "早餐"},
      {"09:00", "站会"},
      {"12:00", "午饭"},
      {"15:00", "散步"},
      {"20:00", "阅读"},
  }};

  const VisibleEvents result = clampTodayEvents(source, 2, 4);

  TEST_ASSERT_EQUAL_UINT32(2, result.visibleCount);
  TEST_ASSERT_EQUAL_STRING("08:00", result.visible[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("早餐", result.visible[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("09:00", result.visible[1].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("站会", result.visible[1].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("", result.footerText.c_str());
}

void test_make_empty_event_message_returns_not_configured_when_disabled() {
  TEST_ASSERT_EQUAL_STRING("未配置个人日程", makeEmptyEventMessage(false, false).c_str());
}

void test_make_empty_event_message_returns_unavailable_when_cache_missing() {
  TEST_ASSERT_EQUAL_STRING("今日日程不可用", makeEmptyEventMessage(true, false).c_str());
}

void test_make_empty_event_message_returns_empty_when_cache_is_available() {
  TEST_ASSERT_EQUAL_STRING("今日日程为空", makeEmptyEventMessage(true, true).c_str());
}

void test_make_holiday_text_returns_not_configured_when_disabled() {
  TEST_ASSERT_EQUAL_STRING("未配置节假日", makeHolidayText(false, "劳动节").c_str());
}

void test_make_holiday_text_returns_none_when_value_is_empty() {
  TEST_ASSERT_EQUAL_STRING("节假日 无", makeHolidayText(true, "").c_str());
}

void test_make_holiday_text_returns_original_value_when_present() {
  TEST_ASSERT_EQUAL_STRING("劳动节", makeHolidayText(true, "劳动节").c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clamp_today_events_truncates_and_reports_footer);
  RUN_TEST(test_clamp_today_events_uses_source_count_for_footer_when_cache_is_truncated);
  RUN_TEST(test_clamp_today_events_without_truncation_keeps_footer_empty);
  RUN_TEST(test_make_empty_event_message_returns_not_configured_when_disabled);
  RUN_TEST(test_make_empty_event_message_returns_unavailable_when_cache_missing);
  RUN_TEST(test_make_empty_event_message_returns_empty_when_cache_is_available);
  RUN_TEST(test_make_holiday_text_returns_not_configured_when_disabled);
  RUN_TEST(test_make_holiday_text_returns_none_when_value_is_empty);
  RUN_TEST(test_make_holiday_text_returns_original_value_when_present);
  return UNITY_END();
}
