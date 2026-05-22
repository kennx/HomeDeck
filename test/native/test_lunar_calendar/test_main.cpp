#include <array>
#include <string>

#include <unity.h>

#include "../../../src/lunar_calendar_service.h"

namespace {

constexpr int kSupportedYearCount = 201;

constexpr std::array<int, kSupportedYearCount> kJune15ReferenceLunarCodes = {
    519, 429, 510, 520, 502, 513, 10424, 505, 517, 428,
    509, 519, 501, 511, 522, 503, 515, 426, 507, 518,
    429, 510, 520, 502, 514, 10425, 506, 516, 428, 509,
    519, 430, 512, 523, 504, 515, 426, 507, 518, 428,
    510, 521, 502, 513, 10425, 506, 516, 427, 509, 519,
    501, 511, 523, 505, 515, 425, 507, 518, 428, 510,
    522, 503, 514, 10424, 506, 516, 427, 508, 520, 501,
    512, 523, 505, 515, 10425, 506, 518, 429, 510, 521,
    503, 514, 10424, 505, 516, 427, 509, 520, 502, 512,
    523, 504, 515, 426, 507, 518, 430, 511, 521, 502,
    514, 10424, 505, 516, 428, 509, 520, 501, 512, 523,
    504, 514, 10426, 508, 518, 429, 511, 521, 502, 513,
    10424, 506, 517, 428, 510, 520, 501, 511, 523, 504,
    515, 426, 508, 519, 429, 510, 521, 502, 513, 524,
    506, 517, 428, 509, 520, 501, 512, 522, 505, 516,
    426, 507, 519, 429, 510, 521, 503, 514, 10425, 506,
    517, 428, 509, 519, 501, 512, 523, 504, 516, 10426,
    507, 518, 429, 510, 521, 503, 514, 10425, 506, 516,
    428, 509, 519, 501, 513, 524, 505, 515, 10426, 507,
    518, 429, 511, 522, 503, 514, 10425, 506, 516, 427,
    508,
};

constexpr std::array<int, kSupportedYearCount> kLichunReferenceDays = {
    4, 4, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5, 4, 4, 5, 5, 4, 4, 5,
    5, 4, 4, 5, 5, 4, 4, 5, 5, 4, 4, 5, 5, 4, 4, 5, 5, 4, 4, 5,
    5, 4, 4, 5, 5, 4, 4, 4, 5, 4, 4, 4, 5, 4, 4, 4, 5, 4, 4, 4,
    5, 4, 4, 4, 5, 4, 4, 4, 5, 4, 4, 4, 5, 4, 4, 4, 5, 4, 4, 4,
    5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4,
    4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4,
    4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 3, 3, 4,
    4, 3, 3, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 3, 3, 4, 4, 3, 3, 4,
    4, 3, 3, 3, 4, 3, 3, 3, 4, 3, 3, 3, 4, 3, 3, 3, 4, 3, 3, 3,
    4,
};

constexpr std::array<int, kSupportedYearCount> kStartOfSummerReferenceDays = {
    6, 6, 6, 7, 6, 6, 6, 7, 6, 6, 6, 7, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 5, 6, 6, 6, 5, 6, 6, 6, 5, 6, 6, 6, 5, 6, 6, 6,
    5, 6, 6, 6, 5, 6, 6, 6, 5, 6, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6,
    5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6,
    5, 5, 6, 6, 5, 5, 5, 6, 5, 5, 5, 6, 5, 5, 5, 6, 5, 5, 5, 6,
    5, 5, 5, 6, 5, 5, 5, 6, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5,
    4, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5, 4, 5, 5, 5,
    5,
};

constexpr std::array<int, kSupportedYearCount> kWinterSolsticeReferenceDays = {
    22, 22, 23, 23, 22, 22, 23, 23, 22, 22, 23, 23, 22, 22, 23, 23, 22, 22, 22, 23,
    22, 22, 22, 23, 22, 22, 22, 23, 22, 22, 22, 23, 22, 22, 22, 23, 22, 22, 22, 23,
    22, 22, 22, 23, 22, 22, 22, 23, 22, 22, 22, 23, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 21, 22, 22, 22, 21, 22, 22, 22, 21, 22, 22, 22,
    21, 22, 22, 22, 21, 22, 22, 22, 21, 22, 22, 22, 21, 22, 22, 22, 21, 22, 22, 22,
    21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 22, 22,
    21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 22, 22, 21, 21, 21, 22,
    21, 21, 21, 22, 21, 21, 21, 22, 21, 21, 21, 22, 21, 21, 21, 22, 21, 21, 21, 22,
    21, 21, 21, 22, 21, 21, 21, 22, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    22,
};

struct LeapMonthReferenceSample {
  int year;
  int month;
  int day;
  int leapMonth;
};

constexpr std::array<LeapMonthReferenceSample, 74> kLeapMonthReferenceSamples = {{
    {1900, 9, 24, 8}, {1903, 6, 25, 5}, {1906, 5, 23, 4}, {1909, 3, 22, 2},
    {1911, 7, 26, 6}, {1914, 6, 23, 5}, {1917, 3, 23, 2}, {1919, 8, 25, 7},
    {1922, 6, 25, 5}, {1925, 5, 22, 4}, {1928, 3, 22, 2}, {1930, 7, 26, 6},
    {1933, 6, 23, 5}, {1936, 4, 21, 3}, {1938, 8, 25, 7}, {1941, 7, 24, 6},
    {1944, 5, 22, 4}, {1947, 3, 23, 2}, {1949, 8, 24, 7}, {1952, 6, 22, 5},
    {1955, 4, 22, 3}, {1957, 9, 24, 8}, {1960, 7, 24, 6}, {1963, 5, 23, 4},
    {1966, 4, 21, 3}, {1968, 8, 24, 7}, {1971, 6, 23, 5}, {1974, 5, 22, 4},
    {1976, 9, 24, 8}, {1979, 7, 24, 6}, {1982, 5, 23, 4}, {1984, 11, 23, 10},
    {1987, 7, 26, 6}, {1990, 6, 23, 5}, {1993, 4, 22, 3}, {1995, 9, 25, 8},
    {1998, 6, 24, 5}, {2001, 5, 23, 4}, {2004, 3, 21, 2}, {2006, 8, 24, 7},
    {2009, 6, 23, 5}, {2012, 5, 21, 4}, {2014, 10, 24, 9}, {2017, 7, 23, 6},
    {2020, 5, 23, 4}, {2023, 3, 22, 2}, {2025, 7, 25, 6}, {2028, 6, 23, 5},
    {2031, 4, 22, 3}, {2033, 12, 22, 11}, {2036, 7, 23, 6}, {2039, 6, 22, 5},
    {2042, 3, 22, 2}, {2044, 8, 23, 7}, {2047, 6, 23, 5}, {2050, 4, 21, 3},
    {2052, 9, 23, 8}, {2055, 7, 24, 6}, {2058, 5, 22, 4}, {2061, 4, 20, 3},
    {2063, 8, 24, 7}, {2066, 6, 23, 5}, {2069, 5, 21, 4}, {2071, 9, 24, 8},
    {2074, 7, 24, 6}, {2077, 5, 22, 4}, {2080, 4, 20, 3}, {2082, 8, 24, 7},
    {2085, 6, 22, 5}, {2088, 5, 21, 4}, {2090, 9, 24, 8}, {2093, 7, 23, 6},
    {2096, 5, 22, 4}, {2099, 3, 22, 2},
}};

constexpr std::array<const char*, 11> kNumberTexts = {
    "日", "一", "二", "三", "四", "五", "六", "七", "八", "九", "十",
};

constexpr std::array<const char*, 4> kDayPrefixes = {
    "初", "十", "廿", "卅",
};

constexpr std::array<const char*, 12> kMonthTexts = {
    "正", "二", "三", "四", "五", "六", "七", "八", "九", "十", "冬", "腊",
};

std::string toExpectedChinaMonth(int month) {
  return std::string(kMonthTexts[month - 1]) + "月";
}

std::string toExpectedChinaDay(int day) {
  if (day == 10) {
    return "初十";
  }
  if (day == 20) {
    return "二十";
  }
  if (day == 30) {
    return "三十";
  }

  return std::string(kDayPrefixes[day / 10]) + kNumberTexts[day % 10];
}

std::string decodeExpectedLunarText(int encoded) {
  const bool isLeap = encoded >= 10000;
  const int normalized = isLeap ? encoded - 10000 : encoded;
  const int month = normalized / 100;
  const int day = normalized % 100;

  std::string text = "农历 ";
  if (isLeap) {
    text += "闰";
  }
  text += toExpectedChinaMonth(month);
  text += toExpectedChinaDay(day);
  return text;
}

void test_describe_date_returns_lunar_and_solar_term_for_2026_05_21() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2026, 5, 21);

  TEST_ASSERT_EQUAL_STRING("农历 四月初五", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 小满", info.solarTermText.c_str());
}

void test_describe_date_returns_lunar_new_year_for_2024_02_10() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2024, 2, 10);

  TEST_ASSERT_EQUAL_STRING("农历 正月初一", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_returns_winter_solstice_for_2024_12_21() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2024, 12, 21);

  TEST_ASSERT_EQUAL_STRING("节气 冬至", info.solarTermText.c_str());
}

void test_describe_date_rejects_dates_before_1900_01_31() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(1900, 1, 30);

  TEST_ASSERT_EQUAL_STRING("农历 无", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_accepts_1900_01_31_base_day() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(1900, 1, 31);

  TEST_ASSERT_EQUAL_STRING("农历 正月初一", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_returns_leap_month_text_for_1949_09_14() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(1949, 9, 14);

  TEST_ASSERT_EQUAL_STRING("农历 闰七月廿二", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_returns_first_solar_term_for_2024_02_04() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2024, 2, 4);

  TEST_ASSERT_EQUAL_STRING("农历 腊月廿五", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 立春", info.solarTermText.c_str());
}

void test_describe_date_accepts_2100_12_31_upper_bound() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2100, 12, 31);

  TEST_ASSERT_EQUAL_STRING("农历 腊月初一", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_rejects_dates_after_2100_12_31() {
  const LunarCalendarService service;
  const LunarDayInfo info = service.describeDate(2101, 1, 1);

  TEST_ASSERT_EQUAL_STRING("农历 无", info.lunarText.c_str());
  TEST_ASSERT_EQUAL_STRING("节气 无", info.solarTermText.c_str());
}

void test_describe_date_matches_reference_lunar_on_june_15_across_supported_years() {
  const LunarCalendarService service;

  for (int year = 1900; year <= 2100; ++year) {
    const LunarDayInfo info = service.describeDate(year, 6, 15);
    const std::string expected =
        decodeExpectedLunarText(kJune15ReferenceLunarCodes[year - 1900]);
    const std::string message = "year=" + std::to_string(year);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        expected.c_str(), info.lunarText.c_str(), message.c_str());
  }
}

void test_describe_date_matches_reference_lichun_day_across_supported_years() {
  const LunarCalendarService service;

  for (int year = 1900; year <= 2100; ++year) {
    const int day = kLichunReferenceDays[year - 1900];
    const LunarDayInfo info = service.describeDate(year, 2, day);
    const std::string message = "year=" + std::to_string(year);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "节气 立春", info.solarTermText.c_str(), message.c_str());
  }
}

void test_describe_date_matches_reference_start_of_summer_day_across_supported_years() {
  const LunarCalendarService service;

  for (int year = 1900; year <= 2100; ++year) {
    const int day = kStartOfSummerReferenceDays[year - 1900];
    const LunarDayInfo info = service.describeDate(year, 5, day);
    const std::string message = "year=" + std::to_string(year);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "节气 立夏", info.solarTermText.c_str(), message.c_str());
  }
}

void test_describe_date_matches_reference_winter_solstice_day_across_supported_years() {
  const LunarCalendarService service;

  for (int year = 1900; year <= 2100; ++year) {
    const int day = kWinterSolsticeReferenceDays[year - 1900];
    const LunarDayInfo info = service.describeDate(year, 12, day);
    const std::string message = "year=" + std::to_string(year);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "节气 冬至", info.solarTermText.c_str(), message.c_str());
  }
}

void test_describe_date_matches_reference_for_each_leap_month_start() {
  const LunarCalendarService service;

  for (const auto& sample : kLeapMonthReferenceSamples) {
    const LunarDayInfo info =
        service.describeDate(sample.year, sample.month, sample.day);
    const std::string expected =
        "农历 闰" + toExpectedChinaMonth(sample.leapMonth) + "初一";
    const std::string message = "year=" + std::to_string(sample.year);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        expected.c_str(), info.lunarText.c_str(), message.c_str());
  }
}

}  // namespace

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_describe_date_returns_lunar_and_solar_term_for_2026_05_21);
  RUN_TEST(test_describe_date_returns_lunar_new_year_for_2024_02_10);
  RUN_TEST(test_describe_date_returns_winter_solstice_for_2024_12_21);
  RUN_TEST(test_describe_date_rejects_dates_before_1900_01_31);
  RUN_TEST(test_describe_date_accepts_1900_01_31_base_day);
  RUN_TEST(test_describe_date_returns_leap_month_text_for_1949_09_14);
  RUN_TEST(test_describe_date_returns_first_solar_term_for_2024_02_04);
  RUN_TEST(test_describe_date_accepts_2100_12_31_upper_bound);
  RUN_TEST(test_describe_date_rejects_dates_after_2100_12_31);
  RUN_TEST(test_describe_date_matches_reference_lunar_on_june_15_across_supported_years);
  RUN_TEST(test_describe_date_matches_reference_lichun_day_across_supported_years);
  RUN_TEST(test_describe_date_matches_reference_start_of_summer_day_across_supported_years);
  RUN_TEST(test_describe_date_matches_reference_winter_solstice_day_across_supported_years);
  RUN_TEST(test_describe_date_matches_reference_for_each_leap_month_start);
  return UNITY_END();
}
