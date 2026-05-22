#include <unity.h>

#include "../../../src/calendar_service.h"
#include "../../../src/calendar_service.cpp"

namespace {

constexpr char kPersonalIcs[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260521T093000\r\n"
    "SUMMARY:今天的会议\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260522T101500\r\n"
    "SUMMARY:明天的会议\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kHolidayIcs[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260521\r\n"
    "SUMMARY:劳动节\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kAllDayPersonalIcs[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260521\r\n"
    "SUMMARY:全天安排\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kPersonalIcsSkippedByUical[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260521\r\n"
    "SUMMARY:回退事件\r\n"
    "X-HOMEDECK-FAKE-UICAL-SKIP:1\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kHolidayIcsWithSummaryParams[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260521\r\n"
    "SUMMARY;LANGUAGE=zh-CN:劳动节\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kFetchedIcs[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260521T120000\r\n"
    "SUMMARY:抓取到的会议\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

constexpr char kFoldedSummaryHolidayIcs[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260521\r\n"
    "SUMMARY:劳动\r\n"
    " 节假期\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

std::string buildOverflowPersonalIcs() {
  std::string ics = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n";
  for (int day = 1; day <= 8; ++day) {
    ics += "BEGIN:VEVENT\r\nDTSTART:202605";
    if (day < 10) {
      ics += '0';
    }
    ics += std::to_string(day);
    ics += "T080000\r\nSUMMARY:无关事件";
    ics += std::to_string(day);
    ics += "\r\nEND:VEVENT\r\n";
  }
  ics += "BEGIN:VEVENT\r\nDTSTART:20260521T093000\r\nSUMMARY:目标事件\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n";
  return ics;
}

void test_parse_personal_calendar_for_day_only_returns_todays_events() {
  const CalendarService service;

  const ParsedPersonalCalendar result =
      service.parsePersonalCalendarForDay(kPersonalIcs, 2026, 5, 21);

  TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
  TEST_ASSERT_EQUAL_STRING("09:30", result.events[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("今天的会议", result.events[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("", result.events[1].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("", result.events[1].titleText.c_str());
}

void test_parse_holiday_calendar_for_day_returns_holiday_text() {
  const CalendarService service;

  const ParsedHolidayCalendar result =
      service.parseHolidayCalendarForDay(kHolidayIcs, 2026, 5, 21);

  TEST_ASSERT_EQUAL_STRING("节假日 劳动节", result.holidayText.c_str());
}

void test_parse_personal_calendar_for_day_includes_all_day_events() {
  const CalendarService service;

  const ParsedPersonalCalendar result =
      service.parsePersonalCalendarForDay(kAllDayPersonalIcs, 2026, 5, 21);

  TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
  TEST_ASSERT_EQUAL_STRING("", result.events[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("全天安排", result.events[0].titleText.c_str());
}

void test_parse_personal_calendar_for_day_falls_back_when_uical_returns_empty() {
  const CalendarService service;

  const ParsedPersonalCalendar result =
      service.parsePersonalCalendarForDay(kPersonalIcsSkippedByUical, 2026, 5, 21);

  TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
  TEST_ASSERT_EQUAL_STRING("", result.events[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("回退事件", result.events[0].titleText.c_str());
}

void test_parse_personal_calendar_fallback_scans_past_first_eight_events() {
  const std::string ics = buildOverflowPersonalIcs();
  const ParsedPersonalCalendar result =
      homedeck::calendar_detail::parsePersonalCalendarFallback(
          ics.c_str(), homedeck::CalendarDate{2026, 5, 21});

  TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
  TEST_ASSERT_EQUAL_STRING("09:30", result.events[0].timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("目标事件", result.events[0].titleText.c_str());
}

void test_parse_holiday_calendar_fallback_supports_summary_parameters() {
  const ParsedHolidayCalendar result =
      homedeck::calendar_detail::parseHolidayCalendarFallback(
          kHolidayIcsWithSummaryParams, homedeck::CalendarDate{2026, 5, 21});

  TEST_ASSERT_EQUAL_STRING("节假日 劳动节", result.holidayText.c_str());
}

void test_parse_holiday_calendar_for_day_falls_back_when_uical_summary_is_empty() {
  const CalendarService service;

  const ParsedHolidayCalendar result =
      service.parseHolidayCalendarForDay(kHolidayIcsWithSummaryParams, 2026, 5, 21);

  TEST_ASSERT_EQUAL_STRING("节假日 劳动节", result.holidayText.c_str());
}

void test_parse_personal_calendar_for_day_rejects_invalid_date() {
  const CalendarService service;

  const ParsedPersonalCalendar result =
      service.parsePersonalCalendarForDay(kPersonalIcs, 2026, 13, 1);

  TEST_ASSERT_EQUAL_UINT32(0, result.eventCount);
}

void test_fetch_text_returns_body_when_fetcher_succeeds() {
  const CalendarService service([](const std::string& url) {
    TEST_ASSERT_EQUAL_STRING("https://calendar.example.com/home.ics", url.c_str());
    return CalendarFetchResult{true, kFetchedIcs, 200, ""};
  });

  const CalendarFetchResult fetched =
      service.fetchText("https://calendar.example.com/home.ics");

  TEST_ASSERT_TRUE(fetched.ok);
  TEST_ASSERT_EQUAL(200, fetched.statusCode);
  TEST_ASSERT_EQUAL_STRING(kFetchedIcs, fetched.body.c_str());
}

void test_fetch_text_reports_failure_when_fetcher_fails() {
  const CalendarService service([](const std::string& url) {
    TEST_ASSERT_EQUAL_STRING("http://calendar.example.com/home.ics", url.c_str());
    return CalendarFetchResult{false, "", 503, "service unavailable"};
  });

  const CalendarFetchResult fetched =
      service.fetchText("http://calendar.example.com/home.ics");

  TEST_ASSERT_FALSE(fetched.ok);
  TEST_ASSERT_EQUAL(503, fetched.statusCode);
  TEST_ASSERT_EQUAL_STRING("service unavailable", fetched.errorMessage.c_str());
}

void test_parse_holiday_calendar_fallback_supports_folded_summary_lines() {
  const ParsedHolidayCalendar result =
      homedeck::calendar_detail::parseHolidayCalendarFallback(
          kFoldedSummaryHolidayIcs, homedeck::CalendarDate{2026, 5, 21});

  TEST_ASSERT_EQUAL_STRING("节假日 劳动节假期", result.holidayText.c_str());
}

void test_decode_personal_calendar_cache_caps_event_count_to_stored_rows() {
  const CalendarService service;

  const ParsedPersonalCalendar result = service.decodePersonalCalendarCache(
      "7\n09:00\t缓存会议\n");

  TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
  TEST_ASSERT_EQUAL_STRING("缓存会议", result.events[0].titleText.c_str());
}

}  // namespace

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_parse_personal_calendar_for_day_only_returns_todays_events);
  RUN_TEST(test_parse_holiday_calendar_for_day_returns_holiday_text);
  RUN_TEST(test_parse_personal_calendar_for_day_includes_all_day_events);
  RUN_TEST(test_parse_personal_calendar_for_day_falls_back_when_uical_returns_empty);
  RUN_TEST(test_parse_personal_calendar_fallback_scans_past_first_eight_events);
  RUN_TEST(test_parse_holiday_calendar_fallback_supports_summary_parameters);
  RUN_TEST(test_parse_holiday_calendar_for_day_falls_back_when_uical_summary_is_empty);
  RUN_TEST(test_parse_personal_calendar_for_day_rejects_invalid_date);
  RUN_TEST(test_fetch_text_returns_body_when_fetcher_succeeds);
  RUN_TEST(test_fetch_text_reports_failure_when_fetcher_fails);
  RUN_TEST(test_parse_holiday_calendar_fallback_supports_folded_summary_lines);
  RUN_TEST(test_decode_personal_calendar_cache_caps_event_count_to_stored_rows);
  return UNITY_END();
}
