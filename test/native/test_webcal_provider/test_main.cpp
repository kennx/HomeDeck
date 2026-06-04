#include <unity.h>
#include "providers/webcal_provider.h"
#include <LittleFS.h>
#include <map>
#include <string>
#include <ctime>
#include <vector>

void setUp() {}
void tearDown() {}

// 辅助函数，构造 2026-06-04 的 std::tm
std::tm makeLocalNow() {
  std::tm tm{};
  tm.tm_year = 2026 - 1900;
  tm.tm_mon = 5; // 6月 (0-indexed)
  tm.tm_mday = 4;
  tm.tm_hour = 12;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  return tm;
}

void test_parse_ics_stream_success() {
  std::string icsData = 
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260604\r\n"
    "SUMMARY:端午节\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20260714\r\n"
    "SUMMARY:边界最大节日\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;TZID=Asia/Shanghai:20260715\r\n"
    "SUMMARY:超限最大节日\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260425\r\n"
    "SUMMARY:边界最小节日\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260424\r\n"
    "SUMMARY:超限最小节日\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);

  // 应保留 2026-06-04, 2026-07-14, 2026-04-25
  // 应过滤 2026-07-15, 2026-04-24
  TEST_ASSERT_EQUAL_UINT(3, festivals.size());
  TEST_ASSERT_EQUAL_STRING("端午节", festivals["2026-06-04"].c_str());
  TEST_ASSERT_EQUAL_STRING("边界最大节日", festivals["2026-07-14"].c_str());
  TEST_ASSERT_EQUAL_STRING("边界最小节日", festivals["2026-04-25"].c_str());
  TEST_ASSERT_TRUE(festivals.find("2026-07-15") == festivals.end());
  TEST_ASSERT_TRUE(festivals.find("2026-04-24") == festivals.end());
}

void test_parse_ics_unfolding() {
  std::string icsData =
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260605\r\n"
    "SUMMARY:端午\r\n"
    " 节假日\r\n"
    "\t 连休\r\n"
    "END:VEVENT\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("端午节假日 连休", festivals["2026-06-05"].c_str());
}

void test_parse_ics_unescaping() {
  std::string icsData =
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260605\r\n"
    "SUMMARY:A\\, B\\; C\\\\D\\nE\\NF\r\n"
    "END:VEVENT\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);
  // A, B; C\nD\nE\nF
  TEST_ASSERT_EQUAL_STRING("A, B; C\\D\nE\nF", festivals["2026-06-05"].c_str());
}

void test_parse_ics_multiple_events_on_same_day() {
  std::string icsData =
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260604\r\n"
    "SUMMARY:节日A\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260604\r\n"
    "SUMMARY:节日B\r\n"
    "END:VEVENT\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("节日A 节日B", festivals["2026-06-04"].c_str());
}

void test_parse_ics_dtstart_variations() {
  std::string icsData =
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260606T090000\r\n"
    "SUMMARY:事件1\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260607T090000Z\r\n"
    "SUMMARY:事件2\r\n"
    "END:VEVENT\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("事件1", festivals["2026-06-06"].c_str());
  TEST_ASSERT_EQUAL_STRING("事件2", festivals["2026-06-07"].c_str());
}

void test_load_cached_festivals_success() {
  fakeLittleFSReset();
  std::string jsonContent = "{\"2026-06-04\":\"端午节\",\"2026-06-05\":\"节日\"}";
  std::vector<std::uint8_t> data(jsonContent.begin(), jsonContent.end());
  fakeLittleFSSetFile("/webcal_cache.json", data);

  std::map<std::string, std::string> festivals;
  bool ok = homedeck::loadCachedFestivals(festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT(2, festivals.size());
  TEST_ASSERT_EQUAL_STRING("端午节", festivals["2026-06-04"].c_str());
  TEST_ASSERT_EQUAL_STRING("节日", festivals["2026-06-05"].c_str());
}

void test_load_cached_festivals_not_exist() {
  fakeLittleFSReset();
  std::map<std::string, std::string> festivals;
  bool ok = homedeck::loadCachedFestivals(festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT(0, festivals.size());
}

void test_sync_webcal_festivals_native_returns_false() {
  bool ok = homedeck::syncWebcalFestivals("https://example.com/ics", "ssid", "pass");
  TEST_ASSERT_FALSE(ok);
}

void test_parse_ics_invalid_dates() {
  std::string icsData =
    "BEGIN:VEVENT\r\n"
    "DTSTART:20261304\r\n"
    "SUMMARY:无效月份\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260004\r\n"
    "SUMMARY:无效零月\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260632\r\n"
    "SUMMARY:无效日期\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART:20260600\r\n"
    "SUMMARY:无效零日\r\n"
    "END:VEVENT\r\n";

  StringStream stream(icsData);
  std::map<std::string, std::string> festivals;
  std::tm localNow = makeLocalNow();

  bool ok = homedeck::parseIcsStream(stream, localNow, festivals);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(festivals.empty());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_ics_stream_success);
  RUN_TEST(test_parse_ics_unfolding);
  RUN_TEST(test_parse_ics_unescaping);
  RUN_TEST(test_parse_ics_multiple_events_on_same_day);
  RUN_TEST(test_parse_ics_dtstart_variations);
  RUN_TEST(test_parse_ics_invalid_dates);
  RUN_TEST(test_load_cached_festivals_success);
  RUN_TEST(test_load_cached_festivals_not_exist);
  RUN_TEST(test_sync_webcal_festivals_native_returns_false);
  return UNITY_END();
}
