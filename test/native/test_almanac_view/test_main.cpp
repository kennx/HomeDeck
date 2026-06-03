#include <unity.h>

#include <LittleFS.h>
#include <M5Unified.h>

#include <cstdint>
#include <string>
#include <vector>

#include "../support/almanac_fixture.h"
#include "views/almanac_view.h"
#include "views/calendar_view.h"

namespace {

}  // namespace

void test_home_calendar_data_uses_almanac_package_when_available() {
  fakeLittleFSSetFile("/almanac.bin", homedeck::test::buildSingleDayFixturePackage());

  std::tm local{};
  local.tm_year = 1900 - 1900;
  local.tm_mon = 0;
  local.tm_mday = 1;
  local.tm_wday = 1;

  const auto data = homedeck::makeHomeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("1900 年", data.year.c_str());
  TEST_ASSERT_EQUAL_STRING("一月", data.month.c_str());
  TEST_ASSERT_EQUAL_STRING("1", data.day.c_str());
  TEST_ASSERT_EQUAL_STRING("星期一", data.weekday.c_str());
  TEST_ASSERT_FALSE(data.isHoliday);
  TEST_ASSERT_EQUAL_STRING("腊月初一", data.lunarDate.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.solarTerm.c_str());
  TEST_ASSERT_EQUAL_STRING("己亥年 丙子月 甲子日 鼠日", data.ganzhi.c_str());
  TEST_ASSERT_EQUAL_STRING("五行海中金", data.wuxing.c_str());
  TEST_ASSERT_EQUAL_STRING("冲马煞南", data.chongsha.c_str());
  TEST_ASSERT_EQUAL_STRING("值神青龙", data.zhishen.c_str());
  TEST_ASSERT_EQUAL_STRING("建除建日", data.jianchu.c_str());
  TEST_ASSERT_EQUAL_STRING("胎神占门碓外东南", data.taishen.c_str());
  TEST_ASSERT_EQUAL_STRING("祭祀 祈福", data.yi.c_str());
  TEST_ASSERT_EQUAL_STRING("嫁娶", data.ji.c_str());
}

void test_calendar_data_reads_almanac_file_once_for_lookahead_scan() {
  fakeLittleFSSetFile("/almanac.bin", homedeck::test::buildSingleDayFixturePackage());

  std::tm local{};
  local.tm_year = 1900 - 1900;
  local.tm_mon = 0;
  local.tm_mday = 1;
  local.tm_wday = 1;

  const auto data = homedeck::makeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("腊月初一", data.lunarDate.c_str());
  TEST_ASSERT_EQUAL(1, LittleFS.beginCount);
  TEST_ASSERT_EQUAL(1, LittleFS.openCount);
  TEST_ASSERT_EQUAL(1, LittleFS.endCount);
}

void test_calendar_data_retries_after_failed_almanac_lookup() {
  std::tm local{};
  local.tm_year = 1900 - 1900;
  local.tm_mon = 0;
  local.tm_mday = 1;
  local.tm_wday = 1;

  (void)homedeck::makeCalendarData(local);

  fakeLittleFSSetFile("/almanac.bin", homedeck::test::buildSingleDayFixturePackage());
  const auto data = homedeck::makeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("腊月初一", data.lunarDate.c_str());
  TEST_ASSERT_EQUAL(2, LittleFS.openCount);
}

void test_home_calendar_data_reuses_calendar_almanac_cache_for_same_day() {
  fakeLittleFSSetFile("/almanac.bin", homedeck::test::buildSingleDayFixturePackage());

  std::tm local{};
  local.tm_year = 1900 - 1900;
  local.tm_mon = 0;
  local.tm_mday = 1;
  local.tm_wday = 1;

  (void)homedeck::makeCalendarData(local);
  const int openCountAfterCalendar = LittleFS.openCount;

  const auto data = homedeck::makeHomeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("腊月初一", data.lunarDate.c_str());
  TEST_ASSERT_EQUAL(openCountAfterCalendar, LittleFS.openCount);
}

void test_home_calendar_data_uses_placeholder_for_empty_almanac_actions() {
  auto day = homedeck::test::singleDayFixture();
  day.yi.clear();
  day.ji.clear();
  fakeLittleFSSetFile("/almanac.bin", homedeck::test::buildAlmanacFixturePackage(1900, 1, 1, {day}));

  std::tm local{};
  local.tm_year = 1900 - 1900;
  local.tm_mon = 0;
  local.tm_mday = 1;
  local.tm_wday = 1;

  const auto data = homedeck::makeHomeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("暂无", data.yi.c_str());
  TEST_ASSERT_EQUAL_STRING("暂无", data.ji.c_str());
}

void test_home_calendar_data_keeps_public_date_when_almanac_missing() {
  std::tm local{};
  local.tm_year = 2030 - 1900;
  local.tm_mon = 8;
  local.tm_mday = 8;
  local.tm_wday = 0;

  const auto data = homedeck::makeHomeCalendarData(local);

  TEST_ASSERT_EQUAL_STRING("2030 年", data.year.c_str());
  TEST_ASSERT_EQUAL_STRING("九月", data.month.c_str());
  TEST_ASSERT_EQUAL_STRING("8", data.day.c_str());
  TEST_ASSERT_EQUAL_STRING("星期日", data.weekday.c_str());
  TEST_ASSERT_TRUE(data.isHoliday);
  TEST_ASSERT_EQUAL_STRING("数据缺失", data.lunarDate.c_str());
  TEST_ASSERT_EQUAL_STRING("", data.solarTerm.c_str());
  TEST_ASSERT_EQUAL_STRING("黄历数据缺失", data.ganzhi.c_str());
  TEST_ASSERT_EQUAL_STRING("五行暂无", data.wuxing.c_str());
  TEST_ASSERT_EQUAL_STRING("冲煞暂无", data.chongsha.c_str());
  TEST_ASSERT_EQUAL_STRING("值神暂无", data.zhishen.c_str());
  TEST_ASSERT_EQUAL_STRING("建除暂无", data.jianchu.c_str());
  TEST_ASSERT_EQUAL_STRING("胎神暂无", data.taishen.c_str());
  TEST_ASSERT_EQUAL_STRING("暂无", data.yi.c_str());
  TEST_ASSERT_EQUAL_STRING("暂无", data.ji.c_str());
}

void setUp() {
  M5 = FakeM5Global{};
  fakeLittleFSReset();
  homedeck::resetAlmanacCacheForTest();
}

void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_home_calendar_data_uses_almanac_package_when_available);
  RUN_TEST(test_calendar_data_reads_almanac_file_once_for_lookahead_scan);
  RUN_TEST(test_calendar_data_retries_after_failed_almanac_lookup);
  RUN_TEST(test_home_calendar_data_reuses_calendar_almanac_cache_for_same_day);
  RUN_TEST(test_home_calendar_data_uses_placeholder_for_empty_almanac_actions);
  RUN_TEST(test_home_calendar_data_keeps_public_date_when_almanac_missing);
  return UNITY_END();
}
